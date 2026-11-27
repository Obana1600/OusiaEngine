// 水面の波: Waves.compute で Gerstner 波を毎フレーム計算し、水のマテリアル（Water.shader）に渡す
// WaterGrid と同じ GameObject に付ける。size はグリッドの大きさ（WaterGrid.obj は 20 m）に合わせる
class WaterWaves : MonoBehavior {
	ComputeShader@ waveShader;
	int resolution = 256;	 // 変位・法線のテクスチャの大きさ（8 の倍数）
	float size = 20.0f;		 // グリッドの大きさ（m）
	float windAngle = 30.0f; // 波の進む向き（度。+X から +Z へ回る）
	float amplitude = 1.0f;	 // 波の高さの倍率
	float steepness = 0.7f;	 // 頂の尖り（0〜1）
	float speed = 1.0f;		 // 時間の進み方の倍率

	private RenderTexture@ displacementMap;
	private RenderTexture@ normalMap;
	private int kernel = -1;

	void Start() {
		if (waveShader is null) {
			Debug::LogError("WaterWaves: waveShader に Waves.compute を設定してください");
			return;
		}

		@displacementMap = CreateMap();
		@normalMap = CreateMap();

		kernel = waveShader.FindKernel("CSMain");
		waveShader.SetTexture(kernel, "DisplacementIndex", displacementMap);
		waveShader.SetTexture(kernel, "NormalIndex", normalMap);

		Material@ material = gameObject.GetComponent<MeshRenderer>().material;
		material.SetTexture("_DisplacementMap", displacementMap);
		material.SetTexture("_NormalMap", normalMap);
		material.SetFloat("_WaveStrength", 1.0f);
	}

	void Update() {
		if (kernel < 0) { return; }

		// 値はインスペクターで変えられるように、毎フレーム渡す
		array<Vector4> waves = MakeWaves();
		waveShader.SetVectorArray("Waves", waves);
		waveShader.SetInt("WaveCount", int(waves.length()));
		waveShader.SetInt("Resolution", resolution);
		waveShader.SetFloat("Size", size);
		waveShader.SetFloat("Steepness", steepness);
		waveShader.SetFloat("Time", Time::time * speed);

		const int groups = (resolution + 7) / 8;
		waveShader.Dispatch(kernel, groups, groups, 1);
	}

	private RenderTexture@ CreateMap() {
		RenderTexture@ map = RenderTexture(resolution, resolution, 0, RenderTextureFormat::ARGBHalf);
		map.enableRandomWrite = true;
		map.Create();
		return map;
	}

	// 風の向きのまわりに、波長の違う波を 6 本（xy = 向き、z = 振幅、w = 波長）
	private array<Vector4> MakeWaves() {
		const array<float> angles = { 0.0f, 25.0f, -30.0f, 60.0f, -55.0f, 10.0f };
		const array<float> wavelengths = { 8.0f, 5.3f, 3.7f, 2.4f, 1.6f, 1.1f };

		array<Vector4> waves;
		for (uint i = 0; i < angles.length(); i++) {
			const float radians = (windAngle + angles[i]) * Mathf::Deg2Rad;
			const float height = wavelengths[i] * 0.018f * amplitude; // 波長が長いほど高い
			waves.insertLast(Vector4(Mathf::Cos(radians), Mathf::Sin(radians), height, wavelengths[i]));
		}
		return waves;
	}
}
