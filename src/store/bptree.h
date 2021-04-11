#ifndef _BPTREE_H_
#define _BPTREE_H_

#include <optional>
#include <string>
#include <vector>
#include "node.h"

class BPTree
{
public:
    /**
     * @brief 构造函数
     * @param filename 文件名
     * @param node_size 每个节点的最大容量
     */
    explicit BPTree(std::string filename, size_t node_size);
    ~BPTree();

    /**
     * @brief 插入一个叶子节点
     * @param key
     * @param value
     * @param size 数据的大小
     * @return 是否成功插入
     */
    bool Insert(const std::string& key, const void* value, size_t size);

    /*
     * @brief 遍历
     * @param 起始节点
     */
    void Traverse(Node* node);
    void TraverseLeaf(Node* leaf_node);
    void TraverseIndex(Node* index_node);

    /**
     * @brief 搜索
     * @param key
     * @return 结果
     */
    std::optional<Data> Search(const std::string& key);

    /**
     * @brief 删除索引节点
     * @param node 索引节点
     */
    void DeleteIndexNode(Node* node);

    /**
     * @brief 获取根节点
     * @return root node
     */
    Node* GetRoot();

private:

    /**
     * @brief 插入一个内部节点
     * @param key
     * @param cursor
     * @param child
     * @return
     */
    bool InsertInternal(const std::string& key, Node* cursor, Node* child);
    /**
     * @brief 以cursor为起点寻找child的父节点
     * @param cursor 目标树
     * @param child 目标子节点
     * @return 父节点
     */
    Node* FindParent(Node* cursor, Node* child);

    /**
     * @brief 为key找到叶子节点
     * @param key
     * @return first：合适的节点， second：该节点的父节点
     */
    std::pair<Node*, Node*> FindLeaf(const std::string& key);

    /**
     * @brief 添加一条记录
     * @param cur 当前节点
     * @param key 新加入的key
     * @param value 新加入的值
     * @param size 值的大小
     */
    void AddRecord(Node* cur, const std::string& key, const void* value, size_t size);

    /**
     * @brief 分裂叶子节点
     * @param cur 需要分裂的节点
     * @param key 新加入的key
     * @param value 新加入的值
     * @param size 值的大小
     * @return 新的节点
     */
    Node* SplitLeafNode(Node* cur, const char* key, const void* value, size_t size);

    /**
     * @brief 删除节点
     */
    Node* DeleteNode(Node* node);

    std::string m_file;
    size_t m_record_max_size;
    Node* m_root;
};

#endif
