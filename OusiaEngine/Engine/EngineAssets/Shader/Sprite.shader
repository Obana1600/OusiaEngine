// UI 用（アルファブレンド。スワップチェーンには深度がないので、深度は使わない）
{
	"queue": "Transparent",
	"passes": [
		{
			"name": "Default",
			"vertex": "Sprite.VS.hlsl",
			"pixel": "Sprite.PS.hlsl",
			"srcBlend": "SrcAlpha",
			"dstBlend": "OneMinusSrcAlpha",
			"zWrite": false,
			"zTest": "Always",
			"cull": "Off"
		}
	]
}
