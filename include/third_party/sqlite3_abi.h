#pragma once
// Exact C ABI declarations for SQLite3 (libsqlite3.so.0 / sqlite3.dll / libsqlite3.dylib)
// Public Domain - Compatible with SQLite 3.35+ / 3.40+

#include <cstdint>

extern "C" {

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
typedef int64_t sqlite3_int64;
typedef uint64_t sqlite3_uint64;
typedef void (*sqlite3_destructor_type)(void*);

#define SQLITE_OK           0
#define SQLITE_ERROR        1
#define SQLITE_BUSY         5
#define SQLITE_DONE         101
#define SQLITE_ROW          100

#define SQLITE_STATIC       ((sqlite3_destructor_type)0)
#define SQLITE_TRANSIENT    ((sqlite3_destructor_type)-1)

const char* sqlite3_libversion(void);
int sqlite3_open(const char* filename, sqlite3** ppDb);
int sqlite3_close(sqlite3* db);
int sqlite3_close_v2(sqlite3* db);
int sqlite3_exec(
    sqlite3* db,
    const char* sql,
    int (*callback)(void*, int, char**, char**),
    void* arg,
    char** errmsg
);
void sqlite3_free(void* ptr);
const char* sqlite3_errmsg(sqlite3* db);
int sqlite3_changes(sqlite3* db);
sqlite3_int64 sqlite3_last_insert_rowid(sqlite3* db);

int sqlite3_prepare_v2(
    sqlite3* db,
    const char* zSql,
    int nByte,
    sqlite3_stmt** ppStmt,
    const char** pzTail
);
int sqlite3_step(sqlite3_stmt* pStmt);
int sqlite3_reset(sqlite3_stmt* pStmt);
int sqlite3_clear_bindings(sqlite3_stmt* pStmt);
int sqlite3_finalize(sqlite3_stmt* pStmt);

int sqlite3_bind_double(sqlite3_stmt* pStmt, int index, double value);
int sqlite3_bind_int(sqlite3_stmt* pStmt, int index, int value);
int sqlite3_bind_int64(sqlite3_stmt* pStmt, int index, sqlite3_int64 value);
int sqlite3_bind_null(sqlite3_stmt* pStmt, int index);
int sqlite3_bind_text(
    sqlite3_stmt* pStmt,
    int index,
    const char* value,
    int nBytes,
    sqlite3_destructor_type destructor
);

int sqlite3_column_count(sqlite3_stmt* pStmt);
double sqlite3_column_double(sqlite3_stmt* pStmt, int iCol);
int sqlite3_column_int(sqlite3_stmt* pStmt, int iCol);
sqlite3_int64 sqlite3_column_int64(sqlite3_stmt* pStmt, int iCol);
const unsigned char* sqlite3_column_text(sqlite3_stmt* pStmt, int iCol);
int sqlite3_column_bytes(sqlite3_stmt* pStmt, int iCol);

} // extern "C"
