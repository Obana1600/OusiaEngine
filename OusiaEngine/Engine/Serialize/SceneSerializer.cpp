#include "SceneSerializer.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Serialize/JsonReader.h"
#include "Serialize/JsonWriter.h"
#include "Serialize/TypeRegistry.h"

#include <algorithm>
#include <cwchar>
#include <format>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <tuple>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace ou;


namespace {

// TODO: version管理をどうするか決める
constexpr int kVersion = 1;

// GameObject / Component の予約のキー
constexpr const char *kTypeKey = "$type";
constexpr const char *kIdKey = "$id";


void CollectHierarchy(GameObject *gameObject, const Scene *scene, std::vector<GameObject *> &out) {
	if (gameObject->IsDestroyed() || gameObject->GetScene() != scene) { return; }

	out.push_back(gameObject);
	for (auto *child : gameObject->GetTransform()->GetChildren()) {
		CollectHierarchy(child->GetGameObject(), scene, out);
	}
}

void AssignFileId(Object *object, std::unordered_set<uint64_t> &usedIds) {
	static std::mt19937_64 engine{std::random_device{}()};
	constexpr uint64_t kMask = (1ull << 53) - 1;

	auto id = object->GetFileId();
	while (id == 0 || usedIds.contains(id)) {
		id = engine() & kMask;
	}

	object->SetFileId(id);
	usedIds.insert(id);
}

const Json *FindMember(const Json &object, const char *key) {
	const auto it = object.find(key);
	return it != object.end() ? &*it : nullptr;
}

void RegisterFileId(Object *object, const Json &json, std::unordered_map<uint64_t, Object *> &objectsById) {
	const auto *id = FindMember(json, kIdKey);
	if (!id || !id->is_number_unsigned() || id->get<uint64_t>() == 0) { return; }

	const auto [it, inserted] = objectsById.emplace(id->get<uint64_t>(), object);
	if (!inserted) {
		Log::Warning("id が重なっています (後ろのものは参照できません): {}", id->get<uint64_t>());
		return;
	}
	object->SetFileId(id->get<uint64_t>());
}


// ---- GameObject ごとのファイル ----

// 兄弟の中での並び。親の子の一覧を持つと、2 人が同じ親に子を足したときに親のファイルで衝突するため、子の側に持つ
constexpr const char *kOrderKey = "$order";

uint64_t ReadId(const Json &json) {
	const auto *id = json.is_object() ? FindMember(json, kIdKey) : nullptr;
	return id && id->is_number_unsigned() ? id->get<uint64_t>() : 0;
}

// Transform は components の先頭（GameObject::ReplaceTransform が先頭を保つ）
Json *FindTransformJson(Json &gameObjectJson) {
	auto it = gameObjectJson.find("components");
	if (it == gameObjectJson.end() || !it->is_array() || it->empty() || !it->front().is_object()) { return nullptr; }
	return &it->front();
}

// 親の Transform の fileId（親がなければ 0）
uint64_t ReadParentTransformId(const Json &transformJson) {
	const auto *parent = FindMember(transformJson, "parent");
	if (!parent || !parent->is_object()) { return 0; }
	const auto *id = FindMember(*parent, "id");
	return id && id->is_number_unsigned() ? id->get<uint64_t>() : 0;
}

std::string ReadNameForLog(const Json &gameObjectJson) {
	const auto *name = FindMember(gameObjectJson, "name");
	return name && name->is_string() ? name->get<std::string>() : std::format("(id {})", ReadId(gameObjectJson));
}

// 並びのキーは、7 桁の整数に、あいだに入れるときだけ小数の桁を続けたもの（"5000003" と "5000004" のあいだは "50000035"）
// 小数の桁の末尾に 0 を置かないので、文字列の大小が数の大小と一致する
// 浮動小数点数にしないのは、同じところへ足し続けると精度が尽きて、兄弟全部のキーを振り直す（全部のファイルが変わる）ことになるため
// 小数だけにしないのは、いちばんよくある「末尾に足す」で毎回あいだを取ると、キーが数回ごとに 1 桁ずつ伸びるため
constexpr size_t kOrderIntegerDigits = 7;
constexpr int64_t kOrderIntegerLimit = 10'000'000;
constexpr int64_t kFirstOrderInteger = 5'000'000;

bool IsValidOrderKey(std::string_view key) {
	return key.size() >= kOrderIntegerDigits && (key.size() == kOrderIntegerDigits || key.back() != '0') &&
		   std::all_of(key.begin(), key.end(), [](char c) { return c >= '0' && c <= '9'; });
}

int64_t ReadOrderInteger(std::string_view key) {
	int64_t integer = 0;
	for (size_t i = 0; i < kOrderIntegerDigits; i++) {
		integer = integer * 10 + (key[i] - '0');
	}
	return integer;
}

std::string_view ReadOrderFraction(std::string_view key) {
	return key.substr(kOrderIntegerDigits);
}

std::string MakeOrderKey(int64_t integer, std::string_view fraction) {
	return std::format("{:07}", integer) + std::string(fraction);
}

// 小数の桁で lower と upper のあいだを取る。lower が空なら 0、upper が空なら 1 として扱う
std::string MakeOrderFractionBetween(std::string_view lower, std::string_view upper) {
	auto isUpperOne = upper.empty() || lower >= upper;
	std::string key;
	for (size_t i = 0;; i++) {
		const int low = i < lower.size() ? lower[i] - '0' : 0;
		const int high = isUpperOne ? 10 : (i < upper.size() ? upper[i] - '0' : 0);
		if (high - low > 1) {
			key += static_cast<char>('0' + (low + high) / 2);
			return key;
		}
		key += static_cast<char>('0' + low);
		// 1 しか違わないなら low の桁を取り、その先は lower の残りと 1 のあいだにする
		if (high != low) { isUpperOne = true; }
	}
}

std::string MakeOrderKeyAfter(std::string_view lower) {
	const auto integer = ReadOrderInteger(lower);
	if (integer + 1 < kOrderIntegerLimit) { return MakeOrderKey(integer + 1, {}); }
	return MakeOrderKey(integer, MakeOrderFractionBetween(ReadOrderFraction(lower), {}));
}

std::string MakeOrderKeyBefore(std::string_view upper) {
	const auto integer = ReadOrderInteger(upper);
	if (!ReadOrderFraction(upper).empty()) { return MakeOrderKey(integer, {}); }
	// 0000000 より前は作れないが、先頭に 500 万回足さないと届かないので、同じキーにして id の順に任せる
	return MakeOrderKey(std::max<int64_t>(integer - 1, 0), {});
}

std::string MakeOrderKeyBetween(std::string_view lower, std::string_view upper) {
	const auto lowerInteger = ReadOrderInteger(lower);
	const auto upperInteger = ReadOrderInteger(upper);
	if (upperInteger - lowerInteger >= 2) { return MakeOrderKey((lowerInteger + upperInteger) / 2, {}); }
	if (upperInteger - lowerInteger == 1) {
		if (!ReadOrderFraction(upper).empty()) { return MakeOrderKey(upperInteger, {}); }
		return MakeOrderKey(lowerInteger, MakeOrderFractionBetween(ReadOrderFraction(lower), {}));
	}
	return MakeOrderKey(lowerInteger, MakeOrderFractionBetween(ReadOrderFraction(lower), ReadOrderFraction(upper)));
}

// lower・upper が空なら、そちら側には何もない
void AssignOrderKeysBetween(std::vector<std::string> &keys, size_t begin, size_t end, std::string lower, std::string upper) {
	if (begin >= end) { return; }

	if (upper.empty()) {
		for (auto i = begin; i < end; i++) {
			keys[i] = lower.empty() ? MakeOrderKey(kFirstOrderInteger, {}) : MakeOrderKeyAfter(lower);
			lower = keys[i];
		}
		return;
	}
	if (lower.empty()) {
		for (auto i = end; i > begin; i--) {
			keys[i - 1] = MakeOrderKeyBefore(upper);
			upper = keys[i - 1];
		}
		return;
	}

	// 端から順に振るとキーが 1 個ごとに伸びるので、真ん中から振る
	const auto middle = begin + (end - begin) / 2;
	keys[middle] = MakeOrderKeyBetween(lower, upper);
	AssignOrderKeysBetween(keys, begin, middle, lower, keys[middle]);
	AssignOrderKeysBetween(keys, middle + 1, end, keys[middle], upper);
}

// keys には今の並びで前回のキー（なければ空）を入れて渡す。前回のキーをなるべく多く残し、残りだけ振り直す
// 全部振り直さないのは、動かしていない GameObject のファイルまで変わって、衝突のもとになるため
void AssignOrderKeys(std::vector<std::string> &keys) {
	const auto count = keys.size();

	// 前回のキーのうち、今の並びのまま使えるもの（最長増加部分列）
	std::vector<size_t> lengths(count, 0);
	std::vector<size_t> previous(count, SIZE_MAX);
	auto last = SIZE_MAX;
	for (size_t i = 0; i < count; i++) {
		if (!IsValidOrderKey(keys[i])) { continue; }
		lengths[i] = 1;
		for (size_t j = 0; j < i; j++) {
			if (lengths[j] > 0 && keys[j] < keys[i] && lengths[j] + 1 > lengths[i]) {
				lengths[i] = lengths[j] + 1;
				previous[i] = j;
			}
		}
		if (last == SIZE_MAX || lengths[i] > lengths[last]) { last = i; }
	}

	std::vector<bool> isKept(count, false);
	for (auto i = last; i != SIZE_MAX; i = previous[i]) {
		isKept[i] = true;
	}

	std::string lower;
	size_t begin = 0;
	for (size_t i = 0; i <= count; i++) {
		if (i < count && !isKept[i]) { continue; }

		const auto upper = i < count ? keys[i] : std::string{};
		AssignOrderKeysBetween(keys, begin, i, lower, upper);
		lower = upper;
		begin = i + 1;
	}
}

std::string ToText(const Json &json) {
	return json.dump(1, '\t') + '\n';
}

std::optional<std::string> LoadText(const std::filesystem::path &path) {
	std::ifstream file(path, std::ios::binary);
	if (!file) { return std::nullopt; }
	return std::string(std::istreambuf_iterator<char>(file), {});
}

// 中身が同じなら書かない（エディタで開いているファイルや、更新日時を見る道具を騒がせないように）
bool SaveTextIfChanged(const std::filesystem::path &path, const std::string &text) {
	if (LoadText(path) == text) { return true; }

	std::ofstream file(path, std::ios::binary);
	if (!file) {
		Log::Error("ファイルの書き込みに失敗しました: {}", ToUtf8(path));
		return false;
	}
	file << text;
	return static_cast<bool>(file);
}

std::filesystem::path ToObjectFileName(uint64_t gameObjectId) {
	return std::format(L"{:014x}.json", gameObjectId);
}

// 前に保存した GameObject のファイル
struct SavedObjectFile {
	std::filesystem::path path;
	uint64_t parentTransformId = 0;
	std::string order;
};

std::unordered_map<uint64_t, SavedObjectFile> LoadSavedObjectFiles(const std::filesystem::path &folder) {
	std::unordered_map<uint64_t, SavedObjectFile> files;
	std::error_code error;
	for (const auto &entry : std::filesystem::directory_iterator(folder, error)) {
		if (!entry.is_regular_file(error) || entry.path().extension() != L".json") { continue; }

		const auto text = LoadText(entry.path());
		if (!text) { continue; }
		auto json = Json::parse(*text, nullptr, false, true);
		if (!json.is_object()) { continue; }

		SavedObjectFile file{entry.path()};
		if (const auto *transform = FindTransformJson(json)) { file.parentTransformId = ReadParentTransformId(*transform); }
		if (const auto *order = FindMember(json, kOrderKey); order && order->is_string()) { file.order = order->get<std::string>(); }
		files.insert_or_assign(ReadId(json), std::move(file));
	}
	return files;
}

// 読んだ GameObject のファイル
struct LoadedObjectFile {
	Json json;
	uint64_t gameObjectId = 0;
	uint64_t transformId = 0;
	uint64_t parentTransformId = 0;
	std::string order;
	bool isAdded = false;
};

// 親子と並びのとおりに、親を子より先に並べる（読むときに親から順に SetParent して、兄弟の並びが決まるため）
void AppendHierarchy(LoadedObjectFile &file, const std::unordered_map<uint64_t, std::vector<LoadedObjectFile *>> &childrenByParent, Json &out) {
	file.isAdded = true;
	const auto transformId = file.transformId;
	out.push_back(std::move(file.json));

	const auto it = childrenByParent.find(transformId);
	if (it == childrenByParent.end()) { return; }
	for (auto *child : it->second) {
		if (!child->isAdded) { AppendHierarchy(*child, childrenByParent, out); }
	}
}

void ClearParent(LoadedObjectFile &file) {
	file.parentTransformId = 0;
	if (auto *transform = FindTransformJson(file.json)) { (*transform)["parent"] = nullptr; }
}

Json LoadObjectsFolder(const std::filesystem::path &folder) {
	std::vector<std::filesystem::path> paths;
	std::error_code error;
	for (const auto &entry : std::filesystem::directory_iterator(folder, error)) {
		if (entry.is_regular_file(error) && entry.path().extension() == L".json") { paths.push_back(entry.path()); }
	}
	std::sort(paths.begin(), paths.end());

	std::vector<LoadedObjectFile> files;
	files.reserve(paths.size());
	for (const auto &path : paths) {
		auto json = JsonFile::Load(path);
		if (!json) { continue; }
		if (!json->is_object()) {
			Log::Warning("GameObject のファイルの一番外側がオブジェクトではありません: {}", ToUtf8(path));
			continue;
		}

		LoadedObjectFile file;
		if (const auto it = json->find(kOrderKey); it != json->end()) {
			if (it->is_string()) { file.order = it->get<std::string>(); }
			json->erase(it);
		}
		file.gameObjectId = ReadId(*json);
		if (const auto *transform = FindTransformJson(*json)) {
			file.transformId = ReadId(*transform);
			file.parentTransformId = ReadParentTransformId(*transform);
		}
		file.json = std::move(*json);
		files.push_back(std::move(file));
	}

	std::unordered_set<uint64_t> transformIds;
	for (const auto &file : files) {
		if (file.transformId != 0) { transformIds.insert(file.transformId); }
	}

	std::vector<LoadedObjectFile *> roots;
	std::unordered_map<uint64_t, std::vector<LoadedObjectFile *>> childrenByParent;
	for (auto &file : files) {
		if (file.parentTransformId != 0 && !transformIds.contains(file.parentTransformId)) {
			// 親だけ消され、子は別の人が触っていた、などで起きる
			Log::Warning("親の GameObject のファイルがないため、一番上に置きます: {}", ReadNameForLog(file.json));
			ClearParent(file);
		}
		(file.parentTransformId == 0 ? roots : childrenByParent[file.parentTransformId]).push_back(&file);
	}

	// 同じキーは 2 人が同じところへ足したときに起きる。どちらが先でもよいので、id で決める
	const auto isBefore = [](const LoadedObjectFile *a, const LoadedObjectFile *b) {
		return std::tie(a->order, a->gameObjectId) < std::tie(b->order, b->gameObjectId);
	};
	std::sort(roots.begin(), roots.end(), isBefore);
	for (auto &[parent, children] : childrenByParent) {
		std::sort(children.begin(), children.end(), isBefore);
	}

	auto gameObjects = Json::array();
	for (auto *root : roots) {
		AppendHierarchy(*root, childrenByParent, gameObjects);
	}

	// 一番上までたどれないものは、親子が輪になっている（2 人が互いを相手の子にした、などで起きる）
	for (auto &file : files) {
		if (file.isAdded) { continue; }
		Log::Warning("親子が輪になっているため、一番上に置きます: {}", ReadNameForLog(file.json));
		ClearParent(file);
		AppendHierarchy(file, childrenByParent, gameObjects);
	}
	return gameObjects;
}

} // namespace



Json ou::SceneSerializer::ToJson(Scene &scene) {
	std::vector<GameObject *> gameObjects;
	for (auto *root : scene.GetRootGameObjects()) {
		CollectHierarchy(root, &scene, gameObjects);
	}

	const auto objects = CollectObjects(scene);

	auto root = Json::object();
	root["version"] = kVersion;
	root["gameObjects"] = WriteGameObjects(gameObjects, objects.savedObjects);
	return root;
}


Json ou::SceneSerializer::ToJson(GameObject &root, bool keepOutsideReferences) {
	auto *scene = root.GetScene();
	auto objects = CollectObjects(*scene);

	std::vector<GameObject *> gameObjects;
	CollectHierarchy(&root, scene, gameObjects);

	if (!keepOutsideReferences) {
		std::unordered_set<const Object *> inside;
		for (auto *gameObject : gameObjects) {
			inside.insert(gameObject);
			for (const auto &component : gameObject->components_) {
				if (objects.savedObjects.contains(component.get())) { inside.insert(component.get()); }
			}
		}
		objects.savedObjects = std::move(inside);
	}

	if (auto *parent = root.GetTransform()->GetParent()) {
		objects.savedObjects.insert(parent);
	}

	auto json = Json::object();
	json["version"] = kVersion;
	json["gameObjects"] = WriteGameObjects(gameObjects, objects.savedObjects);

	if (!json["gameObjects"].empty()) {
		const auto transformId = root.GetTransform()->GetFileId();
		for (auto &componentJson : json["gameObjects"][0]["components"]) {
			if (componentJson[kIdKey] == transformId) { componentJson["parent"] = nullptr; }
		}
	}
	return json;
}


bool ou::SceneSerializer::Save(Scene &scene, const std::filesystem::path &path) {
	auto json = ToJson(scene);
	auto &gameObjects = json["gameObjects"];

	const auto folder = ToObjectsFolder(path);
	std::error_code error;
	std::filesystem::create_directories(folder, error);
	if (error) {
		Log::Error("フォルダを作れませんでした: {} ({})", ToUtf8(folder), error.message());
		return false;
	}
	const auto savedFiles = LoadSavedObjectFiles(folder);

	// 兄弟ごとに並びのキーを決める。gameObjects は親を子より先に、兄弟は今の並びで入っている
	std::vector<uint64_t> parentIds;
	std::unordered_map<uint64_t, std::vector<size_t>> indicesByParent;
	for (size_t i = 0; i < gameObjects.size(); i++) {
		const auto *transform = FindTransformJson(gameObjects[i]);
		const auto parentId = transform ? ReadParentTransformId(*transform) : 0;
		auto [it, isInserted] = indicesByParent.try_emplace(parentId);
		if (isInserted) { parentIds.push_back(parentId); }
		it->second.push_back(i);
	}

	std::vector<std::string> orders(gameObjects.size());
	for (const auto parentId : parentIds) {
		const auto &indices = indicesByParent[parentId];
		std::vector<std::string> keys(indices.size());
		for (size_t i = 0; i < indices.size(); i++) {
			const auto it = savedFiles.find(ReadId(gameObjects[indices[i]]));
			if (it != savedFiles.end() && it->second.parentTransformId == parentId) { keys[i] = it->second.order; }
		}
		AssignOrderKeys(keys);
		for (size_t i = 0; i < indices.size(); i++) {
			orders[indices[i]] = std::move(keys[i]);
		}
	}

	auto isSucceeded = true;
	std::unordered_set<uint64_t> savedIds;
	for (size_t i = 0; i < gameObjects.size(); i++) {
		auto &gameObject = gameObjects[i];
		const auto id = ReadId(gameObject);
		savedIds.insert(id);

		// $id と $order を先頭に置く（ファイルを開いたときに、どれがどこにあるかすぐ分かるように）
		auto file = Json::object();
		file[kIdKey] = id;
		file[kOrderKey] = orders[i];
		for (auto &[key, value] : gameObject.items()) {
			if (key != kIdKey) { file[key] = std::move(value); }
		}
		isSucceeded = SaveTextIfChanged(folder / ToObjectFileName(id), ToText(file)) && isSucceeded;
	}

	for (const auto &[id, file] : savedFiles) {
		if (savedIds.contains(id)) { continue; }
		std::filesystem::remove(file.path, error);
		if (error) {
			Log::Error("消した GameObject のファイルを消せませんでした: {} ({})", ToUtf8(file.path), error.message());
			isSucceeded = false;
		}
	}

	auto root = Json::object();
	root["version"] = kVersion;
	return SaveTextIfChanged(path, ToText(root)) && isSucceeded;
}


std::optional<Json> ou::SceneSerializer::LoadJson(const std::filesystem::path &path) {
	auto root = JsonFile::Load(path);
	if (!root || !root->is_object() || root->contains("gameObjects")) { return root; }

	(*root)["gameObjects"] = LoadObjectsFolder(ToObjectsFolder(path));
	return root;
}


std::filesystem::path ou::SceneSerializer::ToObjectsFolder(const std::filesystem::path &scenePath) {
	auto folder = scenePath;
	folder.replace_extension(L".objects");
	return folder;
}


bool ou::SceneSerializer::IsObjectsFolder(const std::filesystem::path &folder) {
	if (_wcsicmp(folder.extension().c_str(), L".objects") != 0) { return false; }

	auto scenePath = folder;
	scenePath.replace_extension(L".scene");
	std::error_code error;
	return std::filesystem::is_regular_file(scenePath, error);
}


bool ou::SceneSerializer::FromJson(Scene &scene, const Json &root) {
	if (!ValidateRoot(root)) { return false; }

	CreateGameObjects(scene, root, {}, true);
	return true;
}


bool ou::SceneSerializer::Load(Scene &scene, const std::filesystem::path &path) {
	const auto root = LoadJson(path);
	if (!root) { return false; }

	return FromJson(scene, *root);
}


bool ou::SceneSerializer::ValidateRoot(const Json &root) {
	if (!root.is_object()) {
		Log::Error("json の一番外側がオブジェクトではありません");
		return false;
	}

	const auto *version = FindMember(root, "version");
	if (!version || !version->is_number_integer() || version->get<int>() < 1 || version->get<int>() > kVersion) {
		Log::Error("対応していないシーンのバージョンです (対応しているのは {} まで)", kVersion);
		return false;
	}

	const auto *gameObjectsJson = FindMember(root, "gameObjects");
	if (!gameObjectsJson || !gameObjectsJson->is_array()) {
		Log::Error("シーンの json に \"gameObjects\" の配列がありません");
		return false;
	}

	return true;
}


SceneObjects ou::SceneSerializer::CollectObjects(Scene &scene) {
	std::vector<GameObject *> gameObjects;
	for (auto *root : scene.GetRootGameObjects()) {
		CollectHierarchy(root, &scene, gameObjects);
	}

	SceneObjects objects;
	std::unordered_set<uint64_t> usedIds;
	const auto add = [&](Object *object) {
		AssignFileId(object, usedIds);
		objects.savedObjects.insert(object);
		objects.objectsById.emplace(object->GetFileId(), object);
	};

	for (auto *gameObject : gameObjects) {
		add(gameObject);
		for (const auto &component : gameObject->components_) {
			const Object &object = *component;
			if (component->IsDestroyed() || !TypeRegistry::FindName(object)) { continue; }
			add(component.get());
		}
	}
	return objects;
}


GameObject *ou::SceneSerializer::Instantiate(GameObject &original, Scene &scene, const std::function<void(GameObject &)> &place) {
	const auto outside = CollectObjects(*original.GetScene()).objectsById;
	const auto json = ToJson(original, true);

	const auto created = CreateGameObjects(scene, json, outside, false);
	if (created.empty()) { return nullptr; }

	auto *clone = created.front();
	clone->SetName(original.GetName() + "(Clone)");

	place(*clone);
	clone->RefreshActiveInHierarchy();
	return clone;
}


GameObject *ou::SceneSerializer::InstantiateFromJson(const Json &json, Scene &scene, const std::function<void(GameObject &)> &place) {
	if (!ValidateRoot(json)) { return nullptr; }

	// 作ったものが先に登録されるので、json の中の id は外のものより新しいものが優先される
	const auto outside = CollectObjects(scene).objectsById;
	const auto created = CreateGameObjects(scene, json, outside, false);
	if (created.empty()) { return nullptr; }

	for (auto *gameObject : created) {
		gameObject->SetFileId(0);
		for (const auto &component : gameObject->components_) {
			component->SetFileId(0);
		}
	}

	auto *root = created.front();
	place(*root);

#ifdef WITH_EDITOR
	// シーンを読んだときと同じく、エディタで作ったときにも OnValidate を呼ぶ
	for (auto *gameObject : created) {
		for (const auto &component : gameObject->components_) {
			component->OnValidate();
		}
	}
#endif

	root->RefreshActiveInHierarchy();
	return root;
}


std::vector<GameObject *> ou::SceneSerializer::CreateGameObjects(Scene &scene, const Json &root, const std::unordered_map<uint64_t, Object *> &outside, bool awake) {
	const auto *gameObjectsJson = FindMember(root, "gameObjects");
	std::vector<std::pair<GameObject *, const Json *>> loadedGameObjects;
	std::vector<std::pair<Component *, const Json *>> loadedComponents;
	std::unordered_map<uint64_t, Object *> objectsById;

	for (const auto &gameObjectJson : *gameObjectsJson) {
		if (!gameObjectJson.is_object()) {
			Log::Warning("\"gameObjects\" の要素がオブジェクトではありません");
			continue;
		}

		auto *gameObject = scene.CreateGameObject();
		gameObject->isActiveSelf_ = false;
		RegisterFileId(gameObject, gameObjectJson, objectsById);
		loadedGameObjects.emplace_back(gameObject, &gameObjectJson);

		const auto *componentsJson = FindMember(gameObjectJson, "components");
		if (!componentsJson || !componentsJson->is_array()) { continue; }

		bool hasTransform = false;
		for (const auto &componentJson : *componentsJson) {
			const auto *type = componentJson.is_object() ? FindMember(componentJson, kTypeKey) : nullptr;
			if (!type || !type->is_string()) {
				Log::Warning("Component に \"type\" がありません (無視します)");
				continue;
			}

			const auto typeName = type->get<std::string>();
			auto created = TypeRegistry::Create(typeName);
			if (!created) {
				Log::Warning("型が登録されてないため読み込めません: {}", typeName);
				continue;
			}

			Component *component = nullptr;
			if (dynamic_cast<Transform *>(created.get())) {
				if (hasTransform) {
					Log::Warning("Transform が 2 つあります: {}", typeName);
					continue;
				}
				hasTransform = true;

				const Component &createdRef = *created;
				if (typeid(createdRef) == typeid(Transform)) {
					component = gameObject->GetTransform();
				} else {
					component = created.get();
					gameObject->ReplaceTransform(std::unique_ptr<Transform>(static_cast<Transform *>(created.release())));
				}
			} else {
				component = gameObject->AttachComponent(std::move(created));
			}

			RegisterFileId(component, componentJson, objectsById);
			loadedComponents.emplace_back(component, &componentJson);
		}
	}

	for (const auto &[id, object] : outside) {
		objectsById.emplace(id, object);
	}

	for (const auto &[component, json] : loadedComponents) {
		JsonReader reader(*json, objectsById);
		component->Serialize(reader);
	}

	for (const auto &[component, json] : loadedComponents) {
		component->AwakeFromLoad();
	}

	for (const auto &[gameObject, json] : loadedGameObjects) {
		gameObject->isActiveSelf_ = true;

		JsonReader reader(*json, objectsById);
		gameObject->Serialize(reader);
		gameObject->AwakeFromLoad();
	}

	std::vector<GameObject *> created;
	created.reserve(loadedGameObjects.size());
	for (const auto &[gameObject, json] : loadedGameObjects) {
		created.push_back(gameObject);
	}

#ifdef WITH_EDITOR
	// Unity と同じく、エディタではシーンを読んだときにも OnValidate を呼ぶ（Instantiate やプレハブの読み込みでは呼ばない）
	if (awake) {
		for (const auto &[component, json] : loadedComponents) {
			component->OnValidate();
		}
	}
#endif

	if (awake) {
		for (auto *gameObject : created) {
			if (!gameObject->GetTransform()->GetParent()) {
				gameObject->RefreshActiveInHierarchy();
			}
		}
	}

	return created;
}


void ou::SceneSerializer::RestoreGameObjects(Scene &scene, const std::vector<RestoringGameObject> &targets) {
	auto objectsById = CollectObjects(scene).objectsById;
	const auto findGameObject = [&objectsById](uint64_t id) -> GameObject * {
		const auto it = objectsById.find(id);
		return it != objectsById.end() ? dynamic_cast<GameObject *>(it->second) : nullptr;
	};

	// 先に、今あるものの Component をそろえ、ないものを作る（値を読むときに、どの参照先もそろっているように）
	std::vector<std::pair<GameObject *, const Json *>> updated;
	std::vector<GameObject *> removed;
	auto creating = Json::object();
	creating["gameObjects"] = Json::array();
	for (const auto &target : targets) {
		auto *gameObject = findGameObject(target.id);
		if (!target.json) {
			if (gameObject) { removed.push_back(gameObject); }
		} else if (!gameObject) {
			creating["gameObjects"].push_back(*target.json);
		} else {
			MatchComponents(*gameObject, *target.json, objectsById);
			updated.emplace_back(gameObject, target.json);
		}
	}

	std::vector<GameObject *> created;
	if (!creating["gameObjects"].empty()) {
		created = CreateGameObjects(scene, creating, objectsById, false);
		for (auto *gameObject : created) {
			objectsById.insert_or_assign(gameObject->GetFileId(), gameObject);
			for (const auto &component : gameObject->components_) {
				objectsById.insert_or_assign(component->GetFileId(), component.get());
			}
		}
	}

	// CreateGameObjects と同じく、Component の値 → Component の AwakeFromLoad → GameObject の順に読む
	std::vector<Component *> loadedComponents;
	for (const auto &[gameObject, json] : updated) {
		const auto *componentsJson = FindMember(*json, "components");
		if (!componentsJson || !componentsJson->is_array()) { continue; }
		for (const auto &componentJson : *componentsJson) {
			const auto it = objectsById.find(ReadId(componentJson));
			auto *component = it != objectsById.end() ? dynamic_cast<Component *>(it->second) : nullptr;
			if (!component || component->GetGameObject() != gameObject) { continue; }

			JsonReader reader(componentJson, objectsById);
			component->Serialize(reader);
			loadedComponents.push_back(component);
		}
	}
	for (auto *component : loadedComponents) {
		component->AwakeFromLoad();
	}
	for (const auto &[gameObject, json] : updated) {
		JsonReader reader(*json, objectsById);
		gameObject->Serialize(reader);
		gameObject->AwakeFromLoad();
	}

	// 親は AwakeFromLoad でつないだので、そのあとで並べる。動いた兄弟はみな targets にあるので、小さい順に当てれば元の並びになる
	std::vector<std::pair<int, Transform *>> placements;
	for (const auto &target : targets) {
		if (!target.json) { continue; }
		if (auto *gameObject = findGameObject(target.id)) { placements.emplace_back(target.siblingIndex, gameObject->GetTransform()); }
	}
	std::stable_sort(placements.begin(), placements.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
	for (const auto &[siblingIndex, transform] : placements) {
		transform->SetSiblingIndex(siblingIndex);
	}

#ifdef WITH_EDITOR
	// Inspector で値を変えたときと同じく呼ぶ
	for (auto *component : loadedComponents) {
		component->OnValidate();
	}
	for (auto *gameObject : created) {
		for (const auto &component : gameObject->components_) {
			component->OnValidate();
		}
	}
#endif

	for (const auto &[gameObject, json] : updated) {
		gameObject->RefreshActiveInHierarchy();
	}
	for (auto *gameObject : created) {
		gameObject->RefreshActiveInHierarchy();
	}

	// 子を別の親へ移してから消す（先に消すと、残すはずの子まで一緒に消える）
	for (auto *gameObject : removed) {
		Object::Destroy(gameObject);
	}
	scene.DestroyMarkedObjects();
}


void ou::SceneSerializer::MatchComponents(GameObject &gameObject, const Json &gameObjectJson, std::unordered_map<uint64_t, Object *> &objectsById) {
	static const auto kEmpty = Json::array();
	const auto *found = FindMember(gameObjectJson, "components");
	const auto &componentsJson = found && found->is_array() ? *found : kEmpty;

	std::unordered_map<uint64_t, size_t> positions;
	for (size_t i = 0; i < componentsJson.size(); i++) {
		positions.emplace(ReadId(componentsJson[i]), i);
	}

	for (const auto &component : gameObject.components_) {
		const Object &object = *component;
		// 登録していない型は保存されないので、json になくても消さない
		if (component.get() == gameObject.transform_ || component->IsDestroyed() || !TypeRegistry::FindName(object)) { continue; }
		if (!positions.contains(component->GetFileId())) { Object::Destroy(component.get()); }
	}

	for (const auto &componentJson : componentsJson) {
		const auto id = ReadId(componentJson);
		if (id == 0 || objectsById.contains(id)) { continue; }

		const auto *type = FindMember(componentJson, kTypeKey);
		auto created = type && type->is_string() ? TypeRegistry::Create(type->get<std::string>()) : nullptr;
		if (!created) {
			Log::Warning("型が登録されてないため戻せません: {}", type && type->is_string() ? type->get<std::string>() : "");
			continue;
		}

		Component *component = created.get();
		if (dynamic_cast<Transform *>(component)) {
			// RectTransform を足した・外した操作を戻すとき
			objectsById.erase(gameObject.transform_->GetFileId());
			gameObject.ReplaceTransform(std::unique_ptr<Transform>(static_cast<Transform *>(created.release())));
		} else {
			gameObject.AttachComponent(std::move(created));
		}
		component->SetFileId(id);
		objectsById.insert_or_assign(id, component);
	}

	// Component の並びも json にそろえる（Transform は先頭のまま）
	std::stable_sort(gameObject.components_.begin() + 1, gameObject.components_.end(), [&positions](const auto &a, const auto &b) {
		const auto positionOf = [&positions](const Component *component) {
			const auto it = positions.find(component->GetFileId());
			return it != positions.end() ? it->second : SIZE_MAX;
		};
		return positionOf(a.get()) < positionOf(b.get());
	});
}


Json ou::SceneSerializer::WriteGameObjects(const std::vector<GameObject *> &gameObjects, const std::unordered_set<const Object *> &savedObjects) {
	auto gameObjectsJson = Json::array();
	for (auto *gameObject : gameObjects) {
		auto gameObjectJson = Json::object();
		gameObjectJson[kIdKey] = gameObject->GetFileId();
		JsonWriter gameObjectWriter(gameObjectJson, savedObjects);
		gameObject->Serialize(gameObjectWriter);

		auto componentsJson = Json::array();
		for (const auto &component : gameObject->components_) {
			if (component->IsDestroyed()) { continue; }

			const Object &object = *component;
			const auto *typeName = TypeRegistry::FindName(object);
			if (!typeName) {
				Log::Warning("型が登録されてないため保存できません: {} ({})",
							 typeid(object).name(), gameObject->GetName());
				continue;
			}

			auto componentJson = Json::object();
			componentJson[kTypeKey] = *typeName;
			componentJson[kIdKey] = component->GetFileId();
			JsonWriter componentWriter(componentJson, savedObjects);
			component->Serialize(componentWriter);

			componentsJson.push_back(std::move(componentJson));
		}

		gameObjectJson["components"] = std::move(componentsJson);
		gameObjectsJson.push_back(std::move(gameObjectJson));
	}
	return gameObjectsJson;
}
