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
	],
	// TextMeshPro と同じく、縁取りや影はマテリアルのプロパティ（Text.hlsli の TextMaterial と同じ順）
	"properties": [
		{ "name": "_OutlineColor", "type": "Color", "default": [0, 0, 0, 1] },
		{ "name": "_UnderlayColor", "type": "Color", "default": [0, 0, 0, 0] },
		{ "name": "_UnderlayUvOffset", "type": "Vector", "default": [0, 0, 0, 0] },
		{ "name": "_FaceDilate", "type": "Float", "default": 0 },
		{ "name": "_OutlineWidth", "type": "Float", "default": 0 },
		{ "name": "_UnderlayDilate", "type": "Float", "default": 0 },
		{ "name": "_UnderlaySoftness", "type": "Float", "default": 0 }
	]
}