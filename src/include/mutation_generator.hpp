#pragma once

#include "duckdb.hpp"
#include "data.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>
#include "duckdb/parser/parser_options.hpp"
#include "duckdb/parser/parser.hpp"
#include "duckdb/planner/planner.hpp"
#include "postgres_parser.hpp"
#include "duckdb/parser/transformer.hpp"

namespace duckdb
{
    // MutationTreeNode class to represent a single SQL query node
    class MutationTreeNode
    {
    public:
        std::string query;                                       // SQL query string
        std::vector<std::unique_ptr<MutationTreeNode>> children; // Pointers to child nodes

        explicit MutationTreeNode(const std::string &query) : query(query) {}

        // Add a child node
        void AddChild(const std::string &child_query);

        // Recursively print the query tree
        void PrintTree(int depth = 0);
    };

    void GenerateMutations(duckdb_libpgquery::PGList *parse_tree_list, MutationTestFunctionData *functionData);
    void GenerateSelectMutations(duckdb_libpgquery::PGList *parse_tree_list, MutationTestFunctionData *functionData);
    void GenerateSelectMutations(duckdb_libpgquery::PGSelectStmt &stmt, MutationTestFunctionData *functionData);
}