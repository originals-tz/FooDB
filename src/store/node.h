#ifndef _NODE_H_
#define _NODE_H_

#include "sub_node.h"

struct Node
{
    bool m_is_leaf;
    IndexNode* m_index;
    Leaf* m_leaf;

    /*
     * @brief constructor of node
     * @param is_leaf leaf node or not
     */
    Node(bool is_leaf);
    /*
     * @brief get the size of data
     * @return size of data
     */
    size_t GetSize();
    /*
     * @brief compare the key with the key of data[i]
     * @return if key > data[i]'s key, return true
     */
    int Compare(size_t i, const char* key);
    /*
     * @brief get node's pointer as leaf node
     * @return leaf pointer
     */
    Leaf* GetLeaf();
    /*
     * @brief get node's pointer as index node
     * @return index pointer
     */
    IndexNode* GetIndex();

    size_t FindPos(const char* key);
};

#endif
