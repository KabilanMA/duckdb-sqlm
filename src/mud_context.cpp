#include "mud_context.hpp"

namespace duckdb
{
    MudContext::MudContext(DatabaseInstance &database) : internalClientContext(make_shared_ptr<ClientContext>(database.shared_from_this()))
    {
    }

    MudContext::~MudContext()
    {
        internalClientContext->~ClientContext();
    }

    void MudContext::ExecuteMutants(MutationTestFunctionData *functionData)
    {
        // internalClientContext->Query();
        auto lock = internalClientContext->LockContext();

        auto pending_query = internalClientContext->PendingQuery(std::move(functionData->mutated_queries.get(0)->Copy()), false);
        if (pending_query->HasError())
        {
            return;
        }
        pending_query->Execute();
    }

    //     Connection::Connection(DatabaseInstance &database)
    //         : context(make_shared_ptr<ClientContext>(database.shared_from_this()))
    //     {
    //         ConnectionManager::Get(database).AddConnection(*context);
    // #ifdef DEBUG
    //         EnableProfiling();
    //         context->config.emit_profiler_output = false;
    // #endif
    //     }

    //     Connection::Connection(DuckDB &database) : Connection(*database.instance)
    //     {
    //     }

    //     Connection::Connection(Connection &&other) noexcept
    //     {
    //         std::swap(context, other.context);
    //         std::swap(warning_cb, other.warning_cb);
    //     }

} // namespace duckdb
