#pragma once

#include <myvk/Queue.hpp>

#include "RenderGraphAllocator.hpp"
#include "RenderGraphScheduler.hpp"

namespace myvk_rg::_details_ {

class RenderGraphLFInit {
public:
	static void InitLastFrameResources(const myvk::Ptr<myvk::Queue> &queue, const RenderGraphResolver &resolved,
	                                   const RenderGraphScheduler &scheduled, const RenderGraphAllocator &allocated);
};

} // namespace myvk_rg::_details_