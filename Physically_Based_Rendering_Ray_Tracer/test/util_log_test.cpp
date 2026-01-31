#include "util/log.h"
#include <iostream>
#include <thread>
#include <chrono>

// 测试基本日志输出
void test_basic_logging() {
    std::cout << "\n=== Test 1: Basic Logging ===" << std::endl;
    
    REGISTER_LOG("Test1", true);
    LOG_STREAM("Test1") << "Hello, World!" << std::endl;
    LOG_STREAM("Test1") << "Number: " << 42 << ", Float: " << 3.14159 << std::endl;
}

// 测试启用/禁用功能
void test_enable_disable() {
  std::cout << "\n=== Test 2: Enable/Disable Streams ===" << std::endl;
    
    REGISTER_LOG("Stream1", true);
    REGISTER_LOG("Stream2", false);
    
    LOG_STREAM("Stream1") << "Stream1 is enabled" << std::endl;
    LOG_STREAM("Stream2") << "Stream2 is disabled - you should NOT see this" << std::endl;
    
    std::cout << "\n--- Disabling Stream1, Enabling Stream2 ---" << std::endl;
    DISABLE_LOG("Stream1");
    ENABLE_LOG("Stream2");
    
    LOG_STREAM("Stream1") << "Stream1 is now disabled - you should NOT see this" << std::endl;
    LOG_STREAM("Stream2") << "Stream2 is now enabled" << std::endl;
}

// 测试多个日志流
void test_multiple_streams() {
    std::cout << "\n=== Test 3: Multiple Log Streams ===" << std::endl;
    
    REGISTER_LOG("Renderer", true);
 REGISTER_LOG("Physics", true);
 REGISTER_LOG("Audio", true);
    REGISTER_LOG("Network", false);
    
    LOG_STREAM("Renderer") << "Rendering frame 1" << std::endl;
    LOG_STREAM("Physics") << "Physics update: dt = 0.016s" << std::endl;
    LOG_STREAM("Audio") << "Playing sound: explosion.wav" << std::endl;
    LOG_STREAM("Network") << "This is disabled - you should NOT see this" << std::endl;
}

// 测试批量启用/禁用
void test_enable_disable_all() {
    std::cout << "\n=== Test 4: Enable/Disable All ===" << std::endl;
    
    REGISTER_LOG("Log1", true);
    REGISTER_LOG("Log2", true);
    REGISTER_LOG("Log3", true);
    
    LOG_STREAM("Log1") << "Log1 active" << std::endl;
    LOG_STREAM("Log2") << "Log2 active" << std::endl;
  LOG_STREAM("Log3") << "Log3 active" << std::endl;
    
    std::cout << "\n--- Disabling all logs ---" << std::endl;
    LogController::getInstance().disableAll();
    
    LOG_STREAM("Log1") << "Should NOT see this" << std::endl;
    LOG_STREAM("Log2") << "Should NOT see this" << std::endl;
    LOG_STREAM("Log3") << "Should NOT see this" << std::endl;
    
    std::cout << "\n--- Enabling all logs ---" << std::endl;
    LogController::getInstance().enableAll();
    
    LOG_STREAM("Log1") << "Log1 re-enabled" << std::endl;
    LOG_STREAM("Log2") << "Log2 re-enabled" << std::endl;
    LOG_STREAM("Log3") << "Log3 re-enabled" << std::endl;
}

// 测试链式调用
void test_chaining() {
    std::cout << "\n=== Test 5: Chaining Operations ===" << std::endl;
    
    REGISTER_LOG("Chain", true);
    LOG_STREAM("Chain") << "Start" << " -> " << "Middle" << " -> " << "End" << std::endl;
    LOG_STREAM("Chain") << "X=" << 10 << ", Y=" << 20 << ", Z=" << 30 << std::endl;
}

// 测试不同数据类型
void test_different_types() {
    std::cout << "\n=== Test 6: Different Data Types ===" << std::endl;
    
    REGISTER_LOG("Types", true);
    
int intVal = 42;
    float floatVal = 3.14159f;
    double doubleVal = 2.71828;
    bool boolVal = true;
    const char* strVal = "C-style string";
    std::string stdStrVal = "std::string";
  
    LOG_STREAM("Types") << "int: " << intVal << std::endl;
    LOG_STREAM("Types") << "float: " << floatVal << std::endl;
    LOG_STREAM("Types") << "double: " << doubleVal << std::endl;
    LOG_STREAM("Types") << "bool: " << std::boolalpha << boolVal << std::endl;
    LOG_STREAM("Types") << "c-string: " << strVal << std::endl;
    LOG_STREAM("Types") << "std::string: " << stdStrVal << std::endl;
}

// 测试自动注册（通过 getStream")
void test_auto_registration() {
std::cout << "\n=== Test 7: Auto Registration ===" << std::endl;
    
    // 不需要 REGISTER_LOG，直接使用 LOG_STREAM 会自动注册
    LOG_STREAM("AutoStream") << "This stream was auto-registered" << std::endl;
    LOG_STREAM("AutoStream") << "And it works!" << std::endl;
}

// 测试多线程安全性（简单测试）
void thread_worker(const std::string& streamName, int id) {
    for (int i = 0; i < 5; ++i) {
        LOG_STREAM(streamName) << "Thread " << id << " - Message " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void test_thread_safety() {
    std::cout << "\n=== Test 8: Thread Safety ===" << std::endl;
    
    REGISTER_LOG("ThreadTest", true);
    
    std::thread t1(thread_worker, "ThreadTest", 1);
    std::thread t2(thread_worker, "ThreadTest", 2);
std::thread t3(thread_worker, "ThreadTest", 3);
    
    t1.join();
    t2.join();
    t3.join();
    
    std::cout << "Thread test completed (messages may be interleaved)" << std::endl;
}

// 测试 isEnabled 查询
void test_is_enabled() {
    std::cout << "\n=== Test 9: isEnabled Query ===" << std::endl;
    
    REGISTER_LOG("EnabledCheck", true);
    REGISTER_LOG("DisabledCheck", false);
    
    auto& stream1 = LOG_STREAM("EnabledCheck");
    auto& stream2 = LOG_STREAM("DisabledCheck");
    
    std::cout << "EnabledCheck is enabled: " << std::boolalpha << stream1.isEnabled() << std::endl;
    std::cout << "DisabledCheck is enabled: " << std::boolalpha << stream2.isEnabled() << std::endl;
    
    DISABLE_LOG("EnabledCheck");
ENABLE_LOG("DisabledCheck");
    
    std::cout << "After toggle:" << std::endl;
    std::cout << "EnabledCheck is enabled: " << std::boolalpha << stream1.isEnabled() << std::endl;
    std::cout << "DisabledCheck is enabled: " << std::boolalpha << stream2.isEnabled() << std::endl;
}

// 测试手动 flush
void test_manual_flush() {
  std::cout << "\n=== Test 10: Manual Flush ===" << std::endl;
    
    REGISTER_LOG("FlushTest", true);
    auto& stream = LOG_STREAM("FlushTest");
    
    stream << "Message without endl";
    std::cout << " <- No output yet (buffered)" << std::endl;
    
    stream.flush();
    std::cout << " <- Now flushed manually" << std::endl;
}

//int main() {
//    std::cout << "========================================" << std::endl;
//    std::cout << "    LOG SYSTEM TEST SUITE" << std::endl;
//    std::cout << "========================================" << std::endl;
//    
//#ifdef DEBUG
//    std::cout << "Running in DEBUG mode - logs are ENABLED" << std::endl;
//#else
//    std::cout << "Running in RELEASE mode - logs are DISABLED" << std::endl;
//    std::cout << "(Define DEBUG macro to enable logging)" << std::endl;
//#endif
//    
//    try {
//        test_basic_logging();
//        test_enable_disable();
//        test_multiple_streams();
//        test_enable_disable_all();
//        test_chaining();
//        test_different_types();
//        test_auto_registration();
//        test_thread_safety();
//        test_is_enabled();
//        test_manual_flush();
//        
//        std::cout << "\n========================================" << std::endl;
//      std::cout << "    ALL TESTS COMPLETED" << std::endl;
//        std::cout << "========================================" << std::endl;
//   
//    } catch (const std::exception& e) {
//        std::cerr << "Test failed with exception: " << e.what() << std::endl;
//        return 1;
//    }
//    
//    return 0;
//}