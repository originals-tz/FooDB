#ifndef _RECORD_H_
#define _RECORD_H_
#include <string>

struct Data
{
    size_t m_data_size;
    const char* m_data;
};

class Record
{
public:
    /**
     * @brief base data of bptree
     **/
    Record();

    /**
     * @brief copy a record
     **/
    void Copy(const Record* record);

    /**
     * @brief init the record
     * @param raw where record will be place
     **/
    static void Init(size_t key_size, size_t data_size, char* raw);

    /**
     * @brief set the key for record
     **/
    void SetKey(const std::string& key);

    /**
     * @brief set the record data
     **/
    void SetData(const void* data, size_t size);

    /*
     * @brief get the key of record
     */
    char* GetKey();

    /**
     * @brief get data pointer
     **/
    char* GetRawData();

    /**
     * @brief get the data of record
     **/
    Data GetData();

    /**
     * @brief get key size
     **/
    size_t GetKeySize();

    /**
     * @brief get data size
     **/
    size_t GetDataSize();

private:
    char* m_key;
    size_t m_key_size;

    char* m_data;
    size_t m_data_size;
};

#endif
