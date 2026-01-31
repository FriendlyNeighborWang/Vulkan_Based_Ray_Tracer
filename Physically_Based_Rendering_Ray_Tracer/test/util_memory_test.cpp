#ifdef DEBUG
#include <cstdio>
#include <vector>
#include <algorithm>
#include <type_traits>
#include <cstdint>

#include "util/memory.h"
using namespace pstd;

// 单例引用（和你现有代码一致）
static LinearAllocator& linear_allocator = LinearAllocator::Get();
static HeapAllocator& heap_allocator = HeapAllocator::Get();

// ---------------------------------
// 小工具
// ---------------------------------
static inline bool is_aligned(const void* p, size_t a) {
    return (reinterpret_cast<std::uintptr_t>(p) % a) == 0;
}

static void banner(const char* name) {
    std::printf("== %s ==\n", name);
}

// ---------------------------------
// LinearAllocator 测试 (原有基本保留)
// ---------------------------------

static void test_basic_and_alignment_linear() {
    banner("linear_basic_and_alignment");

    linear_allocator.reset_hard(); // 确保从首块开始
    std::vector<void*> addrs;
    for (int i = 1; i <= 16; ++i) {
        const size_t sz = size_t(i) * 11;           // 11,22,33...
        const size_t align = (i % 3 == 0) ? 64 : 16;   // 交替 16/64
        void* p = linear_allocator.allocate(sz, align);
        assert(p && "allocate returned nullptr");
        assert(is_aligned(p, align) && "alignment failed");
        addrs.push_back(p);
        if (i > 1) {
            assert(addrs[i - 1] > addrs[i - 2] &&
                "bump pointer should be monotonic within a block");
        }
    }

    // 默认 reset 回到首块开头附近（地址应回退）
    void* last_before = addrs.back();
    linear_allocator.reset();
    void* p_after = linear_allocator.allocate(32, 16);
    assert(p_after && is_aligned(p_after, 16));
    assert(p_after < last_before &&
        "DEFAULT reset should reuse head block");

    std::puts("[OK] linear_basic_and_alignment");
}

static void test_cross_block_grow_linear() {
    banner("linear_cross_block_grow");

    linear_allocator.reset_hard();

    // 尽量用满首块（保留对齐余量）
    std::vector<void*> head_addrs;
    for (;;) {
        void* p = linear_allocator.allocate(256, 32); // 256B 一次
        if (!p) break; // 理论上不应为 nullptr，这里仅是兜底
        head_addrs.push_back(p);
        // 当下次申请一个较大块时，一定会跨到第二块
        if (head_addrs.size() > 8) break;
    }
    void* last_head = head_addrs.back();

    void* second_begin = linear_allocator.allocate(8192, 64); // 8KB，强制跨块
    assert(second_begin && is_aligned(second_begin, 64));
    assert(second_begin > last_head &&
        "second block address should be greater than last of head");

    std::puts("[OK] linear_cross_block_grow");
}

static void test_default_reset_reuse_next_linear() {
    banner("linear_default_reset_reuse_next");

    linear_allocator.reset_hard();

    // 先跨一次块，记下第二块的第一笔分配地址
    (void)linear_allocator.allocate(256 * 8, 32);     // 让头块消耗一些
    void* second_1 = linear_allocator.allocate(8192, 64);
    assert(second_1 && is_aligned(second_1, 64));

    // 默认 reset，不释放第二块
    linear_allocator.reset();

    // 再次跨块
    (void)linear_allocator.allocate(256 * 8, 32);
    void* second_2 = linear_allocator.allocate(8192, 64);
    assert(second_2 && is_aligned(second_2, 64));

    // 线性分配器复用 next block，会把 current_ptr 重新对齐到块头，
    // 同样大小的请求通常会返回同一地址
    assert(second_2 == second_1 &&
        "DEFAULT reset should reuse the same next block");

    std::puts("[OK] linear_default_reset_reuse_next");
}

static void test_hard_reset_release_tail_linear() {
    banner("linear_hard_reset_release_tail");

    linear_allocator.reset_hard();
    void* head_first = linear_allocator.allocate(64, 16);

    // 跨到第二块
    (void)linear_allocator.allocate(1 << 20, 64); // 1MB，必跨块
    void* in_second = linear_allocator.allocate(128, 32);
    assert(in_second > head_first);

    // 硬重置：释放尾块，仅留首块
    linear_allocator.reset_hard();

    // 再次分配小块，应回到首块区域
    void* head_again = linear_allocator.allocate(64, 16);
    assert(head_again && head_again <= in_second &&
        "HARD reset should drop back to head block");

    std::puts("[OK] linear_hard_reset_release_tail");
}

static void test_single_huge_alloc_linear() {
    banner("linear_single_huge_alloc");

    linear_allocator.reset_hard();

    void* small = linear_allocator.allocate(128, 16);
    (void)small;

    void* huge = linear_allocator.allocate(8 << 20, 64); // 8MB
    assert(huge && is_aligned(huge, 64));

    std::puts("[OK] linear_single_huge_alloc");
}

static void test_zero_size_linear() {
    banner("linear_zero_size");
    linear_allocator.reset_hard();
    void* p = linear_allocator.allocate(0, 16);
    assert(p == nullptr);
    std::puts("[OK] linear_zero_size");
}

static void test_many_default_resets_no_growth_linear() {
    banner("linear_many_default_resets_no_growth");

    linear_allocator.reset_hard();
    void* first = linear_allocator.allocate(64, 16);
    assert(first);

    for (int i = 0; i < 64; ++i) {
        linear_allocator.reset(); // DEFAULT
        void* p = linear_allocator.allocate(64, 16);
        assert(p && is_aligned(p, 16));
        assert(p <= first &&
            "should continue allocating from head block after DEFAULT reset");
    }

    std::puts("[OK] linear_many_default_resets_no_growth");
}

// ---------------------------------
// HeapAllocator 测试
// ---------------------------------

// 1) 基础分配 / 对齐 / used 统计
static void test_heap_basic_and_alignment() {
    banner("heap_basic_and_alignment");

    // 基线
    size_t used0 = heap_allocator.get_used();
    size_t total0 = heap_allocator.get_total_size();

    // 分配一块 256B，16 字节对齐
    void* p1 = heap_allocator.allocate(256, 16);
    assert(p1 && "heap allocate p1 returned nullptr");
    assert(is_aligned(p1, 16) && "heap alignment 16 failed");

    size_t used1 = heap_allocator.get_used();
    assert(used1 >= used0 + 256 &&
        "used should increase at least by ~requested size (tlsf may round up)");

    // 再分配一块 1KB，64字节对齐，走 memalign 路径
    void* p2 = heap_allocator.allocate(1024, 64);
    assert(p2 && "heap allocate p2 failed");
    assert(is_aligned(p2, 64) && "heap alignment 64 failed");

    size_t used2 = heap_allocator.get_used();
    assert(used2 >= used1 + 1024 &&
        "used should keep increasing after second alloc");

    // 释放 p1
    heap_allocator.free(p1);
    size_t used3 = heap_allocator.get_used();
    assert(used3 < used2 &&
        "used should drop after freeing first block");

    // 释放 p2
    heap_allocator.free(p2);
    size_t used4 = heap_allocator.get_used();
    assert(used4 <= used1 &&
        "after freeing both, used should fall back near baseline");

    // 小分配不应触发扩容
    size_t total1 = heap_allocator.get_total_size();
    assert(total1 == total0 &&
        "total_size should not change for small allocs");

    std::puts("[OK] heap_basic_and_alignment");
}

// 2) 自动扩容：当请求超出当前池容量时，应调用 reallocate_impl()→增加 TLSF pool
static void test_heap_auto_grow() {
    banner("heap_auto_grow");

    size_t used_before = heap_allocator.get_used();
    size_t total_before = heap_allocator.get_total_size();

    // 构造一个非常大的分配请求，逼迫 heap_allocator 内部扩容
    size_t bigRequest = total_before * 2 + 1024;
    if (bigRequest < (2u << 20)) { // 至少 ~2MB
        bigRequest = (2u << 20);
    }

    void* big = heap_allocator.allocate(bigRequest, 64);
    assert(big && "heap big allocate failed");
    assert(is_aligned(big, 64) && "heap big allocate alignment failed");

    size_t total_after = heap_allocator.get_total_size();
    size_t used_after = heap_allocator.get_used();

    // total_size 应扩大
    assert(total_after > total_before &&
        "total_size should have grown after large allocation (new pool added)");

    // used 应显著上升
    assert(used_after >= used_before + bigRequest &&
        "used should reflect large allocation");

    // 释放这个大块
    heap_allocator.free(big);
    size_t used_after_free = heap_allocator.get_used();
    assert(used_after_free < used_after &&
        "free(big) should reduce used again");

    std::puts("[OK] heap_auto_grow");
}

// 3) 碎片/释放顺序：按 b→a→c 释放，used 应逐步下降
static void test_heap_fragment_and_free() {
    banner("heap_fragment_and_free");

    void* a = heap_allocator.allocate(4000, 16); // 4 KB
    void* b = heap_allocator.allocate(16000, 32); // 16 KB
    void* c = heap_allocator.allocate(8000, 64); // 8 KB

    assert(a && b && c);
    assert(is_aligned(a, 16));
    assert(is_aligned(b, 32));
    assert(is_aligned(c, 64));

    size_t u_abc = heap_allocator.get_used();

    // free b 中间那块
    heap_allocator.free(b);
    size_t u_ac = heap_allocator.get_used();
    assert(u_ac < u_abc &&
        "free(b) should lower used but not zero it");

    // free a
    heap_allocator.free(a);
    size_t u_c = heap_allocator.get_used();
    assert(u_c < u_ac &&
        "free(a) should lower used further");

    // free c
    heap_allocator.free(c);
    size_t u_0 = heap_allocator.get_used();
    assert(u_0 <= u_c &&
        "after freeing all, used should not increase");

    std::puts("[OK] heap_fragment_and_free");
}

// ---------------------------------
// CRTP“多态”演示：同一套模板对两种 allocator 做不同事
// ---------------------------------

// 这个模板体现的是“编译期多态”：
// - 对任意 AllocT（LinearAllocator 或 HeapAllocator）我们都能做一部分公共操作：allocate()
// - 然后用 if constexpr 按类型分支：
//   * LinearAllocator: 我们测试 reset()
//   * HeapAllocator:   我们测试 free()
template <typename AllocT>
static void test_polymorphic_smoke(AllocT& alloc, const char* tag) {
    banner(tag);

    // 通用：分配一小块
    void* p = alloc.allocate(128, 16);
    assert(p && is_aligned(p, 16));

    // 如果是 LinearAllocator，可以调用 reset()（SFINAE 提供的成员）
    if constexpr (std::is_same_v<AllocT, LinearAllocator>) {
        size_t before_used = alloc.get_used();
        alloc.reset(); // 调用的是 Pallocator<LinearAllocator>::reset() -> reset_impl()
        size_t after_used = alloc.get_used();

        // reset() 会把线性分配器的 current_block 回到头部并清 used
        assert(after_used <= before_used &&
            "linear reset() should not increase used");

        // 如果是 HeapAllocator，可以调用 free()（SFINAE 提供的成员）
    }
    else if constexpr (std::is_same_v<AllocT, HeapAllocator>) {
        size_t before_used = alloc.get_used();
        alloc.free(p); // Pallocator<HeapAllocator>::free() -> free_impl()
        size_t after_used = alloc.get_used();

        assert(after_used < before_used &&
            "heap free() should reduce used after freeing block");
    }

    std::puts("[OK] polymorphic_smoke");
}


struct TestObject {
    int x;
    static inline int live_count = 0;

    TestObject() : x(0) {
        ++live_count;
        std::cout << "TestObject()  x=" << x << "\n";
    }

    explicit TestObject(int v) : x(v) {
        ++live_count;
        std::cout << "TestObject(int)  x=" << x << "\n";
    }

    TestObject(const TestObject& other) : x(other.x) {
        ++live_count;
        std::cout << "TestObject(copy)  x=" << x << "\n";
    }

    TestObject& operator=(const TestObject& other) {
        x = other.x;
        std::cout << "operator=(copy)  x=" << x << "\n";
        return *this;
    }

    ~TestObject() {
        std::cout << "~TestObject()  x=" << x << "\n";
        --live_count;
    }
};

// ------------------------------------------------------------
// 测试 pnew / pdelete
// ------------------------------------------------------------
void test_pnew_pdelete() {
    std::cout << "\n---- test_pnew_pdelete ----\n";

    TestObject* obj = pstd::pnew<TestObject>(42);
    std::cout << "value: " << obj->x << "\n";

    pstd::pdelete(obj);

    std::cout << "live_count after delete: " << TestObject::live_count << "\n";

    pstd::HeapAllocator::Shutdown();
}

// ------------------------------------------------------------
// 测试 uninitialized_default_construct_n
// ------------------------------------------------------------
void test_uninitialized_default_construct_n() {
    std::cout << "\n---- test_uninitialized_default_construct_n ----\n";

    alignas(TestObject) unsigned char buffer[sizeof(TestObject) * 3];
    TestObject* ptr = reinterpret_cast<TestObject*>(buffer);

    pstd::uninitialized_default_construct_n(ptr, 3);
    for (int i = 0; i < 3; ++i)
        std::cout << "ptr[" << i << "].x = " << ptr[i].x << "\n";

    pstd::destroy_n(ptr, 3);
    std::cout << "live_count: " << TestObject::live_count << "\n";
}

// ------------------------------------------------------------
// 测试 uninitialized_copy
// ------------------------------------------------------------
void test_uninitialized_copy() {
    std::cout << "\n---- test_uninitialized_copy ----\n";

    TestObject src[3] = { TestObject(1), TestObject(2), TestObject(3) };

    alignas(TestObject) unsigned char buffer[sizeof(TestObject) * 3];
    TestObject* dst = reinterpret_cast<TestObject*>(buffer);

    pstd::uninitialized_copy(src, src + 3, dst);

    for (int i = 0; i < 3; ++i)
        std::cout << "dst[" << i << "].x = " << dst[i].x << "\n";

    pstd::destroy_n(dst, 3);
    std::cout << "live_count: " << TestObject::live_count << "\n";
}

// ------------------------------------------------------------
// 测试 uninitialized_fill_n
// ------------------------------------------------------------
void test_uninitialized_fill_n() {
    std::cout << "\n---- test_uninitialized_fill_n ----\n";

    alignas(TestObject) unsigned char buffer[sizeof(TestObject) * 4];
    TestObject* dst = reinterpret_cast<TestObject*>(buffer);

    TestObject value(99);
    pstd::uninitialized_fill_n(dst, 4, value);

    for (int i = 0; i < 4; ++i)
        std::cout << "dst[" << i << "].x = " << dst[i].x << "\n";

    pstd::destroy_n(dst, 4);
    std::cout << "live_count: " << TestObject::live_count << "\n";
}

// ------------------------------------------------------------
// 测试 destroy_n（已在各测试中使用）
// ------------------------------------------------------------
void test_destroy_n_alone() {
    std::cout << "\n---- test_destroy_n_alone ----\n";

    alignas(TestObject) unsigned char buffer[sizeof(TestObject) * 2];
    TestObject* objs = reinterpret_cast<TestObject*>(buffer);

    pstd::uninitialized_default_construct_n(objs, 2);
    pstd::destroy_n(objs, 2);
    std::cout << "live_count: " << TestObject::live_count << "\n";
}




// ---------------------------------
// main
// ---------------------------------

//int main() {
//    // 线性分配器测试
//    test_basic_and_alignment_linear();
//    test_cross_block_grow_linear();
//    test_default_reset_reuse_next_linear();
//    test_hard_reset_release_tail_linear();
//    test_single_huge_alloc_linear();
//    test_zero_size_linear();
//    test_many_default_resets_no_growth_linear();
//
//    std::puts("All LinearAllocator tests passed.");
//
//    // 堆分配器测试
//    test_heap_basic_and_alignment();
//    test_heap_auto_grow();
//    test_heap_fragment_and_free();
//
//    std::puts("All HeapAllocator tests passed.");
//
//    // CRTP“多态”演示：同一测试模板驱动两种分配器
//    test_polymorphic_smoke(linear_allocator, "polymorphic_linear");
//    test_polymorphic_smoke(heap_allocator, "polymorphic_heap");
//
//    std::puts("All polymorphic smoke tests passed.");
//
//    
//    // pnew & pdelete
//    test_pnew_pdelete();
//    test_uninitialized_default_construct_n();
//    test_uninitialized_copy();
//    test_uninitialized_fill_n();
//    test_destroy_n_alone();
//
//
//    return 0;
//}

#endif // DEBUG
