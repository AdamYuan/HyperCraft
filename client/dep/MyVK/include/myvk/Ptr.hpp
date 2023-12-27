#ifndef MYVK_PTR_HPP
#define MYVK_PTR_HPP

#include <memory>
#include <type_traits>

namespace myvk {
template <typename T> using Ptr = std::shared_ptr<T>;
template <typename T, typename... Args> inline Ptr<T> MakePtr(Args &&...args) {
	return std::make_shared<T>(std::forward<Args>(args)...);
}
} // namespace myvk

#endif // MYVK_PTR_HPP
