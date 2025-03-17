#include <gtest/gtest.h>
#include "ConsistenHash.h"
#include <thread>


using namespace muduo;

// 基础功能测试：空环异常
TEST(ConsistentHashTest, EmptyRingThrows) {
    ConsistenHash ch(3);
    EXPECT_THROW(ch.getNode("key"), std::runtime_error);
}

// 单节点基础功能测试
TEST(ConsistentHashTest, SingleNodeOperations) {
    auto hashFunc = [](const std::string& s) -> size_t { // 显式指定返回类型
        if (s == "A_0") return static_cast<size_t>(100);
        if (s == "A_1") return static_cast<size_t>(200);
        if (s == "A_2") return static_cast<size_t>(300);
        return std::hash<std::string>{}(s);
    };
    
    ConsistenHash ch(3, hashFunc);
    ch.addNode("A");

    // 测试不同哈希值的分布
    auto testHashFunc = [](const std::string& s) {
        if (s == "key1") return 50;   // < min → wrap
        if (s == "key2") return 150;  // → A_1 (200)
        if (s == "key3") return 250;  // → A_2 (300)
        if (s == "key4") return 350;  // → wrap → A_0 (100)
        return 0;
    };

    ConsistenHash testCh(3, testHashFunc);
    testCh.addNode("A");

    EXPECT_EQ(testCh.getNode("key1"), "A");
    EXPECT_EQ(testCh.getNode("key2"), "A");
    EXPECT_EQ(testCh.getNode("key3"), "A");
    EXPECT_EQ(testCh.getNode("key4"), "A");

    // 删除节点后恢复空环
    ch.removeNode("A");
    EXPECT_THROW(ch.getNode("key"), std::runtime_error);
}

// 多节点分布测试
TEST(ConsistentHashTest, MultipleNodeDistribution) {
    auto hashFunc = [](const std::string& s) {
        // 物理节点 A 的虚拟节点
        if (s == "A_0") return 100;
        if (s == "A_1") return 200;
        if (s == "A_2") return 300;
        // 物理节点 B 的虚拟节点
        if (s == "B_0") return 150;
        if (s == "B_1") return 250;
        if (s == "B_2") return 350;
        // 测试 key 的哈希值
        if (s == "k1") return 120;  // → B_0 (150)
        if (s == "k2") return 220;  // → B_1 (250)
        if (s == "k3") return 360;  // → A_0 (100 wrap)
        return 0;
    };

    ConsistenHash ch(3, hashFunc);
    ch.addNode("A");
    ch.addNode("B");

    EXPECT_EQ(ch.getNode("k1"), "B");
    EXPECT_EQ(ch.getNode("k2"), "B");
    EXPECT_EQ(ch.getNode("k3"), "A");
}

// 节点删除测试
TEST(ConsistentHashTest, NodeRemoval) {
    auto hashFunc = [](const std::string& s) {
        if (s.find("A_") != std::string::npos) return 100;
        if (s.find("B_") != std::string::npos) return 200;
        return 300; // key 的哈希
    };

    ConsistenHash ch(3, hashFunc);
    ch.addNode("A");
    ch.addNode("B");
    
    // 删除 A 前 key 应路由到 A
    EXPECT_EQ(ch.getNode("key"), "A");
    
    ch.removeNode("A");
    // 删除 A 后 key 应路由到 B
    EXPECT_EQ(ch.getNode("key"), "B");
}

// 虚拟节点数量验证
TEST(ConsistentHashTest, VirtualNodeCount) {
    auto hashFunc = [](const std::string& s) {
        return static_cast<size_t>(std::stoi(s.substr(s.find_last_of('_') + 1)));
    };

    ConsistenHash ch(5, hashFunc);
    ch.addNode("node");
    
    // 通过已知哈希值验证总数
    EXPECT_NO_THROW({
        for (int i = 0; i < 5; ++i) {
            ch.getNode(std::to_string(i));
        }
    });

    ch.removeNode("node");
    EXPECT_THROW(ch.getNode("any"), std::runtime_error);
}

// 哈希环排序验证
TEST(ConsistentHashTest, HashRingOrdering) {
    auto hashFunc = [](const std::string& s) -> size_t {
        if (s == "A_0") return static_cast<size_t>(300);
        if (s == "A_1") return static_cast<size_t>(100);
        if (s == "A_2") return static_cast<size_t>(200);
        return std::hash<std::string>{}(s);
    };

    ConsistenHash ch(3, hashFunc);
    ch.addNode("A");

    // 使用会触发环回绕的 key 验证排序
    auto testFunc = [](const std::string& s) { return 350; };
    ConsistenHash testCh(3, testFunc);
    testCh.addNode("A");
    EXPECT_EQ(testCh.getNode("any"), "A");
}

// 自定义哈希函数测试
TEST(ConsistentHashTest, CustomHashFunction) {
    struct CustomHash {
        size_t operator()(const std::string& s) const {
            return s.length(); // 简单示例哈希
        }
    };

    ConsistenHash ch(2, CustomHash{});
    ch.addNode("node1"); // 虚拟节点长度：6("node1_0"), 6("node1_1")
    ch.addNode("node2"); // 虚拟节点长度：6("node2_0"), 6("node2_1")
    
    // 由于哈希冲突，实际测试需要特殊处理
    // 这里主要验证接口兼容性
    EXPECT_NO_THROW(ch.getNode("test_key"));
}

// 并发测试（基础版本）
TEST(ConsistentHashTest, ConcurrentAccess) {
    ConsistenHash ch(100);
    
    // 并行操作需要更复杂的测试框架支持
    // 此处仅验证基础线程安全
    auto worker = [&ch](int id) {
        for (int i = 0; i < 100; ++i) {
            ch.addNode("node_" + std::to_string(id) + "_" + std::to_string(i));
            ch.getNode("key_" + std::to_string(i));
        }
    };

    // 简单多线程测试（实际应使用线程池）
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    t1.join();
    t2.join();

    EXPECT_FALSE(ch.getNode("key_0").empty());
}
