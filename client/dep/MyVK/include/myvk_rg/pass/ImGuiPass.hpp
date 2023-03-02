#ifdef MYVK_ENABLE_IMGUI
#ifndef MYVK_RG_IMGUIPASS_HPP
#define MYVK_RG_IMGUIPASS_HPP

#include <myvk/ImGuiRenderer.hpp>
#include <myvk_rg/RenderGraph.hpp>

namespace myvk_rg {

class ImGuiPass final : public myvk_rg::GraphicsPassBase {
private:
	myvk::Ptr<myvk::ImGuiRenderer> m_imgui_renderer;

public:
	MYVK_RG_INLINE_INITIALIZER(myvk_rg::ImageInput image) {
		AddColorAttachmentInput<0, myvk_rg::Usage::kColorAttachmentRW>({"image"}, image);
	}

	inline void CreatePipeline() final {
		m_imgui_renderer = myvk::ImGuiRenderer::Create(GetVkRenderPass(), GetSubpass(), 1);
	}
	inline void CmdExecute(const myvk::Ptr<myvk::CommandBuffer> &command_buffer) const final {
		m_imgui_renderer->CmdDrawPipeline(command_buffer, 0);
	}

	inline auto GetImageOutput() { return MakeImageOutput({"image"}); }
};

} // namespace myvk_rg

#endif
#endif
