#ifndef _SUB_NODE_H_
#define _SUB_NODE_H_
#include "record.h"

struct Node;

struct DataConf
{
    static DataConf* GetInstance()
    {
        static DataConf conf;
        return &conf;
    }

    DataConf()
        : m_max_size(3)
    {
    }

    void SetMaxSize(size_t size)
    {
        if (!(size & (unsigned)1))
        {
            size++;
        }
        m_max_size = size;
    }

    size_t m_max_size;
};

struct Leaf
{
    Leaf();
    /*
     * @brief allocate a leaf node
     * @return the leaf or nullptr
     */
    static Leaf* Alloc(size_t record_size, size_t key_size, size_t data_size);

    /*
     * @brief free the leaf node
     * @param leaf
     */
    static void Free(Leaf* leaf);

    /*
     * @brief init the leaf node
     * @param leaf the node which will be init
     * @param record_count the number of record
     * @param key_size size of key
     * @param data_size size of record data
     */
    static void InitLeaf(Leaf* leaf, size_t record_count, size_t key_size, size_t data_size);

    /*
     * @brief add a record in a leaf
     * @return if add record successfully
     */
    bool AddRecord(const char* key, const void* data, size_t data_size);

    /**
     * @brief add a record
     * @param record which will be add
     * @return add ok?
     */
    bool AddRecord(Record* record);

    /*
     * @brief check if the leaf hasn't slot for new record
     * @return true if leaf hasn't  slot for new record
     */
    bool IsFull() const;

    /*
     * @brief get record from leaf
     * @return if has record in leaf[index], return pointer of record, else return nullptr
     */
    Record* GetRecord(size_t index);

    /*
     * @brief get the count of record
     * @return count of record
     */
    size_t Size() const;
    /*
     * @brief insert a record
     */
    Record* Insert(size_t index, const char* key, const void* data, size_t data_size);

    /*
     * @brief set m_cur_count = size
     **/
    void Resize(size_t size);

    size_t m_record_size;
    size_t m_record_count;
    size_t m_cur_count;
    Node* m_next_leaf;
    char m_records[];
};

struct IndexNodeData
{
    std::string m_key;
};

struct IndexNode
{
    IndexNode()
    {
        m_nodes = new IndexNodeData[DataConf::GetInstance()->m_max_size];
        m_next = new Node*[DataConf::GetInstance()->m_max_size + 1];
    }

    ~IndexNode()
    {
        delete[] m_nodes;
        delete[] m_next;
    }

    size_t m_cur_count{};
    IndexNodeData* m_nodes;
    Node** m_next;
};

#endif
