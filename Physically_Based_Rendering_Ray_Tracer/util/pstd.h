#ifndef PBRT_UTIL_PSTD_H
#define PBRT_UTIL_PSTD_H


#include <float.h>
#include <limits.h>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <string>
#include <thread>
#include <utility>
#include <new>
#include <type_traits>
#include <vector>
#include <memory>

#include "base/base.h"
#include "util/memory.h"


// PSTD List
// Function:
// swap(), bit_cast()
// Class:
//	Container: array, vector, tuple, initilizer_list
//	Else: optional, span, complex

// Modified: #ifndef NDEBUG -> #ifdef DEBUG

namespace pstd{
	template<typename T>
	inline void swap(T& a, T& b) noexcept {
		T tmp = std::move(a);
		a = std::move(b);
		b = std::move(tmp);
	} 
	

	template <typename T, int N, typename Allocator>
	class array;

	template<typename T, typename Allocator>
	class array<T, 0, Allocator> {
	public:
		using value_type = T;
		using iterator = value_type*;
		using const_iterator = const value_type*;
		using size_t = std::size_t;

		void fill(const T& v) { assert(!"should never be called"); }

		bool operator==(const array<T, 0, Allocator>& a) const { return true; }
		bool operator!=(const array<T, 0, Allocator>& a) const { return false; }
		iterator begin() { return nullptr; }
		iterator end() { return nullptr; }
		const_iterator begin() const { return nullptr; }
		const_iterator end() const { return nullptr; }

		size_t size() const { return 0; }

		T& operator[](size_t i) {
			assert(!"should never be called");
			static T t;
			return t;
		}

		const T& operator[](size_t i) const {
			assert(!"should never be called");
			static T t;
			return t;
		}

		T* data() { return nullptr; }
		const T* data() const { return nullptr; }
	};

	template<typename T, int N, typename Allocator = HeapAllocator>
	class array {
		static_assert(N > 0, "Array's size must > 0");
	public:
		using value_type = T;
		using iterator = value_type*;
		using const_iterator = const value_type*;
		using size_t = std::size_t;

		array() : alloc(&Allocator::Get()), values(static_cast<T*>(alloc->allocate(N * sizeof(T), alignof(T)))) {
			pstd::uninitialized_default_construct_n(values, N);
		}

		array(const T& init_value) : alloc(&Allocator::Get()), values(static_cast<T*>(alloc->allocate(N * sizeof(T), alignof(T)))) {
			pstd::uninitialized_fill_n(values, N, init_value);
		}

		array(std::initializer_list<T> v) : alloc(&Allocator::Get()), values(static_cast<T*>(alloc->allocate(N * sizeof(T), alignof(T)))) {
			assert(v.size() == N);
			pstd::uninitialized_copy(v.begin(), v.end(), values);
		}

		array& operator=(std::initializer_list<T> v) {
			assert(v.size() == N && "size must be the same as array's size N");
			alloc = &Allocator::Get();
			pstd::destroy_n(values, N);
			pstd::uninitialized_copy(v.begin(), v.end(), values);
			return *this;
		}

		array(const array& other) : alloc(&Allocator::Get()), values(static_cast<T*>(alloc->allocate(other.size() * sizeof(T), alignof(T)))) {
			pstd::uninitialized_copy(other.begin(), other.end(), values);
		}

		array& operator=(const array& other) {
			if (this == &other) return *this;
			alloc = &Allocator::Get();
			pstd::destroy_n(values, N);
			pstd::uninitialized_copy(other.begin(), other.end(), values);
			return *this;
		}

		array(array&& other) noexcept : alloc(other.alloc), values(other.values) {
			other.values = nullptr;
		}

		array& operator=(array&& other) noexcept {
			if (this == &other) return *this;
			alloc = other.alloc;
			pstd::swap(values, other.values);
			return *this;
		}

		~array() {
			if (!values) return;
			pstd::destroy_n(values, N);
			alloc->free(values);
		}

		void fill(const T& v) {
			pstd::fill(begin(), end(), v);
		}

		bool operator==(const array<T, N, Allocator>& a) const {
			for (size_t i = 0; i < N; ++i) {
				if (values[i] != a.values[i])
					return false;
			}
			return true;
		}

		bool operator!=(const array<T, N, Allocator>& a) const {
			return !(*this == a);
		}

		iterator begin() { return values; }
		iterator end() { return values + N; }
		const_iterator begin() const { return values; }
		const_iterator end() const { return values + N; }

		size_t size() const noexcept { return N; }

		T& operator[](size_t i) { return values[i]; }
		const T& operator[](size_t i) const { return values[i]; }

		T* data() noexcept { return values; }
		const T* data() const noexcept { return values; }

	private:
		Allocator* alloc;
		T* values = nullptr;
	};

	template<typename T, typename Allocator = HeapAllocator>
	class queue {
	public:
		using value_type = T;
		using allocator_type = Allocator;
		using size_type = std::size_t;
		using reference = T&;
		using const_reference = const T&;

		queue() : alloc(&Allocator::Get()), values(nullptr), _capacity(0), _size(0), _front(0) {}

		explicit queue(size_t reserve_size)
			: alloc(&Allocator::Get())
			, values(static_cast<T*>(alloc->allocate(reserve_size * sizeof(T), alignof(T))))
			, _capacity(reserve_size)
			, _size(0)
			, _front(0) {
		}

		queue(const queue& other)
			: alloc(&Allocator::Get())
			, values(static_cast<T*>(alloc->allocate(other._capacity * sizeof(T), alignof(T))))
			, _capacity(other._capacity)
			, _size(other._size)
			, _front(0) {
			for (size_t i = 0; i < _size; ++i) {
				size_t src_idx = (other._front + i) % other._capacity;
				::new(values + i) T(other.values[src_idx]);
			}
		}

		queue& operator=(const queue& other) {
			if (this == &other) return *this;
			queue tmp(other);
			pstd::swap(*this, tmp);
			return *this;
		}

		queue(queue&& other) noexcept
			: alloc(other.alloc)
			, values(other.values)
			, _capacity(other._capacity)
			, _size(other._size)
			, _front(other._front) {
			other.values = nullptr;
			other._capacity = 0;
			other._size = 0;
			other._front = 0;
		}

		queue& operator=(queue&& other) noexcept {
			if (this == &other) return *this;
			clear();
			if (values) alloc->free(values);
			alloc = other.alloc;
			values = other.values;
			_capacity = other._capacity;
			_size = other._size;
			_front = other._front;
			other.values = nullptr;
			other._capacity = 0;
			other._size = 0;
			other._front = 0;
			return *this;
		}

		~queue() {
			clear();
			if (values) {
				alloc->free(values);
			}
		}

		reference front() {
			assert(!empty() && "queue::front: queue is empty");
			return values[_front];
		}

		const_reference front() const {
			assert(!empty() && "queue::front: queue is empty");
			return values[_front];
		}

		reference back() {
			assert(!empty() && "queue::back: queue is empty");
			return values[(_front + _size - 1) % _capacity];
		}

		const_reference back() const {
			assert(!empty() && "queue::back: queue is empty");
			return values[(_front + _size - 1) % _capacity];
		}

		bool empty() const { return _size == 0; }
		size_type size() const { return _size; }
		size_type capacity() const { return _capacity; }

		void reserve(size_t new_capacity) {
			if (new_capacity <= _capacity) return;
			T* new_values = static_cast<T*>(alloc->allocate(new_capacity * sizeof(T), alignof(T)));
			for (size_t i = 0; i < _size; ++i) {
				size_t old_idx = (_front + i) % _capacity;
				::new(new_values + i) T(std::move_if_noexcept(values[old_idx]));
				values[old_idx].~T();
			}
			if (values) alloc->free(values);
			values = new_values;
			_capacity = new_capacity;
			_front = 0;
		}

		void push(const T& value) {
			if (_size == _capacity) {
				reserve(_capacity == 0 ? 4 : 2 * _capacity);
			}
			size_t back_idx = (_front + _size) % _capacity;
			::new(values + back_idx) T(value);
			++_size;
		}

		void push(T&& value) {
			if (_size == _capacity) {
				reserve(_capacity == 0 ? 4 : 2 * _capacity);
			}
			size_t back_idx = (_front + _size) % _capacity;
			::new(values + back_idx) T(std::move(value));
			++_size;
		}

		template<typename... Args>
		void emplace(Args&&... args) {
			if (_size == _capacity) {
				reserve(_capacity == 0 ? 4 : 2 * _capacity);
			}
			size_t back_idx = (_front + _size) % _capacity;
			::new(values + back_idx) T(std::forward<Args>(args)...);
			++_size;
		}

		void pop() {
			assert(!empty() && "queue::pop: queue is empty");
			values[_front].~T();
			_front = (_front + 1) % _capacity;
			--_size;
			if (_size > 0 && _size < _capacity / 4 && _capacity > 8) {
				compact();
			}
		}

		void clear() {
			for (size_t i = 0; i < _size; ++i) {
				size_t idx = (_front + i) % _capacity;
				values[idx].~T();
			}
			_size = 0;
			_front = 0;
		}

		void swap(queue& other) noexcept {
			pstd::swap(alloc, other.alloc);
			pstd::swap(values, other.values);
			pstd::swap(_capacity, other._capacity);
			pstd::swap(_size, other._size);
			pstd::swap(_front, other._front);
		}

	private:
		void compact() {
			if (_size == 0) return;
			T* new_values = static_cast<T*>(alloc->allocate(_size * sizeof(T), alignof(T)));
			for (size_t i = 0; i < _size; ++i) {
				size_t old_idx = (_front + i) % _capacity;
				::new(new_values + i) T(std::move_if_noexcept(values[old_idx]));
				values[old_idx].~T();
			}
			alloc->free(values);
			values = new_values;
			_capacity = _size;
			_front = 0;
		}

		Allocator* alloc;
		T* values;
		size_t _capacity;
		size_t _size;
		size_t _front;
	};

	template<typename T, typename Allocator = HeapAllocator>
	class stack {
	public:
		using value_type = T;
		using allocator_type = Allocator;
		using size_type = std::size_t;
		using reference = T&;
		using const_reference = const T&;

		stack() : alloc(&Allocator::Get()), values(nullptr), _capacity(0), _size(0) {}

		explicit stack(size_t reserve_size)
			: alloc(&Allocator::Get())
			, values(static_cast<T*>(alloc->allocate(reserve_size * sizeof(T), alignof(T))))
			, _capacity(reserve_size)
			, _size(0) {}

		stack(const stack& other)
			: alloc(&Allocator::Get())
			, values(static_cast<T*>(alloc->allocate(other._capacity * sizeof(T), alignof(T))))
			, _capacity(other._capacity)
			, _size(other._size) {
			for (size_t i = 0; i < _size; ++i) {
				::new(values + i) T(other.values[i]);
			}
		}

		stack& operator=(const stack& other) {
			if (this == &other) return *this;
			stack tmp(other);
			pstd::swap(*this, tmp);
			return *this;
		}

		stack(stack&& other) noexcept
			: alloc(other.alloc)
			, values(other.values)
			, _capacity(other._capacity)
			, _size(other._size) {
			other.values = nullptr;
			other._capacity = 0;
			other._size = 0;
		}

		stack& operator=(stack&& other) noexcept {
			if (this == &other) return *this;
			clear();
			if (values) alloc->free(values);
			alloc = other.alloc;
			values = other.values;
			_capacity = other._capacity;
			_size = other._size;
			other.values = nullptr;
			other._capacity = 0;
			other._size = 0;
			return *this;
		}

		~stack() {
			clear();
			if (values) {
				alloc->free(values);
			}
		}

		reference top() {
			assert(!empty() && "stack::top: stack is empty");
			return values[_size - 1];
		}

		const_reference top() const {
			assert(!empty() && "stack::top: stack is empty");
			return values[_size - 1];
		}

		bool empty() const { return _size == 0; }
		size_type size() const { return _size; }
		size_type capacity() const { return _capacity; }

		void reserve(size_t new_capacity) {
			if (new_capacity <= _capacity) return;
			T* new_values = static_cast<T*>(alloc->allocate(new_capacity * sizeof(T), alignof(T)));
			for (size_t i = 0; i < _size; ++i) {
				::new(new_values + i) T(std::move_if_noexcept(values[i]));
				values[i].~T();
			}
			if (values) alloc->free(values);
			values = new_values;
			_capacity = new_capacity;
		}

		void push(const T& value) {
			if (_size == _capacity) {
				reserve(_capacity == 0 ? 4 : 2 * _capacity);
			}
			::new(values + _size) T(value);
			++_size;
		}

		void push(T&& value) {
			if (_size == _capacity) {
				reserve(_capacity == 0 ? 4 : 2 * _capacity);
			}
			::new(values + _size) T(std::move(value));
			++_size;
		}

		template<typename... Args>
		void emplace(Args&&... args) {
			if (_size == _capacity) {
				reserve(_capacity == 0 ? 4 : 2 * _capacity);
			}
			::new(values + _size) T(std::forward<Args>(args)...);
			++_size;
		}

		void pop() {
			assert(!empty() && "stack::pop: stack is empty");
			--_size;
			values[_size].~T();
		}

		void clear() {
			for (size_t i = 0; i < _size; ++i) {
				values[i].~T();
			}
			_size = 0;
		}

		void swap(stack& other) noexcept {
			pstd::swap(alloc, other.alloc);
			pstd::swap(values, other.values);
			pstd::swap(_capacity, other._capacity);
			pstd::swap(_size, other._size);
		}

	private:
		Allocator* alloc;
		T* values;
		size_t _capacity;
		size_t _size;
	};

	template<typename T>
	class optional {
	public:
		using value_type = T;

		optional() = default;

		optional(const T& v) : set(true) { new(ptr())T(v); }

		optional(T&& v) : set(true) { new(ptr())T(std::move(v)); }

		optional(const optional& v) : set(v.has_value()) {
			if (v.has_value())
				new(ptr())T(v.value());
		}

		optional(optional&& v) : set(v.has_value()) {
			if (v.has_value()) {
				new(ptr())T(std::move(v.value()));
				v.reset();
			}
		}

		optional& operator=(const T& v) {
			if (has_value() && &value() == &v) return *this;
			reset();
			new(ptr())T(v);
			set = true;
			return *this;
		}

		optional& operator=(T&& v) noexcept(std::is_nothrow_move_constructible_v<T>) {
			reset();
			new(ptr()) T(std::move(v));
			set = true;
			return *this;
		}

		optional& operator=(const optional& v) {
			reset();
			if (v.has_value()) {
				new(ptr())T(v.value());
				set = true;
			}
			return *this;
		}

		optional& operator=(optional&& v) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>) {
			reset();
			if (v.has_value()) {
				new(ptr()) T(std::move(v.value()));
				set = true;
				v.reset();
			}
			return *this;
		}

		bool operator==(const optional& v) const {
			if (has_value() != v.has_value())
				return false;
			if (!has_value())
				return true;
			return value() == v.value();
		}

		bool operator==(const T& v) const {
			if (has_value())
				return value() == v;
			return false;
		}

		bool operator==(T&& v) const {
			if (has_value())
				return value() == v;
			return false;
		}

		~optional() { reset(); }

		explicit operator bool() const { return set; }

		T value_or(const T& alt) const { return set ? value() : alt; }

		T& value() {
			assert(set && "bad_optional_access");
			return *ptr();
		}

		const T& value() const {
			assert(set && "bad_optional_access");
			return *ptr();
		}

		void reset() {
			if (set) {
				value().~T();
				set = false;
			}
		}

		bool has_value() const { return set; }

	private:
#ifdef __NVCC__
		T* ptr() { return reinterpret_cast<T*>(&valueStorage); }
		const T* ptr() const { return reinterpret_cast<const T*>(&valueStorage); }
#else
		T* ptr() { return std::launder(reinterpret_cast<T*>(&valueStorage)); }
		const T* ptr() const {
			return std::launder(reinterpret_cast<const T*>(&valueStorage));
		}
#endif

		bool set = false;
		std::aligned_storage_t<sizeof(T), alignof(T)> valueStorage;
	};

	template<typename T>
	inline std::ostream& operator<<(std::ostream& os, const optional<T>& opt) {
		if (opt.has_value()) {
			return os << "[ pstd::optional<" << typeid(T).name() <<
				"> set: true " << "value: " << opt.value() << " ]";
		}
		else {
			return os << "[ pstd::optional<" << typeid(T).name() <<
				"> set: false " << "value: n/a ]";
		}
	}

	namespace span_internal {
		template<typename C>
		inline constexpr auto GetDataImpl(C& c, char) noexcept -> decltype(c.data()) {
			return c.data();
		}

		template<typename C>
		inline constexpr auto GetData(C& c) -> decltype(GetDataImpl(c, 0)) {
			return GetDataImpl(c, 0);
		}

		template<typename T>
		using HasSize = std::is_integral<std::decay_t<decltype(std::declval<T&>().size())>>;

		template <typename From, typename To>
		using HasData =
			std::is_convertible<std::decay_t<decltype(GetData(std::declval<From&>()))>*,
			To* const*>;
	}

	inline constexpr std::size_t dynamic_extent = -1;

	template <typename T>
	class span {
	public:
		template<typename C>
		using EnableIfConvertibleFrom = typename std::enable_if_t<span_internal::HasData<C, T>::value && span_internal::HasSize<C>::value>;

		template<typename U>
		using EnableIfConstView = typename std::enable_if_t<std::is_const_v<T>, U>;

		template<typename U>
		using EnableIfMutableView = typename std::enable_if_t<!std::is_const_v<T>, U>;

		using value_type = typename std::remove_cv_t<T>;
		using iterator = T*;
		using const_iterator = const T*;

		span() : ptr(nullptr), n(0) {}
		span(T* ptr, size_t n) : ptr(ptr), n(n) {}

		template <size_t N>
		span(T(&a)[N]) : span(a, N) {}

		span(std::initializer_list<value_type> v) : span(v.begin(), v.size()) {}

		template<typename V, typename X = EnableIfConvertibleFrom<V>, typename Y = EnableIfMutableView<V>>
		explicit span(V& v) noexcept : span(v.data(), v.size()) {}

		iterator begin() { return ptr; }
		iterator end() { return ptr + n; }
		const_iterator begin() const { return ptr; }
		const_iterator end() const { return ptr + n; }

		T& operator[](size_t i) {
			assert(i < size() && "span index out of range");
			return ptr[i];
		}

		const T& operator[](size_t i) const {
			assert(i < size() && "span index out of range");
			return ptr[i];
		}

		size_t size() const { return n; }
		bool empty() const { return size() == 0; }
		T* data() { return ptr; }
		const T* data() const { return ptr; }

		T& front() { return ptr[0]; }
		T& back() { return ptr[n - 1]; }
		const T& front() const { return ptr[0]; }
		const T& back() const { return ptr[n - 1]; }

		void remove_prefix(size_t count) {
			ptr += count;
			n -= count;
		}

		void remove_suffix(size_t count) {
			n -= count;
		}

		span subspan(size_t pos, size_t count = dynamic_extent) const {
			size_t np = count < (size() - pos) ? count : (size() - pos);
			return span(ptr + pos, np);
		}

	private:
		T* ptr;
		size_t n;
	};

	template<int&...ExplicitArgumentBarrier, typename T>
	inline constexpr span<T> MakeSpan(T* ptr, size_t size) noexcept {
		return span<T>(ptr, size);
	}

	template<int&...ExplicitArgumantBarrier, typename T>
	inline span<T> MakeSpan(T* begin, T* end) noexcept {
		return span<T>(begin, end - begin);
	}

	template <int&...ExplicitArgumentBarrier, typename T>
	inline span<T> MakeSpan(std::vector<T>& v) noexcept {
		return span<T>(v.data(), v.size());
	}

	template <int&...ExplicitArgumentBarrier, typename C>
	inline constexpr auto MakeSpan(C& c) noexcept -> decltype(MakeSpan(span_internal::GetData(c), c.size())) {
		return MakeSpan(span_internal::GetData(c), c.size());
	}

	template <int&...ExplicitArgumentBarrier, typename T, size_t N>
	inline constexpr span<T> MakeSpan(T(&array)[N]) noexcept {
		return span<T>(array, N);
	}

	template <int&...ExplicitArgumentBarrier, typename T>
	inline constexpr span<const T> MakeConstSpan(T* ptr, size_t size) noexcept {
		return span<const T>(ptr, size);
	}

	template <int&...ExplicitArgumentBarrier, typename T>
	inline span<const T> MakeConstSpan(T* begin, T* end) noexcept {
		return span<const T>(begin, end - begin);
	}

	template <int&...ExplicitArgumentBarrier, typename T>
	inline span<const T> MakeConstSpan(const std::vector<T>& v) noexcept {
		return span<const T>(v.data(), v.size());
	}

	template <int&...ExplicitArgumentBarrier, typename C>
	inline constexpr auto MakeConstSpan(const C& c) noexcept -> decltype(MakeSpan(c)) {
		return MakeSpan(c);
	}

	template <int&...ExplicitArgumentBarrier, typename T, size_t N>
	inline constexpr span<const T> MakeConstSpan(const T(&array)[N]) noexcept {
		return span<const T>(array, N);
	}

	template<typename It>
	class reverse_iterator {
	public:
		reverse_iterator() {}
		reverse_iterator(It it) : current(it) {}

		It base() const { return current; }

		auto& operator*() const {
			It tmp = current;
			--tmp;
			return *tmp;
		}

		reverse_iterator& operator++() {
			--current;
			return *this;
		}

		reverse_iterator operator++(int) {
			reverse_iterator tmp = *this;
			--current;
			return tmp;
		}

		reverse_iterator& operator--() {
			++current;
			return *this;
		}

		reverse_iterator operator--(int) {
			reverse_iterator tmp = *this;
			++current;
			return tmp;
		}

		bool operator==(const reverse_iterator& rhs) const {
			return current == rhs.current;
		}

		bool operator!=(const reverse_iterator& rhs) const {
			return current != rhs.current;
		}

	private:
		It current = nullptr;
	};

	template<typename T, typename Allocator = HeapAllocator>
	class vector {
	public:
		using value_type = T;
		using allocator_type = Allocator;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = T*;
		using const_pointer = const T*;
		using iterator = T*;
		using const_iterator = const T*;
		using reverse_iterator = pstd::reverse_iterator<iterator>;
		using const_reverse_iterator = pstd::reverse_iterator<const_iterator>;

		vector() : alloc(&Allocator::Get()) {}

		explicit vector(size_t count): 
			alloc(&Allocator::Get())
			, values(count > 0 ? static_cast<T*>(alloc->allocate(count * sizeof(T), alignof(T))) : nullptr)
			, _capacity(count)
			, _size(count) {
			pstd::uninitialized_default_construct_n(values, count);
		}

		vector(size_t count, const T& v) : alloc(&Allocator::Get()), values(static_cast<T*>(alloc->allocate(count * sizeof(T), alignof(T)))), _capacity(count), _size(count) {
			pstd::uninitialized_fill_n(values, count, v);
		}

		vector(std::initializer_list<T> list) : alloc(&Allocator::Get()), values(static_cast<T*>(alloc->allocate(list.size() * sizeof(T), alignof(T)))), _capacity(list.size()), _size(list.size()) {
			pstd::uninitialized_copy(list.begin(), list.end(), values);
		}

		template<typename InputIt>
		vector(InputIt begin, InputIt end) : alloc(&Allocator::Get()), values(static_cast<T*>(alloc->allocate(static_cast<size_t>(end - begin) * sizeof(T), alignof(T)))), _capacity(static_cast<size_t>(end - begin)), _size(static_cast<size_t>(end - begin)) {
			pstd::uninitialized_copy(begin, end, values);
		}

		vector(const vector& vec) : alloc(&Allocator::Get()), values(static_cast<T*>(alloc->allocate(vec.size() * sizeof(T), alignof(T)))), _capacity(vec.size()), _size(vec.size()) {
			pstd::uninitialized_copy(vec.begin(), vec.end(), values);
		}

		vector& operator=(const vector& vec) {
			if (this == &vec) return *this;
			vector tmp(vec);
			pstd::swap(*this, tmp);
			return *this;
		}

		template<typename FromAllocator>
		vector(const vector<T, FromAllocator>& vec) : alloc(&Allocator::Get()), values(static_cast<T*>(alloc->allocate(vec.size() * sizeof(T), alignof(T)))), _capacity(vec.size()), _size(vec.size()) {
			pstd::uninitialized_copy(vec.begin(), vec.end(), values);
		}

		template<typename FromAllocator>
		vector& operator=(const vector<T, FromAllocator>& vec) {
			if (this == &vec) return *this;
			alloc = &Allocator::Get();
			clear();
			reserve(vec.size());
			pstd::uninitialized_copy(vec.cbegin(), vec.cend(), values);
			_size = vec.size();
			return *this;
		}

		vector(vector&& vec) noexcept : alloc(vec.alloc), values(vec.values), _capacity(vec.capacity()), _size(vec.size()) {
			vec.values = nullptr;
			vec._size = 0;
			vec._capacity = 0;
		}

		vector& operator=(vector&& vec) noexcept {
			if (this == &vec) return *this;
			alloc = &Allocator::Get();
			clear();
			pstd::swap(values, vec.values);
			pstd::swap(_size, vec._size);
			pstd::swap(_capacity, vec._capacity);
			return *this;
		}

		vector& operator=(std::initializer_list<T> list) {
			clear();
			reserve(list.size());
			pstd::uninitialized_copy(list.begin(), list.end(), values);
			_size = list.size();
			return *this;
		}

		void assign(size_type count, const T& value) {
			clear();
			reserve(count);
			for (; count; --count)
				push_back(value);
		}

		template<typename InputIt>
		void assign(InputIt first, InputIt last) {
			size_t new_size = static_cast<size_t>(last - first);
			T* new_values = static_cast<T*>(alloc->allocate(new_size * sizeof(T), alignof(T)));
			size_t new_capacity = new_size;
			pstd::uninitialized_copy(first, last, new_values);
			clear();
			values = new_values;
			_size = new_size;
			_capacity = new_capacity;
		}

		void assign(std::initializer_list<T> list) { assign(list.begin(), list.end()); }

		~vector() {
			clear();
			alloc->free(values);
			values = nullptr;
		}

		iterator begin() { return values; }
		iterator end() { return values + _size; }
		const_iterator begin() const { return values; }
		const_iterator end() const { return values + _size; }
		const_iterator cbegin() const { return values; }
		const_iterator cend() const { return values + _size; }

		reverse_iterator rbegin() { return reverse_iterator(end()); }
		reverse_iterator rend() { return reverse_iterator(begin()); }
		const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
		const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

		allocator_type& get_allocator() const { return *alloc; }

		size_t size() const { return _size; }
		bool empty() const { return size() == 0; }
		size_t capacity() const { return _capacity; }

		void reserve(size_t n) {
			if (_capacity >= n) return;
			T* new_data = static_cast<T*>(alloc->allocate(n * sizeof(T), alignof(T)));
			for (size_t i = 0; i < _size; ++i)
				::new(new_data + i) T(std::move_if_noexcept(values[i]));
			pstd::destroy_n(values, _size);
			alloc->free(values);
			values = new_data;
			_capacity = n;
		}

		reference operator[](size_type index) {
			assert(index < _size && "vector:: index must smaller than size");
			return values[index];
		}

		const_reference operator[](size_type index) const {
			assert(index < _size && "vector:: index must smaller than size");
			return values[index];
		}

		reference front() { return values[0]; }
		const_reference front() const { return values[0]; }
		reference back() { return values[_size - 1]; }
		const_reference back() const { return values[_size - 1]; }
		pointer data() { return values; }
		const_pointer data() const { return values; }

		void clear() {
			pstd::destroy_n(values, _size);
			_size = 0;
		}

		template<typename... Args>
		void emplace_back(Args&&... args) {
			if (_size == _capacity)
				reserve(_capacity == 0 ? 4 : 2 * _capacity);
			::new(values + _size) T(std::forward<Args>(args)...);
			++_size;
		}

		void push_back(const T& v) {
			if (_size == _capacity)
				reserve(_capacity == 0 ? 4 : 2 * _capacity);
			::new(values + _size) T(v);
			++_size;
		}

		void push_back(T&& v) {
			if (_size == _capacity)
				reserve(_capacity == 0 ? 4 : 2 * _capacity);
			::new(values + _size) T(std::move(v));
			++_size;
		}

		void pop_back() {
			if (empty()) return;
			pstd::destroy(values + _size - 1);
			--_size;
		}

		void resize(size_type n) {
			if (n < _size) {
				pstd::destroy(values + n, values + _size);
				if (n == 0) {
					if (values) {
						alloc->free(values);
						values = nullptr;
						_capacity = 0;
					}
				}
			} else if (n > _size) {
				reserve(n);
				pstd::uninitialized_default_construct(values + _size, values + n);
			}
			_size = n;
		}

	private:
		Allocator* alloc;
		T* values = nullptr;
		size_t _capacity = 0, _size = 0;
	};

	template<typename... Ts>
	struct tuple;

	template<>
	struct tuple<> {
		template<size_t>
		using type = void;
	};

	template<typename T, typename... Ts>
	struct tuple<T, Ts...> : tuple<Ts...> {
		using Base = tuple<Ts...>;

		tuple() : Base(), value() {}

		tuple(const tuple& other) : Base((const Base&)other), value(other.value) {}

		tuple(tuple&& other) noexcept : Base((Base&&)other), value(std::move(other.value)) {}

		tuple& operator=(const tuple& other) {
			if (this == &other) return *this;
			Base::operator=((const Base&)other);
			value = other.value;
			return *this;
		}

		tuple& operator=(tuple&& other) noexcept {
			if (this == &other) return *this;
			Base::operator=((Base&&)other);
			value = std::move(other.value);
			return *this;
		}

		tuple(const T& value, const Ts&... values) : Base(values...), value(value) {}

		tuple(T&& value, Ts&&... values) : Base(std::move(values)...), value(std::move(value)) {}

		T value;
	};

	template <typename... Ts>
	tuple(Ts&&...) -> tuple<std::decay_t<Ts>...>;

	template<size_t I, typename T, typename... Ts>
	auto& get(tuple<T, Ts...>& t) {
		if constexpr (I == 0)
			return t.value;
		else
			return get<I - 1>((tuple<Ts...>&)t);
	}

	template<size_t I, typename T, typename... Ts>
	const auto& get(const tuple<T, Ts...>& t) {
		if constexpr (I == 0)
			return t.value;
		else
			return get<I - 1>((const tuple<Ts...>&)t);
	}

	template<typename Req, typename T, typename... Ts>
	auto& get(tuple<T, Ts...>& t) {
		if constexpr (std::is_same_v<Req, T>)
			return t.value;
		else
			return get<Req>((tuple<Ts...>&)t);
	}

	template<typename Req, typename T, typename... Ts>
	const auto& get(const tuple<T, Ts...>& t) {
		if constexpr (std::is_same_v<Req, T>)
			return t.value;
		else
			return get<Req>((const tuple<Ts...>&)t);
	}

	template<typename T>
	struct complex {
		complex() : re(0), im(0) {}
		complex(T re) : re(re), im(0) {}
		complex(T re, T im) : re(re), im(im) {}

		complex operator-() const { return { -re, -im }; }
		complex operator+(const complex& z) const { return { re + z.re, im + z.im }; }
		complex operator-(const complex& z) const { return { re - z.re, im - z.im }; }

		complex operator*(const complex& z) const {
			return { re * z.re - im * z.im, re * z.im + im * z.re };
		}

		complex operator/(const complex& z) const {
			T scale = 1 / (z.re * z.re + z.im * z.im);
			return { (re * z.re + im * z.im) * scale, (im * z.re - re * z.im) * scale };
		}

		bool operator==(const complex& z) const {
			return (re == z.re) && (im == z.im);
		}

		bool operator!=(const complex& z) const {
			return !(*this == z);
		}

		T norm() const { return re * re + im * im; }

		friend complex operator+(T value, complex z) { return complex(value) + z; }
		friend complex operator-(T value, complex z) { return complex(value) - z; }
		friend complex operator*(T value, complex z) { return complex(value) * z; }
		friend complex operator/(T value, complex z) { return complex(value) / z; }

		T re, im;
	};

	inline float sqrt(float f) { return ::sqrt(f); }
	inline double sqrt(double f) { return ::sqrt(f); }
	inline float abs(float f) { return ::fabsf(f); }
	inline double abs(double f) { return ::fabs(f); }

	inline float copysign(float mag, float sign) { return ::copysignf(mag, sign); }
	inline double copysign(double mag, double sign) { return ::copysign(mag, sign); }

	inline float floor(float arg) { return ::floorf(arg); }
	inline double floor(double arg) { return ::floor(arg); }

	inline float ceil(float arg) { return ::ceilf(arg); }
	inline double ceil(double arg) { return ::ceil(arg); }

	inline float round(float arg) { return ::roundf(arg); }
	inline double round(double arg) { return ::round(arg); }

	inline float fmod(float x, float y) { return ::fmodf(x, y); }
	inline double fmod(double x, double y) { return ::fmod(x, y); }

	template<typename T>
	T abs(const complex<T>& z) {
		return z.abs();
	}

	template<typename T>
	complex<T> sqrt(const complex<T>& z) {
		T n = pstd::abs(z);
		T t1 = pstd::sqrt(T(.5) * (n + pstd::abs(z.re)));
		T t2 = T(.5) * z.im / t1;

		if (n == 0)
			return 0;

		if (z.re >= 0)
			return { t1, t2 };
		else
			return { pstd::abs(t2), pstd::copysign(t1, z.im) };
	}

}  // namespace pstd


#endif // PBRT_UTIL_PSTD_H
