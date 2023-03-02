#ifndef MYVK_RG_STATIC_EXTERNAL_BUFFER_HPP
#define MYVK_RG_STATIC_EXTERNAL_BUFFER_HPP

#include <myvk_rg/RenderGraph.hpp>

namespace myvk_rg {
class StaticBuffer final : public myvk_rg::ExternalBufferBase {
private:
	myvk::Ptr<myvk::BufferBase> m_buffer;

	MYVK_RG_OBJECT_FRIENDS
	MYVK_RG_INLINE_INITIALIZER(myvk::Ptr<myvk::BufferBase> buffer) { m_buffer = std::move(buffer); }

public:
	inline const myvk::Ptr<myvk::BufferBase> &GetVkBuffer() const final { return m_buffer; }
	inline bool IsStatic() const final { return true; }
};
} // namespace myvk_rg

#endif
