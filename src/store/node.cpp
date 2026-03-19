#include "node.h"

#include <cassert>
#include <cstring>

Node::Node(bool is_leaf, size_t record_max_size, uint64_t page_id)
    : m_is_leaf(is_leaf)
    , m_page_id(page_id)
    , m_parent_page_id(0)
    , m_record_max_size(record_max_size)
    , m_next_leaf(nullptr)
{
}

size_t Node::GetSize() const
{
    return m_keys.size();
}

int Node::Compare(size_t i, const char* key) const
{
    assert(key && "key is nullptr");
    assert(i < m_keys.size() && "compare index out of range");
    return std::strcmp(key, m_keys[i].c_str());
}

size_t Node::FindPos(const char* key) const
{
    assert(key && "key is nullptr");
    size_t pos = 0;
    for (; pos < m_keys.size() && std::strcmp(m_keys[pos].c_str(), key) < 0; ++pos)
        ;
    return pos;
}
