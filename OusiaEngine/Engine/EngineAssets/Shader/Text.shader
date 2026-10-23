// テキスト用（SDF。アルファブレンド。スワップチェーンには深度がないので、深度は使わない）
{
	"queue": "Transparent",
	"passes": [
		{
			"name": "Default",
			"vertex": "Text.VS.hlsl",
			"pixel": "Text.PS.hlsl",
			"srcBlend": "SrcAlpha",
			"dstBlend": "OneMinusSrcAlpha",
			"zWrite": false,
			"zTest": "Always",
			"cull": "Off"
		}
	]
}