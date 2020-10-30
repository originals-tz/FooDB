#ifndef _BPTREE_H_
#define _BPTREE_H_

#include <string>
#include <vector>
#include "node.h"

class BPTree
{
public:
    /*
     * @brief constructor of bptree
     * @param filename of bptree
     */
    explicit BPTree(const std::string& filename);

    /*
     * @brief insert a leaf node
     * @return result of insert successful
     */
    bool Insert(const std::string& key, const void* value, size_t size);

    /*
     * @brief destructor of bptree
     */
    ~BPTree();

    /*
     * @brief traverse all tree
     * @param which node is the beginning
     */
    void Traverse(Node* node);
    void TraverseLeaf(Node* leaf_node);
    void TraverseIndex(Node* index_node);

    /*
     * @brief search one record by key
     * @param key key will be found
     * @return Data struct with record data in
     */
    Data Search(const std::string& key);

    /*
     * @brief delete children of node
     * @node node's children will delete
     * @return node
     */
    Node* Delete(Node* node);

    Node* GetRoot();

private:
    /*
     * @brief insert one node into a parent index node
     * @param key cursor's minimum key
     * @param cursor parent index node
     * @param child node should be insert
     * @return true for success
     */
    bool InsertInternal(const std::string& key, Node* cursor, Node* child);
    /*
     * @brief find child's parant from cursor
     * @param cursor child's ancestor
     * @param child the node will be found in tree
     */
    Node* FindParent(Node* cursor, Node* child);
    /*
     * @brief find a leaf node for key
     * @return <leaf, parent node of leaf>
     */
    std::pair<Node*, Node*> FindLeaf(const std::string& key);
    /*
     * @brief add a record in cur node
     */
    void AddRecord(Node* cur, const std::string& key, const void* value, size_t size);
    /**
     * @brief split the leaf node
     */
    Node* SplitLeafNode(Node* cur, const char* key, const void* value, size_t size);

    std::string m_file;
    Node* m_root;
};

#endif
