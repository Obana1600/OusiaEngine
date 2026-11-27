// 3D モデル用（不透明）
{
	"queue": "Geometry",
	"passes": [
		{
			"name": "Forward",
			"vertex": "Standard.VS.hlsl",
			"pixel": "Standard.PS.hlsl",
			"zWrite": true,
			"zTest": "Less",
			"cull": "Back"
		},
		{
			"name": "ShadowCaster",
			"tags": { "LightMode": "ShadowCaster" },
			"vertex": "ShadowCaster.VS.hlsl",
			"zWrite": true,
			"zTest": "LEqual",
			"cull": "Back"
		}
	]
}
