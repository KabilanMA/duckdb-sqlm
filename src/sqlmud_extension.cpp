#define DUCKDB_EXTENSION_MAIN

#include "sqlmud_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

// OpenSSL linked through vcpkg
#include <openssl/opensslv.h>

namespace duckdb {

inline void SqlmudScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &name_vector = args.data[0];
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) {
			return StringVector::AddString(result, "Sqlmud "+name.GetString()+" 🐥");;
        });
}

inline void SqlmudOpenSSLVersionScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &name_vector = args.data[0];
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) {
			return StringVector::AddString(result, "Sqlmud " + name.GetString() +
                                                     ", my linked OpenSSL version is " +
                                                     OPENSSL_VERSION_TEXT );;
        });
}

static void LoadInternal(DatabaseInstance &instance) {
    // Register a scalar function
    auto sqlmud_scalar_function = ScalarFunction("sqlmud", {LogicalType::VARCHAR}, LogicalType::VARCHAR, SqlmudScalarFun);
    ExtensionUtil::RegisterFunction(instance, sqlmud_scalar_function);

    // Register another scalar function
    auto sqlmud_openssl_version_scalar_function = ScalarFunction("sqlmud_openssl_version", {LogicalType::VARCHAR},
                                                LogicalType::VARCHAR, SqlmudOpenSSLVersionScalarFun);
    ExtensionUtil::RegisterFunction(instance, sqlmud_openssl_version_scalar_function);
}

void SqlmudExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string SqlmudExtension::Name() {
	return "sqlmud";
}

std::string SqlmudExtension::Version() const {
#ifdef EXT_VERSION_SQLMUD
	return EXT_VERSION_SQLMUD;
#else
	return "";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void sqlmud_init(duckdb::DatabaseInstance &db) {
    duckdb::DuckDB db_wrapper(db);
    db_wrapper.LoadExtension<duckdb::SqlmudExtension>();
}

DUCKDB_EXTENSION_API const char *sqlmud_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
