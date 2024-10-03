
#include "mutation_generator.hpp"

namespace duckdb
{

    void MutationTreeNode::AddChild(const SQLStatement &child_query)
    {
        auto child_node = make_uniq<MutationTreeNode>(child_query);
        children.push_back(std::move(child_node));
    }

    void MutationTreeNode::PrintTree(int depth)
    {
        for (int i = 0; i < depth; i++)
        {
            std::cout << "  ";
        }
        std::cout << "Query: " << statement.ToString() << std::endl;

        for (const auto &child : children)
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

    MutationTreeNode::MutationTreeNode(const SQLStatement &statement) : statement(statement)
    {
    }

    MudStatementGenerator::MudStatementGenerator()
    {
    }

    MudStatementGenerator::~MudStatementGenerator()
    {
    }

    bool MudStatementGenerator::DistinctModifierExist(vector<unique_ptr<ResultModifier>> &modifiers, bool remove_modifier)
    {
        for (idx_t modifier_idx = modifiers.size(); modifier_idx > 0; modifier_idx--)
        {
            auto &modifier = *modifiers[modifier_idx - 1];
            if (modifier.type == ResultModifierType::DISTINCT_MODIFIER)
            {
                auto &distinct_modifier = modifier.Cast<DistinctModifier>();
                if (distinct_modifier.distinct_on_targets.empty())
                {
                    // we have a DISTINCT without an ON clause - this distinct does not need to be added
                    if (remove_modifier)
                        modifiers.erase(modifiers.begin() + (modifier_idx - 1));
                    return true;
                }
            }
            else if (modifier.type == ResultModifierType::LIMIT_MODIFIER ||
                     modifier.type == ResultModifierType::LIMIT_PERCENT_MODIFIER)
            {
                // we encountered a LIMIT or LIMIT PERCENT - these change the result of DISTINCT, so we do need to push a
                // DISTINCT relation
                return false;
            }
        }
        return false;
    }

    MutationTreeNode *MudStatementGenerator::GenerateSelectMutations(SelectStatement &statement, MutationTestFunctionData *functionData, MutationTreeNode *parent_node)
    {
        D_ASSERT(statement.TYPE == StatementType::SELECT_STATEMENT);

        functionData->mutated_queries.push_back(statement.ToString());
        if (!parent_node)
        { // calling the select mutation generator for the first time.
            // therefore we have to create the root node, because root node will initially be nullptr
            parent_node = new MutationTreeNode(statement);
            parent_node->AddChild(statement);

            const auto &a = statement.Copy();
            const auto &dis_statement = a->Cast<SelectStatement>();

            if (!DistinctModifierExist(dis_statement.node->modifiers, true))
            {
                // remove the distinct
                dis_statement.node->modifiers.push_back(make_uniq<DistinctModifier>());
            }

            functionData->mutated_queries.push_back(dis_statement.ToString());
        }

        return parent_node;
    }

    void GenerateMutations(duckdb_libpgquery::PGList *parse_tree_list, MutationTestFunctionData *functionData)
    {
        int i = 0;
        for (auto entry = parse_tree_list->head; entry != nullptr; entry = entry->next)
        {

            auto n = Transformer::PGPointerCast<duckdb_libpgquery::PGNode>(entry->data.ptr_value);
            auto statement_type = findStatementType(*n);

            vector<unique_ptr<SQLStatement>> statements;
            ParserOptions parserOptions;
            Transformer transformer(parserOptions);

            transformer.TransformParseTree(parse_tree_list, statements);
            MudStatementGenerator statement_generator;

            switch (statement_type)
            {
            case duckdb_libpgquery::T_PGSelectStmt:
            {
                auto &select_statement = statements[i]->Cast<SelectStatement>();
                statement_generator.GenerateSelectMutations(select_statement, functionData);
                break;
            }
            default:
                std::cout << "Undefined Query Type: " << static_cast<int>(n->type) << std::endl;
                break;
            }
            i++;
        }
    }
}