#include "catalog/schema.h"

#include <cassert>
#include <string>
#include <utility>

Schema::Schema(std::vector<Column> columns)
    : m_columns(std::move(columns))
{
}

const std::vector<Column>& Schema::Columns() const
{
    return m_columns;
}

const Column* Schema::PrimaryKey() const
{
    for (const Column& column : m_columns)
    {
        if (column.m_primary_key)
        {
            return &column;
        }
    }
    return nullptr;
}

const Column* Schema::FindColumn(const std::string& name) const
{
    for (const Column& column : m_columns)
    {
        if (column.m_name == name)
        {
            return &column;
        }
    }
    return nullptr;
}

bool Schema::Matches(const Schema& other) const
{
    if (m_columns.size() != other.m_columns.size())
    {
        return false;
    }

    for (size_t i = 0; i < m_columns.size(); ++i)
    {
        const Column& lhs = m_columns[i];
        const Column& rhs = other.m_columns[i];
        if (lhs.m_name != rhs.m_name || lhs.m_type != rhs.m_type || lhs.m_size != rhs.m_size ||
            lhs.m_nullable != rhs.m_nullable || lhs.m_primary_key != rhs.m_primary_key)
        {
            return false;
        }
    }
    return true;
}

size_t Schema::Size() const
{
    return m_columns.size();
}

bool Schema::Empty() const
{
    return m_columns.empty();
}
