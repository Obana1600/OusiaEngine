#pragma once


namespace ou {

struct RenderQueueRange {
	int lowerBound = 0;
	int upperBound = 5000;

	static const RenderQueueRange All;
	static const RenderQueueRange Opaque;
	static const RenderQueueRange Transparent;
};

inline const RenderQueueRange RenderQueueRange::All{0, 5000};
inline const RenderQueueRange RenderQueueRange::Opaque{0, 2500};
inline const RenderQueueRange RenderQueueRange::Transparent{2501, 5000};

} // namespace ou
