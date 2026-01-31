#ifndef PBRT_UTIL_MEMORY_H
#define PBRT_UTIL_MEMORY_H

#include <new>
#include <exception>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <type_traits>
#include "deps/tlsf/tlsf.h"
#include "base/base.h"

#include <iostream>

const size_t LINEAR_BLOCK_SIZE = 512 * 1024;
const size_t HEAP_BLOCK_SIZE = 1024 * 1024;


const float REALLOCATE_MEMORY_EXPAND_COEFFICIENT = 1.2;

namespace pstd {


	template<typename Derived>
	class Pallocator {
	public:
		Pallocator() = default;

		Pallocator(size_t total_size):total_size(total_size){}

		void* allocate(size_t size, size_t align = alignof(std::max_align_t)) {
			return derived().allocate_impl(size, align);
		}

		size_t get_total_size() const { return total_size; }
		size_t get_used() const { return used; }


		// For Linear Allocator reset ability
		template <typename U = Derived, std::enable_if_t<std::is_same_v<U, LinearAllocator>, int> = 0>
		void reset() {
			derived().reset_impl();
		}

		template <typename U = Derived, std::enable_if_t<std::is_same_v<U, LinearAllocator>, int> = 0>
		void reset_hard(){
			derived().reset_hard_impl();
		}

		// For Heap Allocator free ability
		template<typename U = Derived, std::enable_if_t<std::is_same_v<U, HeapAllocator>, int> = 0>
		void free(void* ptr) {
			derived().free_impl(ptr);
		}
		
	protected:
		void reallocate(size_t size) {
			derived().reallocate_impl(size);
		}

		Derived& derived() { return *static_cast<Derived*>(this); }
		const Derived& derived() const { return *static_cast<Derived*>(this); }

		size_t total_size = 0;
		size_t used = 0;
	};


	class LinearAllocator: public Pallocator<LinearAllocator> {
	public:
		// ----------------------Singleton----------------------
		static void Init(size_t head_size = LINEAR_BLOCK_SIZE) {
			assert(!singleton && "LinearAllocator has been initialized!");

			singleton = new LinearAllocator(head_size);
		}

		static LinearAllocator& Get() {
			if (!singleton) Init();
			return *singleton;
		}

		static void Shutdown() {
			if (!singleton) return;
			delete singleton;
			singleton = nullptr;
		}

		// -----------------------------------------------------

		void* allocate_impl(size_t size, size_t align){
			if (size == 0)return nullptr;

			// align must be 2^n;
			assert(align && ((align & (align - 1)) == 0));

			std::byte* ptr = align_up(reinterpret_cast<std::byte*>(current_ptr), align);
			size_t rest = static_cast<size_t>(current_block->back() - ptr);

			// reallocate
			if (rest < size) {
				reallocate(std::max(current_block->size, size));
				ptr = align_up(reinterpret_cast<std::byte*>(current_ptr), align);
				rest = static_cast<size_t>(current_block->back() - ptr);
			}

			std::byte* last_ptr = reinterpret_cast<std::byte*>(current_ptr);
			std::byte* end_ptr = ptr + size;

			current_ptr = end_ptr;
			
			size_t size_used = static_cast<size_t>(end_ptr - last_ptr);
			current_block->used += size_used;
			used += size_used;


			return ptr;
		}
		
		void reset_impl() { do_reset();}
		void reset_hard_impl() { do_reset(Reset_level::HARD); }

		void reallocate_impl(size_t size) {

			void* new_memory_resource;
			if (!current_block->next) {
				new_memory_resource = std::malloc(size + sizeof(LinearControlBlock));

				if (!new_memory_resource) throw std::bad_alloc{};
				total_size += size;

				auto new_block = reinterpret_cast<LinearControlBlock*>(new_memory_resource);
				current_block->next = new_block;
				current_block = new_block;
				current_block->size = size;
				current_block->next = nullptr;
				current_block->used = 0;

			}else{
				new_memory_resource = current_block->next;

				auto new_block = reinterpret_cast<LinearControlBlock*>(new_memory_resource);
				current_block = new_block;
				current_block->used = 0;
			}

			std::byte* start = current_block->start();
			current_ptr = align_up(start, alignof(std::max_align_t));
		}

		~LinearAllocator(){
			while (memory_resource) {
				auto current = memory_resource;
				memory_resource = reinterpret_cast<LinearControlBlock*>(memory_resource)->next;
				std::free(current);
			}
		}

		LinearAllocator(const LinearAllocator&) = delete;
		LinearAllocator& operator=(const LinearAllocator&) = delete;
		LinearAllocator(LinearAllocator&&) = delete;
		LinearAllocator& operator=(LinearAllocator&&) = delete;
		
		
	private:
		struct LinearControlBlock {
			size_t size;
			LinearControlBlock* next;
			size_t used;

			std::byte* start() {
				return reinterpret_cast<std::byte*>(this + 1);
			}
			std::byte* back() {
				return start() + size;
			}
		};

		explicit LinearAllocator(size_t total_size):Pallocator(total_size) {
			memory_resource = std::malloc(total_size + sizeof(LinearControlBlock));
			if (!memory_resource) throw std::bad_alloc{};
			
			current_block = reinterpret_cast<LinearControlBlock*>(memory_resource);
			current_block->size = total_size;
			current_block->next = nullptr;
			current_block->used = 0;

			current_ptr = align_up(current_block->start(), alignof(std::max_align_t));

		}

		enum class Reset_level { DEFAULT, HARD };
		void do_reset(Reset_level level = Reset_level::DEFAULT) {
			current_block = reinterpret_cast<LinearControlBlock*>(memory_resource);
			current_block->used = 0;
			used = 0;

			current_ptr = align_up(current_block->start(), alignof(std::max_align_t));

			if (level == Reset_level::HARD) {
				auto next = current_block->next;
				while (next) {
					void* current = next;
					next = next->next;
					std::free(current);
				}
				current_block->next = nullptr;
				total_size = current_block->size;

			}
		}

		std::byte* align_up(std::byte* p, size_t a) {
			auto v = reinterpret_cast<std::uintptr_t>(p);
			return reinterpret_cast<std::byte*>((v + a - 1) & ~(a - 1));
		}

		void* current_ptr = nullptr;
		LinearControlBlock* current_block = nullptr;
		void* memory_resource = nullptr;


		inline static LinearAllocator* singleton = nullptr;
	};


	class HeapAllocator : public Pallocator<HeapAllocator> {
	public:
		// ----------------------Singleton----------------------

		static void Init(size_t size = HEAP_BLOCK_SIZE) {
			assert(!singleton && "HeapAllocator has been initialized!");

			singleton = new HeapAllocator(size); 
		}

		static HeapAllocator& Get() {
			if (!singleton) Init(HEAP_BLOCK_SIZE);
			return *singleton;
		}

		static void Shutdown() {
			if (!singleton) return;
			delete singleton;
			singleton = nullptr;
		}

		// -----------------------------------------------------

		void* allocate_impl(size_t size, size_t align){
			size_t baseAlign = tlsf_align_size();
			void* ptr = nullptr;

			if (size > tlsf_block_size_max())throw std::bad_alloc{};

			while(!ptr) {
				if (align <= baseAlign) {
					ptr = tlsf_malloc(tlsf_handle, size);
				}
				else {
					ptr = tlsf_memalign(tlsf_handle, align, size);
				}

				if (ptr)
					break;
				else
					reallocate(std::max(size, HEAP_BLOCK_SIZE));
			}

			used += tlsf_block_size(ptr);

			return ptr;
		}

		void free_impl(void* ptr) {
			if (!ptr)return;
			used -= tlsf_block_size(ptr);
			tlsf_free(tlsf_handle, ptr);
		}

		void reallocate_impl(size_t size) {
			size = std::max(size, HEAP_BLOCK_SIZE);
			size = expand_reallocation_size_with_align(size);

			void* new_memory_resource = std::malloc(size);
			
			auto pool = tlsf_add_pool(tlsf_handle, new_memory_resource, size);
			assert(pool && "Failed to add new pool to TLSF");

			total_size += size;
			pool_list.emplace_back(new_memory_resource, pool);

		}

		~HeapAllocator() {
			if (tlsf_handle) {
				tlsf_destroy(tlsf_handle);
				tlsf_handle = nullptr;
			}

			for (const auto& pool_block : pool_list) {
				std::free(pool_block.mem);
			}
			pool_list.clear();
		}

	private:
		struct HeapControlBlock {
			HeapControlBlock(void* _mem, pool_t _handle):mem(_mem),handle(_handle){}
			void* mem;
			pool_t handle;
		};

		explicit HeapAllocator(size_t size) :Pallocator(size) {
			void* memory_resource = std::malloc(size);
			if (!memory_resource) throw std::bad_alloc{};
			tlsf_handle = tlsf_create_with_pool(memory_resource, size);
			assert(tlsf_handle && "failed to start TLSF");
			auto pool_handle = tlsf_get_pool(tlsf_handle);
			
			total_size = size;
			pool_list.emplace_back(memory_resource, pool_handle);

		}

		std::size_t expand_reallocation_size_with_align(size_t o_size) {
			double scaled_size_d = static_cast<double>(o_size) * REALLOCATE_MEMORY_EXPAND_COEFFICIENT;
			size_t scaled_size = static_cast<size_t>(scaled_size_d);

			size_t align = tlsf_align_size();
			size_t scaled_size_aligned = (scaled_size + align - 1) & ~(align - 1);
			return scaled_size_aligned;
		}

		tlsf_t tlsf_handle = nullptr;

		std::vector<HeapControlBlock> pool_list;

		inline static HeapAllocator* singleton = nullptr;
	};



	template<typename T, typename Allocator = HeapAllocator, typename... Args>
	T* pnew(Args&&... args) {
		void* mem = Allocator::Get().allocate(sizeof(T), alignof(T));
		if (!mem) throw std::bad_alloc{};
		return new(mem) T(std::forward<Args>(args)...);
	}

	template <typename T, typename Allocator=HeapAllocator>
	std::enable_if_t<!std::is_same_v<Allocator, LinearAllocator>, void>
	pdelete(T* ptr) {
		if (!ptr) return;
		ptr->~T();
		Allocator::Get().free(ptr);
	}


	template <typename InputIt, typename OutputIt>
	OutputIt copy(InputIt first, InputIt last, OutputIt d_first) {
		for (; first != last; ++first, ++d_first)
			*d_first = *first;
		return d_first;
	}

	template <typename ForwardIt, typename U>
	void fill(ForwardIt first, ForwardIt last, const U& value) {
		using T = std::decay_t<decltype(*std::declval<ForwardIt>())>;
		for (; first != last; ++first) {
			*first = value;
		}
	}


	template<typename ForwardIt>
	ForwardIt uninitialized_default_construct(ForwardIt first, ForwardIt last) {
		using T = std::decay_t<decltype(*std::declval<ForwardIt>())>;
		for (; first != last; ++first) {
			::new(static_cast<void*>(&*first)) T{};
		}
		return first;
	}

	template<typename ForwardIt, typename Size, typename T = std::decay_t<decltype(*std::declval<ForwardIt>())> >
	ForwardIt uninitialized_default_construct_n(ForwardIt first, Size n) {
		for (; n; ++first, --n) {
			::new(static_cast<void*>(&*first)) T {};
		}
		return first;
	}

	template<typename InputIt, typename ForwardIt>
	ForwardIt uninitialized_copy(InputIt first, InputIt last, ForwardIt d_first) {
		using T = std::decay_t<decltype(*std::declval<ForwardIt>())>;
		for (; first != last; ++first, ++d_first) {
			::new(static_cast<void*>(&*d_first)) T(*first);
		}
		return d_first;
	}

	template<typename ForwardIt, typename Size, typename U>
	ForwardIt uninitialized_fill_n(ForwardIt first, Size n, const U& value) {
		using T = std::decay_t<decltype(*std::declval<ForwardIt>())>;
		for (; n; ++first, --n) {
			::new(static_cast<void*>(&*first)) T(value);
		}
		return first;
	}

	template<typename ForwardIt>
	void destroy(ForwardIt it) {
		using T = std::decay_t<decltype(*std::declval<ForwardIt>())>;
		(*it).~T();
	}
	template<typename ForwardIt>
	void destroy(ForwardIt begin, ForwardIt end) {
		using T = std::decay_t<decltype(*std::declval<ForwardIt>())>;
		for (; begin != end; ++begin)
			(&*begin)->~T();
	}

	template<typename ForwardIt, typename Size>
	void destroy_n(ForwardIt first, Size n) {
		using T = std::decay_t<decltype(*std::declval<ForwardIt>())>;
		for (; n; ++first, --n) {
			(&*first)->~T();
		}
	}


} // namespace pstd

#endif // !PBRT_UTIL_MEMORY_H
