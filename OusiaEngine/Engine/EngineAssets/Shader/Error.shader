// マテリアルかシェーダがないときに描く（Unity の Hidden/InternalErrorShader）
{
	"queue": "Geometry",
	"passes": [
		{
			"name": "Forward",
			"vertex": "Standard.VS.hlsl",
			"pixel": "Error.PS.hlsl",
			"zWrite": true,
			"zTest": "Less",
			"cull": "Back"
		}
	]
}
