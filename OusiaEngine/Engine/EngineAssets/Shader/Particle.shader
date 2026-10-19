// パーティクル用（既定は加算。ほかのブレンドはマテリアルで上書きする）
{
	"queue": "Transparent",
	"passes": [
		{
			"name": "Default",
			"vertex": "Particle.VS.hlsl",
			"pixel": "Particle.PS.hlsl",
			"srcBlend": "SrcAlpha",
			"dstBlend": "One",
			"zWrite": false,
			"zTest": "LEqual",
			"cull": "Off"
		}
	]
}
