
#include "mutation_generator.hpp"

namespace duckdb
{

    void MutationTreeNode::AddChild(SQLStatement &child_query)
    {
        auto child_node = make_uniq<MutationTreeNode>(child_query.Copy());
        children.push_back(std::move(child_node));
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

    MutationTreeNode::MutationTreeNode(std::unique_ptr<SQLStatement> statement) : statement(std::move(statement))
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
        for (idx_t modifier_idx = 0; modifier_idx < modifiers.size(); modifier_idx++)
        {
            auto &modifier = *modifiers[modifier_idx];
            // std::cout << "Modifier Index: " << modifier_idx << " : " << static_cast<int>(modifier.type) << std::endl;
            if (modifier.type == ResultModifierType::DISTINCT_MODIFIER)
            {
                auto &distinct_modifier = modifier.Cast<DistinctModifier>();
                if (distinct_modifier.distinct_on_targets.empty())
                {
                    // we have a DISTINCT without an ON clause - this distinct does not need to be added
                    if (remove_modifier)
                        modifiers.erase(modifiers.begin() + modifier_idx);
                    return true;
                }
            }
        }
        return false;
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
            std::vector<std::function<void()>> condition_actions = {
                [&]()
                {
                    if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_LESSTHAN) == except_mutation.end())
                    {
                        expression->type = ExpressionType::COMPARE_LESSTHAN;
                        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                        parent_node->AddChild(*stmt);
                        except_mutation.push_back(expression->type);
                    }
                },
                [&]()
                {
                    if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_LESSTHANOREQUALTO) == except_mutation.end())
                    {
                        expression->type = ExpressionType::COMPARE_LESSTHANOREQUALTO;
                        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                        parent_node->AddChild(*stmt);
                        except_mutation.push_back(expression->type);
                    }
                },
                [&]()
                {
                    if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_GREATERTHAN) == except_mutation.end())
                    {
                        expression->type = ExpressionType::COMPARE_GREATERTHAN;
                        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                        parent_node->AddChild(*stmt);
                        except_mutation.push_back(expression->type);
                    }
                },
                [&]()
                {
                    if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_GREATERTHANOREQUALTO) == except_mutation.end())
                    {
                        expression->type = ExpressionType::COMPARE_GREATERTHANOREQUALTO;
                        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                        parent_node->AddChild(*stmt);
                        except_mutation.push_back(expression->type);
                    }
                },
                [&]()
                {
                    if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_EQUAL) == except_mutation.end())
                    {
                        expression->type = ExpressionType::COMPARE_EQUAL;
                        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                        parent_node->AddChild(*stmt);
                        except_mutation.push_back(expression->type);
                    }
                },
                [&]()
                {
                    if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_NOTEQUAL) == except_mutation.end())
                    {
                        expression->type = ExpressionType::COMPARE_NOTEQUAL;
                        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                        parent_node->AddChild(*stmt);
                        except_mutation.push_back(expression->type);
                    }
                }};

            // std::random_device rd;
            // std::mt19937 g(rd());
            // std::shuffle(condition_actions.begin(), condition_actions.end(), g);
            for (auto &action : condition_actions)
            {
                action();
            }
            break;
        }
        case ExpressionType::COMPARE_BETWEEN:
        {
            expression->type = ExpressionType::COMPARE_NOT_BETWEEN;
            functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
            parent_node->AddChild(*stmt);
            expression->type = ExpressionType::COMPARE_BETWEEN;
            break;
        }
        case ExpressionType::COMPARE_NOT_BETWEEN:
        {
            expression->type = ExpressionType::COMPARE_BETWEEN;
            functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
            parent_node->AddChild(*stmt);
            expression->type = ExpressionType::COMPARE_NOT_BETWEEN;
            break;
        }
        case ExpressionType::COMPARE_IN:
        {
            expression->type = ExpressionType::COMPARE_NOT_IN;
            functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
            parent_node->AddChild(*stmt);
            expression->type = ExpressionType::COMPARE_IN;
            break;
        }
        case ExpressionType::COMPARE_NOT_IN:
        {
            expression->type = ExpressionType::COMPARE_IN;
            functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
            parent_node->AddChild(*stmt);
            expression->type = ExpressionType::COMPARE_NOT_IN;
            break;
        }
        case ExpressionType::CONJUNCTION_AND:
        case ExpressionType::CONJUNCTION_OR:
        {
            auto *conjunction_expr = dynamic_cast<ConjunctionExpression *>(expression);
            vector<ExpressionType> except = {};
            if (conjunction_expr->children.size() == 2)
            {
                auto &left_child = conjunction_expr->children.get(0);
                auto &right_child = conjunction_expr->children.get(1);
                std::function<void(MutationTreeNode *, SQLStatement *, MutationTestFunctionData *, ConjunctionExpression *, ParsedExpression *, ParsedExpression *)> mutateChild = [](MutationTreeNode *_parent_node, SQLStatement *_stmt, MutationTestFunctionData *_functionData, ConjunctionExpression *conjunction_expr, ParsedExpression *left_child, ParsedExpression *right_child)
                {
                    std::function<void(MutationTreeNode *, SQLStatement *, MutationTestFunctionData *, ConjunctionExpression *, ParsedExpression *)> permutationMutateExpression = [](MutationTreeNode *_parent_node, SQLStatement *_stmt, MutationTestFunctionData *_functionData, ConjunctionExpression *conjunction_expr, ParsedExpression *right_expression)
                    {
                        _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                        _parent_node->AddChild(*_stmt);
                        MutateParsedExpression(_parent_node, _stmt, _functionData, right_expression, {right_expression->type});

                        if (conjunction_expr->type == ExpressionType::CONJUNCTION_OR)
                        {
                            conjunction_expr->type = ExpressionType::CONJUNCTION_AND;
                            _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                            _parent_node->AddChild(*_stmt);
                            MutateParsedExpression(_parent_node, _stmt, _functionData, right_expression, {right_expression->type});
                            conjunction_expr->type = ExpressionType::CONJUNCTION_OR;
                        }
                        else if (conjunction_expr->type == ExpressionType::CONJUNCTION_AND)
                        {
                            conjunction_expr->type = ExpressionType::CONJUNCTION_OR;
                            _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                            _parent_node->AddChild(*_stmt);
                            MutateParsedExpression(_parent_node, _stmt, _functionData, right_expression, {right_expression->type});
                            conjunction_expr->type = ExpressionType::CONJUNCTION_AND;
                        }
                    };
                    if (left_child->type != ExpressionType::COMPARE_LESSTHAN)
                    {
                        left_child->type = ExpressionType::COMPARE_LESSTHAN;
                        permutationMutateExpression(_parent_node, _stmt, _functionData, conjunction_expr, right_child);
                    }
                    if (left_child->type != ExpressionType::COMPARE_LESSTHANOREQUALTO)
                    {
                        left_child->type = ExpressionType::COMPARE_LESSTHANOREQUALTO;
                        permutationMutateExpression(_parent_node, _stmt, _functionData, conjunction_expr, right_child);
                    }
                    if (left_child->type != ExpressionType::COMPARE_GREATERTHAN)
                    {
                        left_child->type = ExpressionType::COMPARE_GREATERTHAN;
                        permutationMutateExpression(_parent_node, _stmt, _functionData, conjunction_expr, right_child);
                    }
                    if (left_child->type != ExpressionType::COMPARE_GREATERTHANOREQUALTO)
                    {
                        left_child->type = ExpressionType::COMPARE_GREATERTHANOREQUALTO;
                        permutationMutateExpression(_parent_node, _stmt, _functionData, conjunction_expr, right_child);
                    }
                    if (left_child->type != ExpressionType::COMPARE_EQUAL)
                    {
                        left_child->type = ExpressionType::COMPARE_EQUAL;
                        permutationMutateExpression(_parent_node, _stmt, _functionData, conjunction_expr, right_child);
                    }
                    if (left_child->type != ExpressionType::COMPARE_NOTEQUAL)
                    {
                        left_child->type = ExpressionType::COMPARE_NOTEQUAL;
                        permutationMutateExpression(_parent_node, _stmt, _functionData, conjunction_expr, right_child);
                    }
                };
                if (left_child->type == ExpressionType::COMPARE_EQUAL || left_child->type == ExpressionType::COMPARE_NOTEQUAL || left_child->type == ExpressionType::COMPARE_LESSTHAN || left_child->type == ExpressionType::COMPARE_LESSTHANOREQUALTO || left_child->type == ExpressionType::COMPARE_GREATERTHAN || left_child->type == ExpressionType::COMPARE_GREATERTHANOREQUALTO)
                {
                    mutateChild(parent_node, stmt, functionData, conjunction_expr, left_child.get(), right_child.get());
                }
                else if (right_child->type == ExpressionType::COMPARE_EQUAL || right_child->type == ExpressionType::COMPARE_NOTEQUAL || right_child->type == ExpressionType::COMPARE_LESSTHAN || right_child->type == ExpressionType::COMPARE_LESSTHANOREQUALTO || right_child->type == ExpressionType::COMPARE_GREATERTHAN || right_child->type == ExpressionType::COMPARE_GREATERTHANOREQUALTO)
                {
                    mutateChild(parent_node, stmt, functionData, conjunction_expr, right_child.get(), left_child.get());
                }
            }
            else
            {
                // TODO: implement multiple children mutation for where caluse.
            }
            break;
        }
        case ExpressionType::COMPARE_DISTINCT_FROM:
        {
            expression->type = ExpressionType::COMPARE_NOT_DISTINCT_FROM;
            functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
            parent_node->AddChild(*stmt);
            expression->type = ExpressionType::COMPARE_DISTINCT_FROM;
            break;
        }
        case ExpressionType::COMPARE_NOT_DISTINCT_FROM:
        {
            expression->type = ExpressionType::COMPARE_DISTINCT_FROM;
            functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
            parent_node->AddChild(*stmt);
            expression->type = ExpressionType::COMPARE_NOT_DISTINCT_FROM;
            break;
        }
        default:
        {
            throw InternalException("Unimplemented expression type");
            break;
        }
        }
    }

    void MutateRegularJoinRef(MutationTreeNode *parent_node, SQLStatement *stmt, MutationTestFunctionData *functionData, JoinRef *from_table)
    {
        // std::cout << "Calling the Mutate Table Ref function" << std::endl;
        auto &join_ref = from_table->Cast<JoinRef>();
        switch (join_ref.type)
        {
        case JoinType::LEFT:
        case JoinType::RIGHT:
        case JoinType::INNER:
        case JoinType::OUTER:
        case JoinType::SEMI:
        case JoinType::ANTI:
        {
            std::vector<JoinType> mutation_types = {JoinType::LEFT, JoinType::RIGHT, JoinType::INNER, JoinType::OUTER, JoinType::SEMI, JoinType::ANTI};
            for (auto type : mutation_types)
            {
                if (type != join_ref.type)
                {
                    // std::cout << JoinTypeToString(join_ref.type) << std::endl;
                    // MutateJoinType(type, stmt, parent_node, functionData);
                    auto old_type = join_ref.type;
                    stmt->Cast<SelectStatement>().node->Cast<SelectNode>().from_table->Cast<JoinRef>().type = type;
                    functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                    parent_node->AddChild(*stmt);
                    stmt->Cast<SelectStatement>().node->Cast<SelectNode>().from_table->Cast<JoinRef>().type = old_type;
                }
            }
            break;
        }
        default:
            throw InternalException("Implemented expression type");
            break;
        }
    }

    void MutateAggregateFunction(MutationTreeNode *parent_node, SQLStatement *stmt, MutationTestFunctionData *functionData, ParsedExpression *expression)
    {
        parent_node->AddChild(*stmt);
        auto &select_stmt = stmt->Cast<SelectStatement>();
        auto &select_stmt_node = select_stmt.node->Cast<SelectNode>();

        if (expression->type == ExpressionType::FUNCTION)
        {
            auto &function_expr = expression->Cast<FunctionExpression>();
            enum class AggregateType
            {
                COUNT = 0,
                SUM,
                AVG,
                MIN,
                MAX
            };

            std::function<void(MutationTreeNode *, SQLStatement *, MutationTestFunctionData *, FunctionExpression *, AggregateType)> mutateAggregate = [](MutationTreeNode *_parent_node, SQLStatement *_stmt, MutationTestFunctionData *_functionData, FunctionExpression *fun_expr, AggregateType type)
            {
                vector<AggregateType> mutation_types = {AggregateType::COUNT, AggregateType::SUM, AggregateType::AVG, AggregateType::MIN, AggregateType::MAX};
                for (auto mutation_type : mutation_types)
                {
                    if (mutation_type == type)
                    {
                        if (fun_expr->distinct)
                        {
                            fun_expr->distinct = false;
                            _parent_node->AddChild(*_stmt);
                            _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                            fun_expr->distinct = true;
                        }
                        else
                        {
                            fun_expr->distinct = true;
                            _parent_node->AddChild(*_stmt);
                            _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                            fun_expr->distinct = false;
                        }
                    }
                    switch (mutation_type)
                    {
                    case AggregateType::COUNT:
                    {
                        fun_expr->function_name = "count";
                        _parent_node->AddChild(*_stmt);
                        _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                        if (fun_expr->distinct)
                        {
                            fun_expr->distinct = false;
                            _parent_node->AddChild(*_stmt);
                            _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                            fun_expr->distinct = true;
                        }
                        else
                        {
                            fun_expr->distinct = true;
                            _parent_node->AddChild(*_stmt);
                            _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                            fun_expr->distinct = false;
                        }
                        break;
                    }
                    case AggregateType::SUM:
                    {
                        fun_expr->function_name = "sum";
                        _parent_node->AddChild(*_stmt);
                        _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                        if (fun_expr->distinct)
                        {
                            fun_expr->distinct = false;
                            _parent_node->AddChild(*_stmt);
                            _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                            fun_expr->distinct = true;
                        }
                        else
                        {
                            fun_expr->distinct = true;
                            _parent_node->AddChild(*_stmt);
                            _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                            fun_expr->distinct = false;
                        }
                        break;
                    }
                    case AggregateType::AVG:
                    {
                        fun_expr->function_name = "avg";
                        _parent_node->AddChild(*_stmt);
                        _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                        if (fun_expr->distinct)
                        {
                            fun_expr->distinct = false;
                            _parent_node->AddChild(*_stmt);
                            _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                            fun_expr->distinct = true;
                        }
                        else
                        {
                            fun_expr->distinct = true;
                            _parent_node->AddChild(*_stmt);
                            _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                            fun_expr->distinct = false;
                        }
                        break;
                    }
                    case AggregateType::MAX:
                    {
                        fun_expr->function_name = "max";
                        if (fun_expr->distinct)
                        {
                            fun_expr->distinct = false;
                            _parent_node->AddChild(*_stmt);
                            _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                            fun_expr->distinct = true;
                        }
                        else
                        {
                            _parent_node->AddChild(*_stmt);
                            _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                        }
                        break;
                    }
                    case AggregateType::MIN:
                    {
                        fun_expr->function_name = "min";
                        if (fun_expr->distinct)
                        {
                            fun_expr->distinct = false;
                            _parent_node->AddChild(*_stmt);
                            _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                            fun_expr->distinct = true;
                        }
                        else
                        {
                            _parent_node->AddChild(*_stmt);
                            _functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(_stmt->Copy().release()))));
                        }
                        break;
                    }
                    default:
                    {
                        throw InternalException("Unimplemented aggregate type");
                        break;
                    }
                    }
                }
            };

            if (function_expr.function_name == "count")
            {
                mutateAggregate(parent_node, stmt, functionData, &function_expr, AggregateType::COUNT);
            }
            else if (function_expr.function_name == "sum")
            {
                mutateAggregate(parent_node, stmt, functionData, &function_expr, AggregateType::SUM);
            }
            else if (function_expr.function_name == "avg")
            {
                mutateAggregate(parent_node, stmt, functionData, &function_expr, AggregateType::AVG);
            }
            else if (function_expr.function_name == "min")
            {
                mutateAggregate(parent_node, stmt, functionData, &function_expr, AggregateType::MIN);
            }
            else if (function_expr.function_name == "max")
            {
                mutateAggregate(parent_node, stmt, functionData, &function_expr, AggregateType::MAX);
            }
        }
    }

    MutationTreeNode *MudStatementGenerator::GenerateSelectMutations(SelectStatement &statement, MutationTestFunctionData *functionData, MutationTreeNode *parent_node, MutationOperatorTag operator_type)
    {
        D_ASSERT(statement.TYPE == StatementType::SELECT_STATEMENT);
        if (!parent_node)
        { // calling the select mutation generator for the first time.
            // therefore we have to create the root node, because root node will initially be nullptr
            cout << "Creating the root node" << endl;
            functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(statement.Copy().release()))));
            parent_node = new MutationTreeNode(statement.Copy());
            parent_node->AddChild(statement);
            const auto &a = statement.Copy();
            auto &dis_statement = a->Cast<SelectStatement>();
            if (!DistinctModifierExist(dis_statement.node->modifiers, true))
            {
                dis_statement.node->modifiers.push_back(make_uniq<DistinctModifier>());
            }
            parent_node->AddChild(dis_statement);
            functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(dis_statement.Copy().release()))));
            for (const auto &child : parent_node->children)
            {
                const auto &a = child->statement->Copy();
                auto &child_statement = a->Cast<SelectStatement>();
                auto &child_statement_node = child_statement.node->Cast<SelectNode>();
                GenerateSelectMutations(child_statement, functionData, child.get(), MutationOperatorTag::JOIN);
            }
        }
        else
        {
            const auto &a = statement.Copy();
            auto &select_stmt = a->Cast<SelectStatement>();
            auto &select_stmt_node = select_stmt.node->Cast<SelectNode>();

            switch (operator_type)
            {
            case MutationOperatorTag::SEL:
            {
                throw InternalException("Unsupported mutation tree format requested");
                break;
            }
            case MutationOperatorTag::JOIN:
            {
                if (select_stmt_node.from_table.get()->type == TableReferenceType::JOIN)
                {
                    parent_node->AddChild(statement);
                    auto &cp = select_stmt_node.from_table.get()->Cast<JoinRef>();
                    switch (cp.ref_type)
                    {
                    case JoinRefType::REGULAR:
                    {
                        MutateRegularJoinRef(parent_node, &statement, functionData, &cp);
                        break;
                    }
                    case JoinRefType::NATURAL:
                    {
                        cp.ref_type = JoinRefType::CROSS;
                        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(statement.Copy().release()))));
                        parent_node->AddChild(statement);
                        cp.ref_type = JoinRefType::NATURAL;
                        break;
                    }
                    case JoinRefType::CROSS:
                    {
                        cp.ref_type = JoinRefType::NATURAL;
                        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(statement.Copy().release()))));
                        parent_node->AddChild(statement);
                        cp.ref_type = JoinRefType::CROSS;
                        break;
                    }
                    default:
                        break;
                    }
                    // std::cout << static_cast<int>(select_node.from_table->type) << std::endl;
                }
                if (parent_node->children.size() <= 1)
                {
                    // no join statement in the query, therefore no join clause mutation is done.
                    GenerateSelectMutations(statement, functionData, parent_node, MutationOperatorTag::WRO);
                }
                else
                {
                    for (const auto &child : parent_node->children)
                    {
                        const auto &a = child->statement->Copy();
                        auto &child_statement = a->Cast<SelectStatement>();
                        GenerateSelectMutations(child_statement, functionData, child.get(), MutationOperatorTag::WRO);
                    }
                }
                break;
            }
            case MutationOperatorTag::WRO:
            {
                // MutateWhereClauseStatement(parent_node, std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(statement.Copy().release()))), functionData);
                if (select_stmt_node.where_clause)
                {
                    vector<ExpressionType> except_mutations = {select_stmt_node.where_clause->type};
                    MutateParsedExpression(parent_node, a.get(), functionData, select_stmt_node.where_clause.get(), except_mutations);
                    D_ASSERT(parent_node->children.size() != 0);
                    for (const auto &child : parent_node->children)
                    {
                        const auto &a = child->statement->Copy();
                        auto &child_statement = a->Cast<SelectStatement>();
                        // auto &child_statement_node = child_statement.node->Cast<SelectNode>();
                        GenerateSelectMutations(child_statement, functionData, child.get(), MutationOperatorTag::HVGO);
                    }
                }
                else
                {
                    GenerateSelectMutations(statement, functionData, parent_node, MutationOperatorTag::HVGO);
                }
                break;
            }
            case MutationOperatorTag::HVGO:
            {
                if (select_stmt_node.having != nullptr)
                {
                    vector<ExpressionType> except_mutations = {select_stmt_node.having->type};
                    MutateParsedExpression(parent_node, a.get(), functionData, select_stmt_node.having.get(), except_mutations);
                    for (const auto &child : parent_node->children)
                    {
                        const auto &a = child->statement->Copy();
                        auto &child_statement = a->Cast<SelectStatement>();
                        GenerateSelectMutations(child_statement, functionData, child.get(), MutationOperatorTag::AGR);
                    }
                }
                else
                {
                    GenerateSelectMutations(statement, functionData, parent_node, MutationOperatorTag::AGR);
                }

                break;
            }
            case MutationOperatorTag::AGR:
            {
                bool is_aggregate = false;
                for (auto &projection : select_stmt_node.GetSelectList())
                {
                    if (projection->type == ExpressionType::FUNCTION)
                    {
                        is_aggregate = true;
                        // TODO: Implement the columnref condition to differentiate varchar column and integer column.
                        MutateAggregateFunction(parent_node, a.get(), functionData, projection.get());
                    }
                }
                if (is_aggregate)
                {
                    for (const auto &child : parent_node->children)
                    {
                        const auto &a = child->statement->Copy();
                        auto &child_statement = a->Cast<SelectStatement>();
                        GenerateSelectMutations(child_statement, functionData, child.get(), MutationOperatorTag::ORD);
                    }
                }
                else
                    GenerateSelectMutations(statement, functionData, parent_node, MutationOperatorTag::ORD);
                break;
            }
            case MutationOperatorTag::ORD:
            {

                break;
            }
            default:
                break;
            }
        }

        return parent_node;
    }

    void GenerateMutations(duckdb_libpgquery::PGList *parse_tree_list, MutationTestFunctionData *functionData)
    {
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
    }
}