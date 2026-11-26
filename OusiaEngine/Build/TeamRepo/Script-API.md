# スクリプト API リファレンス

[エディターガイド](README.md) · [スクリプトの書き方](Scripting.md) · [シェーダとポストエフェクトの書き方](Shaders.md)

OusiaEditor が AngelScript に公開する API を、型と名前空間ごとに引けます。名前と使い方は Unity に合わせています。MonoBehavior 自身のプロパティ・メソッドとエンジンが呼び出すコールバックは [スクリプトの書き方](Scripting.md) を参照してください。

表の用例は各 API を示す独立した断片です。たとえば `t` は設定済みの `Transform@`、`body` は `Rigidbody@` です。手元にない参照は Inspector で設定するか、`gameObject.GetComponent<T>()` で取得します。例で指定するタグやレイヤーは Project Settings の Tags and Layers の定義に合わせてください。

プロパティの読み書きは `t.position` や `Time::timeScale` の構文で行います。`&in` は入力用の参照、`&out` は値を受け取る変数を指定します。「共通メンバー」は親の型から受け継いだもので、説明は親の型の節にあります。`opAdd` などの宣言には、スクリプトで使う演算子の形を載せています。

**ハンドルの型変換**：派生した型のハンドルは、親の型の変数にそのまま代入できます（`Collider@ c = box;`、`Object@ o = go;`）。親の型から派生した型へは `cast<型>(…)` で取り出し、型が合わなければ null になります（`BoxCollider@ box = cast<BoxCollider>(collider);`）。const のハンドルでも同じです。

この一覧は、エディターの起動時に作られる `as.predefined`（スクリプトの補完に使うファイル）をもとにしています。ここにない名前は、エディターの版が古いか、まだ公開されていません。

- **オブジェクトとシーン** — [Object](#object)、[GameObject](#gameobject)、[Component](#component)、[Behavior](#behavior)、[Transform](#transform)、[Scene](#scene)、[SceneManager](#scenemanager)、[LayerMask](#layermask)
- **物理** — [Physics](#physics)、[Collider](#collider)、[BoxCollider](#boxcollider)、[SphereCollider](#spherecollider)、[CapsuleCollider](#capsulecollider)、[Rigidbody](#rigidbody)、[CharacterController](#charactercontroller)、[PhysicsMaterial](#physicsmaterial)、[Collision](#collision)、[ContactPoint](#contactpoint)、[RaycastHit](#raycasthit)、[ControllerColliderHit](#controllercolliderhit)
- **カメラ・ライト・描画** — [Camera](#camera)、[Light](#light)、[Renderer](#renderer)、[MeshRenderer](#meshrenderer)、[SkinnedMeshRenderer](#skinnedmeshrenderer)、[LineRenderer](#linerenderer)、[Material](#material)、[Shader](#shader)、[Mesh](#mesh)、[Texture2D](#texture2d)、[Sprite](#sprite)、[VolumeProfile](#volumeprofile)
- **アニメーション・パーティクル** — [SkeletalAnimator](#skeletalanimator)、[SkeletalAnimationClip](#skeletalanimationclip)、[ParticleSystem](#particlesystem)、[ParticleSystemRenderer](#particlesystemrenderer)
- **音声** — [AudioSource](#audiosource)、[AudioClip](#audioclip)、[AudioListener](#audiolistener)
- **UI** — [Canvas](#canvas)、[RectTransform](#recttransform)、[Graphic](#graphic)、[Text](#text)、[Image](#image)、[CanvasRenderer](#canvasrenderer)、[FontAsset](#fontasset)、[Selectable](#selectable)、[Button](#button)、[ColorBlock](#colorblock)、[UnityEvent](#unityevent)
- **入力・時間・画面・診断** — [Input](#input)、[Time](#time)、[Screen](#screen)、[Cursor](#cursor)、[Application](#application)、[Debug](#debug)、[Gizmos](#gizmos)
- **値型・文字列・配列** — [Vector2](#vector2)、[Vector3](#vector3)、[Vector4](#vector4)、[Quaternion](#quaternion)、[Color](#color)、[Matrix4x4](#matrix4x4)、[Rect](#rect)、[Bounds](#bounds)、[Mathf](#mathf)、[Random](#random)、[string](#string)、[array](#array)、[グローバル関数と関数型](#グローバル関数と関数型)
- **列挙型** — [Space](#space)、[ForceMode](#forcemode)、[CollisionFlags](#collisionflags)、[QueryTriggerInteraction](#querytriggerinteraction)、[RigidbodyConstraints](#rigidbodyconstraints)、[RigidbodyInterpolation](#rigidbodyinterpolation)、[CollisionDetectionMode](#collisiondetectionmode)、[PhysicsMaterialCombine](#physicsmaterialcombine)、[LightType](#lighttype)、[WrapMode](#wrapmode)、[TextAlignmentOptions](#textalignmentoptions)、[ParticleSystemSimulationSpace](#particlesystemsimulationspace)、[ParticleSystemShapeType](#particlesystemshapetype)、[ParticleSystemSortMode](#particlesystemsortmode)、[FullScreenMode](#fullscreenmode)、[CursorLockMode](#cursorlockmode)、[CursorMode](#cursormode)、[KeyCode](#keycode)
- **自分のクラス** — [スクリプトでプロパティを定義する](#スクリプトでプロパティを定義する)

## オブジェクトとシーン

### Object

エンジンオブジェクトに共通する識別と名前、および生成・破棄の入口。

| API | 用例 | 解説 |
| --- | --- | --- |
| `string name（読み書き）` | `obj.name = "Player"; Debug::Log(obj.name);` | オブジェクトの名前を読み書きする。Component の場合は所属する GameObject の名前。 |
| `int GetInstanceID() const` | `int id = obj.GetInstanceID();` | 実行中の個体を識別する ID を返す。シーンの保存や次回起動の識別には使わない。 |

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
| `Scene scene（読み取り）` | `string sceneName = go.scene.name;` | この GameObject が属するシーン。 |
| `bool TryGetComponent(?&out component) const` | `Rigidbody@ body; bool found = go.TryGetComponent(@body);` | 指定型のコンポーネントを探す。見つけた場合は true を返し、渡したハンドルに結果が入る。 |
| `T@ GetComponent<T>() const` | `Rigidbody@ body = go.GetComponent<Rigidbody>();` | 指定型のコンポーネントを取得する。見つからない場合は null。スクリプトの派生クラスも型として指定できる。 |
| `T@ AddComponent<T>()` | `AudioSource@ audio = go.AddComponent<AudioSource>();` | 追加可能な Component を付けて返す。ファイル名とクラス名が一致する MonoBehavior 派生型も指定できる。 |

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

### Behavior

独自の有効フラグを持つコンポーネント。スクリプトでの型名は Behavior。

**共通メンバー**

- [Component](#component) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `bool enabled（読み書き）` | `behavior.enabled = true; bool enabledNow = behavior.enabled;` | このコンポーネント自身の有効フラグを読み書きする。 |
| `bool isActiveAndEnabled（読み取り）` | `bool active = behavior.isActiveAndEnabled;` | 有効フラグと GameObject の階層のアクティブ状態が両方 true のとき true。 |

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
| `void SetParent(Transform@ parent, bool worldPositionStays = true)` | `t.SetParent(socket, false);` | 親を付け替える。第 2 引数が true ならワールドの位置・回転・拡大率を保ち、false なら今のローカルの値のまま新しい親の下に置く。null でルートに戻す。 |
| `int GetSiblingIndex() const` | `int order = t.GetSiblingIndex();` | 兄弟（同じ親の子どうし）の中での並び順。0 始まり。 |
| `void SetSiblingIndex(int index)` | `t.SetSiblingIndex(0);` | 兄弟の中での並び順を変える。UI では後ろのものほど手前に描かれる。 |
| `void SetAsFirstSibling()` | `t.SetAsFirstSibling();` | 兄弟の先頭に移す。 |
| `void SetAsLastSibling()` | `t.SetAsLastSibling();` | 兄弟の末尾に移す（UI なら一番手前に描かれる）。 |
| `Matrix4x4 localToWorldMatrix（読み取り）` | `Vector3 world = t.localToWorldMatrix.MultiplyPoint(Vector3(0, 1, 0));` | ローカル座標をワールド座標に変える行列。 |
| `Matrix4x4 worldToLocalMatrix（読み取り）` | `Vector3 local = t.worldToLocalMatrix.MultiplyPoint(target);` | ワールド座標をローカル座標に変える行列。 |

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
| `bool isLoaded（読み取り）` | `bool loaded = scene.isLoaded;` | シーンが読み込まれているか。 |
| `int rootCount（読み取り）` | `int count = scene.rootCount;` | ルート（親のない）GameObject の数。 |
| `GameObject@[]@ GetRootGameObjects() const` | `array<GameObject@>@ roots = scene.GetRootGameObjects();` | ルートの GameObject を並び順どおりに配列で返す。 |

### SceneManager

Build Settings のシーンを切り替えたり、現在のシーンを調べたりする。

| API | 用例 | 解説 |
| --- | --- | --- |
| `void LoadScene(const string&in sceneName)` | `SceneManager::LoadScene("Stage2");` | Build Settings で有効にしたシーンへ名前で切り替える。切り替えはフレームの終わりに行われる。 |
| `void LoadScene(int sceneBuildIndex)` | `SceneManager::LoadScene(0);` | Build Settings の有効なシーンへビルド番号で切り替える。切り替えはフレームの終わりに行われる。 |
| `Scene GetActiveScene()` | `Scene scene = SceneManager::GetActiveScene();` | 現在のアクティブシーンの情報を値として返す。 |
| `int sceneCountInBuildSettings（読み取り）` | `int count = SceneManager::sceneCountInBuildSettings;` | Build Settings で有効なシーンの件数を読む。 |
| `void MoveGameObjectToScene(GameObject@ go, const Scene&in scene)` | `SceneManager::MoveGameObjectToScene(go, SceneManager::GetActiveScene());` | ルートの GameObject を別のシーンへ移す。`DontDestroyOnLoad` したものを今のシーンに戻すときなどに使う。 |

### LayerMask

レイヤーの名前と番号、`Physics` の `layerMask` に渡すビットの組を作る。レイヤーは Project Settings の Tags and Layers で名前を付ける（0〜31）。

**名前空間 `LayerMask`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `int NameToLayer(const string&in layerName)` | `go.layer = LayerMask::NameToLayer("Enemy");` | レイヤーの名前から番号を返す。ない名前なら -1。 |
| `string LayerToName(int layer)` | `string layerName = LayerMask::LayerToName(go.layer);` | 番号からレイヤーの名前を返す。名前がなければ空の文字列。 |
| `int GetMask(const string&in layerName)` | `int mask = LayerMask::GetMask("Ground");` | レイヤーの名前から、そのレイヤーのビットだけを立てたマスクを作る。 |
| `int GetMask(const string&in layerName1, …)` | `int mask = LayerMask::GetMask("Ground", "Wall");` | 2〜4 個の名前を並べて、まとめたマスクを作る。 |
| `int GetMask(const string[]&in layerNames)` | `array<string> names = {"Ground", "Wall", "Water"}; int mask = LayerMask::GetMask(names);` | 配列の名前すべてのビットを立てたマスクを作る。 |

## 物理

### Physics

物理判定と重力を扱う。位置と方向はワールド座標で渡す。

問い合わせの関数は、どれも最後の 2 つの引数を省略できる。

- `int layerMask`：対象にするレイヤーのビットの組。`LayerMask::GetMask("Enemy")` で作れる
- `QueryTriggerInteraction queryTriggerInteraction`：トリガーの Collider に当てるか。既定の `UseGlobal` は `Physics::queriesHitTriggers` に従う

表ではこの 2 つを `…` と書いています。`maxDistance` の既定値は `Mathf::Infinity`、方向は長さ 1 でなくてもかまいません。

| API | 用例 | 解説 |
| --- | --- | --- |
| `Vector3 gravity（読み書き）` | `Physics::gravity = Vector3(0, -9.81f, 0); Vector3 g = Physics::gravity;` | シーンで使うワールド重力を読み書きする。 |
| `bool queriesHitTriggers（読み書き）` | `Physics::queriesHitTriggers = false;` | `UseGlobal` の問い合わせがトリガーに当たるか。既定は true。 |
| `void IgnoreLayerCollision(int layer1, int layer2, bool ignore = true)` | `Physics::IgnoreLayerCollision(LayerMask::NameToLayer("Player"), LayerMask::NameToLayer("Bullet"));` | 2 つのレイヤーどうしを衝突させないようにする。false で元に戻す。 |
| `bool GetIgnoreLayerCollision(int layer1, int layer2)` | `bool ignored = Physics::GetIgnoreLayerCollision(8, 9);` | 2 つのレイヤーの衝突を無視しているか。 |
| `bool Raycast(origin, direction, float maxDistance, …)` | `bool found = Physics::Raycast(t.position, t.forward, 20.0f);` | 起点から方向にレイを飛ばし、指定距離以内に何かあるかを返す。 |
| `bool Raycast(origin, direction, RaycastHit&out hitInfo, float maxDistance, …)` | `RaycastHit hit; if (Physics::Raycast(t.position, t.forward, hit, 20.0f)) { Debug::Log(hit.collider.name); }` | いちばん近い当たりを `hitInfo` に入れる。 |
| `RaycastHit[]@ RaycastAll(origin, direction, float maxDistance, …)` | `array<RaycastHit>@ hits = Physics::RaycastAll(t.position, t.forward, 20.0f);` | レイが当たったものをすべて、近い順に返す。 |
| `bool SphereCast(origin, float radius, direction, RaycastHit&out hitInfo, float maxDistance, …)` | `RaycastHit hit; bool found = Physics::SphereCast(t.position, 0.5f, t.forward, hit, 10.0f);` | 球を方向へ動かして、最初に当たるものを調べる。太いレイとして使える。 |
| `RaycastHit[]@ SphereCastAll(origin, float radius, direction, float maxDistance, …)` | `array<RaycastHit>@ hits = Physics::SphereCastAll(t.position, 0.5f, t.forward, 10.0f);` | 球を動かして当たるものをすべて返す。 |
| `bool BoxCast(center, halfExtents, direction, orientation = Quaternion::identity, float maxDistance, …)` | `bool found = Physics::BoxCast(t.position, Vector3(0.5f, 0.5f, 0.5f), t.forward);` | 箱（大きさは半分の長さで渡す）を方向へ動かして、何かに当たるかを返す。 |
| `bool BoxCast(center, halfExtents, direction, RaycastHit&out hitInfo, orientation = Quaternion::identity, float maxDistance, …)` | `RaycastHit hit; bool found = Physics::BoxCast(t.position, Vector3(0.5f, 0.5f, 0.5f), t.forward, hit, t.rotation, 5.0f);` | 最初に当たったものを `hitInfo` に入れる。 |
| `RaycastHit[]@ BoxCastAll(center, halfExtents, direction, orientation = Quaternion::identity, float maxDistance, …)` | `array<RaycastHit>@ hits = Physics::BoxCastAll(t.position, Vector3(1, 1, 1), t.forward);` | 箱を動かして当たるものをすべて返す。 |
| `bool CapsuleCast(point1, point2, float radius, direction, float maxDistance, …)` | `bool blocked = Physics::CapsuleCast(feet, head, 0.4f, t.forward, 1.0f);` | 2 点を結ぶカプセルを方向へ動かして、何かに当たるかを返す。キャラクターの前が空いているか調べるときなどに使う。 |
| `bool CapsuleCast(point1, point2, float radius, direction, RaycastHit&out hitInfo, float maxDistance, …)` | `RaycastHit hit; bool blocked = Physics::CapsuleCast(feet, head, 0.4f, t.forward, hit, 1.0f);` | 最初に当たったものを `hitInfo` に入れる。 |
| `RaycastHit[]@ CapsuleCastAll(point1, point2, float radius, direction, float maxDistance, …)` | `array<RaycastHit>@ hits = Physics::CapsuleCastAll(feet, head, 0.4f, t.forward, 3.0f);` | カプセルを動かして当たるものをすべて返す。 |
| `Collider@[]@ OverlapSphere(position, float radius, …)` | `array<Collider@>@ near = Physics::OverlapSphere(t.position, 3.0f);` | 球と重なっている Collider をすべて返す。 |
| `Collider@[]@ OverlapBox(center, halfExtents, orientation = Quaternion::identity, …)` | `array<Collider@>@ inside = Physics::OverlapBox(t.position, Vector3(1, 1, 1), t.rotation);` | 箱と重なっている Collider をすべて返す。 |
| `Collider@[]@ OverlapCapsule(point0, point1, float radius, …)` | `array<Collider@>@ inside = Physics::OverlapCapsule(feet, head, 0.5f);` | カプセルと重なっている Collider をすべて返す。 |
| `bool CheckSphere(position, float radius, …)` | `bool occupied = Physics::CheckSphere(spawnPoint, 0.5f);` | 球と重なる Collider が 1 つでもあるか。 |
| `bool CheckBox(center, halfExtents, orientation = Quaternion::identity, …)` | `bool occupied = Physics::CheckBox(spawnPoint, Vector3(0.5f, 0.5f, 0.5f));` | 箱と重なる Collider が 1 つでもあるか。 |
| `bool CheckCapsule(start, end, float radius, …)` | `bool grounded = Physics::CheckCapsule(feet, feet + Vector3::down * 0.1f, 0.3f);` | カプセルと重なる Collider が 1 つでもあるか。 |
| `const int DefaultRaycastLayers` | `int mask = Physics::DefaultRaycastLayers;` | レイの既定マスク。Ignore Raycast レイヤーを除く。 |
| `const int AllLayers` | `int mask = Physics::AllLayers;` | すべてのレイヤーを対象とするマスク。Overlap 系の既定値。 |

### Collider

衝突判定の形状に共通する情報。

**共通メンバー**

- [Behavior](#behavior) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `bool isTrigger（読み書き）` | `collider.isTrigger = true; bool trigger = collider.isTrigger;` | トリガーとして扱うかを読み書きする。有効なペアの重なりは OnTrigger 系で受けられる。 |
| `Rigidbody@ attachedRigidbody（読み取り）` | `Rigidbody@ body = collider.attachedRigidbody;` | この Collider が属する物理ボディを返す。存在しなければ null。 |
| `Vector3 ClosestPoint(const Vector3&in position) const` | `Vector3 p = collider.ClosestPoint(Vector3(3, 0, 0));` | 指定ワールド位置に最も近いコライダー上の点を求める。 |
| `Vector3 ClosestPointOnBounds(const Vector3&in position) const` | `Vector3 p = collider.ClosestPointOnBounds(target);` | 指定位置にいちばん近い、`bounds`（ワールドの軸にそろえた箱）の上の点。`ClosestPoint` より大まか。 |
| `Bounds bounds（読み取り）` | `Vector3 size = collider.bounds.size;` | Collider を囲む、ワールドの軸にそろえた箱。 |
| `PhysicsMaterial@ sharedMaterial（読み書き）` | `@collider.sharedMaterial = icy;` | 摩擦と跳ね返りを決める物理マテリアル（アセットそのもの）。書き換えると、同じものを使うすべての Collider に効く。 |
| `PhysicsMaterial@ material（読み書き）` | `collider.material.bounciness = 0.8f;` | この Collider だけの物理マテリアル。読んだときに複製されるので、ほかの Collider には影響しない。 |

### BoxCollider

箱形のコライダー。

**共通メンバー**

- [Collider](#collider) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`isTrigger`、`attachedRigidbody`、`ClosestPoint`、`ClosestPointOnBounds`、`bounds`、`sharedMaterial`、`material`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Vector3 center（読み書き）` | `box.center = Vector3(0, 1, 0); Vector3 p = box.center;` | GameObject のローカル座標で箱の中心を読み書きする。 |
| `Vector3 size（読み書き）` | `box.size = Vector3(2, 1, 3); Vector3 size = box.size;` | 箱のローカル寸法を軸ごとに読み書きする。 |

### SphereCollider

球形のコライダー。

**共通メンバー**

- [Collider](#collider) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`isTrigger`、`attachedRigidbody`、`ClosestPoint`、`ClosestPointOnBounds`、`bounds`、`sharedMaterial`、`material`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Vector3 center（読み書き）` | `sphere.center = Vector3(0, 1, 0); Vector3 p = sphere.center;` | GameObject のローカル座標で球の中心を読み書きする。 |
| `float radius（読み書き）` | `sphere.radius = 0.5f; float radius = sphere.radius;` | 球のローカル半径を読み書きする。 |

### CapsuleCollider

カプセル形のコライダー。

**共通メンバー**

- [Collider](#collider) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`isTrigger`、`attachedRigidbody`、`ClosestPoint`、`ClosestPointOnBounds`、`bounds`、`sharedMaterial`、`material`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Vector3 center（読み書き）` | `capsule.center = Vector3(0, 1, 0); Vector3 p = capsule.center;` | GameObject のローカル座標でカプセルの中心を読み書きする。 |
| `float radius（読み書き）` | `capsule.radius = 0.5f; float radius = capsule.radius;` | カプセルのローカル半径を読み書きする。 |
| `float height（読み書き）` | `capsule.height = 2.0f; float height = capsule.height;` | 丸い端を含むカプセルの高さを読み書きする。 |
| `int direction（読み書き）` | `capsule.direction = 1; int axis = capsule.direction;` | カプセルの長軸を読み書きする。0 は X、1 は Y、2 は Z。 |

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
| `void AddExplosionForce(float explosionForce, const Vector3&in explosionPosition, float explosionRadius, float upwardsModifier = 0.0f, ForceMode mode = ForceMode::Force)` | `body.AddExplosionForce(500.0f, bombPosition, 5.0f, 1.0f, ForceMode::Impulse);` | 爆発の中心から外向きに力を加える。半径より遠ければ何もしない。`upwardsModifier` で上向きに持ち上げる。 |
| `Vector3 GetPointVelocity(const Vector3&in worldPoint) const` | `Vector3 v = body.GetPointVelocity(tip.position);` | ワールドの点の速度（回転による分も含む）。 |
| `RigidbodyConstraints constraints（読み書き）` | `body.constraints = RigidbodyConstraints::FreezeRotation;` | 軸ごとに位置・回転を止める。`FreezePositionX \| FreezeRotation` のように組み合わせる。 |
| `RigidbodyInterpolation interpolation（読み書き）` | `body.interpolation = RigidbodyInterpolation::Interpolate;` | 物理の 1 歩のあいだの見た目の位置を補間するか。カメラが追う物体のカクつきを減らす。 |
| `CollisionDetectionMode collisionDetectionMode（読み書き）` | `body.collisionDetectionMode = CollisionDetectionMode::Continuous;` | 速い物体が薄い壁をすり抜けないように、連続的に当たりを調べるか。 |
| `float maxLinearVelocity（読み書き）` | `body.maxLinearVelocity = 50.0f;` | 並進速度の上限。 |
| `float maxAngularVelocity（読み書き）` | `body.maxAngularVelocity = 7.0f;` | 角速度の上限（ラジアン毎秒）。 |
| `float sleepThreshold（読み書き）` | `body.sleepThreshold = 0.005f;` | これより遅くなったら眠る（計算を止める）目安。 |
| `Vector3 centerOfMass（読み書き）` | `body.centerOfMass = Vector3(0, -0.5f, 0);` | 重心をローカル座標で読み書きする。下げると倒れにくくなる。 |
| `void ResetCenterOfMass()` | `body.ResetCenterOfMass();` | 重心を Collider の形から求め直す。 |

### CharacterController

カプセル形状を衝突に沿わせながらスクリプトから移動させる。

**共通メンバー**

- [Collider](#collider) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`isTrigger`、`attachedRigidbody`、`ClosestPoint`、`ClosestPointOnBounds`、`bounds`、`sharedMaterial`、`material`、`TryGetComponent`、`GetComponent`

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
| `float skinWidth（読み書き）` | `controller.skinWidth = 0.08f;` | 周りの Collider との間にあける隙間。小さすぎると引っかかりやすい。 |
| `float minMoveDistance（読み書き）` | `controller.minMoveDistance = 0.001f;` | これより短い `Move` では動かない。 |
| `CollisionFlags collisionFlags（読み取り）` | `bool hitHead = (controller.collisionFlags & CollisionFlags::Above) != 0;` | 直近の `Move` でどの向きに触れたか（`Move` の戻り値と同じ）。 |

### PhysicsMaterial

Collider の摩擦と跳ね返りを決めるアセット（`.physicsmaterial`）。Project の Create > Physics Material で作り、Collider の Material の欄に入れる。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

| API | 用例 | 解説 |
| --- | --- | --- |
| `float dynamicFriction（読み書き）` | `ice.dynamicFriction = 0.05f;` | 滑っているときの摩擦（0 以上。0 でつるつる）。 |
| `float staticFriction（読み書き）` | `ice.staticFriction = 0.05f;` | 止まっているものが動き出すまでの摩擦。 |
| `float bounciness（読み書き）` | `ball.bounciness = 0.9f;` | 跳ね返りの強さ（0〜1。1 で勢いを失わずに跳ねる）。 |
| `PhysicsMaterialCombine frictionCombine（読み書き）` | `ice.frictionCombine = PhysicsMaterialCombine::Minimum;` | ぶつかった 2 つの摩擦をどう組み合わせるか。 |
| `PhysicsMaterialCombine bounceCombine（読み書き）` | `ball.bounceCombine = PhysicsMaterialCombine::Maximum;` | ぶつかった 2 つの跳ね返りをどう組み合わせるか。 |

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

### ControllerColliderHit

`OnControllerColliderHit(ControllerColliderHit hit)` で受け取る、CharacterController の `Move` が触れたものの情報。

| API | 用例 | 解説 |
| --- | --- | --- |
| `CharacterController@ controller（読み取り）` | `CharacterController@ self = hit.controller;` | 動いた CharacterController。 |
| `Collider@ collider（読み取り）` | `Debug::Log(hit.collider.name);` | 触れた相手の Collider。 |
| `Rigidbody@ rigidbody（読み取り）` | `if (hit.rigidbody !is null) { hit.rigidbody.AddForce(hit.moveDirection * 2.0f, ForceMode::Impulse); }` | 相手の Rigidbody。なければ null。押した箱を動かすときに使う。 |
| `Vector3 point` | `Vector3 p = hit.point;` | 触れた点（ワールド座標）。 |
| `Vector3 normal` | `bool isFloor = hit.normal.y > 0.7f;` | 触れた面の法線。 |
| `Vector3 moveDirection` | `Vector3 push = hit.moveDirection;` | `Move` で動こうとした向き。 |
| `float moveLength` | `float length = hit.moveLength;` | その `Move` で動いた長さ。 |

## カメラ・ライト・描画

### Camera

ゲームの画面を写すカメラ。`MainCamera` のタグを付けたものが Game ビューと書き出したゲームに映る。ポストプロセスのかけ方（Post Processing・Volume Mask）は Inspector で設定する（[エディターガイド](README.md#ポストプロセス)）。

**共通メンバー**

- [Behavior](#behavior) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `float fieldOfView（読み書き）` | `Camera::main.fieldOfView = 40.0f;` | 縦の視野角（度）。小さくするとズームしたように見える。 |
| `float aspect（読み書き）` | `float ratio = camera.aspect;` | 横 ÷ 縦の比。普段は画面の大きさから自動で決まる。書き込むと、その値に固定する。 |
| `void ResetAspect()` | `camera.ResetAspect();` | `aspect` を画面の大きさから決める状態に戻す。 |
| `float nearClipPlane（読み書き）` | `camera.nearClipPlane = 0.1f;` | これより近いものは描かない距離。 |
| `float farClipPlane（読み書き）` | `camera.farClipPlane = 500.0f;` | これより遠いものは描かない距離。 |

**名前空間 `Camera`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Camera@ main（読み取り）` | `Transform@ cameraTransform = Camera::main.transform;` | `MainCamera` のタグを付けた有効なカメラ。なければ、有効な最初のカメラ。どちらもなければ null。 |

### Light

シーンを照らすライト。GameObject メニューの Light から作る。

**共通メンバー**

- [Behavior](#behavior) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `LightType type（読み書き）` | `light.type = LightType::Point;` | ライトの種類（Directional・Point・Spot）。 |
| `Color color（読み書き）` | `light.color = Color(1.0f, 0.9f, 0.7f);` | 光の色。 |
| `float intensity（読み書き）` | `light.intensity = 2.0f;` | 光の強さ。 |
| `float range（読み書き）` | `light.range = 10.0f;` | Point と Spot の光が届く距離。 |
| `float spotAngle（読み書き）` | `light.spotAngle = 45.0f;` | Spot の光の広がり（外側の角度、度）。 |
| `float innerSpotAngle（読み書き）` | `light.innerSpotAngle = 30.0f;` | Spot の光が弱まり始める内側の角度（度）。 |

一度に効くライトは 16 個までです。Directional の向きは Transform の前方向（`forward`）です。

### Renderer

描画するコンポーネントに共通するマテリアル操作。

**共通メンバー**

- [Behavior](#behavior) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Material@ material（読み書き）` | `Material@ mat = renderer.material; @renderer.material = replacement;` | 描画用の個別マテリアルを取得する。取得時に個別化されるので、色などをその Renderer だけ変えられる。代入で付け替えもできる。 |
| `Material@ sharedMaterial（読み書き）` | `Material@ mat = renderer.sharedMaterial; @renderer.sharedMaterial = replacement;` | 共有マテリアルを読む、または付け替える。共有マテリアル自体を編集すると、そのマテリアルを使う他の描画にも影響しうる。 |
| `Material@[]@ materials（読み書き）` | `array<Material@>@ mats = renderer.materials; mats[1].color = Color::red;` | サブメッシュごとの個別マテリアルを配列で読み書きする。読んだときに個別化される。 |
| `Material@[]@ sharedMaterials（読み書き）` | `array<Material@>@ mats = renderer.sharedMaterials;` | サブメッシュごとの共有マテリアルを配列で読み書きする。 |

### MeshRenderer

メッシュを描画するコンポーネント。スクリプトでは Renderer と共通のマテリアル操作を使う。

**共通メンバー**

- [Renderer](#renderer) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`material`、`sharedMaterial`、`materials`、`sharedMaterials`、`TryGetComponent`、`GetComponent`

### SkinnedMeshRenderer

ボーンで変形するメッシュ（キャラクターなど）を描く。glTF のモデルを置くと付く。

**共通メンバー**

- [Renderer](#renderer) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`material`、`sharedMaterial`、`materials`、`sharedMaterials`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Mesh@ sharedMesh（読み書き）` | `int vertices = skinned.sharedMesh.vertexCount;` | 描くメッシュ。 |

### LineRenderer

点を結んだ線を描く。軌跡やレーザーなどに使う。

**共通メンバー**

- [Renderer](#renderer) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`material`、`sharedMaterial`、`materials`、`sharedMaterials`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `int positionCount（読み書き）` | `line.positionCount = 2;` | 点の数。増やした点は (0, 0, 0) から始まる。 |
| `Vector3 GetPosition(int index) const` | `Vector3 p = line.GetPosition(0);` | 指定した点の位置。 |
| `void SetPosition(int index, const Vector3&in position)` | `line.SetPosition(1, target.position);` | 指定した点の位置を設定する。 |
| `void SetPositions(Vector3[]@ positions)` | `array<Vector3> points = {Vector3::zero, Vector3::up}; line.positionCount = 2; line.SetPositions(points);` | 配列の先頭から `positionCount` 個まで、まとめて設定する。 |
| `int GetPositions(Vector3[]@ positions) const` | `array<Vector3> points(line.positionCount); line.GetPositions(points);` | 点を配列に写し、写した数を返す。 |
| `float startWidth（読み書き）` | `line.startWidth = 0.1f;` | 始点での線の太さ。 |
| `float endWidth（読み書き）` | `line.endWidth = 0.0f;` | 終点での線の太さ。 |
| `float widthMultiplier（読み書き）` | `line.widthMultiplier = 2.0f;` | 太さ全体に掛ける倍率。 |
| `Color startColor（読み書き）` | `line.startColor = Color::red;` | 始点での色。 |
| `Color endColor（読み書き）` | `line.endColor = Color(1, 0, 0, 0);` | 終点での色。 |
| `bool useWorldSpace（読み書き）` | `line.useWorldSpace = true;` | true なら点をワールド座標、false なら GameObject のローカル座標として扱う。 |
| `bool loop（読み書き）` | `line.loop = true;` | 最後の点と最初の点もつなぐか。 |

### Material

色・テクスチャ・UV と、シェーダの `"properties"` に書いた値を扱う。`GetFloat` などの名前は、`.shader` の `"properties"` の `"name"` と同じにする（[シェーダとポストエフェクトの書き方](Shaders.md#プロパティ)）。名前のプロパティを使うのは今のところ UI と Full Screen Pass のシェーダだけで、3D のメッシュのシェーダでは `color`・`mainTexture` などを使う。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Color color（読み書き）` | `mat.color = Color::red; Color tint = mat.color;` | マテリアルの基本色を読み書きする。 |
| `Material(Shader@ shader)` | `Material@ mat = Material(otherMaterial.shader);` | シェーダを指定して新しいマテリアルを作る。 |
| `Material(Material@ source)` | `Material@ copy = Material(renderer.sharedMaterial);` | ほかのマテリアルの設定を写した新しいマテリアルを作る。 |
| `Shader@ shader（読み書き）` | `@mat.shader = otherShader;` | 使うシェーダ。替えるとシェーダのプロパティはそのシェーダの初期値に戻る。 |
| `int renderQueue（読み書き）` | `mat.renderQueue = 3000;` | 描画順。小さいものから描く。-1 ならシェーダの値を使う（Geometry 2000、Transparent 3000 など）。 |
| `Vector2 mainTextureOffset（読み書き）` | `mat.mainTextureOffset = Vector2(0.25f, 0.0f); Vector2 offset = mat.mainTextureOffset;` | メインテクスチャを参照するときの UV のずれを読み書きする。 |
| `Vector2 mainTextureScale（読み書き）` | `mat.mainTextureScale = Vector2(2, 2); Vector2 tiling = mat.mainTextureScale;` | メインテクスチャの UV の倍率を読み書きする。 |
| `Color GetColor(const string&in name) const` | `Color tint = mat.GetColor("_Tint");` | シェーダの `Color` のプロパティを読む。 |
| `void SetColor(const string&in name, const Color&in value)` | `mat.SetColor("_Tint", Color::red);` | シェーダの `Color` のプロパティに値を設定する。 |
| `float GetFloat(const string&in name) const` | `float value = mat.GetFloat("_Intensity");` | シェーダの `Float` のプロパティを読む。 |
| `void SetFloat(const string&in name, float value)` | `mat.SetFloat("_Intensity", 0.5f);` | シェーダの `Float` のプロパティに値を設定する。 |
| `Vector4 GetVector(const string&in name) const` | `Vector4 v = mat.GetVector("_Offset");` | シェーダの `Vector` のプロパティを読む。 |
| `void SetVector(const string&in name, const Vector4&in value)` | `mat.SetVector("_Offset", Vector4(1, 0, 0, 0));` | シェーダの `Vector` のプロパティに値を設定する。 |
| `bool HasProperty(const string&in name) const` | `bool hasValue = mat.HasProperty("_Intensity");` | 指定した名前のプロパティがあるか調べる。 |

### Shader

マテリアルが使うシェーダ（`.shader`）。書き方は [シェーダとポストエフェクトの書き方](Shaders.md)。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

| API | 用例 | 解説 |
| --- | --- | --- |
| `int renderQueue（読み取り）` | `int queue = shader.renderQueue;` | `.shader` の `"queue"` の描画順。 |
| `int passCount（読み取り）` | `int passes = shader.passCount;` | Pass の数。 |
| `int FindPropertyIndex(const string&in propertyName) const` | `bool has = shader.FindPropertyIndex("_Intensity") >= 0;` | `"properties"` の中での番号。なければ -1。 |

### Mesh

形のデータ。モデルのファイル（`.obj`・`.gltf`・`.glb`）から読み込まれる。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Bounds bounds（読み取り）` | `Vector3 size = mesh.bounds.size;` | メッシュを囲む箱（メッシュのローカル座標）。 |
| `int vertexCount（読み取り）` | `int count = mesh.vertexCount;` | 頂点の数。 |
| `int subMeshCount（読み取り）` | `int count = mesh.subMeshCount;` | サブメッシュ（マテリアルを分ける単位）の数。 |

### Texture2D

Project から参照する画像（.png / .jpg）。スクリプトの欄に `Texture2D@` を書くと、Inspector で画像を入れられる。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

| API | 用例 | 解説 |
| --- | --- | --- |
| `int width（読み取り）` | `int w = texture.width;` | 画像の幅をピクセルで返す。 |
| `int height（読み取り）` | `int h = texture.height;` | 画像の高さをピクセルで返す。 |
| `bool isReadable（読み取り）` | `bool readable = texture.isReadable;` | CPU 側に画素があるか。画像の Inspector で種類を Cursor にしたものだけ true。 |

### Sprite

テクスチャの中の四角い範囲。UI の Image で描く。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Texture2D@ texture（読み取り）` | `Texture2D@ source = sprite.texture;` | 元のテクスチャ。 |
| `Rect rect（読み取り）` | `Rect area = sprite.rect;` | テクスチャの中の範囲（ピクセル）。 |
| `Vector2 pivot（読み取り）` | `Vector2 p = sprite.pivot;` | 中心にする点（範囲の中の 0〜1）。 |

**名前空間 `Sprite`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Sprite@ Create(Texture2D@ texture, const Rect&in rect, const Vector2&in pivot = Vector2(0.5f, 0.5f))` | `Sprite@ s = Sprite::Create(texture, Rect(0, 0, 64, 64));` | テクスチャの範囲（ピクセル）から Sprite を作る。 |

### VolumeProfile

ポストプロセスの効果と値をまとめたアセット（`.volumeprofile`）。中の値はスクリプトからは変えられない。Volume の Inspector と Project Settings の Graphics で使う。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

## アニメーション・パーティクル

### SkeletalAnimator

ボーンのアニメーション（glTF のアニメーション）を再生する。

**共通メンバー**

- [Behavior](#behavior) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `void Play(SkeletalAnimationClip@ clip)` | `animator.Play(runClip);` | クリップを先頭から再生する。 |
| `void CrossFade(SkeletalAnimationClip@ clip, float fadeLength = 0.3f)` | `animator.CrossFade(idleClip, 0.2f);` | 今のアニメーションから、指定した秒数で混ぜながら切り替える。 |
| `void Stop()` | `animator.Stop();` | 再生を止める。 |
| `bool isPlaying（読み取り）` | `if (!animator.isPlaying) { animator.Play(idleClip); }` | 再生中か。`Once` のクリップが終わると false になる。 |
| `SkeletalAnimationClip@ clip（読み書き）` | `@animator.clip = idleClip;` | 再生するクリップ。 |
| `float time（読み書き）` | `animator.time = 0.0f;` | 再生位置（秒）。 |
| `float speed（読み書き）` | `animator.speed = 1.5f;` | 再生の速さの倍率。 |
| `WrapMode wrapMode（読み書き）` | `animator.wrapMode = WrapMode::Loop;` | 最後まで再生したときの動き。`Default` ならクリップの設定に従う。 |
| `bool playAutomatically（読み書き）` | `animator.playAutomatically = false;` | 始まったときに `clip` を自動で再生するか。 |

### SkeletalAnimationClip

ボーンのアニメーションのデータ。glTF のモデルに含まれるもので、Project でモデルを開くと中に見える。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

| API | 用例 | 解説 |
| --- | --- | --- |
| `float length（読み取り）` | `float seconds = clip.length;` | 長さ（秒）。 |
| `WrapMode wrapMode（読み書き）` | `clip.wrapMode = WrapMode::Loop;` | 最後まで再生したときの動きの既定値。 |

### ParticleSystem

パーティクル（小さな粒）を出す。値はモジュールごとにまとまっている。

**共通メンバー**

- [Component](#component) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `void Play()` | `particles.Play();` | 出し始める。 |
| `void Stop()` | `particles.Stop();` | 新しく出すのをやめる。出ている粒は寿命まで残る。 |
| `void Clear()` | `particles.Clear();` | 出ている粒をすべて消す。 |
| `void Emit(int count)` | `particles.Emit(30);` | 今すぐ粒をまとめて出す（爆発など）。 |
| `void Simulate(float t)` | `particles.Simulate(1.0f);` | 指定した秒数だけ進めた状態にする。 |
| `bool isPlaying（読み取り）` | `bool playing = particles.isPlaying;` | 再生中か。 |
| `float time（読み取り）` | `float t = particles.time;` | 再生を始めてからの時間（秒）。 |
| `ParticleSystem::MainModule main（読み取り）` | `ParticleSystem::MainModule main = particles.main; main.startSpeed = ParticleSystem::MinMaxCurve(5.0f);` | 寿命・速さ・大きさ・色などの基本の値。 |
| `ParticleSystem::EmissionModule emission（読み取り）` | `ParticleSystem::EmissionModule emission = particles.emission; emission.rateOverTime = 50.0f;` | 1 秒に出す数。 |
| `ParticleSystem::ShapeModule shape（読み取り）` | `ParticleSystem::ShapeModule shape = particles.shape; shape.radius = 2.0f;` | 出す場所の形。 |
| `ParticleSystem::ColorOverLifetimeModule colorOverLifetime（読み取り）` | `ParticleSystem::ColorOverLifetimeModule col = particles.colorOverLifetime; col.enabled = true;` | 寿命に合わせて色を変えるか（変え方は Inspector で設定する）。 |
| `ParticleSystem::SizeOverLifetimeModule sizeOverLifetime（読み取り）` | `ParticleSystem::SizeOverLifetimeModule size = particles.sizeOverLifetime; size.enabled = false;` | 寿命に合わせて大きさを変えるか（変え方は Inspector で設定する）。 |

モジュールは、ParticleSystem の値を指す小さな値型です。変数に取り出してから値を書き込むと、ParticleSystem に反映されます。

**ParticleSystem::MainModule**

| API | 用例 | 解説 |
| --- | --- | --- |
| `float duration（読み書き）` | `main.duration = 2.0f;` | 1 回の再生の長さ（秒）。 |
| `bool loop（読み書き）` | `main.loop = false;` | `duration` が終わったら繰り返すか。 |
| `bool playOnAwake（読み書き）` | `main.playOnAwake = false;` | 始まったときに自動で再生するか。 |
| `ParticleSystem::MinMaxCurve startLifetime（読み書き）` | `main.startLifetime = ParticleSystem::MinMaxCurve(0.5f, 1.0f);` | 粒の寿命（秒）。 |
| `ParticleSystem::MinMaxCurve startSpeed（読み書き）` | `main.startSpeed = ParticleSystem::MinMaxCurve(5.0f);` | 出たときの速さ。 |
| `ParticleSystem::MinMaxCurve startSize（読み書き）` | `main.startSize = ParticleSystem::MinMaxCurve(0.1f, 0.3f);` | 出たときの大きさ。 |
| `ParticleSystem::MinMaxGradient startColor（読み書き）` | `main.startColor = ParticleSystem::MinMaxGradient(Color::yellow);` | 出たときの色。 |
| `float gravityModifier（読み書き）` | `main.gravityModifier = 1.0f;` | 重力（`Physics::gravity`）をどれだけ効かせるか。 |
| `int maxParticles（読み書き）` | `main.maxParticles = 500;` | 同時に出せる粒の上限。 |
| `ParticleSystemSimulationSpace simulationSpace（読み書き）` | `main.simulationSpace = ParticleSystemSimulationSpace::World;` | 粒がワールドに残るか（World）、GameObject と一緒に動くか（Local）。 |

**ParticleSystem::EmissionModule**

| API | 用例 | 解説 |
| --- | --- | --- |
| `bool enabled（読み書き）` | `emission.enabled = false;` | 出すか。 |
| `float rateOverTime（読み書き）` | `emission.rateOverTime = 20.0f;` | 1 秒に出す数。 |

**ParticleSystem::ShapeModule**

| API | 用例 | 解説 |
| --- | --- | --- |
| `bool enabled（読み書き）` | `shape.enabled = true;` | 形を使うか。false なら 1 点から出る。 |
| `ParticleSystemShapeType shapeType（読み書き）` | `shape.shapeType = ParticleSystemShapeType::Cone;` | 形（Sphere・Cone）。 |
| `float angle（読み書き）` | `shape.angle = 25.0f;` | Cone の広がり（度）。 |
| `float radius（読み書き）` | `shape.radius = 1.0f;` | 形の半径。 |
| `float radiusThickness（読み書き）` | `shape.radiusThickness = 0.0f;` | 出す場所の厚み（0 なら表面だけ、1 なら中全体）。 |

**ParticleSystem::ColorOverLifetimeModule / SizeOverLifetimeModule**

| API | 用例 | 解説 |
| --- | --- | --- |
| `bool enabled（読み書き）` | `col.enabled = true;` | 使うか。 |

**ParticleSystem::MinMaxCurve**

1 つの数か、2 つの数のあいだのランダムな値を表す値型。

| API | 用例 | 解説 |
| --- | --- | --- |
| `MinMaxCurve(float constant)` | `ParticleSystem::MinMaxCurve(3.0f)` | いつも同じ値。 |
| `MinMaxCurve(float min, float max)` | `ParticleSystem::MinMaxCurve(1.0f, 3.0f)` | 粒ごとに `min`〜`max` のランダムな値。 |
| `float constant（読み書き）` | `float value = curve.constant;` | 1 つの数のときの値。 |
| `float constantMin` / `float constantMax` | `curve.constantMax = 5.0f;` | ランダムの範囲。 |

**ParticleSystem::MinMaxGradient**

1 つの色か、2 つの色のあいだのランダムな色を表す値型。

| API | 用例 | 解説 |
| --- | --- | --- |
| `MinMaxGradient(const Color&in color)` | `ParticleSystem::MinMaxGradient(Color::white)` | いつも同じ色。 |
| `MinMaxGradient(const Color&in min, const Color&in max)` | `ParticleSystem::MinMaxGradient(Color::red, Color::yellow)` | 粒ごとに 2 つの色のあいだのランダムな色。 |
| `Color color（読み書き）` | `Color c = gradient.color;` | 1 つの色のときの色。 |
| `Color colorMin` / `Color colorMax` | `gradient.colorMax = Color::white;` | ランダムの範囲。 |

### ParticleSystemRenderer

ParticleSystem の粒を描く。ParticleSystem を付けると一緒に付く。

**共通メンバー**

- [Renderer](#renderer) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`material`、`sharedMaterial`、`materials`、`sharedMaterials`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `ParticleSystemSortMode sortMode（読み書き）` | `renderer.sortMode = ParticleSystemSortMode::Distance;` | 粒を描く順番。半透明の粒の重なりを正しく見せたいときに変える。 |

## 音声

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

### AudioClip

Project から参照する音声データ。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

| API | 用例 | 解説 |
| --- | --- | --- |
| `float length（読み取り）` | `float seconds = clip.length;` | クリップの長さを秒で返す。 |
| `int frequency（読み取り）` | `int hz = clip.frequency;` | サンプリング周波数を Hz で返す。 |
| `int channels（読み取り）` | `int count = clip.channels;` | モノラルなら 1、ステレオなら 2 など、チャンネル数を返す。 |

### AudioListener

音を聞く位置。ふつうはメインカメラに付ける。

**共通メンバー**

- [Component](#component) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`TryGetComponent`、`GetComponent`

**名前空間 `AudioListener`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `float volume（読み書き）` | `AudioListener::volume = 0.5f;` | ゲーム全体の音量（0〜1）。 |

## UI

### Canvas

UI のいちばん上に置く GameObject に付ける。子の RectTransform を画面の大きさに合わせて並べ、Graphic を画面に重ねて描く。GameObject メニューの UI から作る。

**共通メンバー**

- [Behavior](#behavior) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`TryGetComponent`、`GetComponent`

### RectTransform

UI の GameObject の位置と大きさ。親の四角に対する「アンカー」と「ピボット」で置き場所を決める。`transform` は `Transform@` なので、`cast<RectTransform>(transform)` で取り出す。

**共通メンバー**

- [Transform](#transform) のすべてのメンバー

| API | 用例 | 解説 |
| --- | --- | --- |
| `Vector2 anchorMin（読み書き）` | `rt.anchorMin = Vector2(0, 0);` | 親の四角の中で、左下の角を合わせる点（0〜1）。 |
| `Vector2 anchorMax（読み書き）` | `rt.anchorMax = Vector2(1, 1);` | 親の四角の中で、右上の角を合わせる点（0〜1）。`anchorMin` と同じなら大きさは固定、違えば親に合わせて伸びる。 |
| `Vector2 anchoredPosition（読み書き）` | `rt.anchoredPosition = Vector2(0, -40);` | アンカーから見たピボットの位置（ピクセル）。 |
| `Vector2 sizeDelta（読み書き）` | `rt.sizeDelta = Vector2(200, 50);` | アンカーが 1 点のときは大きさ。伸びるときは、親に合わせた大きさとの差。 |
| `Vector2 pivot（読み書き）` | `rt.pivot = Vector2(0.5f, 0.5f);` | 自分の四角の中で、位置と回転の中心にする点（0〜1）。 |
| `Rect rect（読み取り）` | `float width = rt.rect.width;` | 計算された自分の四角（ローカル座標）。 |

```angelscript
RectTransform@ rt = cast<RectTransform>(transform);
if (rt !is null) {
    rt.anchoredPosition = Vector2(0, 100);
}
```

### Graphic

UI の描画とポインター判定に関わる共通コンポーネント。Canvas の子（RectTransform を持つ GameObject）に付ける。

**共通メンバー**

- [Behavior](#behavior) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Color color（読み書き）` | `graphic.color = Color::yellow; Color tint = graphic.color;` | UI の描画色を読み書きする。 |
| `bool raycastTarget（読み書き）` | `graphic.raycastTarget = false; bool receivesPointer = graphic.raycastTarget;` | UI のポインター判定にこの Graphic を含めるかを読み書きする。 |
| `Material@ material（読み書き）` | `@graphic.material = glowMaterial;` | 描くときのマテリアル。null なら既定のもの（Image は Sprite、Text は Text のシェーダ）。 |
| `CanvasRenderer@ canvasRenderer（読み取り）` | `graphic.canvasRenderer.SetAlpha(0.5f);` | 実際に描く [CanvasRenderer](#canvasrenderer)。 |
| `void CrossFadeColor(const Color&in targetColor, float duration, bool ignoreTimeScale, bool useAlpha)` | `graphic.CrossFadeColor(Color::white, 0.25f, true, true);` | 第 2 引数の秒数で色を補間する。第 3 引数は timeScale を無視するか、第 4 引数はアルファも変えるか。 |
| `void SetAllDirty()` | `graphic.SetAllDirty();` | 形とマテリアルを次の描画で作り直させる。プロパティを変えたときは自動で呼ばれるので、普段は使わない。 |
| `void SetVerticesDirty()` | `graphic.SetVerticesDirty();` | 形（頂点）だけを作り直させる。 |
| `void SetMaterialDirty()` | `graphic.SetMaterialDirty();` | マテリアルだけを設定し直させる。 |

### Text

文字列を描く UI コンポーネント（TextMeshPro に近いもの）。

**共通メンバー**

- [Graphic](#graphic) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`color`、`raycastTarget`、`material`、`canvasRenderer`、`CrossFadeColor`、`SetAllDirty`、`SetVerticesDirty`、`SetMaterialDirty`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `string text（読み書き）` | `label.text = "Score: " + score;` | 画面に表示する文字列を読み書きする。 |
| `FontAsset@ font（読み書き）` | `@label.font = titleFont;` | 使うフォント。Project の `.ttf`・`.otf` を Inspector の欄に入れるか、`FontAsset@` のフィールドから代入する。 |
| `float fontSize（読み書き）` | `label.fontSize = 24.0f; float size = label.fontSize;` | フォントの大きさを読み書きする。 |
| `TextAlignmentOptions alignment（読み書き）` | `label.alignment = TextAlignmentOptions::Center;` | 枠の中での文字の寄せ方。 |
| `bool enableWordWrapping（読み書き）` | `label.enableWordWrapping = false;` | 枠の幅で折り返すか。 |
| `float lineSpacing（読み書き）` | `label.lineSpacing = 1.2f; float spacing = label.lineSpacing;` | 複数行を描くときの行間を読み書きする。 |
| `float characterSpacing（読み書き）` | `label.characterSpacing = 2.0f;` | 文字と文字の間を広げる量。 |
| `Color outlineColor（読み書き）` | `label.outlineColor = Color::black;` | 縁取りの色。 |
| `float outlineWidth（読み書き）` | `label.outlineWidth = 0.2f;` | 縁取りの太さ。0 で縁取りなし。 |

### Image

画像を描く UI コンポーネント。

**共通メンバー**

- [Graphic](#graphic) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`color`、`raycastTarget`、`material`、`canvasRenderer`、`CrossFadeColor`、`SetAllDirty`、`SetVerticesDirty`、`SetMaterialDirty`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Sprite@ sprite（読み書き）` | `@icon.sprite = Sprite::Create(texture, Rect(0, 0, texture.width, texture.height));` | 描く画像。null なら白い四角を `color` で描く。 |
| `void SetNativeSize()` | `icon.SetNativeSize();` | RectTransform の大きさを、画像のピクセルの大きさに合わせる。 |

### CanvasRenderer

Graphic（Text・Image）を実際に描く部分。Graphic を付けると一緒に付く。

**共通メンバー**

- [Component](#component) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `Color GetColor() const` / `void SetColor(const Color&in color)` | `cr.SetColor(Color(1, 1, 1, 0.5f));` | Graphic の色に掛ける色。 |
| `float GetAlpha() const` / `void SetAlpha(float alpha)` | `cr.SetAlpha(0.0f);` | 透明度だけを掛ける。フェードに使う。 |
| `void Clear()` | `cr.Clear();` | 描く内容を消す。 |
| `int materialCount（読み書き）` | `int count = cr.materialCount;` | マテリアルの数。 |
| `void SetMaterial(Material@ material, int index)` / `Material@ GetMaterial(int index = 0) const` | `Material@ m = cr.GetMaterial();` | 描くときのマテリアル。 |
| `void SetTexture(Texture2D@ texture)` | `cr.SetTexture(texture);` | 描くテクスチャ。 |

普段は Graphic の `color` や `material` を使い、CanvasRenderer は直接さわりません。

### FontAsset

Text が使うフォント。Project の `.ttf`・`.otf` から作られる。

**共通メンバー**

- [Object](#object) の `name`、`GetInstanceID`

### Selectable

選んだり押したりできる UI の共通部分。キーボードやゲームパッドでの移動先もここで調べる。

**共通メンバー**

- [Behavior](#behavior) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `bool interactable（読み書き）` | `selectable.interactable = false; bool canPress = selectable.interactable;` | ボタンなどの UI を操作可能にするかを読み書きする。false の間は `colors.disabledColor` で描かれる。 |
| `Graphic@ targetGraphic（読み書き）` | `@button.targetGraphic = background;` | 状態（通常・ハイライト・押下など）に合わせて色を変える Graphic。 |
| `ColorBlock colors（読み書き）` | `ColorBlock c = button.colors; c.pressedColor = Color::red; button.colors = c;` | 状態ごとの色。値型なので、取り出して書き換えてから戻す。 |
| `Selectable@ FindSelectable(const Vector3&in dir) const` | `Selectable@ next = button.FindSelectable(Vector3::right);` | 指定した向きにある、いちばん近い Selectable。なければ null。 |
| `Selectable@ FindSelectableOnLeft() const` | `Selectable@ left = button.FindSelectableOnLeft();` | 左の Selectable。 |
| `Selectable@ FindSelectableOnRight() const` | `Selectable@ right = button.FindSelectableOnRight();` | 右の Selectable。 |
| `Selectable@ FindSelectableOnUp() const` | `Selectable@ up = button.FindSelectableOnUp();` | 上の Selectable。 |
| `Selectable@ FindSelectableOnDown() const` | `Selectable@ down = button.FindSelectableOnDown();` | 下の Selectable。 |

**名前空間 `Selectable`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Selectable@[]@ allSelectablesArray（読み取り）` | `array<Selectable@>@ all = Selectable::allSelectablesArray;` | 今有効な Selectable をすべて返す。 |

### Button

押したときのイベントを持つ UI コンポーネント。

**共通メンバー**

- [Selectable](#selectable) の `name`、`GetInstanceID`、`gameObject`、`transform`、`tag`、`CompareTag`、`enabled`、`isActiveAndEnabled`、`interactable`、`targetGraphic`、`colors`、`FindSelectable`、`FindSelectableOnLeft`、`FindSelectableOnRight`、`FindSelectableOnUp`、`FindSelectableOnDown`、`TryGetComponent`、`GetComponent`

| API | 用例 | 解説 |
| --- | --- | --- |
| `UnityEvent@ onClick（読み取り）` | `UnityEvent@ clicked = button.onClick;` | クリック時に発火するイベントを返す。Inspector の On Click の登録と、スクリプトからのリスナーを扱える。 |

### ColorBlock

[Selectable](#selectable) の状態ごとの色をまとめた値型。

| API | 用例 | 解説 |
| --- | --- | --- |
| `ColorBlock()` | `ColorBlock c;` | 既定の色で作る。 |
| `Color normalColor` | `c.normalColor = Color::white;` | 通常の色。 |
| `Color highlightedColor` | `c.highlightedColor = Color(0.9f, 0.9f, 0.9f);` | マウスが乗っているときの色。 |
| `Color pressedColor` | `c.pressedColor = Color::gray;` | 押しているときの色。 |
| `Color selectedColor` | `c.selectedColor = Color::cyan;` | キーボードなどで選ばれているときの色。 |
| `Color disabledColor` | `c.disabledColor = Color(1, 1, 1, 0.5f);` | `interactable` が false のときの色。 |
| `float colorMultiplier` | `c.colorMultiplier = 1.0f;` | 状態の色に掛ける倍率。 |
| `float fadeDuration` | `c.fadeDuration = 0.1f;` | 色を切り替えるときにかける秒数。 |

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
| `float maximumDeltaTime（読み書き）` | `Time::maximumDeltaTime = 0.1f;` | 1 フレームで進める時間の上限。重いフレームのあとに物理が何歩も進みすぎないようにする。 |
| `float fixedTime（読み取り）` | `float t = Time::fixedTime;` | 最後の `FixedUpdate` の時刻（秒）。 |
| `double timeAsDouble（読み取り）` | `double t = Time::timeAsDouble;` | `time` を double で返す。長く遊んでも精度が落ちにくい。 |
| `double fixedTimeAsDouble（読み取り）` | `double t = Time::fixedTimeAsDouble;` | `fixedTime` を double で返す。 |
| `bool inFixedTimeStep（読み取り）` | `if (Time::inFixedTimeStep) { … }` | 今 `FixedUpdate` の中か。 |

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

### Vector4

4 つの成分を持つ値型。シェーダの `Vector` のプロパティ（`Material.SetVector`）などに使う。

| API | 用例 | 解説 |
| --- | --- | --- |
| `Vector4()` / `Vector4(float x, float y, float z, float w)` | `Vector4 v(1, 2, 3, 4);` | 成分を指定して作る。省略すると 0。 |
| `Vector4(float x, float y, float z)` / `Vector4(float x, float y)` | `Vector4 v(1, 2, 3);` | 足りない成分は 0。 |
| `+` `-` `*` `/`（`float` との掛け算・割り算）、`+=` など、単項の `-` | `Vector4 r = (a + b) * 0.5f;` | 成分ごとの計算。 |
| `float magnitude` / `float sqrMagnitude` / `Vector4 normalized`（読み取り） | `Vector4 n = v.normalized;` | 長さ、長さの二乗、長さを 1 にしたもの。 |
| `Vector3` / `Vector2` への変換 | `Vector3 p = v;` | 先頭の成分を取り出して代入できる。 |
| `float x` `y` `z` `w` | `v.w = 1.0f;` | 各成分。 |
| `string ToString() const` | `Debug::Log(v.ToString());` | 成分を文字列にする。 |

**名前空間 `Vector4`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Dot` / `Distance` / `Lerp` / `LerpUnclamped` / `MoveTowards` / `Normalize` / `Scale` / `Min` / `Max` | `Vector4 p = Vector4::Lerp(a, b, 0.5f);` | Vector3 の同名の関数と同じ。 |
| `const Vector4 zero` / `one` / `positiveInfinity` / `negativeInfinity` | `Vector4 v = Vector4::zero;` | よく使う値。 |

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
| `void SetFromToRotation(const Vector3&in fromDirection, const Vector3&in toDirection)` | `q.SetFromToRotation(Vector3::up, normal);` | `fromDirection` を `toDirection` に向ける回転にする。 |
| `void ToAngleAxis(float&out angle, Vector3&out axis) const` | `float angle; Vector3 axis; q.ToAngleAxis(angle, axis);` | 回転を「軸のまわりに何度回すか」に分ける。 |
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
| `Quaternion SlerpUnclamped(const Quaternion&in a, const Quaternion&in b, float t)` | `Quaternion q = Quaternion::SlerpUnclamped(a, b, 1.2f);` | `t` を 0〜1 に収めない `Slerp`。 |
| `Quaternion LerpUnclamped(const Quaternion&in a, const Quaternion&in b, float t)` | `Quaternion q = Quaternion::LerpUnclamped(a, b, 1.2f);` | `t` を 0〜1 に収めない `Lerp`。 |
| `Quaternion Normalize(const Quaternion&in q)` | `Quaternion unit = Quaternion::Normalize(q);` | 長さを 1 にした四元数を返す。 |
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
| `Color linear（読み取り）` | `Color l = tint.linear;` | ガンマ（sRGB）の色を線形の色に直したもの。シェーダに渡す値の計算などに使う。 |
| `Color gamma（読み取り）` | `Color g = l.gamma;` | 線形の色をガンマ（sRGB）の色に直したもの。 |
| `float grayscale（読み取り）` | `float gray = tint.grayscale;` | 明るさ（灰色にしたときの値）。 |
| `float maxColorComponent（読み取り）` | `float m = tint.maxColorComponent;` | r・g・b のうちいちばん大きい値。 |

**名前空間 `Color`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Color Lerp(const Color&in a, const Color&in b, float t)` | `Color tint = Color::Lerp(Color::red, Color::blue, 0.5f);` | 色同士を 0～1 に制限した割合で線形補間する。 |
| `Color LerpUnclamped(const Color&in a, const Color&in b, float t)` | `Color tint = Color::LerpUnclamped(Color::red, Color::blue, 1.25f);` | 0～1 の範囲外も使える色の線形補間。 |
| `Color HSVToRGB(float h, float s, float v)` | `Color tint = Color::HSVToRGB(0.33f, 1.0f, 1.0f);` | 色相・彩度・明度をそれぞれ 0～1 の値で渡し、RGB の色を得る。 |
| `void RGBToHSV(const Color&in rgbColor, float&out H, float&out S, float&out V)` | `float h, s, v; Color::RGBToHSV(tint, h, s, v);` | 色を色相・彩度・明度（どれも 0〜1）に分ける。 |
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

### Matrix4x4

4×4 の行列。座標の変換に使う。普段は Transform の `localToWorldMatrix` などから受け取る。

| API | 用例 | 解説 |
| --- | --- | --- |
| `Matrix4x4()` / `Matrix4x4(column0, column1, column2, column3)` | `Matrix4x4 m(c0, c1, c2, c3);` | 列（`Vector4`）を 4 つ並べて作る。 |
| `Vector4 GetColumn(int index) const` / `void SetColumn(int index, const Vector4&in column)` | `Vector4 c = m.GetColumn(3);` | 列を読み書きする。 |
| `Vector4 GetRow(int index) const` / `void SetRow(int index, const Vector4&in row)` | `Vector4 r = m.GetRow(0);` | 行を読み書きする。 |
| `Matrix4x4 * Matrix4x4`、`Matrix4x4 * Vector4` | `Matrix4x4 combined = parent * child;` | 行列どうし、行列とベクトルの掛け算。 |
| `Vector3 MultiplyPoint(const Vector3&in point) const` | `Vector3 world = m.MultiplyPoint(local);` | 点を変換する（移動も含む）。 |
| `Vector3 MultiplyPoint3x4(const Vector3&in point) const` | `Vector3 world = m.MultiplyPoint3x4(local);` | 射影のない行列での点の変換。`MultiplyPoint` より速い。 |
| `Vector3 MultiplyVector(const Vector3&in vector) const` | `Vector3 dir = m.MultiplyVector(Vector3::forward);` | 向きを変換する（移動は含まない）。 |
| `Vector3 GetPosition() const` | `Vector3 p = m.GetPosition();` | 移動の成分。 |
| `Quaternion rotation` / `Vector3 lossyScale`（読み取り） | `Quaternion q = m.rotation;` | 回転と拡大率の成分。 |
| `Matrix4x4 inverse` / `Matrix4x4 transpose`（読み取り） | `Matrix4x4 back = m.inverse;` | 逆行列、転置行列。 |
| `float determinant` / `bool isIdentity`（読み取り） | `bool none = m.isIdentity;` | 行列式、単位行列か。 |
| `float m00` 〜 `m33` | `float x = m.m03;` | 各要素（`m行列` の順）。 |
| `string ToString() const` | `Debug::Log(m.ToString());` | 要素を文字列にする。 |

**名前空間 `Matrix4x4`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Matrix4x4 TRS(const Vector3&in pos, const Quaternion&in q, const Vector3&in s)` | `Matrix4x4 m = Matrix4x4::TRS(p, q, Vector3::one);` | 移動・回転・拡大率から行列を作る。 |
| `Translate` / `Rotate` / `Scale` | `Matrix4x4 m = Matrix4x4::Translate(Vector3(0, 1, 0));` | 移動だけ・回転だけ・拡大率だけの行列。 |
| `Inverse` / `Transpose` | `Matrix4x4 inv = Matrix4x4::Inverse(m);` | 逆行列、転置行列。 |
| `const Matrix4x4 identity` / `zero` | `Matrix4x4 m = Matrix4x4::identity;` | 単位行列、すべて 0 の行列。 |

### Rect

2 次元の四角（位置と大きさ）。UI や Sprite の範囲に使う。

| API | 用例 | 解説 |
| --- | --- | --- |
| `Rect()` / `Rect(float x, float y, float width, float height)` | `Rect r(0, 0, 100, 50);` | 左下（最小の角）と大きさで作る。 |
| `Rect(const Vector2&in position, const Vector2&in size)` | `Rect r(Vector2(10, 10), Vector2(64, 64));` | 位置と大きさのベクトルで作る。 |
| `float x` `y` `width` `height` | `r.width = 200;` | 各値。 |
| `Vector2 position` / `size` / `center` / `min` / `max`（読み書き） | `Vector2 c = r.center;` | 位置、大きさ、真ん中、最小の角、最大の角。 |
| `float xMin` / `yMin` / `xMax` / `yMax`（読み書き） | `r.xMax = 300;` | 辺の位置。書くと反対の辺はそのままで大きさが変わる。 |
| `bool Contains(const Vector2&in point) const` | `bool inside = r.Contains(Vector2(5, 5));` | 点が中にあるか（`Vector3` も渡せる。z は無視）。 |
| `bool Overlaps(const Rect&in other) const` | `bool hit = a.Overlaps(b);` | 2 つの四角が重なるか。 |
| `string ToString() const` | `Debug::Log(r.ToString());` | 値を文字列にする。 |

**名前空間 `Rect`**

| API | 用例 | 解説 |
| --- | --- | --- |
| `Rect MinMaxRect(float xmin, float ymin, float xmax, float ymax)` | `Rect r = Rect::MinMaxRect(0, 0, 10, 5);` | 辺の位置から作る。 |
| `Vector2 NormalizedToPoint(const Rect&in rectangle, const Vector2&in normalizedRectCoordinates)` | `Vector2 p = Rect::NormalizedToPoint(r, Vector2(0.5f, 0.5f));` | 四角の中の割合（0〜1）を位置に直す。 |
| `Vector2 PointToNormalized(const Rect&in rectangle, const Vector2&in point)` | `Vector2 t = Rect::PointToNormalized(r, p);` | 位置を四角の中の割合（0〜1）に直す。 |
| `const Rect zero` | `Rect r = Rect::zero;` | すべて 0 の四角。 |

### Bounds

ワールドの軸にそろえた箱（中心と大きさ）。`Collider.bounds` や `Mesh.bounds` で受け取る。

| API | 用例 | 解説 |
| --- | --- | --- |
| `Bounds()` / `Bounds(const Vector3&in center, const Vector3&in size)` | `Bounds b(Vector3::zero, Vector3(2, 2, 2));` | 中心と大きさで作る。 |
| `Vector3 center` / `Vector3 extents` | `Vector3 half = b.extents;` | 中心、大きさの半分。 |
| `Vector3 size` / `min` / `max`（読み書き） | `Vector3 top = b.max;` | 大きさ、最小の角、最大の角。 |
| `void SetMinMax(const Vector3&in min, const Vector3&in max)` | `b.SetMinMax(Vector3::zero, Vector3::one);` | 角の位置で設定する。 |
| `bool Contains(const Vector3&in point) const` | `bool inside = b.Contains(p);` | 点が中にあるか。 |
| `bool Intersects(const Bounds&in bounds) const` | `bool hit = a.Intersects(b);` | 2 つの箱が重なるか。 |
| `Vector3 ClosestPoint(const Vector3&in point) const` | `Vector3 q = b.ClosestPoint(p);` | 箱の上で点にいちばん近い点。 |
| `float SqrDistance(const Vector3&in point) const` | `float d2 = b.SqrDistance(p);` | 点までの距離の二乗（中なら 0）。 |
| `void Encapsulate(const Vector3&in point)` / `void Encapsulate(const Bounds&in bounds)` | `b.Encapsulate(other.bounds);` | 点や箱を含むように広げる。 |
| `void Expand(float amount)` | `b.Expand(0.5f);` | 各辺を広げる（大きさに `amount` を足す）。 |
| `string ToString() const` | `Debug::Log(b.ToString());` | 値を文字列にする。 |

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

### QueryTriggerInteraction

`Physics` の問い合わせがトリガーの Collider に当たるか。

| 値 | 解説 |
| --- | --- |
| `UseGlobal = 0` | `Physics::queriesHitTriggers` に従う（既定）。 |
| `Ignore = 1` | トリガーには当てない。 |
| `Collide = 2` | トリガーにも当てる。 |

### RigidbodyConstraints

Rigidbody の動きを軸ごとに止める。`|` で組み合わせる。

| 値 | 解説 |
| --- | --- |
| `None = 0` | 止めない。 |
| `FreezePositionX = 2` / `FreezePositionY = 4` / `FreezePositionZ = 8` | その軸の移動を止める。 |
| `FreezeRotationX = 16` / `FreezeRotationY = 32` / `FreezeRotationZ = 64` | その軸まわりの回転を止める。 |
| `FreezePosition = 14` | 移動をすべて止める。 |
| `FreezeRotation = 112` | 回転をすべて止める（転ばないキャラクターなど）。 |
| `FreezeAll = 126` | すべて止める。 |

### RigidbodyInterpolation

| 値 | 解説 |
| --- | --- |
| `None = 0` | 補間しない。 |
| `Interpolate = 1` | 前の物理の 1 歩との間を補間して描く。動きがなめらかになり、少し遅れて見える。 |
| `Extrapolate = 2` | 速度から次の位置を予想して描く。 |

### CollisionDetectionMode

| 値 | 解説 |
| --- | --- |
| `Discrete = 0` | 物理の 1 歩ごとの位置だけで当たりを調べる（既定、軽い）。 |
| `Continuous = 1` / `ContinuousDynamic = 2` / `ContinuousSpeculative = 3` | 1 歩のあいだの動きも調べて、速い物体のすり抜けを防ぐ。 |

### PhysicsMaterialCombine

ぶつかった 2 つの PhysicsMaterial の値の組み合わせ方。

| 値 | 解説 |
| --- | --- |
| `Average = 0` | 平均（既定）。 |
| `Minimum = 1` | 小さいほう。 |
| `Multiply = 2` | 掛け算。 |
| `Maximum = 3` | 大きいほう。 |

### LightType

| 値 | 解説 |
| --- | --- |
| `Spot = 0` | 1 点から円すい形に照らす。 |
| `Directional = 1` | 太陽のように、一定の向きから全体を照らす。 |
| `Point = 2` | 1 点から全方向に照らす。 |

### WrapMode

アニメーションが最後まで行ったときの動き。

| 値 | 解説 |
| --- | --- |
| `Default = 0` | クリップの設定に従う。 |
| `Once = 1` | 1 回で止まる。 |
| `Loop = 2` | 先頭から繰り返す。 |
| `PingPong = 3` | 行ったり来たりする。 |
| `ClampForever = 4` | 最後の形のまま止まる（再生中のまま）。 |

### TextAlignmentOptions

Text の寄せ方。

| 値 | 解説 |
| --- | --- |
| `TopLeft = 0` / `Top = 1` / `TopRight = 2` | 上の左・中央・右。 |
| `Left = 3` / `Center = 4` / `Right = 5` | 真ん中の高さの左・中央・右。 |
| `BottomLeft = 6` / `Bottom = 7` / `BottomRight = 8` | 下の左・中央・右。 |

### ParticleSystemSimulationSpace

| 値 | 解説 |
| --- | --- |
| `Local = 0` | 粒が GameObject と一緒に動く。 |
| `World = 1` | 出たあとの粒はワールドに残る（煙の軌跡など）。 |

### ParticleSystemShapeType

| 値 | 解説 |
| --- | --- |
| `Sphere = 0` | 球から外向きに出す。 |
| `Cone = 1` | 円すいの向き（GameObject の前方向）に出す。 |

### ParticleSystemSortMode

| 値 | 解説 |
| --- | --- |
| `None = 0` | 並べ替えない。 |
| `Distance = 1` | カメラから遠い粒から描く。 |
| `OldestInFront = 2` | 古い粒を手前に描く。 |
| `YoungestInFront = 3` | 新しい粒を手前に描く。 |

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
