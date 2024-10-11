#pragma once

#include "duckdb/function/function.hpp"

namespace duckdb
{
    struct MutationTestFunctionData : public TableFunctionData
    {
        MutationTestFunctionData(string query) : original_query(query) {}

        string original_query;
        vector<unique_ptr<SQLStatement>> mutated_queries;
        bool finished = false;
        size_t current_index = 0;
        unique_ptr<QueryResult> original_result = nullptr;
    };

} // namespace duckdb
