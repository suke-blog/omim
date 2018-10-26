#include <iostream>
#include <mutex>
#include "sqlite_helper.hpp"


static SqliteHelper &instance = SqliteHelper::getInstance();
static std::mutex m;


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
