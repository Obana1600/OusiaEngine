// Scene ビューの Gizmo の線（エディタだけ）。物の裏に隠れた部分は薄く、見えている部分はそのまま描く
{
	"queue": "Overlay",
	"passes": [
		{
			"name": "Hidden",
			"vertex": "Gizmo.VS.hlsl",
			"pixel": "Gizmo.PS.hlsl",
			"srcBlend": "SrcAlpha",
			"dstBlend": "OneMinusSrcAlpha",
			"zWrite": false,
			"zTest": "Greater",
			"cull": "Off"
		},
		{
			"name": "Visible",
			"vertex": "Gizmo.VS.hlsl",
			"pixel": "Gizmo.PS.hlsl",
			"srcBlend": "SrcAlpha",
			"dstBlend": "OneMinusSrcAlpha",
			"zWrite": false,
			"zTest": "LEqual",
			"cull": "Off"
		}
	]
}
