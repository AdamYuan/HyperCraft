#pragma once

namespace hc::client {

using BlockAlgoAxis = uint8_t;

template <typename T> struct BlockAlgoBound {
	T min_x, min_y, min_z, max_x, max_y, max_z;
};
struct BlockAlgoSwizzle {
	BlockAlgoAxis a1, a2, a3;
};
constexpr BlockAlgoSwizzle kBlockAlgoSwizzleXYZ = {0, 1, 2};
constexpr BlockAlgoSwizzle kBlockAlgoSwizzleXZY = {0, 2, 1};
constexpr BlockAlgoSwizzle kBlockAlgoSwizzleYXZ = {1, 0, 2};
constexpr BlockAlgoSwizzle kBlockAlgoSwizzleYZX = {1, 2, 0};
constexpr BlockAlgoSwizzle kBlockAlgoSwizzleZXY = {2, 0, 1};
constexpr BlockAlgoSwizzle kBlockAlgoSwizzleZYX = {2, 1, 0};

template <typename T, BlockAlgoBound<T> Bound, BlockAlgoSwizzle Swizzle = kBlockAlgoSwizzleYZX> struct BlockAlgoConfig {
	using Type = T;
	inline static constexpr BlockAlgoAxis kA1 = Swizzle.a1, kA2 = Swizzle.a2, kA3 = Swizzle.a3;
	inline static constexpr BlockAlgoBound<T> kBound = Bound;

	static_assert(kA1 != kA2 && kA2 != kA3 && kA1 != kA3);
	inline static constexpr bool XYZInBound(T x, T y, T z) {
		return Bound.min_x <= x && x < Bound.max_x && Bound.min_y <= y && y < Bound.max_y && Bound.min_z <= z &&
		       z < Bound.max_z;
	}
	template <typename V> inline static constexpr std::tuple<V, V, V> ToXYZ(V a1, V a2, V a3) {
		return {kA1 == 0 ? a1 : (kA2 == 0 ? a2 : a3), kA1 == 1 ? a1 : (kA2 == 1 ? a2 : a3),
		        kA1 == 2 ? a1 : (kA2 == 2 ? a2 : a3)};
	}
	template <BlockAlgoAxis A, typename V> inline static constexpr std::tuple<V, V, V> ToXYZ(V ax, V a1, V a2) {
		if constexpr (A == kA1)
			return ToXYZ<V>(ax, a1, a2);
		else if constexpr (A == kA2)
			return ToXYZ<V>(a1, ax, a2);
		else
			return ToXYZ<V>(a1, a2, ax);
	}
	template <typename Func> inline static constexpr void Call(Func &func, T a1, T a2, T a3) {
		auto [x, y, z] = ToXYZ(a1, a2, a3);
		func(x, y, z);
	}
	template <BlockAlgoAxis A, typename Func> inline static constexpr void Call(Func &func, T ax, T a1, T a2) {
		auto [x, y, z] = ToXYZ<A>(ax, a1, a2);
		func(x, y, z);
	}
	template <BlockAlgoAxis A> inline static constexpr T GetMin() {
		if constexpr (A == 0)
			return Bound.min_x;
		else if constexpr (A == 1)
			return Bound.min_y;
		else
			return Bound.min_z;
	}
	template <BlockAlgoAxis A> inline static constexpr T GetMax() {
		if constexpr (A == 0)
			return Bound.max_x;
		else if constexpr (A == 1)
			return Bound.max_y;
		else
			return Bound.max_z;
	}
	template <BlockAlgoAxis A, typename Func> inline static void Loop1(const Func &func) {
		for (T i = GetMin<A>(); i < GetMax<A>(); ++i)
			func(i);
	}
	template <BlockAlgoAxis A> inline static constexpr BlockAlgoAxis GetNextAxis() {
		if constexpr (A == kA1)
			return kA2;
		else
			return kA1;
	}
	template <BlockAlgoAxis A> inline static constexpr BlockAlgoAxis GetNextAxis2() {
		if constexpr (A == kA1 || A == kA2)
			return kA3;
		else
			return kA2;
	}
	template <BlockAlgoAxis A, typename V> inline static constexpr V GetSpan() { return V(GetMax<A>() - GetMin<A>()); }
	template <BlockAlgoAxis A, typename V> inline static constexpr V GetArea() {
		return GetSpan<GetNextAxis<A>(), V>() * GetSpan<GetNextAxis2<A>(), V>();
	}
	template <BlockAlgoAxis A, typename Func> inline static void Loop2(T a, const Func &func) {
		Loop1<GetNextAxis<A>()>(
		    [&func, a](T a1) { Loop1<GetNextAxis2<A>()>([a1, a, &func](T a2) { Call<A>(func, a, a1, a2); }); });
	}
	template <typename Func> inline static void Loop3(const Func &func) {
		Loop1<kA1>([&func](T a1) {
			Loop1<kA2>([a1, &func](T a2) { Loop1<kA3>([a1, a2, &func](T a3) { Call(func, a1, a2, a3); }); });
		});
	}
};

} // namespace hc::client
