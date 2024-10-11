
#include "mutation_generator.hpp"

namespace duckdb
{

    void MutationTreeNode::AddChild(const SQLStatement &child_query)
    {
        auto child_node = make_uniq<MutationTreeNode>(child_query);
        children.push_back(std::move(child_node));
    }

    void MutationTreeNode::AddChild(const string &child_query)
    {
        Parser parser;
        parser.ParseQuery(child_query);
        D_ASSERT(parser.statements.size() == 1);

        auto &first_stmt = parser.statements.get(0);
        auto child_node = make_uniq<MutationTreeNode>(*first_stmt);
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
        std::cout << "Calling findStatementType() with: " << stmt.type << std::endl;
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

    /**
     * @brief Check whether the given modifier has a distinct modifier; return true if it has distinct modifier, false otherwise.
     *
     * @param modifiers input modifier to check whether distinct modifier exists
     * @param remove_modifier input to confirm whether to delete the distinct modifier from the given modifiers vector if it exists.
     * @return true; if distinct modifier exists
     * @return false: otherwise
     */
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

    ExpressionType GetRelationalOperationType(vector<ExpressionType> except)
    {
        if (std::find(except.begin(), except.end(), ExpressionType::COMPARE_LESSTHAN) == except.end())
        {
            return ExpressionType::COMPARE_LESSTHAN;
        }
        if (std::find(except.begin(), except.end(), ExpressionType::COMPARE_LESSTHANOREQUALTO) == except.end())
        {
            return ExpressionType::COMPARE_LESSTHANOREQUALTO;
        }
        if (std::find(except.begin(), except.end(), ExpressionType::COMPARE_GREATERTHAN) == except.end())
        {
            return ExpressionType::COMPARE_GREATERTHAN;
        }
        if (std::find(except.begin(), except.end(), ExpressionType::COMPARE_GREATERTHANOREQUALTO) == except.end())
        {
            return ExpressionType::COMPARE_GREATERTHANOREQUALTO;
        }
        if (std::find(except.begin(), except.end(), ExpressionType::COMPARE_EQUAL) == except.end())
        {
            return ExpressionType::COMPARE_EQUAL;
        }
        if (std::find(except.begin(), except.end(), ExpressionType::COMPARE_NOTEQUAL) == except.end())
        {
            return ExpressionType::COMPARE_NOTEQUAL;
        }
        return ExpressionType::INVALID;
    }

    void MutateParsedExpression(MutationTreeNode *parent_node, SQLStatement *stmt, MutationTestFunctionData *functionData, ParsedExpression *expression, vector<ExpressionType> except_mutation = {})
    {
        switch (expression->type)
        {
        case ExpressionType::COMPARE_LESSTHAN:
        case ExpressionType::COMPARE_LESSTHANOREQUALTO:
        case ExpressionType::COMPARE_GREATERTHAN:
        case ExpressionType::COMPARE_GREATERTHANOREQUALTO:
        case ExpressionType::COMPARE_EQUAL:
        case ExpressionType::COMPARE_NOTEQUAL:
        {
            if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_LESSTHAN) == except_mutation.end())
            {
                expression->type = ExpressionType::COMPARE_LESSTHAN;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                parent_node->AddChild(*(stmt->Copy()));
            }
            if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_LESSTHANOREQUALTO) == except_mutation.end())
            {
                expression->type = ExpressionType::COMPARE_LESSTHANOREQUALTO;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                parent_node->AddChild(*(stmt->Copy()));
            }
            if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_GREATERTHAN) == except_mutation.end())
            {
                expression->type = ExpressionType::COMPARE_GREATERTHAN;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                parent_node->AddChild(*(stmt->Copy()));
            }
            if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_GREATERTHANOREQUALTO) == except_mutation.end())
            {
                expression->type = ExpressionType::COMPARE_GREATERTHANOREQUALTO;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                parent_node->AddChild(*(stmt->Copy()));
            }
            if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_EQUAL) == except_mutation.end())
            {
                expression->type = ExpressionType::COMPARE_EQUAL;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                parent_node->AddChild(*(stmt->Copy()));
            }
            if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_NOTEQUAL) == except_mutation.end())
            {
                expression->type = ExpressionType::COMPARE_NOTEQUAL;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                parent_node->AddChild(*(stmt->Copy()));
            }
            break;
        }
        case ExpressionType::CONJUNCTION_AND:
        case ExpressionType::CONJUNCTION_OR:
        {
            auto *conjunction_expr = dynamic_cast<ConjunctionExpression *>(expression);
            if (conjunction_expr->children.size() == 2)
            {
                auto &left = conjunction_expr->children.get(0);
                std::cout << "Left of the parsed expression: " << ExpressionTypeToString(left->type) << std::endl;
                vector<ExpressionType> left_except_mutations = {left->type};
                MutateParsedExpression(parent_node, stmt, functionData, left.get(), left_except_mutations);
                left_except_mutations.push_back(left->type);

                auto &right = conjunction_expr->children.get(1);
                vector<ExpressionType> right_except_mutations = {right->type};
                MutateParsedExpression(parent_node, stmt, functionData, right.get(), right_except_mutations);
                right_except_mutations.push_back(right->type);

                MutateParsedExpression(parent_node, stmt, functionData, left.get(), left_except_mutations);
                left_except_mutations.push_back(left->type);
                MutateParsedExpression(parent_node, stmt, functionData, right.get(), right_except_mutations);
                right_except_mutations.push_back(right->type);
                MutateParsedExpression(parent_node, stmt, functionData, left.get(), left_except_mutations);
                left_except_mutations.push_back(left->type);
                MutateParsedExpression(parent_node, stmt, functionData, right.get(), right_except_mutations);
                right_except_mutations.push_back(right->type);
                MutateParsedExpression(parent_node, stmt, functionData, left.get(), left_except_mutations);
                left_except_mutations.push_back(left->type);
                MutateParsedExpression(parent_node, stmt, functionData, right.get(), right_except_mutations);
                right_except_mutations.push_back(right->type);
                MutateParsedExpression(parent_node, stmt, functionData, left.get(), left_except_mutations);
                left_except_mutations.push_back(left->type);
                MutateParsedExpression(parent_node, stmt, functionData, right.get(), right_except_mutations);
                right_except_mutations.push_back(right->type);
            }
            else
            {
                        }
            // for (auto &child : conjunction_expr->children)
            // {
            //     MutateParsedExpression(parent_node, stmt, functionData, child.get(), {});
            // }

            // auto &left = conjunction_expr->children.get(0);
            // vector<ExpressionType> left_except_mutations = {left->type};
            // std::cout << "Child count: " << conjunction_expr->children.size();
            // std::cout << "Left child statment type of AND conjunction" << ExpressionTypeToString(left->type) << std::endl;
            // MutateParsedExpression(parent_node, stmt, functionData, left.get(), left_except_mutations);

            // auto &right = conjunction_expr->children.get(1);
            // vector<ExpressionType> right_except_mutations = {right->type};
            // std::cout << "Right child statment type of AND conjunction" << ExpressionTypeToString(right->type) << std::endl;
            // MutateParsedExpression(parent_node, stmt, functionData, right.get(), right_except_mutations);

            // left_except_mutations.push_back(GetRelationalOperationType(left_except_mutations));
            // MutateParsedExpression(parent_node, stmt, functionData, left.get(), left_except_mutations);

            // MutateParsedExpression(parent_node, stmt, functionData, left.get());
            // MutateParsedExpression(parent_node, stmt, functionData, right.get(), 2);
            // MutateParsedExpression(parent_node, stmt, functionData, left.get());
            // MutateParsedExpression(parent_node, stmt, functionData, right.get(), 3);
            // MutateParsedExpression(parent_node, stmt, functionData, left.get());
            // MutateParsedExpression(parent_node, stmt, functionData, right.get(), 4);
            // MutateParsedExpression(parent_node, stmt, functionData, left.get());
            // MutateParsedExpression(parent_node, stmt, functionData, right.get(), 5);
            // MutateParsedExpression(parent_node, stmt, functionData, left.get());

            // for (auto &child : parent_node->children)
            // {
            //     auto &right = conjunction_expr->children.get(1);
            //     MutateParsedExpression(child.get(), stmt, functionData, right.get());
            //     // expression->type = ExpressionType::CONJUNCTION_OR;
            //     // functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
            //     // child->AddChild(*(stmt->Copy()));
            // }
            break;
        }
        default:
        {
            throw InternalException("Implemented expression type");
            break;
        }
        }
    }

    void MudStatementGenerator::MutateWhereClauseStatement(MutationTreeNode *parent_node, const unique_ptr<SQLStatement> &stmt, MutationTestFunctionData *functionData)
    {
        if (stmt->type == StatementType::SELECT_STATEMENT)
        {
            auto &select_stmt = stmt->Cast<SelectStatement>();
            auto &select_stmt_node = select_stmt.node->Cast<SelectNode>();
            switch (select_stmt_node.where_clause->type)
            {
            case ExpressionType::COMPARE_LESSTHAN:
            {
                select_stmt_node.where_clause->type = ExpressionType::COMPARE_EQUAL;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_GREATERTHAN;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_LESSTHANOREQUALTO;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_GREATERTHANOREQUALTO;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_NOTEQUAL;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));
                break;
            }
            case ExpressionType::COMPARE_GREATERTHAN:
            {
                select_stmt_node.where_clause->type = ExpressionType::COMPARE_EQUAL;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_LESSTHAN;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_LESSTHANOREQUALTO;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_GREATERTHANOREQUALTO;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_NOTEQUAL;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));
                break;
            }
            case ExpressionType::COMPARE_LESSTHANOREQUALTO:
            {
                select_stmt_node.where_clause->type = ExpressionType::COMPARE_EQUAL;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_LESSTHAN;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_GREATERTHAN;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_GREATERTHANOREQUALTO;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_NOTEQUAL;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));
                break;
            }
            case ExpressionType::COMPARE_GREATERTHANOREQUALTO:
            {
                select_stmt_node.where_clause->type = ExpressionType::COMPARE_EQUAL;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_LESSTHAN;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_GREATERTHAN;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_LESSTHANOREQUALTO;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_NOTEQUAL;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));
                break;
            }
            case ExpressionType::COMPARE_NOTEQUAL:
            {
                select_stmt_node.where_clause->type = ExpressionType::COMPARE_EQUAL;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_LESSTHAN;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_GREATERTHAN;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_LESSTHANOREQUALTO;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));

                select_stmt_node.where_clause->type = ExpressionType::COMPARE_GREATERTHANOREQUALTO;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));
                break;
            }
            case ExpressionType::CONJUNCTION_AND:
            {
                std::cout << "Conjunction And in where clause" << std::endl;
                select_stmt_node.where_clause->type = ExpressionType::CONJUNCTION_OR;
                std::cout << "Mutated to OR where clause" << std::endl;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                std::cout << "Mutated query pushed to the list" << std::endl;
                parent_node->AddChild(*(select_stmt.Copy()));
                break;
            }
            case ExpressionType::CONJUNCTION_OR:
            {
                auto *conjunction_expr = dynamic_cast<ConjunctionExpression *>(select_stmt_node.where_clause.get());
                auto &left = conjunction_expr->children.get(0);
                auto &right = conjunction_expr->children.get(1);
                std::cout << "Left child statment type of OR conjunction" << ExpressionTypeToString(left->type) << std::endl;
                switch (left->type)
                {
                case ExpressionType::COMPARE_LESSTHAN:
                {
                }
                default:
                    break;
                }

                select_stmt_node.where_clause->type = ExpressionType::CONJUNCTION_AND;
                functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(select_stmt.Copy().release()))));
                parent_node->AddChild(*(select_stmt.Copy()));
            }
            default:
            {
                std::cout << ExpressionTypeToString(select_stmt_node.where_clause->type) << std::endl;
                break;
            }
            }

            return;
        }
        // child_statement.node->Cast<SelectNode>().where_clause
    }

    MutationTreeNode *MudStatementGenerator::GenerateSelectMutations(const SelectStatement &statement, MutationTestFunctionData *functionData, MutationTreeNode *parent_node, MutationOperatorTag operator_type)
    {
        std::cout << "Generating Select Mutation Function Called with statement: " << statement.ToString() << std::endl;
        D_ASSERT(statement.TYPE == StatementType::SELECT_STATEMENT);
        // std::cout << "Where Operator 1: " << static_cast<int>(statement.node->Cast<SelectNode>().where_clause->type) << std::endl;

        if (!parent_node)
        { // calling the select mutation generator for the first time.
          // therefore we have to create the root node, because root node will initially be nullptr
            std::cout << "Calling the Generating Select Mutation Function for the first time" << std::endl;
            functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(statement.Copy().release()))));

            parent_node = new MutationTreeNode(statement);
            parent_node->AddChild(statement);

            const auto &a = statement.Copy();
            const auto &dis_statement = a->Cast<SelectStatement>();

            // std::cout << "Where Operator 2: " << static_cast<int>(dis_statement.node->Cast<SelectNode>().where_clause->type) << std::endl;

            if (!DistinctModifierExist(dis_statement.node->modifiers, true))
            {
                dis_statement.node->modifiers.push_back(make_uniq<DistinctModifier>());
            }

            parent_node->AddChild(dis_statement);
            functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(dis_statement.Copy().release()))));

            for (const auto &child : parent_node->children)
            {
                std::cout << "Running the for loop" << std::endl;
                const auto &a = child->statement.Copy();
                auto &child_statement = a->Cast<SelectStatement>();
                std::cout << "Casted select statement: " << child_statement.ToString() << std::endl;
                auto &child_statement_node = child_statement.node->Cast<SelectNode>();
                std::cout << "Select Node casting successful" << std::endl;
                std::cout << child->statement.Copy()->Cast<SelectStatement>().ToString() << std::endl;
                // std::cout << "Where Operator 3: " << static_cast<int>(child_statement_node.where_clause->type) << std::endl;
                if (child_statement.node->Cast<SelectNode>().where_clause)
                    GenerateSelectMutations(child_statement, functionData, child.get(), MutationOperatorTag::WRO);
                std::cout << "Completed Processing the statement: " << child_statement.ToString() << std::endl;
            }
            std::cout << "Completed The Statment Processing" << std::endl;
        }
        else
        {
            std::cout << "Generating Select Mutation Function Recursively" << std::endl;

            if (operator_type == MutationOperatorTag::SEL)
            {
                throw InternalException("Unsupported mutation tree format requested");
            }
            else if (operator_type == MutationOperatorTag::WRO)
            {
                std::cout << "Generating mutants of COMPARE operator" << std::endl;

                const auto &a = statement.Copy();
                auto &select_stmt = a->Cast<SelectStatement>();
                auto &select_stmt_node = select_stmt.node->Cast<SelectNode>();
                // MutateWhereClauseStatement(parent_node, std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(statement.Copy().release()))), functionData);
                vector<ExpressionType> except_mutations = {select_stmt_node.where_clause->type};
                MutateParsedExpression(parent_node, a.get(), functionData, select_stmt_node.where_clause.get(), except_mutations);

                // std::cout << "Size of the Children in the parent node: " << parent_node->children.size() << std::endl;

                // for (int i = 0; i < parent_node->children.size(); i++)
                // {
                //     const auto &child = parent_node->children.at(i);
                //     std::cout << "i: " << i << std::endl;
                //     std::cout << "RUNNING the for loop" << std::endl;
                //     std::cout << "Statement Type: " << StatementTypeToString(child->statement.type) << std::endl;
                //     if (child->statement.type != StatementType::SELECT_STATEMENT)
                //     {
                //         // std::cout << "Different statement other than Select: " << child->statement.ToString() << std::endl;
                //         continue;
                //     }
                //     const auto &a = child->statement.Copy();
                //     std::cout << "Copied the statement successfully" << std::endl;
                //     auto &child_statement = a->Cast<SelectStatement>();
                //     std::cout << "Casted select statement: " << child_statement.ToString() << std::endl;
                //     auto &child_statement_node = child_statement.node->Cast<SelectNode>();
                //     std::cout << "Select Node casting successful" << std::endl;
                //     std::cout << child->statement.Copy()->Cast<SelectStatement>().ToString() << std::endl;
                //     // std::cout << "Where Operator 3: " << static_cast<int>(child_statement_node.where_clause->type) << std::endl;
                //     GenerateSelectMutations(child_statement, functionData, child.get(), MutationOperatorTag::LCR);
                // }
            }
            else if (operator_type == MutationOperatorTag::LCR)
            {
                std::cout << "Unimplemented Relational Operator Type" << std::endl;
                // throw InternalException("Unimplemented Relational Operator Type");
            }
            else if (operator_type == MutationOperatorTag::JOI)
            {
                const auto &a = statement.Copy();
                const auto &dis_statement = a->Cast<SelectStatement>();
                auto &select_node = statement.node->Cast<SelectNode>();
                // std::cout << static_cast<int>(select_node.from_table->type) << std::endl;
            }
        }

        std::cout << "Completed generating Select MutatationN" << std::endl;

        return parent_node;
    }

    void GenerateMutations(duckdb_libpgquery::PGList *parse_tree_list, MutationTestFunctionData *functionData)
    {
        std::cout << "Starting the GenerateMutations() function with parse tree size: " << parse_tree_list->length << std::endl;

        vector<unique_ptr<SQLStatement>> statements;
        ParserOptions parserOptions;
        Transformer transformer(parserOptions);
        transformer.TransformParseTree(parse_tree_list, statements);

        MudStatementGenerator statement_generator;

        for (size_t i = 0; i < statements.size(); i++)
        {
            switch (statements[i]->type)
            {
            case StatementType::SELECT_STATEMENT:
            {
                auto &select_statement = static_cast<SelectStatement &>(*statements[i]);
                statement_generator.GenerateSelectMutations(select_statement, functionData);
                break;
            }
            default:
                break;
            }
        }

        // for (auto entry = parse_tree_list->head; entry != nullptr; entry = entry->next)
        // {
        //     std::cout << "GenerateMutations() -> i : " << i << std::endl;
        //     std::cout << "Parse Tree List Length: " << parse_tree_list->length << std::endl;
        //     if (i >= parse_tree_list->length)
        //         break;
        //     if (entry->data.ptr_value == nullptr)
        //     {
        //         std::cout << "Data value in the parse tree is nullptr" << std::endl;
        //         continue;
        //     }
        //     std::cout << "For loop of the parse in GenerateMutations() function" << std::endl;
        //     auto n = Transformer::PGPointerCast<duckdb_libpgquery::PGNode>(entry->data.ptr_value);
        //     std::cout << "Casted successfully nullptr to PGNode" << std::endl;
        //     auto statement_type = findStatementType(*n);

        //     vector<unique_ptr<SQLStatement>> statements;
        //     ParserOptions parserOptions;
        //     Transformer transformer(parserOptions);
        //     std::cout << "Calling the Parse Tree Transformer to parse statement into SQLStatements" << std::endl;
        //     transformer.TransformParseTree(parse_tree_list, statements);
        //     MudStatementGenerator statement_generator;

        //     switch (statement_type)
        //     {
        //     case duckdb_libpgquery::T_PGSelectStmt:
        //     {
        //         auto &select_statement = statements[i]->Cast<SelectStatement>();
        //         statement_generator.GenerateSelectMutations(select_statement, functionData);
        //         std::cout << "Select Mutation From the parse tree completed successfully" << std::endl;
        //         break;
        //     }
        //     default:
        //         std::cout << "Undefined Query Type: " << static_cast<int>(n->type) << std::endl;
        //         break;
        //     }
        //     i++;
        //     std::cout << "Out of Main Parse Tree Switch statement" << std::endl;
        // }

        std::cout << "exiting the generate mutation function" << std::endl;
    }
}