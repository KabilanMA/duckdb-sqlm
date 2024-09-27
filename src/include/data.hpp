#pragma once

#include "duckdb/function/function.hpp"

namespace duckdb
{
    struct MutationTestFunctionData : public TableFunctionData
    {
        MutationTestFunctionData(string query) : original_query(query) {}

        string original_query;
        vector<string> mutated_queries;
        bool finished = false;
        size_t current_index = 0;
    };

} // namespace duckdb
