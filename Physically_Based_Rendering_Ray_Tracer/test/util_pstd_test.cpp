#include <iostream>
#include <cassert>
#include "util/pstd.h"
#include "util/memory.h"

// 一个方便的计数器类，用来测试构造/析构/赋值等行为
struct Tracker {
    static int live_count;
    static int ctor_count;
    static int dtor_count;
    static int copy_ctor_count;
    static int move_ctor_count;
    static int copy_assign_count;
    static int move_assign_count;

    int value = 0;

    Tracker() : value(0) {
        ++live_count;
        ++ctor_count;
    }

    explicit Tracker(int v) : value(v) {
        ++live_count;
        ++ctor_count;
    }

    Tracker(const Tracker& other) : value(other.value) {
        ++live_count;
        ++copy_ctor_count;
    }

    Tracker(Tracker&& other) noexcept : value(other.value) {
        ++live_count;
        ++move_ctor_count;
        other.value = -9999;
    }

    Tracker& operator=(const Tracker& other) {
        value = other.value;
        ++copy_assign_count;
        return *this;
    }

    Tracker& operator=(Tracker&& other) noexcept {
        value = other.value;
        other.value = -9999;
        ++move_assign_count;
        return *this;
    }

    ~Tracker() {
        --live_count;
        ++dtor_count;
    }
};

// 定义静态成员
int Tracker::live_count = 0;
int Tracker::ctor_count = 0;
int Tracker::dtor_count = 0;
int Tracker::copy_ctor_count = 0;
int Tracker::move_ctor_count = 0;
int Tracker::copy_assign_count = 0;
int Tracker::move_assign_count = 0;


// ------------------------------------------------------------
// 测试 pstd::vector
// 我们会测试：
//  - 默认构造
//  - push_back / emplace_back
//  - front/back/data/begin/end
//  - 拷贝构造 / 赋值
//  - 移动构造 / 移动赋值
//  - assign(迭代器范围) / assign({initializer_list})
//  - clear()
// ------------------------------------------------------------
void test_vector() {
    std::cout << "== test_vector ==\n";

    {
        // 基础 push_back / emplace_back 路径
        pstd::vector<int> v;
        assert(v.size() == 0);
        assert(v.empty());

        v.push_back(10);
        v.push_back(20);
        v.emplace_back(30);
        assert(v.size() == 3);
        assert(!v.empty());

        assert(v[0] == 10);
        assert(v[1] == 20);
        assert(v[2] == 30);
        assert(v.front() == 10);
        assert(v.back() == 30);


        // 检查迭代器基本工作
        int sum = 0;
        for (auto x : v) sum += x;
        assert(sum == 60);

        // pop_back
        v.pop_back();
        assert(v.size() == 2);
        assert(v.back() == 20);

        // clear
        v.clear();
        assert(v.size() == 0);
        assert(v.empty());

    }

    {
        // 测试拷贝构造、拷贝赋值
        pstd::vector<int> a;
        a.push_back(1);
        a.push_back(2);
        a.push_back(3);

        pstd::vector<int> b = a;  // 拷贝构造
        assert(b.size() == 3);
        assert(b[0] == 1 && b[1] == 2 && b[2] == 3);

        pstd::vector<int> c;
        c.push_back(42);
        c = a; // 拷贝赋值
        assert(c.size() == 3);
        assert(c[0] == 1 && c[1] == 2 && c[2] == 3);

        // assign(迭代器范围)
        pstd::vector<int> d;
        d.assign(a.begin(), a.end()); // 深拷贝
        assert(d.size() == 3);
        assert(d[0] == 1 && d[1] == 2 && d[2] == 3);

        // assign(initializer_list)
        d.assign({ 9, 8, 7, 6 });
        assert(d.size() == 4);
        assert(d[0] == 9 && d[1] == 8 && d[2] == 7 && d[3] == 6);
    }

    {
        // 测试移动构造、移动赋值
        pstd::vector<int> a;
        a.push_back(11);
        a.push_back(22);
        a.push_back(33);

        // 移动构造
        pstd::vector<int> b = std::move(a);
        assert(b.size() == 3);
        assert(b[0] == 11 && b[1] == 22 && b[2] == 33);

        // a 现在被移走了，size() 应该是0（按你的实现，_size 被置0）
        assert(a.size() == 0);

        // 移动赋值
        pstd::vector<int> c;
        c.push_back(100);
        c.push_back(200);

        c = std::move(b);
        assert(c.size() == 3);
        assert(c[0] == 11 && c[1] == 22 && c[2] == 33);
        // b 现在应该是空
        assert(b.size() == 0);
    }

    {
        // 用 Tracker 看一下构造/析构路径大概是否被调用
        Tracker::live_count = 0;
        Tracker::ctor_count = 0;
        Tracker::dtor_count = 0;
        Tracker::copy_ctor_count = 0;
        Tracker::move_ctor_count = 0;
        Tracker::copy_assign_count = 0;
        Tracker::move_assign_count = 0;

        {
            pstd::vector<Tracker> vt;
            vt.emplace_back(1);
            vt.emplace_back(2);
            vt.push_back(Tracker(3)); // 触发移动构造
            vt.push_back(vt[0]);      // 触发拷贝构造/赋值

            assert(vt.size() == 4);
            assert(vt[0].value == 1);
            assert(vt[1].value == 2);
            assert(vt[2].value == 3);
            assert(vt[3].value == 1);

            // 触发 clear() 再析构
            vt.clear();
            assert(vt.size() == 0);
        }

        // 容器析构后，理论上 live_count 应该回到0
        assert(Tracker::live_count == 0);

        // 我们在这里不做严格数值断言（因为这依赖你的 vector 里是用 move 还是 copy 搬元素）
        // 但可以输出看个感觉
        std::cout << "Tracker stats: "
            << "live=" << Tracker::live_count
            << " ctor=" << Tracker::ctor_count
            << " copy_ctor=" << Tracker::copy_ctor_count
            << " move_ctor=" << Tracker::move_ctor_count
            << " dtor=" << Tracker::dtor_count
            << " copy_assign=" << Tracker::copy_assign_count
            << " move_assign=" << Tracker::move_assign_count
            << "\n";
    }

    std::cout << "test_vector PASSED\n\n";
}


// ------------------------------------------------------------
// 测试 pstd::span
// 我们会测试：
//   - 构造 (指针+长度 / C数组 / std::vector)
//   - 访问 size() / operator[]
//   - front()/back()
//   - remove_prefix/remove_suffix
//   - subspan()
// ------------------------------------------------------------
void test_span() {
    std::cout << "== test_span ==\n";

    // 用一个静态数组做后端存储
    int raw[6] = { 10, 20, 30, 40, 50, 60 };

    // 通过 (ptr, n) 构造
    pstd::span<int> s1(raw, 6);
    assert(s1.size() == 6);
    assert(!s1.empty());
    assert(s1[0] == 10);
    assert(s1[5] == 60);
    assert(s1.front() == 10);
    assert(s1.back() == 60);
    assert(s1.data() == raw);

    // 通过数组构造
    pstd::span<int> s2(raw); // 依赖 span(T(&a)[N]) 构造
    assert(s2.size() == 6);
    assert(s2[2] == 30);

    // subspan
    pstd::span<int> mid = s1.subspan(2, 3); // 期望 {30,40,50}
    assert(mid.size() == 3);
    assert(mid[0] == 30);
    assert(mid[1] == 40);
    assert(mid[2] == 50);

    // remove_prefix / remove_suffix
    pstd::span<int> s3(raw, 6);
    s3.remove_prefix(1); // drop 10
    assert(s3.size() == 5);
    assert(s3[0] == 20);

    s3.remove_suffix(2); // drop last two (50,60 if we started at full)
    assert(s3.size() == 3);
    assert(s3[0] == 20);
    assert(s3.back() == 40);

    // MakeSpan from vector
    pstd::vector<int> vtemp;
    vtemp.push_back(7);
    vtemp.push_back(8);
    vtemp.push_back(9);

    auto s4 = pstd::MakeSpan(vtemp); // span<int>
    assert(s4.size() == 3);
    assert(s4[0] == 7);
    assert(s4[2] == 9);
    assert(s4.data() == vtemp.data());

    // const 视图
    auto cs4 = pstd::MakeConstSpan(vtemp); // span<const int>
    assert(cs4.size() == 3);
    assert(cs4[1] == 8);

    std::cout << "test_span PASSED\n\n";
}


// ------------------------------------------------------------
// 测试 pstd::optional
// 我们会测试：
//   - 默认构造为空
//   - 构造有值 / 复制 / 移动
//   - operator*, operator->, value(), has_value()
//   - reset()
//   - value_or()
//   - operator<< (打印)
// ------------------------------------------------------------
void test_optional() {
    std::cout << "== test_optional ==\n";

    {
        pstd::optional<int> o;
        assert(!o.has_value());
        assert(static_cast<bool>(o) == false);
        assert(o.value_or(1234) == 1234);
    }

    {
        pstd::optional<int> o(42);
        assert(o.has_value());
        assert(static_cast<bool>(o));
        assert(o.value() == 42);
        assert(o.value_or(999) == 42);

        o = 100;
        assert(o.value() == 100);
        assert(o == 100); // o-> is pointer-like, so *o == 100
    }

    {
        // 拷贝
        pstd::optional<int> a(7);
        pstd::optional<int> b = a;
        assert(b.has_value());
        assert(b.value() == 7);

        // 赋值
        pstd::optional<int> c;
        c = a;
        assert(c.has_value());
        assert(c.value() == 7);

        // 移动
        pstd::optional<int> d = std::move(a);
        assert(d.has_value());
        assert(d.value() == 7);

        // a 按你的实现会 reset()，所以现在可能是空
        assert(a.has_value() == false);
    }

    {
        // reset
        pstd::optional<std::string> o(std::string("hello"));
        assert(o.has_value());
        assert(o.value() == "hello");

        o.reset();
        assert(!o.has_value());
    }

    {
        // 输出演示（不是断言关键路径，只是让我们看到格式）
        pstd::optional<int> x(99);
        pstd::optional<int> y;
        std::cout << "opt x: " << x << "\n";
        std::cout << "opt y: " << y << "\n";
    }

    {
        // 测一下复杂类型 Tracker（构造/析构）
        Tracker::live_count = 0;
        {
            pstd::optional<Tracker> ot(Tracker(5));
            assert(ot.has_value());
            assert(ot.value().value == 5);
            assert(Tracker::live_count >= 1);
            ot.reset();
            assert(!ot.has_value());
        }
        // 出作用域后应该没有活着的 Tracker
        assert(Tracker::live_count == 0);
    }

    std::cout << "test_optional PASSED\n\n";
}


// ------------------------------------------------------------
// 测试 pstd::array
// 我们会测试：
//   - 默认构造（N>0）
//   - initializer_list 构造
//   - operator[] / data() / begin()/end()/size()
//   - operator== / operator!=
// 注意：不调用 fill()，因为当前 array::fill() 里用的是 pstd::fill，
// 你那边还没把它换成 std::fill 并加 <algorithm>，可能还没稳。
// ------------------------------------------------------------
void test_array() {
    std::cout << "== test_array ==\n";

    {
        // 默认构造 N>0
        pstd::array<int, 4> arr_default;
        assert(arr_default.size() == 4);

        // 默认构造后元素是通过 uninitialized_default_construct_n() 构过的，
        // 也就是值初始化 T{}，对int来说应该是0。
        // 如果你的实现后面变成“未初始化原始内存 + 不赋值”，
        // 这里的断言可能需要改。
        // 现在先尽量保守地只检查能访问：
        (void)arr_default[0];
        (void)arr_default[1];
        (void)arr_default[2];
        (void)arr_default[3];
    }

    {
        // initializer_list 构造
        pstd::array<int, 4> arr_list{ 10, 20, 30, 40 };
        assert(arr_list.size() == 4);

        assert(arr_list[0] == 10);
        assert(arr_list[1] == 20);
        assert(arr_list[2] == 30);
        assert(arr_list[3] == 40);

        // 迭代器工作吗？
        int sum = 0;
        for (auto x : arr_list) sum += x;
        assert(sum == (10 + 20 + 30 + 40));

        // data()
        int* raw = arr_list.data();
        assert(raw != nullptr);
        assert(raw[2] == 30);

        // 拷贝比较
        pstd::array<int, 4> arr_copy{ 10, 20, 30, 40 };
        pstd::array<int, 4> arr_diff{ 10, 20, 30, 41 };

        assert(arr_list == arr_copy);
        assert(!(arr_list != arr_copy));
        assert(arr_list != arr_diff);
    }

    {
        // 测试对非平凡类型的存储
        Tracker::live_count = 0;
        Tracker::ctor_count = 0;
        Tracker::dtor_count = 0;

        {
            pstd::array<Tracker, 2> arr_track(Tracker(123));
            // 这会调用你的 array(const T&) 填充所有元素为给定值
            assert(arr_track.size() == 2);
            assert(arr_track[0].value == 123);
            assert(arr_track[1].value == 123);
            assert(Tracker::live_count == 2);
        }

        // 出作用域后析构应把 live_count 减到 0
        assert(Tracker::live_count == 0);
    }

    std::cout << "test_array PASSED\n\n";
}


// ------------------------------------------------------------
// main()：执行所有测试
// ------------------------------------------------------------
//int main() {
//    test_vector();
//    test_span();
//    test_optional();
//    test_array();
//
//    std::cout << "All pstd tests completed.\n";
//    return 0;
//}
