include(FetchContent)

set(SQLITE3_URL      "https://sqlite.org/2026/sqlite-amalgamation-3530000.zip")
set(SQLITE3_URL_HASH "SHA3_256=c2325c53b3b41761469f91cfb078e96882ac5d85bac10c11b0bd8f253b031e5b")
FetchContent_Declare(sqlite3_external URL ${SQLITE3_URL} URL_HASH ${SQLITE3_URL_HASH} EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(sqlite3_external)

add_library(sqlite3 STATIC ${sqlite3_external_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${sqlite3_external_SOURCE_DIR})
