#ifndef FOODB_ROW_H_
#define FOODB_ROW_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "catalog/schema.h"

class Row
{
public:
    Row() = default;
    explicit Row(Schema schema);

    const Schema& GetSchema() const;
    bool Has(const std::string& column) const;
    bool MatchesSchema(const Schema& schema) const;

    bool SetInt64(const std::string& column, int64_t value);
    bool SetString(const std::string& column, std::string value);
    bool SetBytes(const std::string& column, std::vector<uint8_t> value);

    std::optional<int64_t> GetInt64(const std::string& column) const;
    std::optional<std::string> GetString(const std::string& column) const;
    std::optional<std::vector<uint8_t>> GetBytes(const std::string& column) const;

    std::vector<uint8_t> Serialize() const;
    static std::optional<Row> Deserialize(const std::vector<uint8_t>& payload);

    const std::unordered_map<std::string, std::vector<uint8_t>>& Values() const;

private:
    Schema m_schema;
    std::unordered_map<std::string, std::vector<uint8_t>> m_values;
};

#endif
