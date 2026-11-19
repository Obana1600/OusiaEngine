# スクリプト API リファレンス

[エディターガイド](README.md) · [スクリプトの書き方](Scripting.md)

OusiaEditor が AngelScript に公開する API を、型と名前空間ごとに引けます。MonoBehavior 自身のプロパティ・メソッドとエンジンが呼び出すコールバックは [スクリプトの書き方](Scripting.md) を参照してください。

表の用例は各 API を示す独立した断片です。たとえば `t` は設定済みの `Transform@`、`body` は `Rigidbody@` です。手元にない参照は Inspector で設定するか、`gameObject.GetComponent<T>()` で取得します。例で指定するタグは Project Settings の Tags and Layers の定義に合わせてください。

プロパティの読み書きは `t.position` や `Time::timeScale` の構文で行います。`&in` は入力用の参照、`&out` は値を受け取る変数を指定します。上位型に共通する API の用例は該当節にあります。`opAdd` などの宣言には、スクリプトで使う演算子の形を載せています。

- **オブジェクトとシーン** — [Object](#object)、[GameObject](#gameobject)、[Component](#component)、[Behavior](#behavior)、[Transform](#transform)、[Scene](#scene)、[SceneManager](#scenemanager)
- **物理** — [Physics](#physics)、[Collider](#collider)、[BoxCollider](#boxcollider)、[SphereCollider](#spherecollider)、[CapsuleCollider](#capsulecollider)、[Rigidbody](#rigidbody)、[CharacterController](#charactercontroller)、[Collision](#collision)、[ContactPoint](#contactpoint)、[RaycastHit](#raycasthit)
- **描画・音声・UI** — [Renderer](#renderer)、[MeshRenderer](#meshrenderer)、[Material](#material)、[AudioSource](#audiosource)、[AudioClip](#audioclip)、[Texture2D](#texture2d)、[Graphic](#graphic)、[Text](#text)、[Image](#image)、[Selectable](#selectable)、[Button](#button)、[UnityEvent](#unityevent)
- **入力・時間・画面・診断** — [Input](#input)、[Time](#time)、[Screen](#screen)、[Cursor](#cursor)、[Application](#application)、[Debug](#debug)、[Gizmos](#gizmos)
- **値型・文字列・配列** — [Vector2](#vector2)、[Vector3](#vector3)、[Quaternion](#quaternion)、[Color](#color)、[Mathf](#mathf)、[Random](#random)、[string](#string)、[array](#array)、[グローバル関数と関数型](#グローバル関数と関数型)
- **列挙型** — [Space](#space)、[ForceMode](#forcemode)、[CollisionFlags](#collisionflags)、[FullScreenMode](#fullscreenmode)、[CursorLockMode](#cursorlockmode)、[CursorMode](#cursormode)、[KeyCode](#keycode)
- **自分のクラス** — [スクリプトでプロパティを定義する](#スクリプトでプロパティを定義する)

## オブジェクトとシーン

### Object

エンジンオブジェクトに共通する識別と名前、および生成・破棄の入口。

| API | 用例 | 解説 |
| --- | --- | --- |
| `string name（読み書き）` | `obj.name = "Player"; Debug::Log(obj.name);` | オブジェクトの名前を読み書きする。Component の場合は所属する GameObject の名前。 |
| `int GetInstanceID() const` | `int id = obj.GetInstanceID();` | 実行中の個体を識別する ID を返す。シーンの保存や次回起動の識別には使わない。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `GameObject@ opCast() / const GameObject@ opCast() const` | `GameObject@ derived = cast<GameObject>(obj); const Object@ original = obj; const GameObject@ view = cast<const GameObject@>(original);` | 実体を GameObject として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Transform@ opCast() / const Transform@ opCast() const` | `Transform@ derived = cast<Transform>(obj); const Object@ original = obj; const Transform@ view = cast<const Transform@>(original);` | 実体を Transform として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `CharacterController@ opCast() / const CharacterController@ opCast() const` | `CharacterController@ derived = cast<CharacterController>(obj); const Object@ original = obj; const CharacterController@ view = cast<const CharacterController@>(original);` | 実体を CharacterController として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `BoxCollider@ opCast() / const BoxCollider@ opCast() const` | `BoxCollider@ derived = cast<BoxCollider>(obj); const Object@ original = obj; const BoxCollider@ view = cast<const BoxCollider@>(original);` | 実体を BoxCollider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `SphereCollider@ opCast() / const SphereCollider@ opCast() const` | `SphereCollider@ derived = cast<SphereCollider>(obj); const Object@ original = obj; const SphereCollider@ view = cast<const SphereCollider@>(original);` | 実体を SphereCollider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `CapsuleCollider@ opCast() / const CapsuleCollider@ opCast() const` | `CapsuleCollider@ derived = cast<CapsuleCollider>(obj); const Object@ original = obj; const CapsuleCollider@ view = cast<const CapsuleCollider@>(original);` | 実体を CapsuleCollider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Collider@ opCast() / const Collider@ opCast() const` | `Collider@ derived = cast<Collider>(obj); const Object@ original = obj; const Collider@ view = cast<const Collider@>(original);` | 実体を Collider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Rigidbody@ opCast() / const Rigidbody@ opCast() const` | `Rigidbody@ derived = cast<Rigidbody>(obj); const Object@ original = obj; const Rigidbody@ view = cast<const Rigidbody@>(original);` | 実体を Rigidbody として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `AudioSource@ opCast() / const AudioSource@ opCast() const` | `AudioSource@ derived = cast<AudioSource>(obj); const Object@ original = obj; const AudioSource@ view = cast<const AudioSource@>(original);` | 実体を AudioSource として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `MeshRenderer@ opCast() / const MeshRenderer@ opCast() const` | `MeshRenderer@ derived = cast<MeshRenderer>(obj); const Object@ original = obj; const MeshRenderer@ view = cast<const MeshRenderer@>(original);` | 実体を MeshRenderer として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Renderer@ opCast() / const Renderer@ opCast() const` | `Renderer@ derived = cast<Renderer>(obj); const Object@ original = obj; const Renderer@ view = cast<const Renderer@>(original);` | 実体を Renderer として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Text@ opCast() / const Text@ opCast() const` | `Text@ derived = cast<Text>(obj); const Object@ original = obj; const Text@ view = cast<const Text@>(original);` | 実体を Text として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Image@ opCast() / const Image@ opCast() const` | `Image@ derived = cast<Image>(obj); const Object@ original = obj; const Image@ view = cast<const Image@>(original);` | 実体を Image として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Graphic@ opCast() / const Graphic@ opCast() const` | `Graphic@ derived = cast<Graphic>(obj); const Object@ original = obj; const Graphic@ view = cast<const Graphic@>(original);` | 実体を Graphic として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Button@ opCast() / const Button@ opCast() const` | `Button@ derived = cast<Button>(obj); const Object@ original = obj; const Button@ view = cast<const Button@>(original);` | 実体を Button として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Selectable@ opCast() / const Selectable@ opCast() const` | `Selectable@ derived = cast<Selectable>(obj); const Object@ original = obj; const Selectable@ view = cast<const Selectable@>(original);` | 実体を Selectable として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Behavior@ opCast() / const Behavior@ opCast() const` | `Behavior@ derived = cast<Behavior>(obj); const Object@ original = obj; const Behavior@ view = cast<const Behavior@>(original);` | 実体を Behavior として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Component@ opCast() / const Component@ opCast() const` | `Component@ derived = cast<Component>(obj); const Object@ original = obj; const Component@ view = cast<const Component@>(original);` | 実体を Component として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Material@ opCast() / const Material@ opCast() const` | `Material@ derived = cast<Material>(obj); const Object@ original = obj; const Material@ view = cast<const Material@>(original);` | 実体を Material として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `AudioClip@ opCast() / const AudioClip@ opCast() const` | `AudioClip@ derived = cast<AudioClip>(obj); const Object@ original = obj; const AudioClip@ view = cast<const AudioClip@>(original);` | 実体を AudioClip として取り出す。型が合わなければ null。const の参照からも変換できる。 |

**名前空間 `Object`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `void Destroy(Object@ obj)` | `Object::Destroy(go);` | GameObject または Component の破棄を予約する。GameObject の子とコンポーネントも破棄される。 |
| `void DontDestroyOnLoad(Object@ target)` | `Object::DontDestroyOnLoad(go);` | ルートの GameObject をシーン切り替え後も保持する。 |
| `GameObject@ Instantiate(GameObject@ original)` | `GameObject@ copy = Object::Instantiate(prefab);` | シーンの GameObject または Prefab を複製する。 |
| `GameObject@ Instantiate(GameObject@ original, Transform@ parent, bool instantiateInWorldSpace = false)` | `GameObject@ copy = Object::Instantiate(prefab, t, true);` | 親を指定する。第 3 引数が true ならワールド変換を維持して親に付ける。 |
| `GameObject@ Instantiate(GameObject@ original, const Vector3&in position, const Quaternion&in rotation, Transform@ parent = null)` | `GameObject@ copy = Object::Instantiate(prefab, Vector3::zero, Quaternion::identity, t);` | ワールド位置と回転を指定する。最後の親は省略できる。 |

### GameObject

シーンに配置する 1 個のオブジェクト。コンポーネントの検索・追加や階層の有効状態を扱う。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

| API | 用例 | 解説 |
| --- | --- | --- |
| `GameObject(const string&in name = "GameObject")` | `GameObject@ go = GameObject("Marker");` | 現在のシーンに指定名で空の GameObject を作る。名前の既定値は "GameObject"。 |
| `bool activeSelf（読み取り）` | `bool active = go.activeSelf;` | この GameObject 自身のアクティブ状態を読む。 |
| `bool activeInHierarchy（読み取り）` | `bool active = go.activeInHierarchy;` | 親を含めてアクティブな階層にあるかを読む。 |
| `void SetActive(bool value)` | `go.SetActive(false);` | この GameObject 自身の状態を切り替える。子は親の状態の影響を受ける。 |
| `int layer（読み書き）` | `go.layer = 0; int layer = go.layer;` | 所属レイヤーの番号を読み書きする。タグとは別に、物理判定や描画の対象分けに使う。 |
| `string tag（読み書き）` | `go.tag = "Player"; string label = go.tag;` | GameObject のタグを読み書きする。タグ名を先に Project Settings で登録する。 |
| `bool CompareTag(const string&in tag) const` | `bool isPlayer = go.CompareTag("Player");` | この GameObject のタグが登録済みの指定タグと一致するか調べる。 |
| `Transform@ transform（読み取り）` | `Transform@ t = go.transform;` | この GameObject の Transform を返す。 |
| `bool TryGetComponent(?&out component) const` | `Rigidbody@ body; bool found = go.TryGetComponent(@body);` | 指定型のコンポーネントを探す。見つけた場合は true を返し、渡したハンドルに結果が入る。 |
| `T@ GetComponent<T>() const` | `Rigidbody@ body = go.GetComponent<Rigidbody>();` | 指定型のコンポーネントを取得する。見つからない場合は null。スクリプトの派生クラスも型として指定できる。 |
| `T@ AddComponent<T>()` | `AudioSource@ audio = go.AddComponent<AudioSource>();` | 追加可能な Component を付けて返す。ファイル名とクラス名が一致する MonoBehavior 派生型も指定できる。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = go; const GameObject@ original = go; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

**名前空間 `GameObject`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `GameObject@ Find(const string&in name)` | `GameObject@ player = GameObject::Find("Player");` | 指定名のアクティブな GameObject を探す。見つからない場合は null。 |
| `GameObject@ FindWithTag(const string&in tag)` | `GameObject@ player = GameObject::FindWithTag("Player");` | 登録済みの指定タグを持つアクティブな GameObject を 1 つ返す。見つからない場合は null。 |
| `GameObject@ FindGameObjectWithTag(const string&in tag)` | `GameObject@ player = GameObject::FindGameObjectWithTag("Player");` | FindWithTag と同じ検索を行う別名。 |
| `GameObject@[]@ FindGameObjectsWithTag(const string&in tag)` | `array<GameObject@>@ players = GameObject::FindGameObjectsWithTag("Player");` | 登録済みの指定タグを持つアクティブな GameObject をすべて配列で返す。 |

### Component

GameObject に付くコンポーネントに共通するメンバー。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

| API | 用例 | 解説 |
| --- | --- | --- |
| `GameObject@ gameObject（読み取り）` | `GameObject@ owner = component.gameObject;` | 所属する GameObject を返す。 |
| `Transform@ transform（読み取り）` | `Transform@ t = component.transform;` | 所属する GameObject の Transform を返す。 |
| `string tag（読み書き）` | `component.tag = "Player"; string label = component.tag;` | 所属する GameObject のタグを読み書きする。タグは Project Settings で登録する。 |
| `bool CompareTag(const string&in tag) const` | `bool isPlayer = component.CompareTag("Player");` | 所属する GameObject のタグが登録済みの指定タグと一致するか調べる。 |
| `bool TryGetComponent(?&out component) const` | `Rigidbody@ body; bool found = component.TryGetComponent(@body);` | 同じ GameObject から指定型の Component を探し、見つけた参照を出力する。 |
| `T@ GetComponent<T>() const` | `Rigidbody@ body = component.GetComponent<Rigidbody>();` | 同じ GameObject で指定型の Component を探す。見つからないと null。スクリプトの派生クラスも指定できる。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Transform@ opCast() / const Transform@ opCast() const` | `Transform@ derived = cast<Transform>(component); const Component@ original = component; const Transform@ view = cast<const Transform@>(original);` | 実体を Transform として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `CharacterController@ opCast() / const CharacterController@ opCast() const` | `CharacterController@ derived = cast<CharacterController>(component); const Component@ original = component; const CharacterController@ view = cast<const CharacterController@>(original);` | 実体を CharacterController として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `BoxCollider@ opCast() / const BoxCollider@ opCast() const` | `BoxCollider@ derived = cast<BoxCollider>(component); const Component@ original = component; const BoxCollider@ view = cast<const BoxCollider@>(original);` | 実体を BoxCollider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `SphereCollider@ opCast() / const SphereCollider@ opCast() const` | `SphereCollider@ derived = cast<SphereCollider>(component); const Component@ original = component; const SphereCollider@ view = cast<const SphereCollider@>(original);` | 実体を SphereCollider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `CapsuleCollider@ opCast() / const CapsuleCollider@ opCast() const` | `CapsuleCollider@ derived = cast<CapsuleCollider>(component); const Component@ original = component; const CapsuleCollider@ view = cast<const CapsuleCollider@>(original);` | 実体を CapsuleCollider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Collider@ opCast() / const Collider@ opCast() const` | `Collider@ derived = cast<Collider>(component); const Component@ original = component; const Collider@ view = cast<const Collider@>(original);` | 実体を Collider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Rigidbody@ opCast() / const Rigidbody@ opCast() const` | `Rigidbody@ derived = cast<Rigidbody>(component); const Component@ original = component; const Rigidbody@ view = cast<const Rigidbody@>(original);` | 実体を Rigidbody として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `AudioSource@ opCast() / const AudioSource@ opCast() const` | `AudioSource@ derived = cast<AudioSource>(component); const Component@ original = component; const AudioSource@ view = cast<const AudioSource@>(original);` | 実体を AudioSource として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `MeshRenderer@ opCast() / const MeshRenderer@ opCast() const` | `MeshRenderer@ derived = cast<MeshRenderer>(component); const Component@ original = component; const MeshRenderer@ view = cast<const MeshRenderer@>(original);` | 実体を MeshRenderer として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Renderer@ opCast() / const Renderer@ opCast() const` | `Renderer@ derived = cast<Renderer>(component); const Component@ original = component; const Renderer@ view = cast<const Renderer@>(original);` | 実体を Renderer として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Text@ opCast() / const Text@ opCast() const` | `Text@ derived = cast<Text>(component); const Component@ original = component; const Text@ view = cast<const Text@>(original);` | 実体を Text として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Image@ opCast() / const Image@ opCast() const` | `Image@ derived = cast<Image>(component); const Component@ original = component; const Image@ view = cast<const Image@>(original);` | 実体を Image として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Graphic@ opCast() / const Graphic@ opCast() const` | `Graphic@ derived = cast<Graphic>(component); const Component@ original = component; const Graphic@ view = cast<const Graphic@>(original);` | 実体を Graphic として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Button@ opCast() / const Button@ opCast() const` | `Button@ derived = cast<Button>(component); const Component@ original = component; const Button@ view = cast<const Button@>(original);` | 実体を Button として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Selectable@ opCast() / const Selectable@ opCast() const` | `Selectable@ derived = cast<Selectable>(component); const Component@ original = component; const Selectable@ view = cast<const Selectable@>(original);` | 実体を Selectable として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Behavior@ opCast() / const Behavior@ opCast() const` | `Behavior@ derived = cast<Behavior>(component); const Component@ original = component; const Behavior@ view = cast<const Behavior@>(original);` | 実体を Behavior として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = component; const Component@ original = component; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### Behavior

独自の有効フラグを持つコンポーネント。スクリプトでの型名は Behavior。

**共通メンバー**

- [Component](#component) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `bool enabled（読み書き）` | `behavior.enabled = true; bool enabledNow = behavior.enabled;` | このコンポーネント自身の有効フラグを読み書きする。 |
| `bool isActiveAndEnabled（読み取り）` | `bool active = behavior.isActiveAndEnabled;` | 有効フラグと GameObject の階層のアクティブ状態が両方 true のとき true。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `CharacterController@ opCast() / const CharacterController@ opCast() const` | `CharacterController@ derived = cast<CharacterController>(behavior); const Behavior@ original = behavior; const CharacterController@ view = cast<const CharacterController@>(original);` | 実体を CharacterController として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `BoxCollider@ opCast() / const BoxCollider@ opCast() const` | `BoxCollider@ derived = cast<BoxCollider>(behavior); const Behavior@ original = behavior; const BoxCollider@ view = cast<const BoxCollider@>(original);` | 実体を BoxCollider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `SphereCollider@ opCast() / const SphereCollider@ opCast() const` | `SphereCollider@ derived = cast<SphereCollider>(behavior); const Behavior@ original = behavior; const SphereCollider@ view = cast<const SphereCollider@>(original);` | 実体を SphereCollider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `CapsuleCollider@ opCast() / const CapsuleCollider@ opCast() const` | `CapsuleCollider@ derived = cast<CapsuleCollider>(behavior); const Behavior@ original = behavior; const CapsuleCollider@ view = cast<const CapsuleCollider@>(original);` | 実体を CapsuleCollider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Collider@ opCast() / const Collider@ opCast() const` | `Collider@ derived = cast<Collider>(behavior); const Behavior@ original = behavior; const Collider@ view = cast<const Collider@>(original);` | 実体を Collider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `MeshRenderer@ opCast() / const MeshRenderer@ opCast() const` | `MeshRenderer@ derived = cast<MeshRenderer>(behavior); const Behavior@ original = behavior; const MeshRenderer@ view = cast<const MeshRenderer@>(original);` | 実体を MeshRenderer として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Renderer@ opCast() / const Renderer@ opCast() const` | `Renderer@ derived = cast<Renderer>(behavior); const Behavior@ original = behavior; const Renderer@ view = cast<const Renderer@>(original);` | 実体を Renderer として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Text@ opCast() / const Text@ opCast() const` | `Text@ derived = cast<Text>(behavior); const Behavior@ original = behavior; const Text@ view = cast<const Text@>(original);` | 実体を Text として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Image@ opCast() / const Image@ opCast() const` | `Image@ derived = cast<Image>(behavior); const Behavior@ original = behavior; const Image@ view = cast<const Image@>(original);` | 実体を Image として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Graphic@ opCast() / const Graphic@ opCast() const` | `Graphic@ derived = cast<Graphic>(behavior); const Behavior@ original = behavior; const Graphic@ view = cast<const Graphic@>(original);` | 実体を Graphic として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Button@ opCast() / const Button@ opCast() const` | `Button@ derived = cast<Button>(behavior); const Behavior@ original = behavior; const Button@ view = cast<const Button@>(original);` | 実体を Button として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Selectable@ opCast() / const Selectable@ opCast() const` | `Selectable@ derived = cast<Selectable>(behavior); const Behavior@ original = behavior; const Selectable@ view = cast<const Selectable@>(original);` | 実体を Selectable として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = behavior; const Behavior@ original = behavior; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = behavior; const Behavior@ original = behavior; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### Transform

位置・回転・スケールと親子関係を扱う。座標などの値を変えるときはプロパティに値全体を書き戻す。

**共通メンバー**

- [Component](#component) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Vector3 position（読み書き）` | `t.position = Vector3(1, 2, 3); Vector3 p = t.position;` | ワールド座標での位置を読み書きする。 |
| `Vector3 localPosition（読み書き）` | `t.localPosition = Vector3::zero; Vector3 p = t.localPosition;` | 親を基準にした位置を読み書きする。 |
| `Quaternion rotation（読み書き）` | `t.rotation = Quaternion::identity; Quaternion q = t.rotation;` | ワールド回転を Quaternion で読み書きする。 |
| `Quaternion localRotation（読み書き）` | `t.localRotation = Quaternion::identity; Quaternion q = t.localRotation;` | 親に対する回転を Quaternion で読み書きする。 |
| `Vector3 eulerAngles（読み書き）` | `t.eulerAngles = Vector3(0, 90, 0); Vector3 degrees = t.eulerAngles;` | ワールド回転を度単位のオイラー角で読み書きする。 |
| `Vector3 localEulerAngles（読み書き）` | `t.localEulerAngles = Vector3(0, 90, 0); Vector3 degrees = t.localEulerAngles;` | 親に対する回転を度単位のオイラー角で読み書きする。 |
| `Vector3 localScale（読み書き）` | `t.localScale = Vector3(2, 1, 1); Vector3 scale = t.localScale;` | 親に対する各軸の拡大率を読み書きする。 |
| `Vector3 lossyScale（読み取り）` | `Vector3 scale = t.lossyScale;` | 親の変換を含めたワールドでのおおよその拡大率。 |
| `Vector3 forward（読み取り）` | `Vector3 direction = t.forward;` | 回転を反映したワールドの前方向。 |
| `Vector3 right（読み取り）` | `Vector3 direction = t.right;` | 回転を反映したワールドの右方向。 |
| `Vector3 up（読み取り）` | `Vector3 direction = t.up;` | 回転を反映したワールドの上方向。 |
| `Transform@ parent（読み書き）` | `Transform@ p = t.parent; @t.parent = parentTransform;` | 親 Transform を読み書きする。null を設定するとルートになる。親の付け替え時はワールド変換を維持する。 |
| `int childCount（読み取り）` | `int count = t.childCount;` | 直下にある子の数。 |
| `Transform@ GetChild(int index) const` | `if (t.childCount > 0) { Transform@ child = t.GetChild(0); }` | 0 始まりの番号で直下の子を取得する。 |
| `Transform@ Find(const string&in name) const` | `Transform@ muzzle = t.Find("Weapon/Muzzle");` | 直下の子の名前や / 区切りの相対パスで探す。見つからない場合は null。 |
| `void Translate(const Vector3&in translation, Space relativeTo = Space::Self)` | `t.Translate(Vector3::forward * 0.5f);` | ベクトルで変位を渡す。基準を省略した場合は Space::Self。 |
| `void Translate(float x, float y, float z, Space relativeTo = Space::Self)` | `t.Translate(1.0f, 0.0f, 0.0f, Space::World);` | X・Y・Z の変位を別々に渡す。例はワールド X へ 1 移動する。 |
| `void Rotate(const Vector3&in eulers, Space relativeTo = Space::Self)` | `t.Rotate(Vector3(0, 45, 0));` | 度単位のオイラー角を渡して現在の回転に加える。既定の基準は Space::Self。 |
| `void Rotate(float xAngle, float yAngle, float zAngle, Space relativeTo = Space::Self)` | `t.Rotate(0.0f, 45.0f, 0.0f, Space::World);` | 軸ごとの角度を個別に渡す。例はワールド Y 軸まわりに 45 度回転する。 |
| `void LookAt(const Vector3&in worldPosition, const Vector3&in worldUp = Vector3::up)` | `t.LookAt(Vector3(0, 1, 10));` | 前方向を指定ワールド座標へ向ける。必要に応じてワールドの上方向も指定する。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = t; const Transform@ original = t; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = t; const Transform@ original = t; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### Scene

シーンの名前、パス、ビルド番号を持つ値型。

| API | 用例 | 解説 |
| --- | --- | --- |
| `~Scene()` | `{ Scene value; }` | 値がスコープから出たときに自動的に破棄される。 |
| `Scene()` | `Scene scene;` | 空の Scene 値を作る。IsValid() で読み込まれたシーン情報を持つか確認できる。 |
| `Scene(const Scene&in other)` | `Scene scene(SceneManager::GetActiveScene());` | 得られたシーン情報をコピーして保持する。 |
| `Scene& opAssign(const Scene&in other)` | `scene = SceneManager::GetActiveScene();` | 右辺の内容を左辺へコピーする。 |
| `bool opEquals(const Scene&in other) const` | `bool result = scene == SceneManager::GetActiveScene();` | シーンのパス・名前・ビルド番号の 3 項目がすべて等しいかを比較する。 |
| `string name（読み取り）` | `string title = scene.name;` | シーンの名前を読む。 |
| `string path（読み取り）` | `string path = scene.path;` | シーンファイルのパスを読む。 |
| `int buildIndex（読み取り）` | `int index = scene.buildIndex;` | Build Settings の有効なシーン間での番号。番号が付いていない場合は -1。 |
| `bool IsValid() const` | `bool valid = scene.IsValid();` | Scene 値がシーン情報を表しているか調べる。 |

### SceneManager

Build Settings のシーンを切り替えたり、現在のシーンを調べたりする。

| API | 用例 | 解説 |
| --- | --- | --- |
| `void LoadScene(const string&in sceneName)` | `SceneManager::LoadScene("Stage2");` | Build Settings で有効にしたシーンへ名前で切り替える。切り替えはフレームの終わりに行われる。 |
| `void LoadScene(int sceneBuildIndex)` | `SceneManager::LoadScene(0);` | Build Settings の有効なシーンへビルド番号で切り替える。切り替えはフレームの終わりに行われる。 |
| `Scene GetActiveScene()` | `Scene scene = SceneManager::GetActiveScene();` | 現在のアクティブシーンの情報を値として返す。 |
| `int sceneCountInBuildSettings（読み取り）` | `int count = SceneManager::sceneCountInBuildSettings;` | Build Settings で有効なシーンの件数を読む。 |

## 物理

### Physics

物理判定と重力を扱う。レイキャストにはワールド座標の起点と方向を渡す。

| API | 用例 | 解説 |
| --- | --- | --- |
| `Vector3 gravity（読み書き）` | `Physics::gravity = Vector3(0, -9.81f, 0); Vector3 g = Physics::gravity;` | シーンで使うワールド重力を読み書きする。 |
| `bool Raycast(const Vector3&in origin, const Vector3&in direction, float maxDistance = Mathf::Infinity, int layerMask = Physics::DefaultRaycastLayers)` | `bool found = Physics::Raycast(t.position, t.forward, 20.0f);` | 起点から方向にレイを飛ばし、指定距離以内に対象があるかを返す。 |
| `bool Raycast(const Vector3&in origin, const Vector3&in direction, RaycastHit&out hitInfo, float maxDistance = Mathf::Infinity, int layerMask = Physics::DefaultRaycastLayers)` | `RaycastHit hit; bool found = Physics::Raycast(t.position, t.forward, hit, 20.0f, Physics::AllLayers);` | ヒットの成否とヒット情報を取得する。距離とレイヤーマスクを省略できる。 |
| `const int DefaultRaycastLayers` | `int mask = Physics::DefaultRaycastLayers;` | レイの既定マスク。Ignore Raycast レイヤーを除く。 |
| `const int AllLayers` | `int mask = Physics::AllLayers;` | すべてのレイヤーを対象とするマスク。 |

### Collider

衝突判定の形状に共通する情報。

**共通メンバー**

- [Behavior](#behavior) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `bool isTrigger（読み書き）` | `collider.isTrigger = true; bool trigger = collider.isTrigger;` | トリガーとして扱うかを読み書きする。有効なペアの重なりは OnTrigger 系で受けられる。 |
| `Rigidbody@ attachedRigidbody（読み取り）` | `Rigidbody@ body = collider.attachedRigidbody;` | この Collider が属する物理ボディを返す。存在しなければ null。 |
| `Vector3 ClosestPoint(const Vector3&in position) const` | `Vector3 p = collider.ClosestPoint(Vector3(3, 0, 0));` | 指定ワールド位置に最も近いコライダー上の点を求める。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `CharacterController@ opCast() / const CharacterController@ opCast() const` | `CharacterController@ derived = cast<CharacterController>(collider); const Collider@ original = collider; const CharacterController@ view = cast<const CharacterController@>(original);` | 実体を CharacterController として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `BoxCollider@ opCast() / const BoxCollider@ opCast() const` | `BoxCollider@ derived = cast<BoxCollider>(collider); const Collider@ original = collider; const BoxCollider@ view = cast<const BoxCollider@>(original);` | 実体を BoxCollider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `SphereCollider@ opCast() / const SphereCollider@ opCast() const` | `SphereCollider@ derived = cast<SphereCollider>(collider); const Collider@ original = collider; const SphereCollider@ view = cast<const SphereCollider@>(original);` | 実体を SphereCollider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `CapsuleCollider@ opCast() / const CapsuleCollider@ opCast() const` | `CapsuleCollider@ derived = cast<CapsuleCollider>(collider); const Collider@ original = collider; const CapsuleCollider@ view = cast<const CapsuleCollider@>(original);` | 実体を CapsuleCollider として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Behavior@ opImplCast() / const Behavior@ opImplCast() const` | `Behavior@ base = collider; const Collider@ original = collider; const Behavior@ view = original;` | 代入で Behavior として参照する。const の参照も代入できる。 |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = collider; const Collider@ original = collider; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = collider; const Collider@ original = collider; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### BoxCollider

箱形のコライダー。

**共通メンバー**

- [Collider](#collider) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`isTrigger`、`attachedRigidbody`、`ClosestPoint`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Vector3 center（読み書き）` | `box.center = Vector3(0, 1, 0); Vector3 p = box.center;` | GameObject のローカル座標で箱の中心を読み書きする。 |
| `Vector3 size（読み書き）` | `box.size = Vector3(2, 1, 3); Vector3 size = box.size;` | 箱のローカル寸法を軸ごとに読み書きする。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Collider@ opImplCast() / const Collider@ opImplCast() const` | `Collider@ base = box; const BoxCollider@ original = box; const Collider@ view = original;` | 代入で Collider として参照する。const の参照も代入できる。 |
| `Behavior@ opImplCast() / const Behavior@ opImplCast() const` | `Behavior@ base = box; const BoxCollider@ original = box; const Behavior@ view = original;` | 代入で Behavior として参照する。const の参照も代入できる。 |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = box; const BoxCollider@ original = box; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = box; const BoxCollider@ original = box; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### SphereCollider

球形のコライダー。

**共通メンバー**

- [Collider](#collider) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`isTrigger`、`attachedRigidbody`、`ClosestPoint`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Vector3 center（読み書き）` | `sphere.center = Vector3(0, 1, 0); Vector3 p = sphere.center;` | GameObject のローカル座標で球の中心を読み書きする。 |
| `float radius（読み書き）` | `sphere.radius = 0.5f; float radius = sphere.radius;` | 球のローカル半径を読み書きする。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Collider@ opImplCast() / const Collider@ opImplCast() const` | `Collider@ base = sphere; const SphereCollider@ original = sphere; const Collider@ view = original;` | 代入で Collider として参照する。const の参照も代入できる。 |
| `Behavior@ opImplCast() / const Behavior@ opImplCast() const` | `Behavior@ base = sphere; const SphereCollider@ original = sphere; const Behavior@ view = original;` | 代入で Behavior として参照する。const の参照も代入できる。 |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = sphere; const SphereCollider@ original = sphere; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = sphere; const SphereCollider@ original = sphere; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### CapsuleCollider

カプセル形のコライダー。

**共通メンバー**

- [Collider](#collider) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`isTrigger`、`attachedRigidbody`、`ClosestPoint`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Vector3 center（読み書き）` | `capsule.center = Vector3(0, 1, 0); Vector3 p = capsule.center;` | GameObject のローカル座標でカプセルの中心を読み書きする。 |
| `float radius（読み書き）` | `capsule.radius = 0.5f; float radius = capsule.radius;` | カプセルのローカル半径を読み書きする。 |
| `float height（読み書き）` | `capsule.height = 2.0f; float height = capsule.height;` | 丸い端を含むカプセルの高さを読み書きする。 |
| `int direction（読み書き）` | `capsule.direction = 1; int axis = capsule.direction;` | カプセルの長軸を読み書きする。0 は X、1 は Y、2 は Z。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Collider@ opImplCast() / const Collider@ opImplCast() const` | `Collider@ base = capsule; const CapsuleCollider@ original = capsule; const Collider@ view = original;` | 代入で Collider として参照する。const の参照も代入できる。 |
| `Behavior@ opImplCast() / const Behavior@ opImplCast() const` | `Behavior@ base = capsule; const CapsuleCollider@ original = capsule; const Behavior@ view = original;` | 代入で Behavior として参照する。const の参照も代入できる。 |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = capsule; const CapsuleCollider@ original = capsule; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = capsule; const CapsuleCollider@ original = capsule; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### Rigidbody

力・速度・質量などを扱う物理ボディ。継続して力を加える処理は FixedUpdate に置ける。

**共通メンバー**

- [Component](#component) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `float mass（読み書き）` | `body.mass = 2.0f; float mass = body.mass;` | 物理ボディの質量を読み書きする。 |
| `bool useGravity（読み書き）` | `body.useGravity = false; bool gravityEnabled = body.useGravity;` | シーンの重力の影響を受けるかを読み書きする。 |
| `bool isKinematic（読み書き）` | `body.isKinematic = true; bool kinematic = body.isKinematic;` | スクリプトから位置や回転を制御するキネマティック状態を読み書きする。 |
| `float linearDamping（読み書き）` | `body.linearDamping = 0.5f; float damping = body.linearDamping;` | 並進速度の減衰を読み書きする。 |
| `float angularDamping（読み書き）` | `body.angularDamping = 0.5f; float damping = body.angularDamping;` | 角速度の減衰を読み書きする。 |
| `Vector3 linearVelocity（読み書き）` | `body.linearVelocity = Vector3::forward * 3.0f; Vector3 speed = body.linearVelocity;` | ワールドでの並進速度を読み書きする。 |
| `Vector3 velocity（読み書き）` | `body.velocity = Vector3::zero; Vector3 speed = body.velocity;` | linearVelocity の別名。どちらからでも同じ並進速度を読み書きできる。 |
| `Vector3 angularVelocity（読み書き）` | `body.angularVelocity = Vector3::up; Vector3 omega = body.angularVelocity;` | ワールド各軸まわりの角速度を読み書きする。 |
| `Vector3 position（読み取り）` | `Vector3 p = body.position;` | ボディのワールド位置を取得する。 |
| `Quaternion rotation（読み取り）` | `Quaternion q = body.rotation;` | ボディのワールド回転を取得する。 |
| `Vector3 worldCenterOfMass（読み取り）` | `Vector3 p = body.worldCenterOfMass;` | ワールド座標での重心を取得する。 |
| `void AddForce(const Vector3&in force, ForceMode mode = ForceMode::Force)` | `body.AddForce(Vector3::up * 5.0f, ForceMode::Impulse);` | ワールド方向に力を加える。例では押した瞬間などに力積を一度与える。 |
| `void AddTorque(const Vector3&in torque, ForceMode mode = ForceMode::Force)` | `body.AddTorque(Vector3::up * 2.0f);` | ワールドの軸を基準に回転させる力を加える。 |
| `void AddRelativeForce(const Vector3&in force, ForceMode mode = ForceMode::Force)` | `body.AddRelativeForce(Vector3::forward * 2.0f);` | 物体のローカル方向を基準に力を加える。 |
| `void AddRelativeTorque(const Vector3&in torque, ForceMode mode = ForceMode::Force)` | `body.AddRelativeTorque(Vector3::right * 2.0f);` | 物体のローカル軸を基準に回転させる力を加える。 |
| `void AddForceAtPosition(const Vector3&in force, const Vector3&in position, ForceMode mode = ForceMode::Force)` | `body.AddForceAtPosition(Vector3::up, body.position + Vector3::right);` | ワールド座標の指定点に力を加える。重心からずれた点なら回転にも影響する。 |
| `void MovePosition(const Vector3&in position)` | `body.MovePosition(body.position + Vector3::forward * 0.02f);` | ワールド位置を指定して移動させる。固定更新でキネマティックな物体を動かすときに使える。 |
| `void MoveRotation(const Quaternion&in rotation)` | `body.MoveRotation(Quaternion::Euler(0, 45, 0));` | ワールド回転を指定して回す。 |
| `void Sleep()` | `body.Sleep();` | 物理ボディをスリープさせる。 |
| `void WakeUp()` | `body.WakeUp();` | スリープ状態の物理ボディを起こす。 |
| `bool IsSleeping() const` | `bool sleeping = body.IsSleeping();` | ボディがスリープ中かを調べる。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = body; const Rigidbody@ original = body; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = body; const Rigidbody@ original = body; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### CharacterController

カプセル形状を衝突に沿わせながらスクリプトから移動させる。

**共通メンバー**

- [Collider](#collider) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`isTrigger`、`attachedRigidbody`、`ClosestPoint`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `CollisionFlags Move(const Vector3&in motion)` | `CollisionFlags flags = controller.Move(Vector3::forward * 2.0f * Time::deltaTime);` | ワールド座標の変位を与え、衝突方向をフラグで受け取る。重力は呼び出し側で変位に反映できる。 |
| `bool SimpleMove(const Vector3&in speed)` | `bool grounded = controller.SimpleMove(Vector3::forward * 2.0f);` | ワールドでの移動速度を与える。垂直速度は内部の重力処理で決まり、戻り値は接地状態。 |
| `bool isGrounded（読み取り）` | `bool grounded = controller.isGrounded;` | 直近の移動で接地したかを読む。 |
| `Vector3 velocity（読み取り）` | `Vector3 speed = controller.velocity;` | 移動結果から得た速度を読む。 |
| `Vector3 center（読み書き）` | `controller.center = Vector3(0, 1, 0); Vector3 p = controller.center;` | ローカル座標でカプセルの中心を読み書きする。 |
| `float radius（読み書き）` | `controller.radius = 0.5f; float radius = controller.radius;` | カプセルの半径を読み書きする。 |
| `float height（読み書き）` | `controller.height = 2.0f; float height = controller.height;` | カプセル全体の高さを読み書きする。 |
| `float slopeLimit（読み書き）` | `controller.slopeLimit = 45.0f; float limit = controller.slopeLimit;` | 移動で登れる斜面の上限を度で読み書きする。 |
| `float stepOffset（読み書き）` | `controller.stepOffset = 0.3f; float step = controller.stepOffset;` | 乗り越えられる段差の高さを読み書きする。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Collider@ opImplCast() / const Collider@ opImplCast() const` | `Collider@ base = controller; const CharacterController@ original = controller; const Collider@ view = original;` | 代入で Collider として参照する。const の参照も代入できる。 |
| `Behavior@ opImplCast() / const Behavior@ opImplCast() const` | `Behavior@ base = controller; const CharacterController@ original = controller; const Behavior@ view = original;` | 代入で Behavior として参照する。const の参照も代入できる。 |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = controller; const CharacterController@ original = controller; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = controller; const CharacterController@ original = controller; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### Collision

OnCollisionEnter などで受け取る衝突情報。

| API | 用例 | 解説 |
| --- | --- | --- |
| `~Collision()` | `{ Collision value; }` | 値がスコープから出たときに自動的に破棄される。 |
| `Collision()` | `Collision collision;` | 空の衝突情報を作る。 |
| `Collision(const Collision&in other)` | `Collision copy(collision);` | 受け取った衝突情報をコピーする。 |
| `Collision& opAssign(const Collision&in other)` | `collision = otherCollision;` | 右辺の内容を左辺へコピーする。 |
| `GameObject@ gameObject（読み取り）` | `GameObject@ other = collision.gameObject;` | 衝突相手の GameObject。 |
| `Transform@ transform（読み取り）` | `Transform@ t = collision.transform;` | 衝突相手の Transform。 |
| `Collider@ collider（読み取り）` | `Collider@ other = collision.collider;` | 衝突相手の Collider。 |
| `Rigidbody@ rigidbody（読み取り）` | `Rigidbody@ body = collision.rigidbody;` | 衝突相手の Rigidbody。存在しなければ null。 |
| `Vector3 relativeVelocity（読み取り）` | `Vector3 speed = collision.relativeVelocity;` | 衝突した物体同士の相対速度。 |
| `Vector3 impulse（読み取り）` | `Vector3 impulse = collision.impulse;` | 衝突で報告された力積。 |
| `int contactCount（読み取り）` | `int count = collision.contactCount;` | 参照できる接触点の個数。 |
| `ContactPoint GetContact(int index) const` | `if (collision.contactCount > 0) { ContactPoint point = collision.GetContact(0); }` | 0 始まりの番号で接触点を取得する。 |

### ContactPoint

衝突に含まれる個々の接触点。通常は Collision.GetContact から取得する。

| API | 用例 | 解説 |
| --- | --- | --- |
| `ContactPoint()` | `ContactPoint contact;` | 接触点の値を初期化する。実際の接触情報は Collision.GetContact() で得られる。 |
| `Collider@ thisCollider（読み取り）` | `Collider@ self = contact.thisCollider;` | コールバックを受けた側の Collider。 |
| `Collider@ otherCollider（読み取り）` | `Collider@ other = contact.otherCollider;` | 衝突相手の Collider。 |
| `Vector3 point（読み書き）` | `Vector3 p = contact.point;` | 接触点のワールド座標。 |
| `Vector3 normal（読み書き）` | `Vector3 n = contact.normal;` | 接触面の法線。 |
| `float separation（読み書き）` | `float gap = contact.separation;` | 接触面同士の離隔。負の値はめり込みを表す。 |

### RaycastHit

レイが対象に当たった場合の結果。Raycast が true を返したときに参照する。

| API | 用例 | 解説 |
| --- | --- | --- |
| `RaycastHit()` | `RaycastHit hit;` | Physics::Raycast の結果を受ける変数を宣言する。 |
| `Collider@ collider（読み取り）` | `Collider@ collider = hit.collider;` | レイが当たった Collider。 |
| `Rigidbody@ rigidbody（読み取り）` | `Rigidbody@ body = hit.rigidbody;` | ヒットした Collider に対応する Rigidbody。存在しなければ null。 |
| `Transform@ transform（読み取り）` | `Transform@ t = hit.transform;` | ヒットしたオブジェクトの Transform。 |
| `Vector3 point（読み書き）` | `Vector3 p = hit.point;` | ヒットしたワールド座標。 |
| `Vector3 normal（読み書き）` | `Vector3 n = hit.normal;` | ヒットした位置の面の法線。 |
| `float distance（読み書き）` | `float length = hit.distance;` | レイの起点からヒット点までの距離。 |

## 描画・音声・UI

### Renderer

描画するコンポーネントに共通するマテリアル操作。

**共通メンバー**

- [Behavior](#behavior) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Material@ material（読み書き）` | `Material@ mat = renderer.material; @renderer.material = replacement;` | 描画用の個別マテリアルを取得する。取得時に個別化されるので、色などをその Renderer だけ変えられる。代入で付け替えもできる。 |
| `Material@ sharedMaterial（読み書き）` | `Material@ mat = renderer.sharedMaterial; @renderer.sharedMaterial = replacement;` | 共有マテリアルを読む、または付け替える。共有マテリアル自体を編集すると、そのマテリアルを使う他の描画にも影響しうる。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `MeshRenderer@ opCast() / const MeshRenderer@ opCast() const` | `MeshRenderer@ derived = cast<MeshRenderer>(renderer); const Renderer@ original = renderer; const MeshRenderer@ view = cast<const MeshRenderer@>(original);` | 実体を MeshRenderer として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Behavior@ opImplCast() / const Behavior@ opImplCast() const` | `Behavior@ base = renderer; const Renderer@ original = renderer; const Behavior@ view = original;` | 代入で Behavior として参照する。const の参照も代入できる。 |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = renderer; const Renderer@ original = renderer; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = renderer; const Renderer@ original = renderer; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### MeshRenderer

メッシュを描画するコンポーネント。スクリプトでは Renderer と共通のマテリアル操作を使う。

**共通メンバー**

- [Renderer](#renderer) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`material`、`sharedMaterial`、`TryGetComponent`、`GetComponent`

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Renderer@ opImplCast() / const Renderer@ opImplCast() const` | `Renderer@ base = meshRenderer; const MeshRenderer@ original = meshRenderer; const Renderer@ view = original;` | 代入で Renderer として参照する。const の参照も代入できる。 |
| `Behavior@ opImplCast() / const Behavior@ opImplCast() const` | `Behavior@ base = meshRenderer; const MeshRenderer@ original = meshRenderer; const Behavior@ view = original;` | 代入で Behavior として参照する。const の参照も代入できる。 |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = meshRenderer; const MeshRenderer@ original = meshRenderer; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = meshRenderer; const MeshRenderer@ original = meshRenderer; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### Material

色やテクスチャの UV、シェーダーの名前付きパラメーターを扱う。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Color color（読み書き）` | `mat.color = Color::red; Color tint = mat.color;` | マテリアルの基本色を読み書きする。 |
| `Vector2 mainTextureOffset（読み書き）` | `mat.mainTextureOffset = Vector2(0.25f, 0.0f); Vector2 offset = mat.mainTextureOffset;` | メインテクスチャを参照するときの UV のずれを読み書きする。 |
| `Vector2 mainTextureScale（読み書き）` | `mat.mainTextureScale = Vector2(2, 2); Vector2 tiling = mat.mainTextureScale;` | メインテクスチャの UV の倍率を読み書きする。 |
| `Color GetColor(const string&in name) const` | `Color tint = mat.GetColor("baseColor");` | 対応するシェーダーで宣言された名前付きの色を読む。プロパティ名は使うシェーダーに合わせる。 |
| `void SetColor(const string&in name, const Color&in value)` | `mat.SetColor("baseColor", Color::red);` | シェーダーの名前付きの色に値を設定する。 |
| `float GetFloat(const string&in name) const` | `float value = mat.GetFloat("roughness");` | シェーダーの名前付きの数値を読む。 |
| `void SetFloat(const string&in name, float value)` | `mat.SetFloat("roughness", 0.8f);` | シェーダーの名前付きの数値に値を設定する。 |
| `bool HasProperty(const string&in name) const` | `bool hasValue = mat.HasProperty("roughness");` | 指定した名前のプロパティがあるか調べる。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = mat; const Material@ original = mat; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### AudioSource

GameObject に付けて AudioClip を再生するコンポーネント。

**共通メンバー**

- [Component](#component) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `void Play()` | `source.Play();` | clip に設定した音を先頭から再生する。 |
| `void Stop()` | `source.Stop();` | 再生を停止する。再度 Play すると先頭から始まる。 |
| `void Pause()` | `source.Pause();` | 再生位置を保ったまま一時停止する。 |
| `void UnPause()` | `source.UnPause();` | Pause で止めた位置から再開する。 |
| `void PlayOneShot(AudioClip@ clip, float volumeScale = 1.0f)` | `source.PlayOneShot(clip, 0.6f);` | クリップを一度鳴らす。設定済みの clip は差し替えず、効果音を重ねて鳴らせる。 |
| `bool isPlaying（読み取り）` | `bool playing = source.isPlaying;` | 現在再生中かどうかを読む。 |
| `AudioClip@ clip（読み書き）` | `@source.clip = clip; AudioClip@ current = source.clip;` | Play に使われるクリップを読み書きする。 |
| `float volume（読み書き）` | `source.volume = 0.5f; float gain = source.volume;` | 音源の音量を読み書きする。 |
| `float pitch（読み書き）` | `source.pitch = 1.2f; float pitch = source.pitch;` | 再生ピッチの倍率を読み書きする。 |
| `bool loop（読み書き）` | `source.loop = true; bool loops = source.loop;` | 通常再生で繰り返すかを読み書きする。 |
| `bool mute（読み書き）` | `source.mute = true; bool muted = source.mute;` | 消音するかを読み書きする。 |
| `bool playOnAwake（読み書き）` | `source.playOnAwake = true; bool autoplay = source.playOnAwake;` | 初期化時に clip を自動再生する設定。開始時に鳴らす場合は Inspector であらかじめ設定する。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = source; const AudioSource@ original = source; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = source; const AudioSource@ original = source; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### AudioClip

Project から参照する音声データ。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

| API | 用例 | 解説 |
| --- | --- | --- |
| `float length（読み取り）` | `float seconds = clip.length;` | クリップの長さを秒で返す。 |
| `int frequency（読み取り）` | `int hz = clip.frequency;` | サンプリング周波数を Hz で返す。 |
| `int channels（読み取り）` | `int count = clip.channels;` | モノラルなら 1、ステレオなら 2 など、チャンネル数を返す。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = clip; const AudioClip@ original = clip; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### Texture2D

Project から参照する画像（.png / .jpg）。スクリプトの欄に `Texture2D@` を書くと、Inspector で画像を入れられる。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

| API | 用例 | 解説 |
| --- | --- | --- |
| `int width（読み取り）` | `int w = texture.width;` | 画像の幅をピクセルで返す。 |
| `int height（読み取り）` | `int h = texture.height;` | 画像の高さをピクセルで返す。 |
| `bool isReadable（読み取り）` | `bool readable = texture.isReadable;` | CPU 側に画素があるか。画像の Inspector で種類を Cursor にしたものだけ true。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = texture;` | 代入で Object として参照する。const の参照も代入できる。 |

### Graphic

UI の描画とポインター判定に関わる共通コンポーネント。

**共通メンバー**

- [Behavior](#behavior) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Color color（読み書き）` | `graphic.color = Color::yellow; Color tint = graphic.color;` | UI の描画色を読み書きする。 |
| `bool raycastTarget（読み書き）` | `graphic.raycastTarget = false; bool receivesPointer = graphic.raycastTarget;` | UI のポインター判定にこの Graphic を含めるかを読み書きする。 |
| `void CrossFadeColor(const Color&in targetColor, float duration, bool ignoreTimeScale, bool useAlpha)` | `graphic.CrossFadeColor(Color::white, 0.25f, true, true);` | 第 2 引数の秒数で色を補間する。第 3 引数は timeScale を無視するか、第 4 引数はアルファも変えるか。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Text@ opCast() / const Text@ opCast() const` | `Text@ derived = cast<Text>(graphic); const Graphic@ original = graphic; const Text@ view = cast<const Text@>(original);` | 実体を Text として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Image@ opCast() / const Image@ opCast() const` | `Image@ derived = cast<Image>(graphic); const Graphic@ original = graphic; const Image@ view = cast<const Image@>(original);` | 実体を Image として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Behavior@ opImplCast() / const Behavior@ opImplCast() const` | `Behavior@ base = graphic; const Graphic@ original = graphic; const Behavior@ view = original;` | 代入で Behavior として参照する。const の参照も代入できる。 |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = graphic; const Graphic@ original = graphic; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = graphic; const Graphic@ original = graphic; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### Text

文字列と文字サイズなどを扱う UI コンポーネント。

**共通メンバー**

- [Graphic](#graphic) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`color`、`raycastTarget`、`CrossFadeColor`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `string text（読み書き）` | `label.text = "Ready"; string caption = label.text;` | 画面に表示する文字列を読み書きする。 |
| `float fontSize（読み書き）` | `label.fontSize = 24.0f; float size = label.fontSize;` | フォントの大きさを読み書きする。 |
| `float lineSpacing（読み書き）` | `label.lineSpacing = 1.2f; float spacing = label.lineSpacing;` | 複数行を描くときの行間を読み書きする。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Graphic@ opImplCast() / const Graphic@ opImplCast() const` | `Graphic@ base = label; const Text@ original = label; const Graphic@ view = original;` | 代入で Graphic として参照する。const の参照も代入できる。 |
| `Behavior@ opImplCast() / const Behavior@ opImplCast() const` | `Behavior@ base = label; const Text@ original = label; const Behavior@ view = original;` | 代入で Behavior として参照する。const の参照も代入できる。 |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = label; const Text@ original = label; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = label; const Text@ original = label; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### Image

画像の表示に使う UI コンポーネント。スクリプトからは Graphic と共通の描画色や入力判定を扱える。

**共通メンバー**

- [Graphic](#graphic) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`color`、`raycastTarget`、`CrossFadeColor`、`TryGetComponent`、`GetComponent`

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Graphic@ opImplCast() / const Graphic@ opImplCast() const` | `Graphic@ base = image; const Image@ original = image; const Graphic@ view = original;` | 代入で Graphic として参照する。const の参照も代入できる。 |
| `Behavior@ opImplCast() / const Behavior@ opImplCast() const` | `Behavior@ base = image; const Image@ original = image; const Behavior@ view = original;` | 代入で Behavior として参照する。const の参照も代入できる。 |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = image; const Image@ original = image; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = image; const Image@ original = image; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### Selectable

操作の可否を持つ UI コンポーネント。有効状態は Behavior 型として参照できる。

**共通メンバー**

- [Behavior](#behavior) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `bool interactable（読み書き）` | `selectable.interactable = false; bool canPress = selectable.interactable;` | ボタンなどの UI を操作可能にするかを読み書きする。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Button@ opCast() / const Button@ opCast() const` | `Button@ derived = cast<Button>(selectable); const Selectable@ original = selectable; const Button@ view = cast<const Button@>(original);` | 実体を Button として取り出す。型が合わなければ null。const の参照からも変換できる。 |
| `Behavior@ opImplCast() / const Behavior@ opImplCast() const` | `Behavior@ base = selectable; const Selectable@ original = selectable; const Behavior@ view = original;` | 代入で Behavior として参照する。const の参照も代入できる。 |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = selectable; const Selectable@ original = selectable; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = selectable; const Selectable@ original = selectable; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### Button

押したときのイベントを持つ UI コンポーネント。有効状態をスクリプトから扱う場合は Behavior 型として参照できる。

**共通メンバー**

- [Selectable](#selectable) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`interactable`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `UnityEvent@ onClick（読み取り）` | `UnityEvent@ clicked = button.onClick;` | クリック時に発火するイベントを返す。Inspector の On Click の登録と、スクリプトからのリスナーを扱える。 |

**ハンドルの型変換**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Selectable@ opImplCast() / const Selectable@ opImplCast() const` | `Selectable@ base = button; const Button@ original = button; const Selectable@ view = original;` | 代入で Selectable として参照する。const の参照も代入できる。 |
| `Behavior@ opImplCast() / const Behavior@ opImplCast() const` | `Behavior@ base = button; const Button@ original = button; const Behavior@ view = original;` | 代入で Behavior として参照する。const の参照も代入できる。 |
| `Component@ opImplCast() / const Component@ opImplCast() const` | `Component@ base = button; const Button@ original = button; const Component@ view = original;` | 代入で Component として参照する。const の参照も代入できる。 |
| `Object@ opImplCast() / const Object@ opImplCast() const` | `Object@ base = button; const Button@ original = button; const Object@ view = original;` | 代入で Object として参照する。const の参照も代入できる。 |

### UnityEvent

Inspector とスクリプトから関数を登録できるイベント。例の clicked は button.onClick で取得した UnityEvent@。

| API | 用例 | 解説 |
| --- | --- | --- |
| `void AddListener(UnityAction@ call)` | `clicked.AddListener(UnityAction(this.HandleClick));` | 実行中のリスナーに引数なし・戻り値 void の関数を登録する。 |
| `void RemoveListener(UnityAction@ call)` | `clicked.RemoveListener(UnityAction(this.HandleClick));` | 同じ登録先とメソッドを指定して、実行中のリスナーを解除する。 |
| `void RemoveAllListeners()` | `clicked.RemoveAllListeners();` | スクリプトが追加した実行中のリスナーをすべて解除する。Inspector で設定したリスナーは残る。 |
| `void Invoke()` | `clicked.Invoke();` | イベントを明示的に発火して登録済みの処理を呼び出す。 |
| `int GetPersistentEventCount() const` | `int count = clicked.GetPersistentEventCount();` | Inspector から保存されたリスナーの件数を返す。 |
| `string GetPersistentMethodName(int index) const` | `if (clicked.GetPersistentEventCount() > 0) { string method = clicked.GetPersistentMethodName(0); }` | Inspector のリスナーのメソッド名を 0 始まりの番号で取得する。 |

## 入力・時間・画面・診断

### Input

ゲームに届いたキーボード・マウス・入力軸を読む。エディターでは Play 中に Game パネルにフォーカスを置く。

| API | 用例 | 解説 |
| --- | --- | --- |
| `bool GetKey(KeyCode key)` | `bool held = Input::GetKey(KeyCode::Space);` | 指定キーを押し続けている間 true。 |
| `bool GetKeyDown(KeyCode key)` | `bool pressed = Input::GetKeyDown(KeyCode::Space);` | 指定キーを押したフレームのみ true。 |
| `bool GetKeyUp(KeyCode key)` | `bool released = Input::GetKeyUp(KeyCode::Space);` | 指定キーを離したフレームのみ true。 |
| `bool GetMouseButton(int button)` | `bool held = Input::GetMouseButton(0);` | 指定ボタンが押されているかを読む。0 は左、1 は右、2 は中、3・4 は追加ボタン。 |
| `bool GetMouseButtonDown(int button)` | `bool pressed = Input::GetMouseButtonDown(0);` | 指定マウスボタンを押したフレームのみ true。 |
| `bool GetMouseButtonUp(int button)` | `bool released = Input::GetMouseButtonUp(0);` | 指定マウスボタンを離したフレームのみ true。 |
| `Vector3 mousePosition（読み取り）` | `Vector3 cursor = Input::mousePosition;` | 画面内のカーソル位置。左上が原点で Y は下向きに増える。 |
| `Vector2 mouseScrollDelta（読み取り）` | `Vector2 scroll = Input::mouseScrollDelta;` | そのフレームのマウスホイール移動。上下方向は y を読む。 |
| `float GetAxis(const string&in axisName)` | `float x = Input::GetAxis("Horizontal");` | 名前で入力軸を読む。Horizontal は A/D、左右キー、左スティック X。Vertical は W/S、上下キー、左スティック Y。ほかに Mouse X、Mouse Y、Mouse ScrollWheel がある。パッドは Right Stick X / Right Stick Y（右スティック、上が +）、Left Trigger / Right Trigger（0〜1）、Dpad X / Dpad Y（十字キー、-1・0・1）。パッドの軸は 1 番目のパッドを読む。 |
| `float GetAxisRaw(const string&in axisName)` | `float y = Input::GetAxisRaw("Vertical");` | 名前で入力軸を読む。この版では GetAxis と同じ値を返す。 |

### Time

ゲーム時間、フレームの経過時間と固定更新の間隔を扱う。

| API | 用例 | 解説 |
| --- | --- | --- |
| `float deltaTime（読み取り）` | `float seconds = Time::deltaTime;` | 時間スケールを反映した経過秒。移動速度に掛けて 1 フレームの変位を求める。 |
| `float unscaledDeltaTime（読み取り）` | `float seconds = Time::unscaledDeltaTime;` | 時間スケールを掛ける前の経過秒。スロー中も等速の UI などに使える。 |
| `float time（読み取り）` | `float seconds = Time::time;` | ゲーム開始からの時間スケールを反映した経過秒。 |
| `int frameCount（読み取り）` | `int frames = Time::frameCount;` | 進んだゲームフレームの数。 |
| `float timeScale（読み書き）` | `Time::timeScale = 0.5f; float scale = Time::timeScale;` | ゲーム時間の倍率を読み書きする。1 で等速、0 でゲーム時間が止まる。 |
| `float fixedDeltaTime（読み書き）` | `Time::fixedDeltaTime = 0.02f; float step = Time::fixedDeltaTime;` | 物理と固定更新の時間間隔を秒で読み書きする。 |

### Screen

描画サイズや表示モードを扱う。

| API | 用例 | 解説 |
| --- | --- | --- |
| `int width（読み取り）` | `int width = Screen::width;` | ゲームの描画幅をピクセルで読む。 |
| `int height（読み取り）` | `int height = Screen::height;` | ゲームの描画高をピクセルで読む。 |
| `FullScreenMode fullScreenMode（読み書き）` | `Screen::fullScreenMode = FullScreenMode::Windowed; FullScreenMode mode = Screen::fullScreenMode;` | ウィンドウの表示方式を読み書きする。 |
| `bool fullScreen（読み書き）` | `Screen::fullScreen = true; bool full = Screen::fullScreen;` | フルスクリーンかどうかを読み書きする。方式を指定するときは fullScreenMode を使う。 |
| `void SetResolution(int width, int height, FullScreenMode mode)` | `Screen::SetResolution(1280, 720, FullScreenMode::Windowed);` | 幅、高さと表示方式を指定する。書き出したゲームで外側のウィンドウを確認できる。 |

### Cursor

マウスカーソルの表示・画像と、ゲーム画面への固定を扱う。エディタでは Game ビューにフォーカスがあるときだけ効き、Esc で固定と非表示が一時的に解除され（画像はそのまま）、Game ビューをクリックすると戻る。Play を止めると visible は true、lockState は None、画像は矢印に戻る。シーンを切り替えても戻らないので、シーンごとに必要な状態を Start で設定する。

| API | 用例 | 解説 |
| --- | --- | --- |
| `bool visible（読み書き）` | `Cursor::visible = false; bool shown = Cursor::visible;` | ゲーム画面の上でカーソルを表示するかを読み書きする。 |
| `CursorLockMode lockState（読み書き）` | `Cursor::lockState = CursorLockMode::Locked; CursorLockMode mode = Cursor::lockState;` | カーソルの固定方法を読み書きする。Locked のときは visible に関係なく隠れる。 |
| `void SetCursor(Texture2D@ texture, const Vector2 &in hotspot, CursorMode cursorMode)` | `Cursor::SetCursor(cursorTexture, Vector2(0, 0), CursorMode::Auto);` | カーソルを画像にする。hotspot は画像の左上からのピクセルで、クリックの位置になる。texture に null を渡すと矢印に戻る。画像は Inspector で種類を Cursor にしておく（違うとエラーが出て変わらない）。32x32 ほどの大きさを勧める。 |

### Application

再生状態の確認やアプリケーションの終了に使う。

| API | 用例 | 解説 |
| --- | --- | --- |
| `void Quit()` | `Application::Quit();` | 書き出したゲームの終了を要求する。エディターでは何も起きない（Unity と同じ）。 |
| `bool isPlaying（読み取り）` | `bool playing = Application::isPlaying;` | ゲームが再生中かを読む。エディターの編集モードと Play を区別できる。 |

### Debug

Console とログへメッセージを出す。

| API | 用例 | 解説 |
| --- | --- | --- |
| `void Log(const string&in message)` | `Debug::Log("Loaded stage");` | 通常の情報を Console とログに記録する。 |
| `void LogWarning(const string&in message)` | `Debug::LogWarning("Target is missing");` | 警告を記録する。回復できる設定漏れなどに使う。 |
| `void LogError(const string&in message)` | `Debug::LogError("Could not load data");` | エラーを記録する。重大な失敗の原因を残す。 |

### Gizmos

エディターで配置や向きの目印を描く。描画は OnDrawGizmos または OnDrawGizmosSelected に書く。

| API | 用例 | 解説 |
| --- | --- | --- |
| `Color color（読み書き）` | `Gizmos::color = Color::cyan; Color previous = Gizmos::color;` | この後に描くギズモの色を読み書きする。 |
| `void DrawLine(const Vector3&in from, const Vector3&in to)` | `Gizmos::DrawLine(t.position, t.position + Vector3::up);` | 指定したワールド座標の 2 点を線で結ぶ。 |
| `void DrawRay(const Vector3&in from, const Vector3&in direction)` | `Gizmos::DrawRay(t.position, t.forward * 2.0f);` | 起点と方向ベクトルで線を描く。ベクトルの長さが線の長さ。 |
| `void DrawWireCube(const Vector3&in center, const Vector3&in size)` | `Gizmos::DrawWireCube(t.position, Vector3(2, 1, 2));` | 中心と各軸の寸法を指定し、箱の輪郭を描く。 |
| `void DrawWireSphere(const Vector3&in center, float radius)` | `Gizmos::DrawWireSphere(t.position, 2.0f);` | 中心と半径を指定し、球の輪郭を描く。 |
| `void DrawFrustum(const Vector3&in center, float fov, float maxRange, float minRange, float aspect)` | `Gizmos::DrawFrustum(t.position, 60.0f, 10.0f, 0.1f, 16.0f / 9.0f);` | 中心、縦視野角、遠距離、近距離、横縦比を指定し、視錐台の輪郭を描く。 |
| `void DrawIcon(const Vector3&in center, const string&in name, bool allowScaling = true)` | `Gizmos::DrawIcon(t.position, "Camera.png");` | ワールド座標を基準にアイコンを表示する。Assets/Gizmos/ と EngineAssets/Gizmos/ が検索対象。 |
| `void DrawIcon(const Vector3&in center, const string&in name, bool allowScaling, const Color&in tint)` | `Gizmos::DrawIcon(t.position, "Camera.png", false, Color::cyan);` | 拡縮の指定に加え、アイコンに掛ける色を渡す。 |

## 値型・文字列・配列

### Vector2

X・Y の 2 成分を持つ値型。平面の座標や UV などに使う。

| API | 用例 | 解説 |
| --- | --- | --- |
| `Vector2()` | `Vector2 v;` | 各成分が 0 の 2 次元ベクトルを作る。 |
| `Vector2(float x, float y)` | `Vector2 v(3.0f, 4.0f);` | X と Y を指定して 2 次元ベクトルを作る。 |
| `Vector2 opAdd(const Vector2&in) const` | `Vector2 result = v + Vector2::one;` | 値を加える。 |
| `Vector2 opSub(const Vector2&in) const` | `Vector2 result = v - Vector2::one;` | 成分ごとに差を取る。 |
| `Vector2 opMul(float) const` | `Vector2 result = v * 2.0f;` | ベクトルや色の各成分を指定した数でスケールする。 |
| `Vector2 opMul_r(float) const` | `Vector2 result = 2.5f * v;` | 数値を左側に置いて値を掛ける。 |
| `Vector2 opDiv(float) const` | `Vector2 result = v / 2.0f;` | ベクトルや色の各成分を指定した数で割る。 |
| `bool opEquals(const Vector2&in) const` | `bool result = v == Vector2::one;` | 値が等しいかを比較する。 |
| `Vector2& opAddAssign(const Vector2&in)` | `v += Vector2::one;` | 加えた結果を左側の変数に書き戻す。 |
| `Vector2& opSubAssign(const Vector2&in)` | `v -= Vector2::one;` | 差を左側の変数に書き戻す。 |
| `Vector2& opMulAssign(float)` | `v *= 2.0f;` | 積を左側の変数に書き戻す。 |
| `Vector2& opDivAssign(float)` | `v /= 2.0f;` | 商を左側の変数に書き戻す。 |
| `string ToString() const` | `Debug::Log(v.ToString());` | 成分をデバッグ表示しやすい文字列にする。 |
| `Vector2 opNeg() const` | `Vector2 inverse = -v;` | 各成分の符号を反転する。 |
| `float magnitude（読み取り）` | `float length = v.magnitude;` | 原点からのベクトルの長さを返す。 |
| `float sqrMagnitude（読み取り）` | `float lengthSquared = v.sqrMagnitude;` | 長さの二乗。距離を比較するだけなら平方根の計算を省ける。 |
| `Vector2 normalized（読み取り）` | `Vector2 direction = v.normalized;` | 向きを保ったまま長さを 1 にした値を返す。 |
| `Vector3 opImplConv() const` | `Vector3 position = v;` | Vector2 の X と Y を保って Z に 0 を入れ、Vector3 に暗黙変換する。 |
| `float x（読み書き）` | `v.x = 2.0f; float x = v.x;` | X 成分。 |
| `float y（読み書き）` | `v.y = 2.0f; float y = v.y;` | Y 成分。 |

**名前空間 `Vector2`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `float Dot(const Vector2&in lhs, const Vector2&in rhs)` | `float dot = Vector2::Dot(Vector2::right, Vector2::up);` | 2 つのベクトルの内積を求める。直交する単位軸の内積は 0。 |
| `float Distance(const Vector2&in a, const Vector2&in b)` | `float d = Vector2::Distance(Vector2::zero, Vector2(3, 4));` | 2 点間の距離を求める。例の結果は 5。 |
| `Vector2 Lerp(const Vector2&in a, const Vector2&in b, float t)` | `Vector2 p = Vector2::Lerp(Vector2::zero, Vector2::one, 0.25f);` | 0～1 に制限した割合で 2 点間を線形補間する。 |
| `Vector2 LerpUnclamped(const Vector2&in a, const Vector2&in b, float t)` | `Vector2 p = Vector2::LerpUnclamped(Vector2::zero, Vector2::one, 2.0f);` | 範囲外の割合も使える線形補間。例では終点を越える。 |
| `Vector2 MoveTowards(const Vector2&in current, const Vector2&in target, float maxDistanceDelta)` | `Vector2 p = Vector2::MoveTowards(Vector2::zero, Vector2::one, 0.1f);` | 目標に向かって指定した最大距離だけ進める。 |
| `Vector2 Scale(const Vector2&in a, const Vector2&in b)` | `Vector2 p = Vector2::Scale(Vector2(2, 3), Vector2(4, 5));` | 対応する成分同士を掛ける。例の結果は (8, 15)。 |
| `Vector2 Min(const Vector2&in lhs, const Vector2&in rhs)` | `Vector2 p = Vector2::Min(Vector2(2, 5), Vector2(3, 4));` | 各軸の小さい成分を選ぶ。 |
| `Vector2 Max(const Vector2&in lhs, const Vector2&in rhs)` | `Vector2 p = Vector2::Max(Vector2(2, 5), Vector2(3, 4));` | 各軸の大きい成分を選ぶ。 |
| `const Vector2 zero` | `Vector2 value = Vector2::zero;` | (0, 0) の零ベクトル。 |
| `const Vector2 one` | `Vector2 value = Vector2::one;` | (1, 1) のベクトル。 |
| `const Vector2 up` | `Vector2 value = Vector2::up;` | 上、Y が +1 の単位ベクトル。 |
| `const Vector2 down` | `Vector2 value = Vector2::down;` | 下、Y が -1 の単位ベクトル。 |
| `const Vector2 left` | `Vector2 value = Vector2::left;` | 左、X が -1 の単位ベクトル。 |
| `const Vector2 right` | `Vector2 value = Vector2::right;` | 右、X が +1 の単位ベクトル。 |

### Vector3

X・Y・Z の 3 成分を持つ値型。位置や方向、速度などに使う。

| API | 用例 | 解説 |
| --- | --- | --- |
| `Vector3()` | `Vector3 v;` | 各成分が 0 の 3 次元ベクトルを作る。 |
| `Vector3(float x, float y, float z)` | `Vector3 v(1.0f, 2.0f, 3.0f);` | X・Y・Z を指定して 3 次元ベクトルを作る。 |
| `Vector3(float x, float y)` | `Vector3 v(1.0f, 2.0f);` | X・Y を指定し、Z は 0 にする。 |
| `Vector3 opAdd(const Vector3&in) const` | `Vector3 result = v + Vector3::one;` | 値を加える。 |
| `Vector3 opSub(const Vector3&in) const` | `Vector3 result = v - Vector3::one;` | 成分ごとに差を取る。 |
| `Vector3 opMul(float) const` | `Vector3 result = v * 2.0f;` | ベクトルや色の各成分を指定した数でスケールする。 |
| `Vector3 opMul_r(float) const` | `Vector3 result = 2.5f * v;` | 数値を左側に置いて値を掛ける。 |
| `Vector3 opDiv(float) const` | `Vector3 result = v / 2.0f;` | ベクトルや色の各成分を指定した数で割る。 |
| `bool opEquals(const Vector3&in) const` | `bool result = v == Vector3::one;` | 値が等しいかを比較する。 |
| `Vector3& opAddAssign(const Vector3&in)` | `v += Vector3::one;` | 加えた結果を左側の変数に書き戻す。 |
| `Vector3& opSubAssign(const Vector3&in)` | `v -= Vector3::one;` | 差を左側の変数に書き戻す。 |
| `Vector3& opMulAssign(float)` | `v *= 2.0f;` | 積を左側の変数に書き戻す。 |
| `Vector3& opDivAssign(float)` | `v /= 2.0f;` | 商を左側の変数に書き戻す。 |
| `string ToString() const` | `Debug::Log(v.ToString());` | 成分をデバッグ表示しやすい文字列にする。 |
| `Vector3 opNeg() const` | `Vector3 inverse = -v;` | 各成分の符号を反転する。 |
| `float magnitude（読み取り）` | `float length = v.magnitude;` | 原点からのベクトルの長さを返す。 |
| `float sqrMagnitude（読み取り）` | `float lengthSquared = v.sqrMagnitude;` | 長さの二乗。距離を比較するだけなら平方根の計算を省ける。 |
| `Vector3 normalized（読み取り）` | `Vector3 direction = v.normalized;` | 向きを保ったまま長さを 1 にした値を返す。 |
| `float x（読み書き）` | `v.x = 2.0f; float x = v.x;` | X 成分。 |
| `float y（読み書き）` | `v.y = 2.0f; float y = v.y;` | Y 成分。 |
| `float z（読み書き）` | `v.z = 2.0f; float z = v.z;` | Z 成分。 |

**名前空間 `Vector3`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `float Dot(const Vector3&in lhs, const Vector3&in rhs)` | `float dot = Vector3::Dot(Vector3::forward, Vector3::up);` | 3 次元ベクトルの内積を求める。単位方向同士なら向きが揃うほど 1 に近づく。 |
| `Vector3 Cross(const Vector3&in lhs, const Vector3&in rhs)` | `Vector3 perpendicular = Vector3::Cross(Vector3::up, Vector3::forward);` | 2 方向に垂直な外積を求める。入力順を逆にすると符号が反転する。 |
| `float Distance(const Vector3&in a, const Vector3&in b)` | `float d = Vector3::Distance(Vector3::zero, Vector3(0, 3, 4));` | ワールド位置など、3 次元の 2 点の距離を求める。 |
| `float Angle(const Vector3&in from, const Vector3&in to)` | `float degrees = Vector3::Angle(Vector3::forward, Vector3::up);` | 2 本のベクトルがなす角度を度で求める。例の結果は 90。 |
| `Vector3 Lerp(const Vector3&in a, const Vector3&in b, float t)` | `Vector3 p = Vector3::Lerp(Vector3::zero, Vector3::one, 0.5f);` | 0～1 に制限した割合で 2 点を線形補間する。 |
| `Vector3 LerpUnclamped(const Vector3&in a, const Vector3&in b, float t)` | `Vector3 p = Vector3::LerpUnclamped(Vector3::zero, Vector3::one, 2.0f);` | 範囲外の割合も使える線形補間。 |
| `Vector3 MoveTowards(const Vector3&in current, const Vector3&in target, float maxDistanceDelta)` | `Vector3 p = Vector3::MoveTowards(Vector3::zero, Vector3::one, 0.1f);` | 終点を飛び越えないよう、目標方向に最大指定距離だけ進める。 |
| `Vector3 Normalize(const Vector3&in value)` | `Vector3 direction = Vector3::Normalize(Vector3(0, 3, 4));` | 長さを 1 にそろえた方向ベクトルを返す。 |
| `Vector3 ClampMagnitude(const Vector3&in vector, float maxLength)` | `Vector3 velocity = Vector3::ClampMagnitude(Vector3(0, 3, 4), 2.0f);` | 方向を保ちながら長さを上限以下にする。 |
| `Vector3 Scale(const Vector3&in a, const Vector3&in b)` | `Vector3 p = Vector3::Scale(Vector3(1, 2, 3), Vector3(2, 3, 4));` | 対応する成分同士を掛ける。 |
| `Vector3 Min(const Vector3&in lhs, const Vector3&in rhs)` | `Vector3 p = Vector3::Min(Vector3(2, 1, 4), Vector3(3, 0, 5));` | 各軸の小さい成分を選ぶ。 |
| `Vector3 Max(const Vector3&in lhs, const Vector3&in rhs)` | `Vector3 p = Vector3::Max(Vector3(2, 1, 4), Vector3(3, 0, 5));` | 各軸の大きい成分を選ぶ。 |
| `const Vector3 zero` | `Vector3 value = Vector3::zero;` | (0, 0, 0) の零ベクトル。 |
| `const Vector3 one` | `Vector3 value = Vector3::one;` | (1, 1, 1) のベクトル。 |
| `const Vector3 up` | `Vector3 value = Vector3::up;` | 上、Y が +1 の単位ベクトル。 |
| `const Vector3 down` | `Vector3 value = Vector3::down;` | 下、Y が -1 の単位ベクトル。 |
| `const Vector3 left` | `Vector3 value = Vector3::left;` | 左、X が -1 の単位ベクトル。 |
| `const Vector3 right` | `Vector3 value = Vector3::right;` | 右、X が +1 の単位ベクトル。 |
| `const Vector3 forward` | `Vector3 value = Vector3::forward;` | 前、Z が +1 の単位ベクトル。 |
| `const Vector3 back` | `Vector3 value = Vector3::back;` | 後ろ、Z が -1 の単位ベクトル。 |

### Quaternion

回転を表す四元数。角度から作る場合は Euler や AngleAxis を使う。

| API | 用例 | 解説 |
| --- | --- | --- |
| `Quaternion()` | `Quaternion q;` | 回転を表す値を初期化する。回転なしを明示する場合は Quaternion::identity を使える。 |
| `Quaternion(float x, float y, float z, float w)` | `Quaternion q(0.0f, 0.0f, 0.0f, 1.0f);` | 四元数の 4 成分を直接指定する。例は単位四元数。 |
| `Quaternion opMul(const Quaternion&in) const` | `Quaternion result = q * Quaternion::Euler(0, 45, 0);` | 2 つの回転を合成する。 |
| `Vector3 opMul(const Vector3&in) const` | `Vector3 result = q * Vector3::forward;` | ベクトルを四元数の回転で回す。 |
| `bool opEquals(const Quaternion&in) const` | `bool result = q == Quaternion::identity;` | 値が等しいかを比較する。 |
| `string ToString() const` | `Debug::Log(q.ToString());` | 成分をデバッグ表示しやすい文字列にする。 |
| `Vector3 eulerAngles（読み書き）` | `q.eulerAngles = Vector3(0, 90, 0); Vector3 degrees = q.eulerAngles;` | 度単位のオイラー角として回転を読み書きする。 |
| `Quaternion normalized（読み取り）` | `Quaternion unit = q.normalized;` | 四元数の長さを 1 にそろえた値を返す。 |
| `float x（読み書き）` | `q.x = 0.0f; float x = q.x;` | 四元数の X 成分。角度で指定するときは Euler を使う。 |
| `float y（読み書き）` | `q.y = 0.0f; float y = q.y;` | 四元数の Y 成分。 |
| `float z（読み書き）` | `q.z = 0.0f; float z = q.z;` | 四元数の Z 成分。 |
| `float w（読み書き）` | `q.w = 1.0f; float w = q.w;` | 四元数の W 成分。 |

**名前空間 `Quaternion`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Quaternion Euler(float x, float y, float z)` | `Quaternion q = Quaternion::Euler(0, 90, 0);` | 軸ごとの度数を個別に渡して回転を作る。 |
| `Quaternion Euler(const Vector3&in euler)` | `Quaternion q = Quaternion::Euler(Vector3(0, 90, 0));` | 軸ごとの度数を Vector3 で渡して回転を作る。 |
| `Quaternion AngleAxis(float angle, const Vector3&in axis)` | `Quaternion q = Quaternion::AngleAxis(90, Vector3::up);` | 指定した軸を中心とする、指定度数の回転を作る。 |
| `float Angle(const Quaternion&in a, const Quaternion&in b)` | `float degrees = Quaternion::Angle(Quaternion::identity, Quaternion::Euler(0, 90, 0));` | 2 つの回転の差を角度（度）で求める。 |
| `float Dot(const Quaternion&in a, const Quaternion&in b)` | `float dot = Quaternion::Dot(Quaternion::identity, q);` | 四元数同士の内積を求める。 |
| `Quaternion Inverse(const Quaternion&in rotation)` | `Quaternion undo = Quaternion::Inverse(q);` | 指定した回転を打ち消す逆回転を求める。 |
| `Quaternion Lerp(const Quaternion&in a, const Quaternion&in b, float t)` | `Quaternion halfway = Quaternion::Lerp(Quaternion::identity, q, 0.5f);` | 回転間を線形補間し、結果を正規化する。 |
| `Quaternion Slerp(const Quaternion&in a, const Quaternion&in b, float t)` | `Quaternion halfway = Quaternion::Slerp(Quaternion::identity, q, 0.5f);` | 回転間を球面線形補間する。 |
| `Quaternion FromToRotation(const Vector3&in fromDirection, const Vector3&in toDirection)` | `Quaternion q = Quaternion::FromToRotation(Vector3::forward, Vector3::right);` | 最初の方向から次の方向へ向きを変える回転を作る。 |
| `Quaternion LookRotation(const Vector3&in forward, const Vector3&in upwards = Vector3::up)` | `Quaternion q = Quaternion::LookRotation(Vector3::right, Vector3::up);` | 指定した前方向と上方向から姿勢を作る。上方向の既定は Vector3::up。 |
| `const Quaternion identity` | `Quaternion q = Quaternion::identity;` | 回転なしを表す定数。 |

### Color

赤・緑・青・アルファの 4 成分を持つ値型。

| API | 用例 | 解説 |
| --- | --- | --- |
| `Color()` | `Color tint;` | 色の値を既定の状態で初期化する。 |
| `Color(float r, float g, float b)` | `Color tint(1.0f, 0.5f, 0.0f);` | RGB を指定して不透明な色を作る。 |
| `Color(float r, float g, float b, float a)` | `Color tint(1.0f, 0.5f, 0.0f, 0.5f);` | RGB に加えて透明度を指定する。例では半透明。 |
| `Color opAdd(const Color&in) const` | `Color result = tint + Color::white;` | 値を加える。 |
| `Color opSub(const Color&in) const` | `Color result = tint - Color::white;` | 成分ごとに差を取る。 |
| `Color opMul(float) const` | `Color result = tint * 2.0f;` | ベクトルや色の各成分を指定した数でスケールする。 |
| `Color opMul_r(float) const` | `Color result = 2.5f * tint;` | 数値を左側に置いて値を掛ける。 |
| `Color opDiv(float) const` | `Color result = tint / 2.0f;` | ベクトルや色の各成分を指定した数で割る。 |
| `bool opEquals(const Color&in) const` | `bool result = tint == Color::white;` | 値が等しいかを比較する。 |
| `Color& opAddAssign(const Color&in)` | `tint += Color::white;` | 加えた結果を左側の変数に書き戻す。 |
| `Color& opSubAssign(const Color&in)` | `tint -= Color::white;` | 差を左側の変数に書き戻す。 |
| `Color& opMulAssign(float)` | `tint *= 2.0f;` | 積を左側の変数に書き戻す。 |
| `Color& opDivAssign(float)` | `tint /= 2.0f;` | 商を左側の変数に書き戻す。 |
| `string ToString() const` | `Debug::Log(tint.ToString());` | 成分をデバッグ表示しやすい文字列にする。 |
| `Color opMul(const Color&in) const` | `Color result = tint * Color::white;` | 対応する色の成分同士を掛ける。 |
| `float r（読み書き）` | `tint.r = 1.0f; float red = tint.r;` | 赤の成分。 |
| `float g（読み書き）` | `tint.g = 0.5f; float green = tint.g;` | 緑の成分。 |
| `float b（読み書き）` | `tint.b = 0.25f; float blue = tint.b;` | 青の成分。 |
| `float a（読み書き）` | `tint.a = 0.5f; float opacity = tint.a;` | アルファ成分。通常 0 は透明、1 は不透明。 |

**名前空間 `Color`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Color Lerp(const Color&in a, const Color&in b, float t)` | `Color tint = Color::Lerp(Color::red, Color::blue, 0.5f);` | 色同士を 0～1 に制限した割合で線形補間する。 |
| `Color LerpUnclamped(const Color&in a, const Color&in b, float t)` | `Color tint = Color::LerpUnclamped(Color::red, Color::blue, 1.25f);` | 0～1 の範囲外も使える色の線形補間。 |
| `Color HSVToRGB(float h, float s, float v)` | `Color tint = Color::HSVToRGB(0.33f, 1.0f, 1.0f);` | 色相・彩度・明度をそれぞれ 0～1 の値で渡し、RGB の色を得る。 |
| `const Color white` | `Color value = Color::white;` | 白。 |
| `const Color black` | `Color value = Color::black;` | 黒。 |
| `const Color clear` | `Color value = Color::clear;` | RGBA がすべて 0 の透明色。 |
| `const Color red` | `Color value = Color::red;` | 赤。 |
| `const Color green` | `Color value = Color::green;` | 緑。 |
| `const Color blue` | `Color value = Color::blue;` | 青。 |
| `const Color yellow` | `Color value = Color::yellow;` | 黄。 |
| `const Color cyan` | `Color value = Color::cyan;` | シアン。 |
| `const Color magenta` | `Color value = Color::magenta;` | マゼンタ。 |
| `const Color gray` | `Color value = Color::gray;` | 灰色。 |

### Mathf

浮動小数点や整数の計算。Sin・Cos・Tan などの三角関数に渡す角度の単位はラジアン。

| API | 用例 | 解説 |
| --- | --- | --- |
| `float Abs(float f)` | `float x = Mathf::Abs(-2.5f);` | 小数の絶対値を返す。 |
| `int Abs(int value)` | `int x = Mathf::Abs(-2);` | 整数の絶対値を返す。 |
| `float Acos(float f)` | `float radians = Mathf::Acos(0.5f);` | 逆余弦。入力は -1～1、結果はラジアン。 |
| `float Asin(float f)` | `float radians = Mathf::Asin(0.5f);` | 逆正弦。入力は -1～1、結果はラジアン。 |
| `float Atan2(float y, float x)` | `float radians = Mathf::Atan2(1.0f, 1.0f);` | Y と X から四象限を区別した角度をラジアンで求める。 |
| `bool Approximately(float a, float b)` | `bool near = Mathf::Approximately(0.1f + 0.2f, 0.3f);` | 浮動小数点の丸め誤差を考慮して、2 値が十分に近いか調べる。 |
| `float Ceil(float f)` | `float high = Mathf::Ceil(2.2f);` | 入力以上の最小の整数値を float で返す。 |
| `int CeilToInt(float f)` | `int high = Mathf::CeilToInt(2.2f);` | 上方向に丸め、結果を int で返す。 |
| `float Clamp(float value, float min, float max)` | `float v = Mathf::Clamp(2.5f, 0.0f, 2.0f);` | 小数を下限と上限の間に制限する。 |
| `int Clamp(int value, int min, int max)` | `int v = Mathf::Clamp(12, 0, 10);` | 整数を下限と上限の間に制限する。 |
| `float Clamp01(float value)` | `float v = Mathf::Clamp01(1.2f);` | 小数を 0～1 に制限する。 |
| `float Cos(float f)` | `float x = Mathf::Cos(Mathf::PI);` | ラジアンで指定した角度の余弦。 |
| `float Exp(float power)` | `float x = Mathf::Exp(2.0f);` | 自然対数の底 e を指定した指数で累乗する。 |
| `float Floor(float f)` | `float low = Mathf::Floor(2.8f);` | 入力以下の最大の整数値を float で返す。 |
| `int FloorToInt(float f)` | `int low = Mathf::FloorToInt(2.8f);` | 下方向に丸め、結果を int で返す。 |
| `float InverseLerp(float a, float b, float value)` | `float t = Mathf::InverseLerp(0.0f, 10.0f, 3.0f);` | 開始値から終了値までのどの位置に値があるかを 0～1 の割合で求める。 |
| `float Lerp(float a, float b, float t)` | `float x = Mathf::Lerp(0.0f, 10.0f, 0.25f);` | 0～1 に制限した割合で小数を線形補間する。 |
| `float LerpUnclamped(float a, float b, float t)` | `float x = Mathf::LerpUnclamped(0.0f, 10.0f, 1.5f);` | 範囲外の割合も使える線形補間。例の結果は 15。 |
| `float Log(float f)` | `float x = Mathf::Log(2.0f);` | 正の入力の自然対数を返す。 |
| `float Max(float a, float b)` | `float biggest = Mathf::Max(2.0f, 3.5f);` | 小数の大きい方を返す。 |
| `int Max(int a, int b)` | `int biggest = Mathf::Max(2, 3);` | 整数の大きい方を返す。 |
| `float Min(float a, float b)` | `float smallest = Mathf::Min(2.0f, 3.5f);` | 小数の小さい方を返す。 |
| `int Min(int a, int b)` | `int smallest = Mathf::Min(2, 3);` | 整数の小さい方を返す。 |
| `float MoveTowards(float current, float target, float maxDelta)` | `float x = Mathf::MoveTowards(0.0f, 10.0f, 2.0f);` | 目標に向けて最大変化量まで近づける。 |
| `float PingPong(float t, float length)` | `float x = Mathf::PingPong(Time::time, 2.0f);` | 値が進むにつれ、0 と指定長さの間を往復する。 |
| `float Pow(float f, float p)` | `float x = Mathf::Pow(2.0f, 3.0f);` | 底を指数で累乗する。例の結果は 8。 |
| `float Repeat(float t, float length)` | `float x = Mathf::Repeat(Time::time, 2.0f);` | 指定長さを周期として 0 から繰り返す値を求める。 |
| `float Round(float f)` | `float x = Mathf::Round(2.8f);` | 最も近い整数値に丸め、結果を float で返す。 |
| `int RoundToInt(float f)` | `int x = Mathf::RoundToInt(2.8f);` | 最も近い整数に丸め、結果を int で返す。 |
| `float Sign(float f)` | `float sign = Mathf::Sign(-3.0f);` | 入力の符号を返す。負なら -1、それ以外は 1。 |
| `float Sin(float f)` | `float x = Mathf::Sin(Mathf::PI * 0.5f);` | ラジアンで指定した角度の正弦。 |
| `float Sqrt(float f)` | `float x = Mathf::Sqrt(9.0f);` | 平方根を返す。例の結果は 3。 |
| `float Tan(float f)` | `float x = Mathf::Tan(Mathf::PI * 0.25f);` | ラジアンで指定した角度の正接。 |
| `const float PI` | `float circle = 2.0f * Mathf::PI;` | 円周率。円の計算や角度の変換に使う。 |
| `const float Deg2Rad` | `float radians = 90.0f * Mathf::Deg2Rad;` | 度からラジアンへの変換係数。 |
| `const float Rad2Deg` | `float degrees = Mathf::PI * Mathf::Rad2Deg;` | ラジアンから度への変換係数。 |
| `const float Infinity` | `float maxDistance = Mathf::Infinity;` | 正の無限大。たとえばレイの距離上限を指定しないときに使う。 |
| `const float NegativeInfinity` | `float best = Mathf::NegativeInfinity;` | 負の無限大。最大値を探す前の初期値にも使える。 |
| `const float Epsilon` | `float epsilon = Mathf::Epsilon;` | 小さな正の浮動小数点定数。近似比較には Approximately も使える。 |

### Random

乱数とランダムな位置・方向を得る。

| API | 用例 | 解説 |
| --- | --- | --- |
| `void InitState(int seed)` | `Random::InitState(42);` | 乱数生成器の種を指定する。同じ種と呼び出し順なら乱数を再現できる。 |
| `float value（読み取り）` | `float sample = Random::value;` | 0～1 の範囲の乱数を 1 つ得る。 |
| `float Range(float minInclusive, float maxInclusive)` | `float speed = Random::Range(1.0f, 3.0f);` | 最小値・最大値を含む小数の範囲から乱数を得る。 |
| `int Range(int minInclusive, int maxExclusive)` | `int index = Random::Range(0, 3);` | 上限を含まない整数の範囲から乱数を得る。例では 0・1・2。 |
| `Vector3 insideUnitSphere（読み取り）` | `Vector3 offset = Random::insideUnitSphere;` | 半径 1 の球の内側からランダムな座標を得る。 |
| `Vector3 onUnitSphere（読み取り）` | `Vector3 direction = Random::onUnitSphere;` | 半径 1 の球面上からランダムな方向を得る。 |
| `Vector2 insideUnitCircle（読み取り）` | `Vector2 offset = Random::insideUnitCircle;` | 半径 1 の円の内側からランダムな座標を得る。 |

### string

AngelScript の文字列型。添字や検索結果の位置は 0 始まりのバイト位置。

| API | 用例 | 解説 |
| --- | --- | --- |
| `~string()` | `{ string value; }` | 値がスコープから出たときに自動的に破棄される。 |
| `string()` | `string text;` | 空の文字列を作る。 |
| `string(const string&in)` | `string copy(text);` | 既存の文字列をコピーする。 |
| `string& opAssign(const string&in)` | `text = "abc";` | 右辺の内容を左辺へコピーする。 |
| `string& opAddAssign(const string&in)` | `text += "abc";` | 加えた結果を左側の変数に書き戻す。 |
| `bool opEquals(const string&in) const` | `bool result = text == "abc";` | 値が等しいかを比較する。 |
| `int opCmp(const string&in) const` | `bool result = text < "abc";` | 文字列を辞書順で比較する。 |
| `string opAdd(const string&in) const` | `string result = text + "abc";` | 2 つの文字列を連結する。 |
| `uint length() const` | `uint bytes = text.length();` | 文字列の長さをバイト数で返す。UTF-8 の日本語を数えるときは表示上の文字数と異なる。 |
| `void resize(uint)` | `text.resize(3);` | 長さを指定バイト数にする。短くすると末尾が切り詰められ、長くするとヌル文字で埋まる。 |
| `bool isEmpty() const` | `bool empty = text.isEmpty();` | 長さが 0 かを調べる。 |
| `uint8& opIndex(uint)` | `text[0] = 65;` | 指定位置のバイトを読み書きする。添字は 0 始まり。 |
| `const uint8& opIndex(uint) const` | `const string readOnly = text; uint8 value = readOnly[0];` | const 値の添字から、指定位置のバイトを読む。 |
| `string& opAssign(double)` | `text = double(2.5);` | 値を文字列化して代入する。 |
| `string& opAddAssign(double)` | `text += double(2.5);` | 値を文字列化して現在の文字列の末尾へ加える。 |
| `string opAdd(double) const` | `string result = text + double(2.5);` | 右側を文字列化して連結する。 |
| `string opAdd_r(double) const` | `string result = double(2.5) + text;` | 左側の数値または真偽値を文字列化し、右側の文字列と連結する。 |
| `string& opAssign(float)` | `text = 2.5f;` | 値を文字列化して代入する。 |
| `string& opAddAssign(float)` | `text += 2.5f;` | 値を文字列化して現在の文字列の末尾へ加える。 |
| `string opAdd(float) const` | `string result = text + 2.5f;` | 右側を文字列化して連結する。 |
| `string opAdd_r(float) const` | `string result = 2.5f + text;` | 左側の数値または真偽値を文字列化し、右側の文字列と連結する。 |
| `string& opAssign(int64)` | `text = int64(42);` | 値を文字列化して代入する。 |
| `string& opAddAssign(int64)` | `text += int64(42);` | 値を文字列化して現在の文字列の末尾へ加える。 |
| `string opAdd(int64) const` | `string result = text + int64(42);` | 右側を文字列化して連結する。 |
| `string opAdd_r(int64) const` | `string result = int64(42) + text;` | 左側の数値または真偽値を文字列化し、右側の文字列と連結する。 |
| `string& opAssign(uint64)` | `text = uint64(42);` | 値を文字列化して代入する。 |
| `string& opAddAssign(uint64)` | `text += uint64(42);` | 値を文字列化して現在の文字列の末尾へ加える。 |
| `string opAdd(uint64) const` | `string result = text + uint64(42);` | 右側を文字列化して連結する。 |
| `string opAdd_r(uint64) const` | `string result = uint64(42) + text;` | 左側の数値または真偽値を文字列化し、右側の文字列と連結する。 |
| `string& opAssign(bool)` | `text = true;` | 値を文字列化して代入する。 |
| `string& opAddAssign(bool)` | `text += true;` | 値を文字列化して現在の文字列の末尾へ加える。 |
| `string opAdd(bool) const` | `string result = text + true;` | 右側を文字列化して連結する。 |
| `string opAdd_r(bool) const` | `string result = true + text;` | 左側の数値または真偽値を文字列化し、右側の文字列と連結する。 |
| `string substr(uint start = 0, int count = -1) const` | `string part = text.substr(2, 3);` | バイト位置とバイト数で部分文字列を得る。個数を省略すると末尾まで。 |
| `int findFirst(const string&in, uint start = 0) const` | `int index = text.findFirst("ab");` | 開始位置以降で最初に一致する部分文字列の位置を返す。見つからないと -1。 |
| `int findFirstOf(const string&in, uint start = 0) const` | `int index = text.findFirstOf("abc");` | 指定した文字集合に含まれる最初のバイトの位置を返す。 |
| `int findFirstNotOf(const string&in, uint start = 0) const` | `int index = text.findFirstNotOf(" ");` | 指定した文字集合に含まれない最初のバイトの位置を返す。 |
| `int findLast(const string&in, int start = -1) const` | `int index = text.findLast("ab");` | 末尾側から検索し、最後に一致する部分文字列の位置を返す。 |
| `int findLastOf(const string&in, int start = -1) const` | `int index = text.findLastOf("abc");` | 指定した文字集合に含まれる最後のバイトの位置を返す。 |
| `int findLastNotOf(const string&in, int start = -1) const` | `int index = text.findLastNotOf(" ");` | 指定した文字集合に含まれない最後のバイトの位置を返す。 |
| `void insert(uint pos, const string&in other)` | `text.insert(0, "ID: ");` | 指定したバイト位置に文字列を挿入する。 |
| `void erase(uint pos, int count = -1)` | `text.erase(0, 3);` | 指定位置から指定バイト数を削除する。個数を省略すると末尾まで。 |
| `int regexFind(const string&in regex, uint start = 0, uint&out lengthOfMatch = void) const` | `uint size; int index = text.regexFind("[0-9]+", 0, size);` | 正規表現で検索して最初の一致位置を返す。第 3 引数で長さも取得できる。 |

### array

可変長の配列。添字は 0 始まり。例の items は array<int>、objects は array<GameObject@>。

初期値リストから作る場合は `array<int> items = {1, 2, 3};` と書けます。

| API | 用例 | 解説 |
| --- | --- | --- |
| `array<T>()` | `array<int> items;` | 要素数 0 の配列を作る。 |
| `array<T>(uint length)` | `array<int> items(3);` | 指定個数を型の既定値で初期化する。 |
| `array<T>(uint length, const T&in value)` | `array<int> items(3, 7);` | 指定個数の要素を同じ値で初期化する。 |
| `T& opIndex(uint index)` | `items[0] = 9;` | 指定位置の要素を読み書きする。添字は 0 始まり。 |
| `const T& opIndex(uint index) const` | `const array<int> readOnly = items; int value = readOnly[0];` | const 値の添字から、指定位置の要素を読む。 |
| `uint opForBegin() const` | `foreach (const int value, uint index : items) { Debug::Log(index + ": " + value); }` | foreach の内部で使われ、反復の開始位置を作る。 |
| `bool opForEnd(uint) const` | `foreach (const int value, uint index : items) { Debug::Log(index + ": " + value); }` | foreach の内部で使われ、反復の終了位置を判定する。 |
| `uint opForNext(uint) const` | `foreach (const int value, uint index : items) { Debug::Log(index + ": " + value); }` | foreach の内部で使われ、反復位置を次の要素へ進める。 |
| `const T& opForValue0(uint index) const` | `foreach (const int value, uint index : items) { Debug::Log(index + ": " + value); }` | foreach の内部で使われ、foreach の第 1 変数へ値を渡す。 |
| `uint opForValue1(uint index) const` | `foreach (const int value, uint index : items) { Debug::Log(index + ": " + value); }` | foreach の内部で使われ、foreach の第 2 変数へ添字を渡す。 |
| `T[]& opAssign(const T[]&in)` | `array<int> other = {1, 2}; items = other;` | 右辺の内容を左辺へコピーする。 |
| `void insertAt(uint index, const T&in value)` | `items.insertAt(1, 7);` | 指定位置の前に要素を 1 つ挿入する。 |
| `void insertAt(uint index, const T[]&inout arr)` | `array<int> more = {4, 5}; items.insertAt(1, more);` | 指定位置の前に別の配列の全要素を挿入する。 |
| `void insertLast(const T&in value)` | `items.insertLast(7);` | 配列の末尾に要素を追加する。 |
| `void removeAt(uint index)` | `items.removeAt(0);` | 指定位置の要素を削除し、後ろの要素を詰める。 |
| `void removeLast()` | `items.removeLast();` | 末尾の要素を 1 つ削除する。要素があることを確認して使う。 |
| `void removeRange(uint start, uint count)` | `items.removeRange(0, 2);` | 指定位置から指定個数の要素を削除する。 |
| `uint length() const` | `uint count = items.length();` | 配列の現在の要素数を返す。 |
| `void reserve(uint length)` | `items.reserve(64);` | 追加時の再確保を減らすための容量を予約する。要素数はそのまま。 |
| `void resize(uint length)` | `items.resize(5);` | 要素数を変更する。増えた要素には型の既定値が入る。 |
| `void sortAsc()` | `items.sortAsc();` | 全要素を昇順に並べ替える。 |
| `void sortAsc(uint startAt, uint count)` | `items.sortAsc(0, 2);` | 開始位置と要素数で指定した部分を昇順に並べ替える。 |
| `void sortDesc()` | `items.sortDesc();` | 全要素を降順に並べ替える。 |
| `void sortDesc(uint startAt, uint count)` | `items.sortDesc(0, 2);` | 開始位置と要素数で指定した部分を降順に並べ替える。 |
| `void reverse()` | `items.reverse();` | 配列の順番を逆にする。 |
| `int find(const T&in value) const` | `int index = items.find(7);` | 値が等しい最初の要素の位置を返す。見つからないと -1。 |
| `int find(uint startAt, const T&in value) const` | `int index = items.find(1, 7);` | 指定位置から後ろで値が等しい要素を探す。 |
| `int findByRef(const T&in value) const` | `int index = objects.findByRef(go);` | ハンドル配列から同じ参照先を持つ要素を探す。例の objects は array<GameObject@>。 |
| `int findByRef(uint startAt, const T&in value) const` | `int index = objects.findByRef(1, go);` | 指定位置から先で、同じ参照先の要素を探す。 |
| `bool opEquals(const T[]&in) const` | `array<int> other = {1, 2}; bool result = items == other;` | 要素数と各要素の値を比較する。 |
| `bool isEmpty() const` | `bool empty = items.isEmpty();` | 要素が 0 個かを調べる。 |
| `void sort(T[]::less&in, uint startAt = 0, uint count = uint(-1))` | `items.sort(function(a, b) { return a < b; });` | 比較関数が a を b より先に置くとき true を返す。開始位置と要素数を省略すると配列全体を並べ替える。 |

### グローバル関数と関数型

名前空間の指定なしで呼べる文字列関数と、UI イベント用の関数型。

| API | 用例 | 解説 |
| --- | --- | --- |
| `funcdef void UnityAction()` | `UnityAction@ handler = UnityAction(this.HandleClick);` | 引数なしで void を返す関数の型。HandleClick は現在のクラスのメソッド。 |
| `uint scan(const string&in str, ?&out...)` | `int n; float speed; uint count = scan("4 2.5", n, speed);` | 文字列から空白区切りで値を順に出力引数へ読み取り、読み取れた個数を返す。 |
| `string format(const string&in fmt, const ?&in...)` | `string text = format("{} has {} lives", "Player", 3);` | {} を順に引数の値に置き換える。数値、真偽値、文字列などを渡せる。 |
| `string formatInt(int64 val, const string&in options = "", uint width = 0)` | `string text = formatInt(-7);` | 符号付き整数を文字列化する。オプションで表示方法や幅を指定できる。 |
| `string formatUInt(uint64 val, const string&in options = "", uint width = 0)` | `string text = formatUInt(255, "h");` | 符号なし整数を文字列化する。例の h は小文字の 16 進数。 |
| `string formatFloat(double val, const string&in options = "", uint width = 0, uint precision = 0)` | `string text = formatFloat(3.14159, "", 0, 2);` | 実数を文字列化する。例の第 4 引数は小数部の精度指定。 |
| `int64 parseInt(const string&in, uint base = 10, uint&out byteCount = 0)` | `int64 n = parseInt("-42");` | 文字列の先頭から符号付き整数を読む。基数や読み取ったバイト数の出力を指定できる。 |
| `uint64 parseUInt(const string&in, uint base = 10, uint&out byteCount = 0)` | `uint64 n = parseUInt("ff", 16);` | 先頭から符号なし整数を読む。例は基数 16 で結果が 255。 |
| `double parseFloat(const string&in, uint&out byteCount = 0)` | `uint bytes; double n = parseFloat("2.5 m", bytes);` | 先頭から実数を読み、必要なら読み取ったバイト数を受け取る。 |

## 列挙型

### Space

Transform の移動・回転で使う座標系。

| API | 用例 | 解説 |
| --- | --- | --- |
| `Space::World = 0` | `t.Translate(Vector3::forward, Space::World);` | ワールド軸を基準にする。 |
| `Space::Self = 1` | `t.Translate(Vector3::forward, Space::Self);` | 対象のローカル軸を基準にする。 |

### ForceMode

Rigidbody に力を与えるときの適用方式。

| API | 用例 | 解説 |
| --- | --- | --- |
| `ForceMode::Force = 0` | `body.AddForce(Vector3::up * 2.0f, ForceMode::Force);` | 質量を考慮した継続的な力。 |
| `ForceMode::Acceleration = 1` | `body.AddForce(Vector3::up * 2.0f, ForceMode::Acceleration);` | 質量によらない継続的な加速度。 |
| `ForceMode::Impulse = 2` | `body.AddForce(Vector3::up * 2.0f, ForceMode::Impulse);` | 質量を考慮した瞬間的な力積。 |
| `ForceMode::VelocityChange = 3` | `body.AddForce(Vector3::up * 2.0f, ForceMode::VelocityChange);` | 質量によらない瞬間的な速度変化。 |

### CollisionFlags

CharacterController.Move が返す衝突方向。複数方向を同時に表せる。

| API | 用例 | 解説 |
| --- | --- | --- |
| `CollisionFlags::None = 0` | `bool clear = flags == CollisionFlags::None;` | どの方向にも接触していない。 |
| `CollisionFlags::Sides = 1` | `bool contact = (int(flags) & int(CollisionFlags::Sides)) != 0;` | 側面に接触した。 |
| `CollisionFlags::Above = 2` | `bool contact = (int(flags) & int(CollisionFlags::Above)) != 0;` | 上側に接触した。 |
| `CollisionFlags::Below = 4` | `bool contact = (int(flags) & int(CollisionFlags::Below)) != 0;` | 下側に接触した。 |

### FullScreenMode

Screen で設定するウィンドウ表示方式。

| API | 用例 | 解説 |
| --- | --- | --- |
| `FullScreenMode::ExclusiveFullScreen = 0` | `Screen::fullScreenMode = FullScreenMode::ExclusiveFullScreen;` | 排他的フルスクリーンを指定する。 |
| `FullScreenMode::FullScreenWindow = 1` | `Screen::fullScreenMode = FullScreenMode::FullScreenWindow;` | フルスクリーンのウィンドウを指定する。 |
| `FullScreenMode::MaximizedWindow = 2` | `Screen::fullScreenMode = FullScreenMode::MaximizedWindow;` | 最大化ウィンドウを指定する。 |
| `FullScreenMode::Windowed = 3` | `Screen::fullScreenMode = FullScreenMode::Windowed;` | 通常のウィンドウ表示を指定する。 |

### CursorLockMode

Cursor::lockState に設定するカーソルの固定方法。

| API | 用例 | 解説 |
| --- | --- | --- |
| `CursorLockMode::None = 0` | `Cursor::lockState = CursorLockMode::None;` | 固定しない。 |
| `CursorLockMode::Locked = 1` | `Cursor::lockState = CursorLockMode::Locked;` | ゲーム画面の中心に固定して隠す。移動量は `Input::GetAxis("Mouse X")` などで読む。 |
| `CursorLockMode::Confined = 2` | `Cursor::lockState = CursorLockMode::Confined;` | ゲーム画面の外に出ないようにする。 |

### CursorMode

Cursor::SetCursor に渡すカーソルの出し方。

| API | 用例 | 解説 |
| --- | --- | --- |
| `CursorMode::Auto = 0` | `Cursor::SetCursor(cursorTexture, Vector2(0, 0), CursorMode::Auto);` | OS のカーソルとして出す。 |
| `CursorMode::ForceSoftware = 1` | `Cursor::SetCursor(cursorTexture, Vector2(0, 0), CursorMode::ForceSoftware);` | 今は Auto と同じ動きになる。 |

### KeyCode

Input に渡すキー識別子。各行の例では押した瞬間を確認する。

| API | 用例 | 解説 |
| --- | --- | --- |
| `KeyCode::None = 0` | `KeyCode key = KeyCode::None;` | キーを割り当てない値。 |
| `KeyCode::Backspace = 8` | `bool pressed = Input::GetKeyDown(KeyCode::Backspace);` | Backspace キー。 |
| `KeyCode::Tab = 9` | `bool pressed = Input::GetKeyDown(KeyCode::Tab);` | Tab キー。 |
| `KeyCode::Return = 13` | `bool pressed = Input::GetKeyDown(KeyCode::Return);` | Enter キー。 |
| `KeyCode::Pause = 19` | `bool pressed = Input::GetKeyDown(KeyCode::Pause);` | Pause キー。 |
| `KeyCode::CapsLock = 20` | `bool pressed = Input::GetKeyDown(KeyCode::CapsLock);` | Caps Lock キー。 |
| `KeyCode::Escape = 27` | `bool pressed = Input::GetKeyDown(KeyCode::Escape);` | Esc キー。 |
| `KeyCode::Space = 32` | `bool pressed = Input::GetKeyDown(KeyCode::Space);` | スペースキー。 |
| `KeyCode::PageUp = 33` | `bool pressed = Input::GetKeyDown(KeyCode::PageUp);` | Page Up キー。 |
| `KeyCode::PageDown = 34` | `bool pressed = Input::GetKeyDown(KeyCode::PageDown);` | Page Down キー。 |
| `KeyCode::End = 35` | `bool pressed = Input::GetKeyDown(KeyCode::End);` | End キー。 |
| `KeyCode::Home = 36` | `bool pressed = Input::GetKeyDown(KeyCode::Home);` | Home キー。 |
| `KeyCode::LeftArrow = 37` | `bool pressed = Input::GetKeyDown(KeyCode::LeftArrow);` | 左矢印キー。 |
| `KeyCode::UpArrow = 38` | `bool pressed = Input::GetKeyDown(KeyCode::UpArrow);` | 上矢印キー。 |
| `KeyCode::RightArrow = 39` | `bool pressed = Input::GetKeyDown(KeyCode::RightArrow);` | 右矢印キー。 |
| `KeyCode::DownArrow = 40` | `bool pressed = Input::GetKeyDown(KeyCode::DownArrow);` | 下矢印キー。 |
| `KeyCode::Insert = 45` | `bool pressed = Input::GetKeyDown(KeyCode::Insert);` | Insert キー。 |
| `KeyCode::Delete = 46` | `bool pressed = Input::GetKeyDown(KeyCode::Delete);` | Delete キー。 |
| `KeyCode::Alpha0 = 48` | `bool pressed = Input::GetKeyDown(KeyCode::Alpha0);` | メインキーボードの数字 0。 |
| `KeyCode::Alpha1 = 49` | `bool pressed = Input::GetKeyDown(KeyCode::Alpha1);` | メインキーボードの数字 1。 |
| `KeyCode::Alpha2 = 50` | `bool pressed = Input::GetKeyDown(KeyCode::Alpha2);` | メインキーボードの数字 2。 |
| `KeyCode::Alpha3 = 51` | `bool pressed = Input::GetKeyDown(KeyCode::Alpha3);` | メインキーボードの数字 3。 |
| `KeyCode::Alpha4 = 52` | `bool pressed = Input::GetKeyDown(KeyCode::Alpha4);` | メインキーボードの数字 4。 |
| `KeyCode::Alpha5 = 53` | `bool pressed = Input::GetKeyDown(KeyCode::Alpha5);` | メインキーボードの数字 5。 |
| `KeyCode::Alpha6 = 54` | `bool pressed = Input::GetKeyDown(KeyCode::Alpha6);` | メインキーボードの数字 6。 |
| `KeyCode::Alpha7 = 55` | `bool pressed = Input::GetKeyDown(KeyCode::Alpha7);` | メインキーボードの数字 7。 |
| `KeyCode::Alpha8 = 56` | `bool pressed = Input::GetKeyDown(KeyCode::Alpha8);` | メインキーボードの数字 8。 |
| `KeyCode::Alpha9 = 57` | `bool pressed = Input::GetKeyDown(KeyCode::Alpha9);` | メインキーボードの数字 9。 |
| `KeyCode::A = 65` | `bool pressed = Input::GetKeyDown(KeyCode::A);` | A キー。 |
| `KeyCode::B = 66` | `bool pressed = Input::GetKeyDown(KeyCode::B);` | B キー。 |
| `KeyCode::C = 67` | `bool pressed = Input::GetKeyDown(KeyCode::C);` | C キー。 |
| `KeyCode::D = 68` | `bool pressed = Input::GetKeyDown(KeyCode::D);` | D キー。 |
| `KeyCode::E = 69` | `bool pressed = Input::GetKeyDown(KeyCode::E);` | E キー。 |
| `KeyCode::F = 70` | `bool pressed = Input::GetKeyDown(KeyCode::F);` | F キー。 |
| `KeyCode::G = 71` | `bool pressed = Input::GetKeyDown(KeyCode::G);` | G キー。 |
| `KeyCode::H = 72` | `bool pressed = Input::GetKeyDown(KeyCode::H);` | H キー。 |
| `KeyCode::I = 73` | `bool pressed = Input::GetKeyDown(KeyCode::I);` | I キー。 |
| `KeyCode::J = 74` | `bool pressed = Input::GetKeyDown(KeyCode::J);` | J キー。 |
| `KeyCode::K = 75` | `bool pressed = Input::GetKeyDown(KeyCode::K);` | K キー。 |
| `KeyCode::L = 76` | `bool pressed = Input::GetKeyDown(KeyCode::L);` | L キー。 |
| `KeyCode::M = 77` | `bool pressed = Input::GetKeyDown(KeyCode::M);` | M キー。 |
| `KeyCode::N = 78` | `bool pressed = Input::GetKeyDown(KeyCode::N);` | N キー。 |
| `KeyCode::O = 79` | `bool pressed = Input::GetKeyDown(KeyCode::O);` | O キー。 |
| `KeyCode::P = 80` | `bool pressed = Input::GetKeyDown(KeyCode::P);` | P キー。 |
| `KeyCode::Q = 81` | `bool pressed = Input::GetKeyDown(KeyCode::Q);` | Q キー。 |
| `KeyCode::R = 82` | `bool pressed = Input::GetKeyDown(KeyCode::R);` | R キー。 |
| `KeyCode::S = 83` | `bool pressed = Input::GetKeyDown(KeyCode::S);` | S キー。 |
| `KeyCode::T = 84` | `bool pressed = Input::GetKeyDown(KeyCode::T);` | T キー。 |
| `KeyCode::U = 85` | `bool pressed = Input::GetKeyDown(KeyCode::U);` | U キー。 |
| `KeyCode::V = 86` | `bool pressed = Input::GetKeyDown(KeyCode::V);` | V キー。 |
| `KeyCode::W = 87` | `bool pressed = Input::GetKeyDown(KeyCode::W);` | W キー。 |
| `KeyCode::X = 88` | `bool pressed = Input::GetKeyDown(KeyCode::X);` | X キー。 |
| `KeyCode::Y = 89` | `bool pressed = Input::GetKeyDown(KeyCode::Y);` | Y キー。 |
| `KeyCode::Z = 90` | `bool pressed = Input::GetKeyDown(KeyCode::Z);` | Z キー。 |
| `KeyCode::LeftWindows = 91` | `bool pressed = Input::GetKeyDown(KeyCode::LeftWindows);` | 左 Windows キー。 |
| `KeyCode::RightWindows = 92` | `bool pressed = Input::GetKeyDown(KeyCode::RightWindows);` | 右 Windows キー。 |
| `KeyCode::Keypad0 = 96` | `bool pressed = Input::GetKeyDown(KeyCode::Keypad0);` | テンキーの数字 0。 |
| `KeyCode::Keypad1 = 97` | `bool pressed = Input::GetKeyDown(KeyCode::Keypad1);` | テンキーの数字 1。 |
| `KeyCode::Keypad2 = 98` | `bool pressed = Input::GetKeyDown(KeyCode::Keypad2);` | テンキーの数字 2。 |
| `KeyCode::Keypad3 = 99` | `bool pressed = Input::GetKeyDown(KeyCode::Keypad3);` | テンキーの数字 3。 |
| `KeyCode::Keypad4 = 100` | `bool pressed = Input::GetKeyDown(KeyCode::Keypad4);` | テンキーの数字 4。 |
| `KeyCode::Keypad5 = 101` | `bool pressed = Input::GetKeyDown(KeyCode::Keypad5);` | テンキーの数字 5。 |
| `KeyCode::Keypad6 = 102` | `bool pressed = Input::GetKeyDown(KeyCode::Keypad6);` | テンキーの数字 6。 |
| `KeyCode::Keypad7 = 103` | `bool pressed = Input::GetKeyDown(KeyCode::Keypad7);` | テンキーの数字 7。 |
| `KeyCode::Keypad8 = 104` | `bool pressed = Input::GetKeyDown(KeyCode::Keypad8);` | テンキーの数字 8。 |
| `KeyCode::Keypad9 = 105` | `bool pressed = Input::GetKeyDown(KeyCode::Keypad9);` | テンキーの数字 9。 |
| `KeyCode::KeypadMultiply = 106` | `bool pressed = Input::GetKeyDown(KeyCode::KeypadMultiply);` | テンキーの *。 |
| `KeyCode::KeypadPlus = 107` | `bool pressed = Input::GetKeyDown(KeyCode::KeypadPlus);` | テンキーの +。 |
| `KeyCode::KeypadMinus = 109` | `bool pressed = Input::GetKeyDown(KeyCode::KeypadMinus);` | テンキーの -。 |
| `KeyCode::KeypadPeriod = 110` | `bool pressed = Input::GetKeyDown(KeyCode::KeypadPeriod);` | テンキーの小数点。 |
| `KeyCode::KeypadDivide = 111` | `bool pressed = Input::GetKeyDown(KeyCode::KeypadDivide);` | テンキーの /。 |
| `KeyCode::F1 = 112` | `bool pressed = Input::GetKeyDown(KeyCode::F1);` | F1 キー。 |
| `KeyCode::F2 = 113` | `bool pressed = Input::GetKeyDown(KeyCode::F2);` | F2 キー。 |
| `KeyCode::F3 = 114` | `bool pressed = Input::GetKeyDown(KeyCode::F3);` | F3 キー。 |
| `KeyCode::F4 = 115` | `bool pressed = Input::GetKeyDown(KeyCode::F4);` | F4 キー。 |
| `KeyCode::F5 = 116` | `bool pressed = Input::GetKeyDown(KeyCode::F5);` | F5 キー。 |
| `KeyCode::F6 = 117` | `bool pressed = Input::GetKeyDown(KeyCode::F6);` | F6 キー。 |
| `KeyCode::F7 = 118` | `bool pressed = Input::GetKeyDown(KeyCode::F7);` | F7 キー。 |
| `KeyCode::F8 = 119` | `bool pressed = Input::GetKeyDown(KeyCode::F8);` | F8 キー。 |
| `KeyCode::F9 = 120` | `bool pressed = Input::GetKeyDown(KeyCode::F9);` | F9 キー。 |
| `KeyCode::F10 = 121` | `bool pressed = Input::GetKeyDown(KeyCode::F10);` | F10 キー。 |
| `KeyCode::F11 = 122` | `bool pressed = Input::GetKeyDown(KeyCode::F11);` | F11 キー。 |
| `KeyCode::F12 = 123` | `bool pressed = Input::GetKeyDown(KeyCode::F12);` | F12 キー。 |
| `KeyCode::Numlock = 144` | `bool pressed = Input::GetKeyDown(KeyCode::Numlock);` | Num Lock キー。 |
| `KeyCode::ScrollLock = 145` | `bool pressed = Input::GetKeyDown(KeyCode::ScrollLock);` | Scroll Lock キー。 |
| `KeyCode::LeftShift = 160` | `bool pressed = Input::GetKeyDown(KeyCode::LeftShift);` | 左 Shift キー。 |
| `KeyCode::RightShift = 161` | `bool pressed = Input::GetKeyDown(KeyCode::RightShift);` | 右 Shift キー。 |
| `KeyCode::LeftControl = 162` | `bool pressed = Input::GetKeyDown(KeyCode::LeftControl);` | 左 Ctrl キー。 |
| `KeyCode::RightControl = 163` | `bool pressed = Input::GetKeyDown(KeyCode::RightControl);` | 右 Ctrl キー。 |
| `KeyCode::LeftAlt = 164` | `bool pressed = Input::GetKeyDown(KeyCode::LeftAlt);` | 左 Alt キー。 |
| `KeyCode::RightAlt = 165` | `bool pressed = Input::GetKeyDown(KeyCode::RightAlt);` | 右 Alt キー。 |
| `KeyCode::JoystickButton0 = 330` | `bool pressed = Input::GetKeyDown(KeyCode::JoystickButton0);` | どのパッドでもよい A ボタン。番号は Unity（Windows・Xbox のパッド）と同じで、1 = B、2 = X、3 = Y、4 = LB、5 = RB、6 = Back、7 = Start、8 = 左スティック押し込み、9 = 右スティック押し込み。`JoystickButton19 = 349` まであるが、10〜19 は押されない。 |
| `KeyCode::Joystick1Button0 = 350` | `bool pressed = Input::GetKeyDown(KeyCode::Joystick1Button0);` | 1 番目のパッドの A ボタン。ボタンの番号は JoystickButton と同じ。2〜4 番目のパッドは `Joystick2Button0 = 370`、`Joystick3Button0 = 390`、`Joystick4Button0 = 410` から 20 個ずつ。 |

## スクリプトでプロパティを定義する

`t.position` のように読み書きできるプロパティは、自分のクラスにも C# と同じ形で作れます。`get` と `set` のブロックを書くと、値を読むときに `get`、代入するときに `set` が呼ばれます。`set` の中では、代入された値を `value` で受け取ります。

```angelscript
class Player : MonoBehavior
{
    private int m_hp = 100;

    int hp
    {
        get const { return m_hp; }
        set { m_hp = Mathf::Clamp(value, 0, 100); }
    }
}
```

```angelscript
player.hp = 50;       // set が呼ばれる
int now = player.hp;  // get が呼ばれる
player.hp -= 10;      // get で読んでから set で書き戻す
```

- `get` だけを書くと読み取り専用、`set` だけを書くと書き込み専用になります。
- `get` には `const` を付けます。付けないと、`const` の参照からは読めません。
- C# の `int hp { get; set; }` のような自動実装プロパティはありません。値は上の `m_hp` のように private のメンバーへ自分で持たせます。
- 同じプロパティは、`get_` と `set_` で始まるメソッドに `property` を付けて 2 つに分けても書けます。`property` を付けないと、ただのメソッドとして扱われ、`player.hp` の形では呼べません。

```angelscript
int get_hp() const property { return m_hp; }
void set_hp(int value) property { m_hp = Mathf::Clamp(value, 0, 100); }
```
