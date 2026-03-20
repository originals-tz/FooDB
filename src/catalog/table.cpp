#include "catalog/table.h"

#include <array>
#include <cstring>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <utility>

namespace
{
constexpr uint32_t kTableMagic = 0x54424C31;  // TBL1
constexpr uint32_t kTableVersion = 1;

void WriteUint32(std::ostream& out, uint32_t value)
{
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void WriteUint64(std::ostream& out, uint64_t value)
{
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

uint32_t ReadUint32(std::istream& in)
{
    uint32_t value = 0;
    in.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

uint64_t ReadUint64(std::istream& in)
{
    uint64_t value = 0;
    in.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}
}  // namespace

Table::Table(std::string name, Schema schema)
    : m_name(std::move(name))
    , m_data_file(MakeDataFileName(m_name))
    , m_schema(std::move(schema))
    , m_primary_index(MakeIndexFileName(m_name), 64)
{
    if (!m_schema.PrimaryKey())
    {
        throw std::invalid_argument("table schema requires a primary key");
    }
    if (!LoadRows())
    {
        throw std::runtime_error("failed to load table data");
    }
}

const std::string& Table::Name() const
{
    return m_name;
}

const Schema& Table::GetSchema() const
{
    return m_schema;
}

bool Table::Insert(Row row)
{
    if (!row.MatchesSchema(m_schema))
    {
        return false;
    }

    const std::optional<std::string> primary_key = GetPrimaryKeyValue(row);
    if (!primary_key)
    {
        return false;
    }

    const char* empty_value = "";
    if (!m_primary_index.Insert(*primary_key, empty_value, 0))
    {
        return false;
    }

    m_rows[*primary_key] = std::move(row);
    return FlushRows();
}

std::optional<Row> Table::GetRow(const std::string& primary_key) const
{
    const std::optional<Data> indexed = m_primary_index.Search(primary_key);
    if (!indexed)
    {
        return std::nullopt;
    }

    const auto it = m_rows.find(primary_key);
    if (it == m_rows.end())
    {
        return std::nullopt;
    }
    return it->second;
}

size_t Table::Size() const
{
    return m_rows.size();
}

bool Table::LoadRows()
{
    std::ifstream in(m_data_file, std::ios::binary);
    if (!in.good())
    {
        return true;
    }

    const uint32_t magic = ReadUint32(in);
    const uint32_t version = ReadUint32(in);
    const uint32_t column_count = ReadUint32(in);
    if (!in.good() || magic != kTableMagic || version != kTableVersion || column_count != m_schema.Size())
    {
        return false;
    }

    for (uint32_t i = 0; i < column_count; ++i)
    {
        const uint32_t name_size = ReadUint32(in);
        std::string name(name_size, '\0');
        in.read(name.data(), static_cast<std::streamsize>(name_size));
        const uint32_t type = ReadUint32(in);
        const uint64_t size = ReadUint64(in);
        const uint32_t nullable = ReadUint32(in);
        const uint32_t primary_key = ReadUint32(in);
        (void) type;
        (void) size;
        (void) nullable;
        (void) primary_key;
        if (!in.good())
        {
            return false;
        }
    }

    const uint64_t row_count = ReadUint64(in);
    for (uint64_t i = 0; i < row_count; ++i)
    {
        const uint64_t payload_size = ReadUint64(in);
        std::vector<uint8_t> payload(static_cast<size_t>(payload_size));
        in.read(reinterpret_cast<char*>(payload.data()), static_cast<std::streamsize>(payload_size));
        if (!in.good())
        {
            return false;
        }

        const std::optional<Row> row = Row::Deserialize(payload);
        if (!row || !row->MatchesSchema(m_schema))
        {
            return false;
        }

        const std::optional<std::string> primary_key = GetPrimaryKeyValue(*row);
        if (!primary_key)
        {
            return false;
        }

        m_rows[*primary_key] = *row;
        const char* empty_value = "";
        if (!m_primary_index.Insert(*primary_key, empty_value, 0))
        {
            return false;
        }
    }

    return true;
}

bool Table::FlushRows() const
{
    std::ofstream out(m_data_file, std::ios::binary | std::ios::trunc);
    if (!out.good())
    {
        return false;
    }

    WriteUint32(out, kTableMagic);
    WriteUint32(out, kTableVersion);
    WriteUint32(out, static_cast<uint32_t>(m_schema.Size()));
    for (const Column& column : m_schema.Columns())
    {
        WriteUint32(out, static_cast<uint32_t>(column.m_name.size()));
        out.write(column.m_name.data(), static_cast<std::streamsize>(column.m_name.size()));
        WriteUint32(out, static_cast<uint32_t>(column.m_type));
        WriteUint64(out, static_cast<uint64_t>(column.m_size));
        WriteUint32(out, column.m_nullable ? 1U : 0U);
        WriteUint32(out, column.m_primary_key ? 1U : 0U);
    }

    WriteUint64(out, static_cast<uint64_t>(m_rows.size()));
    for (const auto& [key, row] : m_rows)
    {
        (void) key;
        const std::vector<uint8_t> payload = row.Serialize();
        WriteUint64(out, static_cast<uint64_t>(payload.size()));
        out.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
    }

    return out.good();
}

std::optional<std::string> Table::GetPrimaryKeyValue(const Row& row) const
{
    const Column* pk = m_schema.PrimaryKey();
    if (!pk)
    {
        return std::nullopt;
    }

    const std::optional<std::string> value = row.GetString(pk->m_name);
    if (!value || value->empty())
    {
        return std::nullopt;
    }
    return value;
}

std::string Table::MakeIndexFileName(const std::string& name) const
{
    return name + ".idx";
}

std::string Table::MakeDataFileName(const std::string& name) const
{
    return name + ".tbl";
}
