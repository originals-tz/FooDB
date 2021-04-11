#include "node.h"

#include <cstring>

#include "macro.h"
#include "trace.h"

Node::Node(bool is_leaf, size_t record_max_size)
    : m_is_leaf(is_leaf)
    , m_index(nullptr)
    , m_leaf(nullptr)
    , m_record_max_size(record_max_size)
{
    if (m_is_leaf)
    {
        m_leaf = Leaf::Alloc(m_record_max_size, 10, 10);
    }
    else
    {
        m_index = new IndexNode(m_record_max_size);
    }
}

size_t Node::GetSize() const
{
    if (m_is_leaf)
    {
        assert(m_is_leaf && m_leaf);
    }
    else
    {
        assert(m_index);
    }
    return m_is_leaf ? m_leaf->m_cur_count : m_index->m_cur_index;
}

int Node::Compare(size_t i, const char* key)
{
    assert(key && "key is nullptr");
    const char* record_key = m_is_leaf ? GetLeaf()->GetRecord(i)->GetKey() : GetIndex()->m_nodes[i].m_key.c_str();
    return std::strcmp(key, record_key);
}

Leaf* Node::GetLeaf() const
{
    return m_leaf;
}

IndexNode* Node::GetIndex() const
{
    return m_index;
}

size_t Node::FindPos(const char* key)
{
    assert(key && "key is nullptr");
    size_t i = 0;
    for (; i < GetSize() && Compare(i, key) > 0; i++)
        ;
    Trace(key, " pos =", i);
    return i;
}
