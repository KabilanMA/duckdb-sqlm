#pragma once

#include "duckdb.hpp"
#include "mud_context.hpp"
#include "mutation_generator.hpp"

#include <iostream>

namespace duckdb
{
    class MudConnection
    {
    public:
        MutationTestFunctionData *functionData;
        shared_ptr<MudContext> context;

        explicit MudConnection(DuckDB &database, MutationTestFunctionData *functionData);
        void Query();
    };
}