#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <algorithm>
#include <stdexcept>

inline namespace muduo {

/**
 * @class ConsistentHash
 * @brief 一致性哈希算法
 * 
 * 一致性哈希算法是一种分布式哈希算法，它能够在节点的增减时，最小化数据的迁移。
 * 一致性哈希算法的核心思想是将节点和数据都映射到一个环上，通过计算节点和数据的哈希值，
 * 将节点和数据映射到环上的某个点，然后将数据映射到最近的节点上。
 * 
 * 可通过增加虚拟节点的方式解决节点分布不均匀的问题，改善负载均衡效果，减少数据倾斜。
 */
class ConsistenHash {
public:
    /**
     * @brief 构造函数
     * @param numReplicas 每个物理节点对应的虚拟节点个数
     * @param hashFunc 哈希函数，默认使用 std::hash<std::string>
     */
    ConsistenHash(size_t numReplicas, std::function<size_t(const std::string&)> hashFunc = std::hash<std::string>())
    : numReplicas_(numReplicas), hashFunc_(hashFunc) {}

    /**
     * @brief 向哈希环中添加节点
     * @param node 节点名称
     * 
     * 物理节点会被映射到多个虚拟节点上，每个虚拟节点对应一个哈希值（node_i）。
     * 哈希值有序排列，存储在环上。
     */
    void addNode(const std::string& node) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < numReplicas_; ++i) {
            size_t hash = hashFunc_(node + "_" + std::to_string(i));
            hashRing_[hash] = node;
            sortedHashes_.push_back(hash);
        }
        std::sort(sortedHashes_.begin(), sortedHashes_.end());
    }
    /**
     * @brief 从哈希环中删除节点
     * @param node 节点名称
     */
    void removeNode(const std::string& node) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < numReplicas_; ++i) {
            size_t hash = hashFunc_(node + "_" + std::to_string(i));
            hashRing_.erase(hash);
            auto it = std::find(sortedHashes_.begin(), sortedHashes_.end(), hash);
            if (it != sortedHashes_.end()) {
                sortedHashes_.erase(it);
            }
        }
    }
    /**
     * @brief 根据 key 在哈希环上查找对应的节点，没有找到则返回第一个节点
     * @param key 数据的键（如IP地址）
     * @return 数据对应的节点名
     */
    std::string getNode(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (hashRing_.empty()) {
            throw std::runtime_error("hash ring is empty");
        }
    
        size_t hash = hashFunc_(key);
        auto it = std::upper_bound(sortedHashes_.begin(), sortedHashes_.end(), hash);
    
        if (it == sortedHashes_.end()) {
            return hashRing_.at(sortedHashes_.front());
        } else {
            return hashRing_.at(*it);
        }
    }
private:
    size_t numReplicas_;  // 每个物理节点对应的虚拟节点个数
    std::function<size_t(const std::string&)> hashFunc_;  // 哈希函数
    std::unordered_map<size_t, std::string> hashRing_;  // 哈希环，哈希值到节点的映射
    std::vector<size_t> sortedHashes_;  // 有序的哈希值
    std::mutex mutex_;  // 互斥锁，保护哈希环的修改

};

}