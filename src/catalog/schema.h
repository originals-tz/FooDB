#ifndef FOODB_SCHEMA_H_
#define FOODB_SCHEMA_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class ColumnType : uint8_t
{
    kInt64 = 1,
    kString = 2,
    kBytes = 3,
};

struct Column
{
    std::string m_name;
    ColumnType m_type { ColumnType::kBytes };
    size_t m_size { 0 };
    bool m_nullable { true };
    bool m_primary_key { false };
};

class Schema
{
public:
    Schema() = default;
    explicit Schema(std::vector<Column> columns);

    const std::vector<Column>& Columns() const;
    const Column* PrimaryKey() const;
    const Column* FindColumn(const std::string& name) const;
    bool Matches(const Schema& other) const;
    size_t Size() const;
    bool Empty() const;

private:
    std::vector<Column> m_columns;
};

#endif
