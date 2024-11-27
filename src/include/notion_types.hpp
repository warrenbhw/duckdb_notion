#pragma once

#include <string>
#include "duckdb/common/types.hpp"

namespace duckdb {

enum class NotionPropertyType {
    TEXT,
    NUMBER,
    DATE,
    CHECKBOX,
    SELECT,
    MULTI_SELECT,
    RELATION
};

struct NotionProperty {
    std::string name;
    NotionPropertyType type;
};

} // namespace duckdb 