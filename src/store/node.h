#ifndef _NODE_H_
#define _NODE_H_

#include <cstdint>
#include <string>
#include <vector>

struct Node
{
    explicit Node(bool is_leaf, size_t record_max_size, uint64_t page_id = 0);

    size_t GetSize() const;
    int Compare(size_t i, const char* key) const;
    size_t FindPos(const char* key) const;

    bool m_is_leaf;
    uint64_t m_page_id;
    uint64_t m_parent_page_id;
    size_t m_record_max_size;
    std::vector<std::string> m_keys;
    std::vector<std::string> m_values;
    std::vector<Node*> m_children;
    Node* m_next_leaf;
};

#endif
