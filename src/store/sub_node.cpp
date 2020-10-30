#include "sub_node.h"

#include <cstring>

#include "util/macro.h"
#include "util/trace.h"

Leaf::Leaf()
    : m_record_size(0)
    , m_record_count(0)
    , m_cur_count(0)
    , m_next_leaf(nullptr)
{
}

Leaf* Leaf::Alloc(size_t record_size, size_t key_size, size_t data_size)
{
    AssertRequire(record_size && key_size && data_size, "param error");

    size_t raw_size = sizeof(Leaf) + (sizeof(Record) + key_size + data_size) * record_size;
    void* raw = malloc(raw_size);
    bzero(raw, raw_size);

    Leaf* leaf = new (raw) Leaf();
    Require(leaf != nullptr, nullptr, Trace("malloc error"));
    InitLeaf(leaf, record_size, key_size, data_size);
    return leaf;
}

void Leaf::Free(Leaf* leaf)
{
    if (leaf)
    {
        free(leaf);
    }
}

void Leaf::InitLeaf(Leaf* leaf, size_t record_count, size_t key_size, size_t data_size)
{
    AssertRequire(leaf && record_count && key_size && data_size, "leaf init : param error");
    leaf->m_record_count = record_count;
    leaf->m_record_size = key_size + data_size + sizeof(Record);
    leaf->m_next_leaf = nullptr;

    char* record_buffer = leaf->m_records;
    for (size_t i = 0; i < record_count; i++)
    {
        Record::Init(key_size, data_size, record_buffer);
        record_buffer += leaf->m_record_size;
    }
}

bool Leaf::AddRecord(const char* key, const void* data, size_t data_size)
{
    Require(IsFull() == false, false, Trace("is full"));
    GetRecord(m_cur_count)->SetKey(key);
    GetRecord(m_cur_count)->SetData(data, data_size);
    m_cur_count++;
    return true;
}

bool Leaf::AddRecord(Record* record)
{
    return AddRecord(record->GetKey(), record->GetRawData(), record->GetDataSize());
}

bool Leaf::IsFull()
{
    return m_cur_count == m_record_count;
}

Record* Leaf::GetRecord(size_t index)
{
    Require(index < m_record_count, nullptr, Trace("Leaf GetRecord : out of index"));
    return reinterpret_cast<Record*>(m_records + index * m_record_size);
};

size_t Leaf::Size()
{
    return m_record_count;
}

Record* Leaf::Insert(size_t index, const char* key, const void* data, size_t data_size)
{
    AssertRequire(index < m_record_count, "out of index");
    AssertRequire(key, "key is nullptr");
    Require(IsFull() == false, nullptr, Trace("leaf is full, fail to insert"));

    if (index >= m_cur_count)
    {
        m_cur_count++;
        GetRecord(index)->SetKey(key);
        GetRecord(index)->SetData(data, data_size);
        return GetRecord(index);
    }

    for (size_t new_end = m_cur_count; new_end > index; new_end--)
    {
        Record* new_record = GetRecord(new_end);
        Record* old_record = GetRecord(new_end - 1);
        new_record->Copy(old_record);
    }
    GetRecord(index)->SetKey(key);
    GetRecord(index)->SetData(data, data_size);
    m_cur_count++;
    return GetRecord(index);
}

void Leaf::Resize(size_t size)
{
    Require(size < m_record_count, , );
    m_cur_count = size;
}