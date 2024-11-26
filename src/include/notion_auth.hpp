#pragma once

#include <string>
#include "duckdb/main/database.hpp"

namespace duckdb
{

    std::string read_token_from_file(const std::string &file_path);

    std::string InitiateOAuthFlow();

    struct CreateNotionSecretFunctions
    {
    public:
        //! Register all CreateSecretFunctions
        static void Register(DatabaseInstance &instance);
    };

} // namespace duckdb