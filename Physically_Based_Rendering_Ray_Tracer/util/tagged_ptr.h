#ifndef PBRT_UTIL_TAGGED_PTR_H
#define PBRT_UTIL_TAGGED_PTR_H

#include <cassert>
#include <type_traits>

#include "base/base.h"

namespace pstd {
	namespace detail {

		template <typename F, typename R, typename T>
		R Dispatch(F&& func, const void* ptr, int index) {
			assert(index == 0 && "TaggedPtr::Dispatch: index problem");
			return func((const T*)ptr);
		}

		template<typename F, typename R, typename T>
		R Dispatch(F&& func, void* ptr, int index) {
			assert(index == 0 && "TaggedPtr::Dispatch: index problem");
			return func((T*)ptr);
		}

		template<typename F, typename R, typename T0, typename T1>
		R Dispatch(F&& func, const void* ptr, int index) {
			assert(index >= 0 && index < 2 && "TaggedPtr::Dispatch: index problem");
			if (index == 0)
				return func((const T0*)ptr);
			else
				return func((const T1*)ptr);
		}

		template<typename F, typename R, typename T0, typename T1>
		R Dispatch(F&& func, void* ptr, int index) {
			assert(index >= 0 && index < 2 && "TaggedPtr::Dispatch: index problem");
			if (index == 0)
				return func((T0*)ptr);
			else
				return func((T1*)ptr);
		}

		template<typename F, typename R, typename T0, typename T1, typename T2>
		R Dispatch(F&& func, const void* ptr, int index) {
			assert(index >= 0 && index < 3 && "TaggedPtr::Dispatch: index problem");
			switch (index) {
			case 0:
				return func((const T0*)ptr);
			case 1:
				return func((const T1*)ptr);
			default:
				return func((const T2*)ptr);
			}
		}

		template<typename F, typename R, typename T0, typename T1, typename T2>
		R Dispatch(F&& func, void* ptr, int index) {
			assert(index >= 0 && index < 3 && "TaggedPtr::Dispatch: index problem");
			switch (index) {
			case 0:
				return func((T0*)ptr);
			case 1:
				return func((T1*)ptr);
			default:
				return func((T2*)ptr);
			}
		}

		template<typename F, typename R, typename T0, typename T1, typename T2, typename T3>
		R Dispatch(F&& func, const void* ptr, int index) {
			assert(index >= 0 && index < 4 && "TaggedPtr::Dispatch: index problem");
			switch (index) {
			case 0:
				return func((const T0*)ptr);
			case 1:
				return func((const T1*)ptr);
			case 2:
				return func((const T2*)ptr);
			default:
				return func((const T3*)ptr);
			}
		}

		template<typename F, typename R, typename T0, typename T1, typename T2, typename T3>
		R Dispatch(F&& func, void* ptr, int index) {
			assert(index >= 0 && index < 4 && "TaggedPtr::Dispatch: index problem");
			switch (index) {
			case 0:
				return func((T0*)ptr);
			case 1:
				return func((T1*)ptr);
			case 2:
				return func((T2*)ptr);
			default:
				return func((T3*)ptr);
			}
		}

		template<typename F, typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
		R Dispatch(F&& func, const void* ptr, int index) {
			assert(index >= 0 && index < 5 && "TaggedPtr::Dispatch: index problem");
			switch (index) {
			case 0:
				return func((const T0*)ptr);
			case 1:
				return func((const T1*)ptr);
			case 2:
				return func((const T2*)ptr);
			case 3:
				return func((const T3*)ptr);
			default:
				return func((const T4*)ptr);
			}
		}

		template<typename F, typename R, typename T0, typename T1, typename T2, typename T3, typename T4>
		R Dispatch(F&& func, void* ptr, int index) {
			assert(index >= 0 && index < 5 && "TaggedPtr::Dispatch: index problem");
			switch (index) {
			case 0:
				return func((T0*)ptr);
			case 1:
				return func((T1*)ptr);
			case 2:
				return func((T2*)ptr);
			case 3:
				return func((T3*)ptr);
			default:
				return func((T4*)ptr);
			}
		}

		template<typename F, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
		R Dispatch(F&& func, const void* ptr, int index) {
			assert(index >= 0 && index < 6 && "TaggedPtr::Dispatch: index problem");
			switch (index) {
			case 0:
				return func((const T0*)ptr);
			case 1:
				return func((const T1*)ptr);
			case 2:
				return func((const T2*)ptr);
			case 3:
				return func((const T3*)ptr);
			case 4:
				return func((const T4*)ptr);
			default:
				return func((const T5*)ptr);
			}
		}

		template<typename F, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
		R Dispatch(F&& func, void* ptr, int index) {
			assert(index >= 0 && index < 6 && "TaggedPtr::Dispatch: index problem");
			switch (index) {
			case 0:
				return func((T0*)ptr);
			case 1:
				return func((T1*)ptr);
			case 2:
				return func((T2*)ptr);
			case 3:
				return func((T3*)ptr);
			case 4:
				return func((T4*)ptr);
			default:
				return func((T5*)ptr);
			}
		}

		template<typename F, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
		R Dispatch(F&& func, const void* ptr, int index) {
			assert(index >= 0 && index < 7 && "TaggedPtr::Dispatch: index problem");
			switch (index) {
			case 0:
				return func((const T0*)ptr);
			case 1:
				return func((const T1*)ptr);
			case 2:
				return func((const T2*)ptr);
			case 3:
				return func((const T3*)ptr);
			case 4:
				return func((const T4*)ptr);
			case 5:
				return func((const T5*)ptr);
			default:
				return func((const T6*)ptr);
			}
		}

		template<typename F, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
		R Dispatch(F&& func, void* ptr, int index) {
			assert(index >= 0 && index < 7 && "TaggedPtr::Dispatch: index problem");
			switch (index) {
			case 0:
				return func((T0*)ptr);
			case 1:
				return func((T1*)ptr);
			case 2:
				return func((T2*)ptr);
			case 3:
				return func((T3*)ptr);
			case 4:
				return func((T4*)ptr);
			case 5:
				return func((T5*)ptr);
			default:
				return func((T6*)ptr);
			}
		}

		template<typename F, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
		R Dispatch(F&& func, const void* ptr, int index) {
			assert(index >= 0 && index < 8 && "TaggedPtr::Dispatch: index problem");
			switch (index) {
			case 0:
				return func((const T0*)ptr);
			case 1:
				return func((const T1*)ptr);
			case 2:
				return func((const T2*)ptr);
			case 3:
				return func((const T3*)ptr);
			case 4:
				return func((const T4*)ptr);
			case 5:
				return func((const T5*)ptr);
			case 6:
				return func((const T6*)ptr);
			default:
				return func((const T7*)ptr);
			}
		}

		template<typename F, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
		R Dispatch(F&& func, void* ptr, int index) {
			assert(index >= 0 && index < 8 && "TaggedPtr::Dispatch: index problem");
			switch (index) {
			case 0:
				return func((T0*)ptr);
			case 1:
				return func((T1*)ptr);
			case 2:
				return func((T2*)ptr);
			case 3:
				return func((T3*)ptr);
			case 4:
				return func((T4*)ptr);
			case 5:
				return func((T5*)ptr);
			case 6:
				return func((T6*)ptr);
			default:
				return func((T7*)ptr);
			}
		}

		template<typename F, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename... Ts, typename = std::enable_if_t<(sizeof...(Ts) > 0)>>
			R Dispatch(F&& func, const void* ptr, int index) {
			assert(index >= 0 && "TaggedPtr::Dispatch: index problem");
			switch (index) {
			case 0:
				return func((const T0*)ptr);
			case 1:
				return func((const T1*)ptr);
			case 2:
				return func((const T2*)ptr);
			case 3:
				return func((const T3*)ptr);
			case 4:
				return func((const T4*)ptr);
			case 5:
				return func((const T5*)ptr);
			case 6:
				return func((const T6*)ptr);
			case 7:
				return func((const T7*)ptr);
			default:
				return Dispatch<F, R, Ts...>(func, ptr, index - 8);
			}
		}

		template<typename F, typename R, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename... Ts, typename = std::enable_if_t<(sizeof...(Ts) > 0)>>
			R Dispatch(F&& func, void* ptr, int index) {
			assert(index >= 0 && "TaggedPtr::Dispatch: index problem");
			switch (index) {
			case 0:
				return func((T0*)ptr);
			case 1:
				return func((T1*)ptr);
			case 2:
				return func((T2*)ptr);
			case 3:
				return func((T3*)ptr);
			case 4:
				return func((T4*)ptr);
			case 5:
				return func((T5*)ptr);
			case 6:
				return func((T6*)ptr);
			case 7:
				return func((T7*)ptr);
			default:
				return Dispatch<F, R, Ts...>(func, ptr, index - 8);
			}
		}

		template<typename... Ts>
		struct IsSameType;

		template<>
		struct IsSameType<> {
			static constexpr bool value = true;
		};
		template<typename T>
		struct IsSameType<T> {
			static constexpr bool value = true;
		};
		template<typename T1, typename T2, typename... Ts>
		struct IsSameType<T1, T2, Ts...> {
			static constexpr bool value = (std::is_same_v<T1, T2> && IsSameType<T2, Ts...>::value);
		};

		template<typename... Ts>
		struct SameType;
		template<typename T, typename... Ts>
		struct SameType<T, Ts...> {
			using type = T;
			static_assert(IsSameType<T, Ts...>::value, "Not all types in pack are the same");
		};

		template<typename F, typename... Ts>
		struct ReturnType {
			using type = typename SameType<std::invoke_result_t<F, Ts*>...>::type;
		};

		template<typename F, typename... Ts>
		struct ReturnTypeConst {
			using type = typename SameType<std::invoke_result_t<F, const Ts*>...>::type;
		};

		

		template<typename... Ts>
		struct TypePack {
			static constexpr int size = sizeof...(Ts);
		};

		template<typename T, typename... Ts>
		struct IndexOf {
			static constexpr int index = 0;
			static_assert(!std::is_same_v<T, T>, "IndexOf: This class is not belong t the typeback");
		};

		template<typename T, typename... Ts>
		struct IndexOf<T, TypePack<T, Ts...>> {
			static constexpr int index = 0;
		};

		template<typename T, typename U, typename... Ts>
		struct IndexOf<T, TypePack<U, Ts...>> {
			static constexpr int index = 1 + IndexOf<T, TypePack<Ts...>>::index;
		};

		template<uint16_t N, typename Pack>
		struct TypeAt;

		template<typename T, typename... Ts>
		struct TypeAt<0, TypePack<T, Ts...>> { using type = T; };

		template<uint16_t N, typename T, typename... Ts>
		struct TypeAt<N, TypePack<T, Ts...>> {
			static_assert (N <= sizeof...(Ts), "TypeAt:: Index out of range");
			using type = typename TypeAt<N - 1, TypePack<Ts...>>::type;
		};

		template<uint16_t N, typename Pack>
		using TypeAt_t = typename TypeAt<N, Pack>::type;

	}

	template<typename... Ts>
	class TaggedPointer {
	public:
		using Types = detail::TypePack<Ts...>;

		TaggedPointer() = default;
		template<typename T>
		TaggedPointer(T* ptr) {
			uint64_t iptr = reinterpret_cast<uint64_t>(ptr);
			assert((iptr & ptrMask) == iptr && "TaggedPointer:: Construct: head of ptr is not empty");
			constexpr unsigned int type = TypeIndex<T>();
			bits = iptr | ((uint64_t)type << tagShift);
		}
		TaggedPointer(std::nullptr_t np){}

		TaggedPointer(const TaggedPointer& t) { bits = t.bits; }

		TaggedPointer& operator=(const TaggedPointer& t) {
			bits = t.bits;
			return *this;
		}
		
		template<typename T>
		static constexpr unsigned int TypeIndex() {
			using Tp = std::remove_cv_t<T>;
			if constexpr (std::is_same_v<Tp, std::nullptr_t>)
				return 0;
			else
				return 1 + detail::IndexOf<Tp, Types>::index;
		}

		unsigned Tag() const { return ((bits & tagMask) >> tagShift); }

		template<typename T>
		bool Is() const {
			return Tag() == TypeIndex<T>();
		}

		static constexpr unsigned MaxTag() { return sizeof...(Ts); }
		static constexpr unsigned NumTags() { return MaxTag() + 1; }

		explicit operator bool() const { return (bits & ptrMask) != 0; }

		bool operator<(const TaggedPointer& tp) const {
			return bits < tp.bits;
		}

		template<typename T>
		T* Cast() {
			if (Is<T>())
				return reinterpret_cast<T*>(ptr());
			else
				return nullptr;
		}

		template<typename T>
		const T* Cast() const {
			if (Is<T>())
				return reinterpret_cast<const T*> (ptr());
			else
				return nullptr;
		}

		bool operator==(const TaggedPointer& tp) const { return bits == tp.bits; }
		bool operator!=(const TaggedPointer& tp) const { return bits != tp.bits; }
		

		/*auto get() {
			assert(ptr() && "TaggedPointer::get: ptr is null");
			return (detail::TypeAt_t<Tag() - 1, Types>*)ptr();
		}
		auto get() const {
			assert(ptr() && "TaggedPointer::get: ptr is null");
			return (detail::TypeAt_t<Tag() - 1, Types>*)ptr();
		}*/

		void* ptr() { return reinterpret_cast<void*>(bits & ptrMask); }
		const void* ptr() const { return reinterpret_cast<const void*> (bits & ptrMask); }

		template<typename F>
		decltype(auto) Dispatch(F&& func) {
			assert(ptr() && "TaggedPointer::Dispatch: ptr is null");
			using R = typename detail::ReturnType<F, Ts...>::type;
			return detail::Dispatch<F, R, Ts...>(func, ptr(), Tag() - 1);
		}

		template<typename F>
		decltype(auto) Dispatch(F&& func) const {
			assert(ptr() && "TaggedPointer::Dispatch: ptr is null");
			using R = typename detail::ReturnTypeConst<F, Ts...>::type;
			return detail::Dispatch<F, R, Ts...>(func, ptr(), Tag() - 1);
		}

	private:


		static_assert(sizeof(uintptr_t) <= sizeof(uint64_t), "Excepted pointer size to be <= 64 bits");

		static constexpr int tagShift = 57;
		static constexpr int tagBits = 7;
		static constexpr uint64_t tagMask = 0xFE00000000000000;
		static constexpr uint64_t ptrMask = 0x01FFFFFFFFFFFFFF;
		uint64_t bits = 0;
	};

}

#endif // !PBRT_UTIL_TAGGED_PTR_H