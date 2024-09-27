
#include "mutation_generator.hpp"

namespace duckdb
{

    void MutationTreeNode::AddChild(const std::string &child_query)
    {
        children.push_back(make_uniq<MutationTreeNode>(child_query));
    }

    void MutationTreeNode::PrintTree(int depth)
    {
        for (int i = 0; i < depth; ++i)
            std::cout << "  ";
        std::cout << query << std::endl;
        for (auto &child : children)
        {
            child->PrintTree(depth + 1);
        }
    }

    static duckdb_libpgquery::PGNodeTag findStatementType(duckdb_libpgquery::PGNode &stmt)
    {
        switch (stmt.type)
        {
        case duckdb_libpgquery::T_PGRawStmt:
        {
            auto &raw_stmt = Transformer::PGCast<duckdb_libpgquery::PGRawStmt>(stmt);
            return findStatementType(*raw_stmt.stmt);
        }
        default:
            return stmt.type;
        }
    }

    void GenerateSelectMutations(duckdb_libpgquery::PGList *parse_tree_list, MutationTestFunctionData *functionData)
    {
        vector<unique_ptr<SQLStatement>> statements;
        ParserOptions parserOptions;
        Transformer transformer(parserOptions);

        transformer.TransformParseTree(parse_tree_list, statements);
        vector<string> mutations;
        mutations.push_back(statements[0]->ToString());

        string mutation2 = statements[0]->ToString();
        size_t pos = mutation2.find("SELECT");
        if (pos != string::npos)
        {
            mutation2.replace(pos, 6, "SELECT DISTINCT");
            mutations.push_back(mutation2);
        }
        functionData->mutated_queries = mutations;
        // std::cout << "Mutated query: " << functionData->mutated_queries.get(0) << std::endl;
    }

    void GenerateSelectMutations(duckdb_libpgquery::PGSelectStmt &stmt, MutationTestFunctionData *functionData)
    {
        D_ASSERT(stmt.type == duckdb_libpgquery::T_PGSelectStmt);
        vector<string> mutations;
        MutationTreeNode mutation_tree(functionData->original_query);
        mutation_tree.AddChild(functionData->original_query);

        if (stmt.distinctClause)
        {
            stmt.distinctClause = nullptr;
        }

        functionData->mutated_queries.push_back("DUMMY A");
        // std::cout << "Mutated query: " << functionData->mutated_queries.get(0) << std::endl;
    }

    void GenerateMutations(duckdb_libpgquery::PGList *parse_tree_list, MutationTestFunctionData *functionData)
    {
        for (auto entry = parse_tree_list->head; entry != nullptr; entry = entry->next)
        {
            auto n = Transformer::PGPointerCast<duckdb_libpgquery::PGNode>(entry->data.ptr_value);
            auto statement_type = findStatementType(*n);
            switch (statement_type)
            {
            case duckdb_libpgquery::T_PGSelectStmt:
                GenerateSelectMutations(Transformer::PGCast<duckdb_libpgquery::PGSelectStmt>(*n), functionData);
                // GenerateSelectMutations(parse_tree_list, functionData);
                break;
            default:
                std::cout << "Undefined Query Type: " << static_cast<int>(n->type) << std::endl;
                break;
            }
        }
    }
}