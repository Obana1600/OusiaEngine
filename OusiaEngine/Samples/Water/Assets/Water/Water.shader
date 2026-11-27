// 水面（Waves.compute が書いた変位と法線を読む）
// 不透明なものの後に描き、深度の写し（水の深さ・岸の泡）と色の写し（屈折）を読む
{
	"queue": "Transparent",
	"passes": [
		{
			"name": "Forward",
			"vertex": "Water.VS.hlsl",
			"pixel": "Water.PS.hlsl",
			"zWrite": true,
			"zTest": "Less",
			"cull": "Back"
		}
	],
	// Water.hlsli の WaterProperties と同じ順
	"properties": [
		{ "name": "_DisplacementMap", "type": "Texture" },
		{ "name": "_NormalMap", "type": "Texture" },
		{ "name": "_WaveStrength", "type": "Float", "default": 0 },
		{ "name": "_RefractionStrength", "type": "Float", "default": 0.03 },
		{ "name": "_ShallowColor", "type": "Color", "default": [0.75, 0.95, 0.95, 1] },
		{ "name": "_DeepColor", "type": "Color", "default": [0.02, 0.18, 0.25, 1] },
		{ "name": "_SkyColor", "type": "Color", "default": [0.55, 0.75, 0.95, 1] },
		{ "name": "_FoamColor", "type": "Color", "default": [1, 1, 1, 0.9] },
		{ "name": "_Absorption", "type": "Float", "default": 0.6 },
		{ "name": "_ShoreFoamDistance", "type": "Float", "default": 0.25 },
		{ "name": "_CrestFoam", "type": "Float", "default": 0.45 },
		{ "name": "_Shininess", "type": "Float", "default": 400 }
	]
}
