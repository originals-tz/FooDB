#ifndef _NODE_H_
#define _NODE_H_

#include "leaf.h"
#include "index.h"

struct Node
{
    /*
     * @brief constructor of node
     * @param is_leaf leaf node or not
     */
    explicit Node(bool is_leaf, size_t record_max_size);
    /*
     * @brief get the size of data
     * @return size of data
     */
    size_t GetSize() const;
    /*
     * @brief compare the key with the key of data[i]
     * @return strcmp
     */
    int Compare(size_t i, const char* key);
    /*
     * @brief get node's pointer as leaf node
     * @return leaf pointer
     */
    Leaf* GetLeaf() const;
    /*
     * @brief get node's pointer as index node
     * @return index pointer
     */
    IndexNode* GetIndex() const;

    size_t FindPos(const char* key);

    bool m_is_leaf;
    IndexNode* m_index;
    Leaf* m_leaf;
    size_t m_record_max_size;
};

#endif
