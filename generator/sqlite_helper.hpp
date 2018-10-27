#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <sqlite3.h>

using namespace std;
class SqliteHelper
{
    public:
        static SqliteHelper &getInstance();
        sqlite3* getConnection(string const &filepath);
        void close(sqlite3 *db);
    private:
        SqliteHelper();
        SqliteHelper(const SqliteHelper &other){}
        SqliteHelper &operator=(const SqliteHelper &other){}

        using DbconAndCounter = pair<sqlite3 *, int>;
        unordered_map<string, DbconAndCounter> m_DBTable;
};

