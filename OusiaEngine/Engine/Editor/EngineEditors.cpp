#include "EngineEditors.h"
#include "Editor/EditorGuiLayout.h"
#include "Editor/EditorGuiUtility.h"
#include "Editor/ParticleEffectPreview.h"
#include "Editor/UnityEventDrawer.h"

#include <algorithm>
#include <format>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <imgui/imgui.h>

using namespace ou;


namespace {

// path の欄を描く。ラベルは日本語、ツールチップは英語の名前（スクリプトで使う名前）
void Field(SerializedObject &so, std::string_view path, std::string_view label) {
	if (auto property = so.FindProperty(path)) {
		EditorGuiLayout::PropertyField(*property, EditorGuiUtility::TrTextContent(label, property->GetName()), true);
	}
}

// Enum の選択肢の説明
struct EnumOption {
	std::string_view name;
	std::string_view tooltip;
};

// Enum の欄。選択肢は英語の名前のまま（スクリプトで書く名前）で、説明をツールチップに出す。Count は出さない
void EnumField(SerializedObject &so, std::string_view path, std::string_view label, std::initializer_list<EnumOption> options) {
	auto property = so.FindProperty(path);
	if (!property) { return; }

	const auto names = property->GetEnumNames();
	std::vector<GuiContent> contents;
	std::vector<int> indices;
	for (int i = 0; i < static_cast<int>(names.size()); i++) {
		if (names[i] == "Count") { continue; }

		std::string_view tooltip;
		for (const auto &option : options) {
			if (option.name == names[i]) { tooltip = option.tooltip; }
		}
		contents.push_back(EditorGuiUtility::TrTextContent(names[i], tooltip));
		indices.push_back(i);
	}

	const auto it = std::find(indices.begin(), indices.end(), property->GetEnumValueIndex());
	const auto selected = it != indices.end() ? static_cast<int>(it - indices.begin()) : -1;
	const auto next = EditorGuiLayout::Popup(EditorGuiUtility::TrTextContent(label, property->GetName()), selected, contents);
	if (next != selected && next >= 0) { property->SetEnumValueIndex(indices[next]); }
}

// 今の Enum の名前（欄の出し分けに使う）
std::string EnumName(SerializedObject &so, std::string_view path) {
	const auto property = so.FindProperty(path);
	return property ? property->GetStringValue() : std::string();
}

// 入れ子・配列の見出し。開いていれば字下げして true を返す（そのときは呼ぶ側が EndFoldout を呼ぶ）
bool BeginFoldout(SerializedObject &so, std::string_view path, std::string_view label) {
	auto property = so.FindProperty(path);
	if (!property || !EditorGuiLayout::PropertyField(*property, EditorGuiUtility::TrTextContent(label, property->GetName()), false)) { return false; }

	ImGui::Indent();
	return true;
}

void EndFoldout() {
	ImGui::Unindent();
}

// 配列の大きさの欄
void ArraySizeField(SerializedObject &so, std::string_view path) {
	auto property = so.FindProperty(path);
	if (!property) { return; }

	ImGui::PushID(property->GetPropertyPath().c_str());
	auto size = property->GetArraySize();
	EditorGuiLayout::PrefixLabel(EditorGuiUtility::TrTextContent("数", "size"));
	if (ImGui::DragInt("##Size", &size, 0.1f, 0, 1024)) { property->SetArraySize(std::max(size, 0)); }
	ImGui::PopID();
}

// 最小と最大を持つ欄（ParticleSystem の初速など）
void RangeField(SerializedObject &so, std::string_view path, std::string_view label) {
	if (!BeginFoldout(so, path, label)) { return; }

	Field(so, std::string(path) + "/min", "最小");
	Field(so, std::string(path) + "/max", "最大");
	EndFoldout();
}


// ---- いくつかのコンポーネントで同じ欄 ----

void BehaviorFields(SerializedObject &so) {
	Field(so, "enabled", "有効");
}

void ColliderFields(SerializedObject &so) {
	BehaviorFields(so);
	Field(so, "isTrigger", "トリガー");
	Field(so, "material", "物理マテリアル");
}

void RendererFields(SerializedObject &so) {
	BehaviorFields(so);
	Field(so, "materials", "マテリアル");
}

void GraphicFields(SerializedObject &so) {
	BehaviorFields(so);
	Field(so, "material", "マテリアル");
	Field(so, "color", "色");
	Field(so, "raycastTarget", "クリックを受ける");
}

void TransformFields(SerializedObject &so) {
	Field(so, "localPosition", "位置");
	Field(so, "localRotation", "回転");
	Field(so, "localScale", "スケール");
	Field(so, "parent", "親");
}

} // namespace



// ---- Scene ----

void ou::TransformEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	TransformFields(so);
	so.ApplyModifiedProperties();
}


void ou::RectTransformEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	TransformFields(so);
	Field(so, "anchorMin", "アンカーの最小");
	Field(so, "anchorMax", "アンカーの最大");
	Field(so, "anchoredPosition", "アンカーからの位置");
	Field(so, "sizeDelta", "大きさ（アンカーとの差）");
	Field(so, "pivot", "ピボット");
	so.ApplyModifiedProperties();
}


// ---- Render ----

void ou::CameraEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	BehaviorFields(so);
	Field(so, "fieldOfView", "視野角（度）");
	Field(so, "aspect", "アスペクト比（0 で自動）");
	Field(so, "nearClipPlane", "描く近さの限界");
	Field(so, "farClipPlane", "描く遠さの限界");
	so.ApplyModifiedProperties();
}


void ou::LightEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	BehaviorFields(so);
	EnumField(so, "type", "種類", {
		{"Spot", "スポットライト。向いている方向の円錐の中を照らす"},
		{"Directional", "太陽のような平行な光。位置は関係なく、向きだけで照らす"},
		{"Point", "電球のように、位置から全方向を照らす"},
	});
	Field(so, "color", "色");
	Field(so, "intensity", "強さ");

	// Unity と同じく、種類に関係のない欄は出さない
	const auto type = EnumName(so, "type");
	if (type != "Directional") { Field(so, "range", "届く距離"); }
	if (type == "Spot") {
		Field(so, "spotAngle", "スポットの角度（度）");
		Field(so, "innerSpotAngle", "内側の角度（度）");
	}
	so.ApplyModifiedProperties();
}


void ou::MeshRendererEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	RendererFields(so);
	Field(so, "mesh", "メッシュ");
	so.ApplyModifiedProperties();
}


void ou::SkinnedMeshRendererEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	RendererFields(so);
	Field(so, "mesh", "メッシュ");
	Field(so, "boneRoot", "ボーンの根");
	Field(so, "bonePaths", "ボーンのパス");
	so.ApplyModifiedProperties();
}


void ou::LineRendererEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	RendererFields(so);
	Field(so, "positions", "点");
	Field(so, "startWidth", "始めの幅");
	Field(so, "endWidth", "終わりの幅");
	Field(so, "widthMultiplier", "幅の倍率");
	Field(so, "startColor", "始めの色");
	Field(so, "endColor", "終わりの色");
	Field(so, "useWorldSpace", "ワールド座標で使う");
	Field(so, "loop", "最後の点を最初につなぐ");
	so.ApplyModifiedProperties();
}


void ou::SkeletalAnimatorEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	BehaviorFields(so);
	Field(so, "clip", "クリップ");
	Field(so, "playAutomatically", "自動で再生");
	EnumField(so, "wrapMode", "終わったとき", {
		{"Default", "クリップの設定に従う"},
		{"Once", "1 回再生して止まり、最初のポーズに戻る"},
		{"Loop", "最初から繰り返す"},
		{"PingPong", "最後まで行ったら逆に再生し、往復を繰り返す"},
		{"ClampForever", "1 回再生して、最後のポーズのまま止まる"},
	});
	Field(so, "speed", "速さ");
	so.ApplyModifiedProperties();
}


// ---- Physics ----

void ou::BoxColliderEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	ColliderFields(so);
	Field(so, "center", "中心");
	Field(so, "size", "大きさ");
	so.ApplyModifiedProperties();
}


void ou::SphereColliderEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	ColliderFields(so);
	Field(so, "center", "中心");
	Field(so, "radius", "半径");
	so.ApplyModifiedProperties();
}


void ou::CapsuleColliderEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	ColliderFields(so);
	Field(so, "center", "中心");
	Field(so, "radius", "半径");
	Field(so, "height", "高さ");
	Field(so, "direction", "向き（0: X・1: Y・2: Z）");
	so.ApplyModifiedProperties();
}


void ou::CharacterControllerEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	// Unity と同じく、トリガーと物理マテリアルは出さない
	BehaviorFields(so);
	Field(so, "slopeLimit", "登れる坂の角度（度）");
	Field(so, "stepOffset", "登れる段差の高さ");
	Field(so, "skinWidth", "当たりの余白");
	Field(so, "minMoveDistance", "これより短い移動は無視");
	Field(so, "center", "中心");
	Field(so, "radius", "半径");
	Field(so, "height", "高さ");
	so.ApplyModifiedProperties();
}


void ou::RigidbodyEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	Field(so, "mass", "質量");
	Field(so, "useGravity", "重力を使う");
	Field(so, "isKinematic", "キネマティック");
	Field(so, "linearDamping", "移動の減衰");
	Field(so, "angularDamping", "回転の減衰");
	Field(so, "maxLinearVelocity", "最大の速さ");
	Field(so, "maxAngularVelocity", "最大の回転の速さ");
	Field(so, "sleepThreshold", "止まったとみなす速さ");
	EnumField(so, "collisionDetectionMode", "衝突の判定", {
		{"Discrete", "1 ステップごとの位置だけで判定する。いちばん軽いが、速い物体は薄い壁をすり抜けることがある"},
		{"Continuous", "動かない物体に対して、すり抜けを防ぐ"},
		{"ContinuousDynamic", "動く物体どうしでも、すり抜けを防ぐ。いちばん重い"},
		{"ContinuousSpeculative", "先を読んで、すり抜けを防ぐ。回転にも効くが、当たる前に止まって見えることがある"},
	});
	EnumField(so, "interpolation", "補間", {
		{"None", "補間しない。物理の更新が描画より少ないと、カクついて見えることがある"},
		{"Interpolate", "前の物理の更新との間をなめらかにつなぐ。少し遅れて見える"},
		{"Extrapolate", "速度から先を予測してなめらかにする。ぶつかったときに行き過ぎて見えることがある"},
	});
	Field(so, "constraints", "固定する軸");
	so.ApplyModifiedProperties();
}


// ---- Audio ----

void ou::AudioSourceEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	Field(so, "clip", "音");
	EnumField(so, "submixType", "音量の分類", {
		{"SoundEffect", "効果音。効果音の音量の設定が効く"},
		{"Music", "BGM。BGM の音量の設定が効く"},
	});
	Field(so, "mute", "ミュート");
	Field(so, "playOnAwake", "始まったら再生");
	Field(so, "loop", "繰り返す");
	Field(so, "volume", "音量（0～1）");
	Field(so, "pitch", "ピッチ（1 で元の高さ）");
	so.ApplyModifiedProperties();
}


void ou::AudioListenerEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	BehaviorFields(so);
	so.ApplyModifiedProperties();
}


// ---- UI ----

void ou::CanvasEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	BehaviorFields(so);
	so.ApplyModifiedProperties();
}


void ou::CanvasRendererEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	BehaviorFields(so);
	so.ApplyModifiedProperties();
}


void ou::GraphicRaycasterEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	BehaviorFields(so);
	so.ApplyModifiedProperties();
}


void ou::ImageEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	GraphicFields(so);
	if (BeginFoldout(so, "sprite", "スプライト")) {
		Field(so, "sprite/texture", "テクスチャ");
		Field(so, "sprite/rect", "切り出す範囲（ピクセル）");
		Field(so, "sprite/pivot", "ピボット");
		EndFoldout();
	}
	so.ApplyModifiedProperties();
}


void ou::TextEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	GraphicFields(so);
	Field(so, "text", "文字");
	Field(so, "font", "フォント");
	Field(so, "fontSize", "文字の大きさ");
	EnumField(so, "alignment", "揃え", {
		{"TopLeft", "左上"},
		{"Top", "上・左右の中央"},
		{"TopRight", "右上"},
		{"Left", "左・上下の中央"},
		{"Center", "真ん中"},
		{"Right", "右・上下の中央"},
		{"BottomLeft", "左下"},
		{"Bottom", "下・左右の中央"},
		{"BottomRight", "右下"},
	});
	Field(so, "wordWrappingEnabled", "折り返す");
	Field(so, "lineSpacing", "行の間隔");
	Field(so, "characterSpacing", "文字の間隔");
	Field(so, "faceDilate", "文字の太さ");
	Field(so, "outlineWidth", "縁取りの幅");
	Field(so, "outlineColor", "縁取りの色");
	Field(so, "underlayDilate", "影の太さ");
	Field(so, "underlaySoftness", "影のぼかし");
	Field(so, "underlayOffset", "影のずれ");
	Field(so, "underlayColor", "影の色");
	so.ApplyModifiedProperties();
}


void ou::ButtonEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	BehaviorFields(so);
	Field(so, "interactable", "押せる");
	Field(so, "targetGraphic", "色を変える Graphic");
	if (BeginFoldout(so, "colors", "色")) {
		Field(so, "colors/normalColor", "ふだん");
		Field(so, "colors/highlightedColor", "マウスが乗ったとき");
		Field(so, "colors/pressedColor", "押しているとき");
		Field(so, "colors/selectedColor", "選ばれているとき");
		Field(so, "colors/disabledColor", "押せないとき");
		Field(so, "colors/colorMultiplier", "色の倍率");
		Field(so, "colors/fadeDuration", "色が変わる時間（秒）");
		EndFoldout();
	}
	UnityEventDrawer::Draw(so, "onClick", "押したとき（On Click ()）");
	so.ApplyModifiedProperties();
}


void ou::EventSystemEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	BehaviorFields(so);
	Field(so, "firstSelected", "最初に選ぶもの");
	so.ApplyModifiedProperties();
}


void ou::StandaloneInputModuleEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	BehaviorFields(so);
	Field(so, "upKeys", "上へ移るキー");
	Field(so, "downKeys", "下へ移るキー");
	Field(so, "submitKeys", "決定のキー");
	so.ApplyModifiedProperties();
}


// ---- Particle ----

void ou::ParticleSystemEditor::OnSceneGui(const Rect &sceneRect) {
	// 動かすのは ParticleEffectPreview（選択を見て毎フレーム進める）。ここはパネルを描くだけ
	ParticleEffectPreview::DrawPanel(sceneRect);
}


void ou::ParticleSystemEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	BehaviorFields(so);

	if (BeginFoldout(so, "main", "メイン")) {
		Field(so, "main/duration", "1 回の長さ（秒）");
		Field(so, "main/looping", "繰り返す");
		Field(so, "main/playOnAwake", "始まったら再生");
		RangeField(so, "main/startLifetime", "寿命（秒）");
		RangeField(so, "main/startSpeed", "初めの速さ");
		RangeField(so, "main/startSize", "初めの大きさ");
		RangeField(so, "main/startColor", "初めの色");
		Field(so, "main/gravityModifier", "重力の倍率");
		Field(so, "main/maxParticles", "最大の数");
		EnumField(so, "main/simulationSpace", "動かす座標", {
			{"Local", "出たあとも Transform に付いて動く"},
			{"World", "出たあとは、Transform が動いてもその場に残る"},
		});
		EndFoldout();
	}

	if (BeginFoldout(so, "emission", "放出")) {
		Field(so, "emission/enabled", "有効");
		Field(so, "emission/rateOverTime", "1 秒に出す数");
		if (BeginFoldout(so, "emission/bursts", "まとめて出す")) {
			ArraySizeField(so, "emission/bursts");
			const auto bursts = so.FindProperty("emission/bursts");
			const auto count = bursts ? bursts->GetArraySize() : 0;
			for (int i = 0; i < count; i++) {
				const auto path = std::format("emission/bursts/{}", i);
				if (BeginFoldout(so, path, std::format("{} 回目", i + 1))) {
					Field(so, path + "/time", "出す時間（秒）");
					Field(so, path + "/count", "出す数");
					EndFoldout();
				}
			}
			EndFoldout();
		}
		EndFoldout();
	}

	if (BeginFoldout(so, "shape", "形")) {
		Field(so, "shape/enabled", "有効");
		EnumField(so, "shape/shapeType", "形", {
			{"Sphere", "球の中から、全方向へ出す"},
			{"Cone", "円錐の向きへ出す"},
		});
		if (EnumName(so, "shape/shapeType") == "Cone") { Field(so, "shape/angle", "広がる角度（度）"); }
		Field(so, "shape/radius", "半径");
		Field(so, "shape/radiusThickness", "出す厚さ（0 で表面だけ、1 で中まで）");
		EndFoldout();
	}

	if (BeginFoldout(so, "colorOverLifetime", "寿命での色の変化")) {
		Field(so, "colorOverLifetime/enabled", "有効");
		Field(so, "colorOverLifetime/start", "始め");
		Field(so, "colorOverLifetime/end", "終わり");
		EndFoldout();
	}

	if (BeginFoldout(so, "sizeOverLifetime", "寿命での大きさの変化")) {
		Field(so, "sizeOverLifetime/enabled", "有効");
		Field(so, "sizeOverLifetime/start", "始め");
		Field(so, "sizeOverLifetime/end", "終わり");
		EndFoldout();
	}

	so.ApplyModifiedProperties();
}


void ou::ParticleSystemRendererEditor::OnInspectorGui() {
	auto &so = *GetSerializedObject();
	so.Update();
	RendererFields(so);
	EnumField(so, "blendMode", "重ね方", {
		{"Additive", "色を足して明るくする。光や炎に向く"},
		{"Subtract", "色を引いて暗くする"},
		{"Multiply", "色を掛ける。煙や影に向く"},
		{"Alpha", "透明度で重ねる。ふつうの半透明"},
	});
	EnumField(so, "sortMode", "描く順番", {
		{"None", "並べ替えない。いちばん軽い"},
		{"Distance", "カメラから遠いものから描く"},
		{"OldestInFront", "古いものを手前に描く"},
		{"YoungestInFront", "新しいものを手前に描く"},
	});
	so.ApplyModifiedProperties();
}
