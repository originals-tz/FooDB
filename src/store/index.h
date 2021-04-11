#ifndef FOODB_INDEX_H
#define FOODB_INDEX_H

struct IndexNodeData
{
    std::string m_key;
};

struct IndexNode
{
    explicit IndexNode(size_t m_max_record_size)
    {
        m_nodes = new IndexNodeData[m_max_record_size];
        m_next = new Node*[m_max_record_size + 1];
    }

    ~IndexNode()
    {
        delete[] m_nodes;
        delete[] m_next;
    }

    size_t m_cur_index{};
    IndexNodeData* m_nodes;
    Node** m_next;
};

#endif  // FOODB_INDEX_H
