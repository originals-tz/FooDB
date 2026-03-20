#ifndef FOODB_TABLE_H_
#define FOODB_TABLE_H_

#include <optional>
#include <fstream>
#include <string>
#include <unordered_map>

#include "catalog/row.h"
#include "store/bptree.h"

class Table
{
public:
    Table(std::string name, Schema schema);

    const std::string& Name() const;
    const Schema& GetSchema() const;

    bool Insert(Row row);
    std::optional<Row> GetRow(const std::string& primary_key) const;
    size_t Size() const;

private:
    bool LoadRows();
    bool FlushRows() const;
    std::optional<std::string> GetPrimaryKeyValue(const Row& row) const;
    std::string MakeIndexFileName(const std::string& name) const;
    std::string MakeDataFileName(const std::string& name) const;

    std::string m_name;
    std::string m_data_file;
    Schema m_schema;
    BPTree m_primary_index;
    std::unordered_map<std::string, Row> m_rows;
};

#endif
