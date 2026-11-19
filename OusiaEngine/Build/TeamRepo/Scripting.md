# スクリプトの書き方

[エディターガイド](README.md) · [スクリプト API リファレンス](Script-API.md)

ゲーム内の GameObject に処理を追加するときは、AngelScript の `MonoBehavior` を継承します。たとえば同じ Cube を、入力に反応して動く Cube にできます。

## Cube に最初のスクリプトを付ける

1. Play を停止します。Project の `Assets/` 内で右クリックし、**Create > MonoBehavior Script** から `MoveCube` を作ります。
2. `MoveCube.as` を開いて次のコードを保存します。ファイル名とクラス名を大文字・小文字まで合わせます。基底クラスの綴りは `MonoBehavior` です。
3. GameObject メニューから Cube を作り、選択して **Inspector > Add Component** で `MoveCube` を追加します。
4. Play を押して **Game** をクリックします。**A/D** または **←/→** で横に動き、**Space** で Console にログが出ます。

```angelscript
class MoveCube : MonoBehavior {
    float speed = 3.0f;

    void Start() {
        Debug::Log(name + " is ready");
    }

    void Update() {
        const float x = Input::GetAxis("Horizontal");
        transform.Translate(x * speed * Time::deltaTime, 0.0f, 0.0f, Space::World);

        if (Input::GetKeyDown(KeyCode::Space)) {
            Debug::Log(gameObject.name + " received Space");
        }
    }
}
```

`speed` は 1 秒あたりの移動量です。`Time::deltaTime` を掛けることで、そのフレームで進む量に変換します。`Space::World` を渡したので、Cube を回転してもワールドの X 軸に沿って動きます。停止後に Inspector の `speed` を変えて試してください。

VS Code を使う場合は **AngelScript Language Server**（`sashi0034.angel-lsp`）を入れ、OusiaEditor の **Edit > Preferences...** にある **External Script Editor** で **Visual Studio Code** を選びます。`.as` をダブルクリックするとプロジェクトごと開き、起動時に生成された `as.predefined` が補完に使われます。 [Visual Studio Marketplace](https://marketplace.visualstudio.com/items?itemName=sashi0034.angel-lsp)

編集モードでは、保存された `.as` の変更を約 1 秒間隔で検出します。すべてのスクリプトがまとめてコンパイルされます。追加したクラスが見つからないときは Console を開き、`Assets/….as(行,列)` のエラーを直してからもう一度試します。

## AngelScript 固有の書き方

以下の短い例は、特に指定がなければ **MonoBehavior を継承したクラスのメソッド内**に書きます。

### ハンドル（@）と null

GameObject や Component を参照する変数は `GameObject@` や `Rigidbody@` と宣言します。既存の変数が指す先を入れ替えるときは、左辺にも `@` を付けます。`is` と `!is` は、同じオブジェクトを指しているか、あるいは未設定の `null` かを調べる演算子です。

```angelscript
GameObject@ target;                        // 初期値は null
@target = GameObject::Find("Player");       // ハンドルの参照先を設定
if (target !is null) {
    Debug::Log(target.name);
}
@target = null;                            // ハンドルの参照を外す
```

`@target = null` は参照を外す操作です。GameObject の破棄には `Destroy(target)` を使います。ほかの処理が GameObject を破棄した場合、手元のハンドルは `null` に変わらず、アクセス時に `MissingReferenceException` が出ることがあります。`is null` は参照の未設定を調べるときに使い、破棄を管理する処理では不要になったハンドルもクリアしてください。 [AngelScript マニュアルのハンドル解説](https://angelcode.com/angelscript/sdk/docs/manual/doc_script_handle.html)

より具体的な型が必要なら `cast<型>(参照)` を使います。たとえば `OnTriggerEnter(Collider@ other)` の中で `BoxCollider@ box = cast<BoxCollider>(other);` と書くと、相手が BoxCollider の場合にその参照を取得できます。変換できなかった場合は `box is null` になります。

### :: と . の使い分け

`Time::deltaTime` や `KeyCode::Space` のように、名前空間や列挙型に属する名前へは `::` でアクセスします。`transform.position` のように、特定のオブジェクトが持つプロパティやメソッドへは `.` を使います。

```angelscript
const float step = 2.0f * Time::deltaTime;
if (Input::GetKey(KeyCode::Space)) {
    transform.Translate(Vector3::up * step, Space::World);
}
```

`2.0f` の `f` は `float` 型のリテラルを表します。また、`Transform.position` のように `Vector3` の値を返すプロパティはコピーを返します。X 成分だけ変更する場合も、値を取り出してからプロパティに書き戻します。

```angelscript
Vector3 position = transform.position;
position.x = 2.0f;
transform.position = position;
```

### 引数の &in・&out・&inout

参照引数を宣言するときは、データを渡す方向を型の後ろに書きます。呼び出す側では、受け渡しに使う値や変数をそのまま渡します。

| 宣言 | 意味 | 例 |
| --- | --- | --- |
| `const Vector3&in from` | 関数が読み取る入力。`const` を付けると関数の中から変更できない。 | `Physics::Raycast(transform.position, transform.forward)` |
| `RaycastHit&out hit` | 関数が結果を書き込み、呼び出し元の変数に返す出力。 | `RaycastHit hit; bool found = Physics::Raycast(transform.position, transform.forward, hit);` |
| `array<int>&inout values` | 渡した配列そのものを関数の中で読み書きする。ハンドルを作れる参照型に使える。 | `void AddScore(array<int>&inout values) { values.insertLast(100); }` |

レイが当たった位置を使う例です。`hit` の内容は `Raycast` が `true` を返してから読みます。

```angelscript
RaycastHit hit;
if (Physics::Raycast(transform.position, transform.forward, hit, 10.0f)) {
    Debug::Log(hit.point.ToString());
}
```

`TryGetComponent` は出力先の**ハンドル変数**を受け取る API です。`Rigidbody@ body;` を宣言し、`gameObject.TryGetComponent(@body)` のように渡します。 [AngelScript マニュアルの参照引数解説](https://angelcode.com/angelscript/sdk/docs/manual/doc_script_func_ref.html)

### array<T> と文字列

`array<T>` は要素型を指定する可変長の配列です。`length()` で要素数を取得し、添字は `0` から数えます。文字列は `string` で宣言し、`+` で連結できます。

```angelscript
array<int> scores = {10, 20};
scores.insertLast(30);

for (uint i = 0; i < scores.length(); ++i) {
    Debug::Log("score[" + i + "] = " + scores[i]);
}

array<GameObject@> party;
party.insertLast(gameObject);
```

`array<GameObject@>` は GameObject を参照するハンドルの配列です。追加方法や並べ替えなどの詳細は [配列の API](Script-API.md#array) を参照してください。 [AngelScript マニュアルの配列解説](https://angelcode.com/angelscript/sdk/docs/manual/doc_datatypes_arrays.html)

## MonoBehavior のプロパティ

例は、`MonoBehavior` を継承したクラスのメソッドの中に書きます。

| プロパティ | 例 | 解説 |
| --- | --- | --- |
| `GameObject@ gameObject` | `gameObject.SetActive(false);` | このスクリプトを付けた GameObject。無効にすると、その階層の動作が止まる。 |
| `Transform@ transform` | `transform.localPosition = Vector3::zero;` | 自分の GameObject の Transform。例では親を基準にした原点へ移動する。 |
| `string name` | `name = "Player"; Debug::Log(name);` | Component を付けた GameObject の名前。読み書きできる。 |
| `bool enabled` | `enabled = false;` | このスクリプト自身の有効フラグ。GameObject の他のコンポーネントは個別の状態を保つ。 |
| `bool isActiveAndEnabled` | `if (isActiveAndEnabled) { Debug::Log("active"); }` | 自分が有効で、親を含む GameObject の階層もアクティブかを返す。読み取り専用。 |

`enabled` や GameObject を無効にすると、そのスクリプトの `Update()` は止まります。再開には、Inspector や別のスクリプトなど、その外から有効にする操作を使います。

## MonoBehavior のメソッド

`prefab` は Project から割り当てた `GameObject@`、`socket` は Hierarchy から割り当てた `Transform@` とします。

| メソッド | 例 | 解説 |
| --- | --- | --- |
| `Instantiate(GameObject@ original)` | `GameObject@ copy = Instantiate(prefab);` | シーン上の GameObject または Prefab を複製し、生成した GameObject を返す。 |
| `Instantiate(GameObject@ original, Transform@ parent, bool instantiateInWorldSpace = false)` | `GameObject@ copy = Instantiate(prefab, socket, false);` | 指定した Transform の子として複製する。第 3 引数を `true` にすると、複製元のワールド変換を保って親に付ける。 |
| `Instantiate(GameObject@ original, const Vector3&in position, const Quaternion&in rotation, Transform@ parent = null)` | `GameObject@ copy = Instantiate(prefab, socket.position, socket.rotation);` | ワールド位置と回転を指定して複製する。第 4 引数を渡せば親も指定できる。 |
| `Destroy(Object@ obj)` | `Destroy(gameObject);` | GameObject や Component の破棄を予約する。`gameObject` を渡すと子やコンポーネントも破棄される。 |
| `DontDestroyOnLoad(Object@ target)` | `DontDestroyOnLoad(gameObject);` | シーンを切り替えても GameObject を保持する。使い回したい管理オブジェクトのルートに付けて使う。 |

次は Space で Prefab を 1 個生成し、もう一度 Space を押すと消す例です。`ToggleSpawn.as` を GameObject に付け、`prefab` と `socket` を Inspector で設定します。

```angelscript
class ToggleSpawn : MonoBehavior {
    GameObject@ prefab;
    Transform@ socket;
    private GameObject@ spawned;

    void Update() {
        if (!Input::GetKeyDown(KeyCode::Space)) {
            return;
        }

        if (spawned !is null) {
            Destroy(spawned);
            @spawned = null;
            return;
        }

        if (prefab is null || socket is null) {
            Debug::LogWarning("Set prefab and socket in the Inspector");
            return;
        }

        @spawned = Instantiate(prefab, socket.position, socket.rotation);
    }
}
```

この例では `Destroy(spawned)` で破棄を予約し、その直後に `@spawned = null` で手元のハンドルをクリアします。ハンドルの代入と判定は [AngelScript 固有の書き方](#angelscript-固有の書き方) を参照してください。

## Inspector の値と他のコンポーネント

対応型の public フィールドが Inspector に出ます。private や protected のフィールドを保存対象にするには `[SerializeField]` を付けます。

```angelscript
class JumpOnSpace : MonoBehavior {
    [SerializeField] private float impulse = 5.0f;
    private Rigidbody@ body;

    void Awake() {
        @body = gameObject.GetComponent<Rigidbody>();
        if (body is null) {
            Debug::LogWarning("Add a Rigidbody to " + name);
        }
    }

    void Update() {
        if (body !is null && Input::GetKeyDown(KeyCode::Space)) {
            body.AddForce(Vector3::up * impulse, ForceMode::Impulse);
        }
    }
}
```

`JumpOnSpace.as` を付けた GameObject に **Rigidbody** と **Collider** を追加します。Inspector の `impulse` を調整すると、一度の押下で加わる力が変わります。GameObject を移動させ続けたい場合は `FixedUpdate()` で力を加える方法もあります。キーを押した瞬間は `Update()` で読み、固定更新側で使うならフラグに記録します。

`Rigidbody@ rb; if (gameObject.TryGetComponent(@rb)) { rb.mass = 2.0f; }` のように、成否と参照を同時に取得できます。

| 保存できるフィールド | 割り当て方 |
| --- | --- |
| `bool`、`int`、`uint`、`float`、`string`、`Vector2`、`Vector3`、`Quaternion`、`Color` | Inspector で直接編集する。 |
| `GameObject@`、`Transform@`、`Rigidbody@` など公開済み Component の型 | Hierarchy から対象の GameObject をドラッグする。`GameObject@` には Project から Prefab も渡せる。 |
| `Material@`、`AudioClip@` | Project から対応するアセットをドラッグする。 |

`array<T>`、列挙型、ユーザーが定義したスクリプトクラスの参照は、スクリプトの処理内で扱えます。Inspector から保存できる型は上の表のとおりです。

## エンジンから呼ばれるメソッド

使うものだけをクラスに宣言します。戻り値は `void` です。初期化はコンストラクターの代わりに `Awake()` から始めると、エンジンがセットした `gameObject` などにアクセスできます。

| コールバック | 例 | 呼ばれるとき・用途 |
| --- | --- | --- |
| `Awake()` | `void Awake() { Debug::Log(name); }` | 初めてアクティブな階層に入ったとき。自分のコンポーネント参照を取得する。 |
| `OnEnable()` | `void OnEnable() { Debug::Log("enabled"); }` | スクリプトがアクティブかつ有効になったとき。リスナー登録などに使う。 |
| `Start()` | `void Start() { Debug::Log("start"); }` | 有効な状態で最初の更新を迎える前に一度。ゲーム開始時の処理に使う。 |
| `FixedUpdate()` | `void FixedUpdate() { body.AddForce(Vector3::forward * 2.0f); }` | 固定間隔で物理を進めるとき。例の `body` は有効な `Rigidbody@`。 |
| `Update()` | `void Update() { transform.Rotate(0.0f, 45.0f * Time::deltaTime, 0.0f); }` | 各フレーム。入力や毎フレームの動作を処理する。 |
| `LateUpdate()` | `void LateUpdate() { transform.LookAt(target.position); }` | 全体の `Update()` の後。例では有効な `Transform@ target` の移動後の位置を見る。 |
| `OnDisable()` | `void OnDisable() { Debug::Log("disabled"); }` | 無効になったとき。登録したリスナーの解除などに使う。 |
| `OnApplicationQuit()` | `void OnApplicationQuit() { isQuitting = true; }` | ゲームを閉じるとき（エディターでは Play を止めるとき）。`OnDisable()`・`OnDestroy()` より先に呼ばれる。例の `isQuitting` は `bool` フィールドで、`OnDestroy()` で終了中かどうかを見分けるのに使う。 |
| `OnDestroy()` | `void OnDestroy() { Debug::Log("destroyed"); }` | 初期化済みのスクリプトが破棄されるとき。Play を止めたときやゲームを閉じたときも呼ばれ、そのあいだ `Application::isPlaying` は `true` のまま。 |
| `OnCollisionEnter(Collision collision)` | `void OnCollisionEnter(Collision c) { Debug::Log(c.gameObject.name); }` | 物理接触が始まったとき。相手や接触の情報を受け取る。 |
| `OnCollisionStay(Collision collision)` | `void OnCollisionStay(Collision c) { touching = true; }` | 物理接触が続いているとき。例の `touching` は `bool` フィールド。 |
| `OnCollisionExit(Collision collision)` | `void OnCollisionExit(Collision c) { Debug::Log("contact ended"); }` | 物理接触が終わったとき。 |
| `OnTriggerEnter(Collider@ other)` | `void OnTriggerEnter(Collider@ other) { Debug::Log(other.name); }` | トリガーとの重なりが始まったとき。相手の Collider を受け取る。 |
| `OnTriggerStay(Collider@ other)` | `void OnTriggerStay(Collider@ other) { Debug::Log(other.name); }` | 重なりが続いているとき。ログは動作確認用で、頻繁に出る。 |
| `OnTriggerExit(Collider@ other)` | `void OnTriggerExit(Collider@ other) { Debug::Log("left trigger"); }` | 重なりが終わったとき。 |
| `OnValidate()` | `void OnValidate() { radius = Mathf::Max(0.0f, radius); }` | Inspector などで保存対象の値を検証するとき。例の `radius` は `float` フィールド。 |
| `OnDrawGizmos()` | `void OnDrawGizmos() { Gizmos::DrawRay(transform.position, transform.forward); }` | エディターで補助線を描くとき。例では前方向を描く。 |
| `OnDrawGizmosSelected()` | `void OnDrawGizmosSelected() { Gizmos::DrawWireSphere(transform.position, 2.0f); }` | 選択中だけ補助表示を描くとき。例では半径 2 の目安を描く。 |

`OnValidate()` とギズモのコールバックは編集モードでも呼ばれます。Play 開始前にも使える値で処理し、必要ならハンドルを確認してください。API の型、オーバーロード、用途別の用例は [スクリプト API リファレンス](Script-API.md) にあります。
