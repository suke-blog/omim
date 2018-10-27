#include <iostream>
#include <mutex>
#include "sqlite_helper.hpp"


static SqliteHelper &instance = SqliteHelper::getInstance();
static std::mutex m;

// SQLite mmap buffer
static const int64_t MMAP_BUFFER_DEFAULT = 256*1024*1024;   //256MByte
static const int64_t MMAP_BUFFER_MAX = 1024*1024*1024;      //1024MByte


SqliteHelper::SqliteHelper() {
    // using mmap
    sqlite3_config(SQLITE_CONFIG_MMAP_SIZE, MMAP_BUFFER_DEFAULT, MMAP_BUFFER_MAX);

    // disable memory statistics
    sqlite3_config(SQLITE_CONFIG_MEMSTATUS, false);
}

SqliteHelper& SqliteHelper::getInstance() {
    static SqliteHelper instance;
    return instance;
}

sqlite3* SqliteHelper::getConnection(string const &filepath) {
    std::lock_guard<std::mutex> lg(m);

    DbconAndCounter dbpair = m_DBTable[filepath];
    if(dbpair.first == nullptr){
        int rtn = sqlite3_open_v2(
            filepath.c_str(), 
            &(dbpair.first), 
            SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, 
            nullptr);
        if(rtn != SQLITE_OK)
        {
            cerr << "[ERR]:failed to open database. " << sqlite3_errmsg(dbpair.first) << endl;
            return nullptr;
        }

        // PRAGMA temp_store=FILE|MEMORY
        rtn = sqlite3_exec(dbpair.first, "PRAGMA temp_store=MEMORY", nullptr, nullptr, nullptr);
        if(rtn != SQLITE_OK)
            cerr << sqlite3_errmsg(dbpair.first) << endl;

        // PRAGMA synchronous=EXTRA|FULL|NORMAL|OFF
        rtn = sqlite3_exec(dbpair.first, "PRAGMA synchronous=OFF", nullptr, nullptr, nullptr);
        if(rtn != SQLITE_OK)
            cerr << sqlite3_errmsg(dbpair.first) << endl;

        // PRAGMA journal_mode=DELETE|TRUNCATE|PERSIST|MEMORY|WAL|OFF
        rtn = sqlite3_exec(dbpair.first, "PRAGMA journal_mode=OFF", nullptr, nullptr, nullptr);
        if(rtn != SQLITE_OK)
            cerr << sqlite3_errmsg(dbpair.first) << endl;

        // start transaction
        // sqlite3_exec(dbpair.first, "BEGIN", nullptr, nullptr, nullptr);
    }

    // increment db counter
    dbpair.second++;

    return dbpair.first;
}

void SqliteHelper::close(sqlite3 *db){
    // get dbname
    string str(sqlite3_db_filename(db, "main"));

    DbconAndCounter dbpair = m_DBTable[str];

    // check db connection is still exist
    if(dbpair.first != nullptr){
        // decrement db counter
        dbpair.second--;
        if(dbpair.second <= 0){
            // commit transaction
            // sqlite3_exec(dbpair.first, "COMMIT", nullptr, nullptr, nullptr);
            // close db
            sqlite3_close_v2(dbpair.first);
            dbpair.first = nullptr;
            dbpair.second = 0;
        }
    }
}
