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
#include "../third_party/transform/include/transformer.hpp"
#include "duckdb/parser/transformer.hpp"

namespace duckdb
{
    typedef enum MutationOperatorTag
    {
        SEL,
        OR
    } MutationOperatorTag;

    // MutationTreeNode class to represent a single SQL query node
    class MutationTreeNode
    {
    public:
        const SQLStatement &statement;                           // SQL query string
        std::vector<std::unique_ptr<MutationTreeNode>> children; // Pointers to child nodes

        explicit MutationTreeNode(const SQLStatement &statement);

        // Add a child node
        void AddChild(const SQLStatement &child_query);

        // Recursively print the query tree
        void PrintTree(int depth = 0);
    };

    class MudStatementGenerator
    {
    public:
        MudStatementGenerator();
        ~MudStatementGenerator();

    public:
        MutationTreeNode *GenerateSelectMutations(SelectStatement &statement, MutationTestFunctionData *functionData, MutationTreeNode *parent_node = nullptr);

    private:
        bool DistinctModifierExist(vector<unique_ptr<ResultModifier>> &modifiers, bool remove_modifier = false);
    };

    void GenerateMutations(duckdb_libpgquery::PGList *parse_tree_list, MutationTestFunctionData *functionData);
    // void GenerateSelectMutations(duckdb_libpgquery::PGList *parse_tree_list, MutationTestFunctionData *functionData);
    // void GenerateSelectMutations(duckdb_libpgquery::PGSelectStmt &stmt, MutationTestFunctionData *functionData);
    // void GenerateSelectMutations(duckdb_libpgquery::PGListCell *entry, unique_ptr<SQLStatement> statement, MutationTestFunctionData *functionData);
}