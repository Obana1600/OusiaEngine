// LineRenderer 用（アルファブレンド。物の裏の線は隠れる）
{
	"queue": "Transparent",
	"passes": [
		{
			"name": "Default",
			"vertex": "Line.VS.hlsl",
			"pixel": "Line.PS.hlsl",
			"srcBlend": "SrcAlpha",
			"dstBlend": "OneMinusSrcAlpha",
			"zWrite": false,
			"zTest": "LEqual",
			"cull": "Off"
		}
	]
}