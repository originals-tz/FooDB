#include "catalog/row.h"

#include <cstring>
#include <utility>

namespace
{
constexpr uint32_t kRowMagic = 0x524F5731;  // ROW1
constexpr uint32_t kRowVersion = 1;

void WriteUint32(std::vector<uint8_t>& buffer, uint32_t value)
{
    const size_t offset = buffer.size();
    buffer.resize(offset + sizeof(value));
    std::memcpy(buffer.data() + offset, &value, sizeof(value));
}

void WriteUint64(std::vector<uint8_t>& buffer, uint64_t value)
{
    const size_t offset = buffer.size();
    buffer.resize(offset + sizeof(value));
    std::memcpy(buffer.data() + offset, &value, sizeof(value));
}

uint32_t ReadUint32(const std::vector<uint8_t>& buffer, size_t& offset)
{
    uint32_t value = 0;
    std::memcpy(&value, buffer.data() + offset, sizeof(value));
    offset += sizeof(value);
    return value;
}

uint64_t ReadUint64(const std::vector<uint8_t>& buffer, size_t& offset)
{
    uint64_t value = 0;
    std::memcpy(&value, buffer.data() + offset, sizeof(value));
    offset += sizeof(value);
    return value;
}

void WriteString(std::vector<uint8_t>& buffer, const std::string& value)
{
    WriteUint32(buffer, static_cast<uint32_t>(value.size()));
    const size_t offset = buffer.size();
    buffer.resize(offset + value.size());
    std::memcpy(buffer.data() + offset, value.data(), value.size());
}

std::optional<std::string> ReadString(const std::vector<uint8_t>& buffer, size_t& offset)
{
    if (offset + sizeof(uint32_t) > buffer.size())
    {
        return std::nullopt;
    }

    const uint32_t size = ReadUint32(buffer, offset);
    if (offset + size > buffer.size())
    {
        return std::nullopt;
    }

    std::string value(size, '\0');
    std::memcpy(value.data(), buffer.data() + offset, size);
    offset += size;
    return value;
}
}  // namespace

Row::Row(Schema schema)
    : m_schema(std::move(schema))
{
}

const Schema& Row::GetSchema() const
{
    return m_schema;
}

bool Row::Has(const std::string& column) const
{
    return m_values.find(column) != m_values.end();
}

bool Row::MatchesSchema(const Schema& schema) const
{
    return m_schema.Matches(schema);
}

bool Row::SetInt64(const std::string& column, int64_t value)
{
    const Column* meta = m_schema.FindColumn(column);
    if (!meta || meta->m_type != ColumnType::kInt64)
    {
        return false;
    }

    std::vector<uint8_t> bytes(sizeof(value));
    std::memcpy(bytes.data(), &value, sizeof(value));
    m_values[column] = std::move(bytes);
    return true;
}

bool Row::SetString(const std::string& column, std::string value)
{
    const Column* meta = m_schema.FindColumn(column);
    if (!meta || meta->m_type != ColumnType::kString)
    {
        return false;
    }

    std::vector<uint8_t> bytes(value.begin(), value.end());
    m_values[column] = std::move(bytes);
    return true;
}

bool Row::SetBytes(const std::string& column, std::vector<uint8_t> value)
{
    const Column* meta = m_schema.FindColumn(column);
    if (!meta || meta->m_type != ColumnType::kBytes)
    {
        return false;
    }

    m_values[column] = std::move(value);
    return true;
}

std::optional<int64_t> Row::GetInt64(const std::string& column) const
{
    const auto it = m_values.find(column);
    if (it == m_values.end() || it->second.size() != sizeof(int64_t))
    {
        return std::nullopt;
    }

    int64_t value = 0;
    std::memcpy(&value, it->second.data(), sizeof(value));
    return value;
}

std::optional<std::string> Row::GetString(const std::string& column) const
{
    const auto it = m_values.find(column);
    if (it == m_values.end())
    {
        return std::nullopt;
    }
    return std::string(it->second.begin(), it->second.end());
}

std::optional<std::vector<uint8_t>> Row::GetBytes(const std::string& column) const
{
    const auto it = m_values.find(column);
    if (it == m_values.end())
    {
        return std::nullopt;
    }
    return it->second;
}

std::vector<uint8_t> Row::Serialize() const
{
    std::vector<uint8_t> buffer;
    buffer.reserve(64);

    WriteUint32(buffer, kRowMagic);
    WriteUint32(buffer, kRowVersion);
    WriteUint32(buffer, static_cast<uint32_t>(m_schema.Size()));

    for (const Column& column : m_schema.Columns())
    {
        WriteString(buffer, column.m_name);
        WriteUint32(buffer, static_cast<uint32_t>(column.m_type));
        WriteUint64(buffer, static_cast<uint64_t>(column.m_size));
        WriteUint32(buffer, column.m_nullable ? 1U : 0U);
        WriteUint32(buffer, column.m_primary_key ? 1U : 0U);
    }

    for (const Column& column : m_schema.Columns())
    {
        const auto it = m_values.find(column.m_name);
        const bool has_value = it != m_values.end();
        WriteUint32(buffer, has_value ? 1U : 0U);
        if (!has_value)
        {
            continue;
        }

        WriteUint32(buffer, static_cast<uint32_t>(it->second.size()));
        const size_t offset = buffer.size();
        buffer.resize(offset + it->second.size());
        std::memcpy(buffer.data() + offset, it->second.data(), it->second.size());
    }

    return buffer;
}

std::optional<Row> Row::Deserialize(const std::vector<uint8_t>& payload)
{
    if (payload.size() < sizeof(uint32_t) * 3)
    {
        return std::nullopt;
    }

    size_t offset = 0;
    const uint32_t magic = ReadUint32(payload, offset);
    const uint32_t version = ReadUint32(payload, offset);
    const uint32_t column_count = ReadUint32(payload, offset);
    if (magic != kRowMagic || version != kRowVersion)
    {
        return std::nullopt;
    }

    std::vector<Column> columns;
    columns.reserve(column_count);
    for (uint32_t i = 0; i < column_count; ++i)
    {
        const std::optional<std::string> name = ReadString(payload, offset);
        if (!name || offset + sizeof(uint32_t) * 3 + sizeof(uint64_t) > payload.size())
        {
            return std::nullopt;
        }

        Column column {};
        column.m_name = *name;
        column.m_type = static_cast<ColumnType>(ReadUint32(payload, offset));
        column.m_size = static_cast<size_t>(ReadUint64(payload, offset));
        column.m_nullable = ReadUint32(payload, offset) != 0;
        column.m_primary_key = ReadUint32(payload, offset) != 0;
        columns.push_back(std::move(column));
    }

    Row row(Schema(std::move(columns)));
    for (const Column& column : row.GetSchema().Columns())
    {
        if (offset + sizeof(uint32_t) > payload.size())
        {
            return std::nullopt;
        }

        const uint32_t has_value = ReadUint32(payload, offset);
        if (!has_value)
        {
            continue;
        }

        if (offset + sizeof(uint32_t) > payload.size())
        {
            return std::nullopt;
        }

        const uint32_t value_size = ReadUint32(payload, offset);
        if (offset + value_size > payload.size())
        {
            return std::nullopt;
        }

        std::vector<uint8_t> value(value_size);
        std::memcpy(value.data(), payload.data() + offset, value_size);
        offset += value_size;

        if (column.m_type == ColumnType::kInt64 && value_size != sizeof(int64_t))
        {
            return std::nullopt;
        }
        if (column.m_type == ColumnType::kString && column.m_size != 0 && value_size > column.m_size)
        {
            return std::nullopt;
        }
        if (column.m_type == ColumnType::kBytes && column.m_size != 0 && value_size > column.m_size)
        {
            return std::nullopt;
        }

        row.m_values[column.m_name] = std::move(value);
    }

    return row;
}

const std::unordered_map<std::string, std::vector<uint8_t>>& Row::Values() const
{
    return m_values;
}
