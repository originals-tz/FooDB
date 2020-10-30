#include "record.h"

#include <cstring>

#include "util/macro.h"
#include "util/trace.h"

Record::Record()
    : m_key(nullptr)
    , m_key_size(0)
    , m_data(nullptr)
    , m_data_size(0)
{
}

void Record::Copy(const Record* record)
{
    strncpy(m_key, record->m_key, record->m_key_size);
    memcpy(m_data, record->m_data, record->m_data_size);
}

void Record::Init(size_t key_size, size_t data_size, char* raw)
{
    AssertRequire(key_size > 0 && data_size > 0 && raw, "record : init size <= 0 or nullptr");

    Record* record = reinterpret_cast<Record*>(raw);
    record->m_key_size = key_size;
    record->m_data_size = data_size;

    record->m_key = raw + sizeof(Record);
    record->m_data = raw + sizeof(Record) + key_size;
}

void Record::SetKey(const std::string& key)
{
    AssertRequire(m_key && m_key_size, "record not init");
    AssertRequire(key.size() + 1 <= m_key_size, "key size is too big");

    strncpy(m_key, key.c_str(), key.size() + 1);
}

void Record::SetData(const void* data, size_t size)
{
    AssertRequire(m_data && m_data_size, "record not init");
    AssertRequire(size <= m_data_size, "data size is too big");

    memcpy(m_data, data, size);
}

char* Record::GetKey()
{
    return m_key;
}

char* Record::GetRawData()
{
    return m_data;
}

Data Record::GetData()
{
    AssertRequire(m_data && m_data_size, "record not init");
    Data data;
    data.m_data = m_data;
    data.m_data_size = m_data_size;
    return data;
}

size_t Record::GetKeySize()
{
    return m_key_size;
}

size_t Record::GetDataSize()
{
    return m_data_size;
}
