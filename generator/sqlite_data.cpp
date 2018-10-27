#include "sqlite_data.hpp"
#include "sqlite_helper.hpp"

#include "coding/constants.hpp"
#include "coding/reader.hpp" // For Reader exceptions.
#include "coding/writer.hpp" // For Writer exceptions.

#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/cerrno.hpp"
#include "std/cstring.hpp"
#include "std/exception.hpp"
#include "std/fstream.hpp"
#include "std/target_os.hpp"
#include "std/thread.hpp"
#include "std/mutex.hpp"
#include "std/iostream.hpp"
#include "std/fstream.hpp"

// #include "sqlite_data.hpp"
// #include "sqlite_helper.hpp"
// #include <cerrno>
// #include <mutex>
// #include <iostream>
// #include <fstream>


static std::mutex m;

const char* SqliteData::queryCreateTable = "create table file_tbl (id integer, name text, primary key(id)); create table data_tbl (id integer, file_id integer, data blob, primary key(id));";
const char* SqliteData::querySize = "select length(data) from data_tbl where file_id=? group by file_id;";
const char* SqliteData::queryInsertFile = "insert into file_tbl(id, name) values(?, ?);";
const char* SqliteData::queryInsertData = "insert into data_tbl(file_id, data) values(?, ?);";
const char* SqliteData::querySelectData = "select data from data_tbl where file_id=? order by id;";

SqliteData::SqliteData(SqliteData && rhs) :
    m_DB(rhs.m_DB),
    m_FilePath(rhs.m_FilePath),
    m_FileID(rhs.m_FileID),
    m_FileSize(rhs.m_FileSize), 
    m_Op(rhs.m_Op), 
    m_bTruncOnClose(rhs.m_bTruncOnClose),
    m_pStmtInsertData(rhs.m_pStmtInsertData),
    m_pStmtSize(rhs.m_pStmtSize)
{}

SqliteData::SqliteData(string const & filePath, Op op, bool bTruncOnClose)
    : m_FilePath(filePath), m_Op(op), m_FileSize(0), m_bTruncOnClose(bTruncOnClose)
{
  int dir_index = filePath.find_last_of('/')+1;
  int ext_index = filePath.find_last_of('.');

  string dbpath = "";
  string filename = "";
  if(dir_index == -1){
    dbpath.append("./");
    filename.append(filePath);
  }else{
    dbpath.append(filePath.substr(0, dir_index));
    filename.append(filePath.substr(dir_index, filePath.size() - dir_index));
  }
  
  string ext = "";
  if(ext_index == -1){
    ext.append("default");
  }else{
    ext.append(filePath.substr(ext_index+1, filePath.size() - ext_index+1));
  }
  
  dbpath.append(ext).append(".db");

  m_DB = SqliteHelper::getInstance().getConnection(dbpath);
  m_FileID = GetFileID();

  // create table
  sqlite3_exec(m_DB, queryCreateTable, nullptr, nullptr, nullptr);

  // insert file_tbl
  sqlite3_stmt *pStmt = nullptr;
  sqlite3_prepare_v2(m_DB, queryInsertFile, -1, &pStmt, nullptr);
  sqlite3_bind_int(pStmt, 1, m_FileID);
  sqlite3_bind_text(pStmt, 2, filename.c_str(), filename.length(), SQLITE_TRANSIENT);
  while(sqlite3_step(pStmt) == SQLITE_BUSY);
  sqlite3_finalize(pStmt);

  //create prepare statement
  sqlite3_prepare_v2(m_DB, queryInsertData, -1, &m_pStmtInsertData, nullptr);
  sqlite3_prepare_v2(m_DB, querySize, -1, &m_pStmtSize, nullptr);
}

SqliteData::~SqliteData()
{
  if(m_DB){
    Flush();
    sqlite3_finalize(m_pStmtInsertData);
    sqlite3_finalize(m_pStmtSize);
    SqliteHelper::getInstance().close(m_DB);
  }
}

uint64_t SqliteData::Pos() const
{
  return static_cast<uint64_t>(m_FileSize);
}

void SqliteData::Seek(uint64_t pos)
{
  return;
}

void SqliteData::Write(void const * p, size_t size)
{
  sqlite3_bind_int(m_pStmtInsertData, 1, m_FileID);
  sqlite3_bind_blob(m_pStmtInsertData, 2, p, size, SQLITE_TRANSIENT);

  while(sqlite3_step(m_pStmtInsertData) == SQLITE_BUSY);
  
  //clear bind and state
  sqlite3_reset(m_pStmtInsertData);
  sqlite3_clear_bindings(m_pStmtInsertData);

  m_FileSize += size;
}

uint64_t SqliteData::Size() const
{
  int64_t size = 0;

  sqlite3_bind_int(m_pStmtSize, 1, m_FileID);
  while(sqlite3_step(m_pStmtSize) == SQLITE_ROW){
    size = sqlite3_column_int64(m_pStmtSize, 0);
  }

  //clear bind and state
  sqlite3_reset(m_pStmtSize);
  sqlite3_clear_bindings(m_pStmtSize);

  // ASSERT_GREATER_OR_EQUAL(size, 0, ());
  return static_cast<uint64_t>(size);
}

void SqliteData::Flush()
{
  ofstream fout;

  fout.open(m_FilePath, ios::out | ios::binary | ios::trunc);
  if(!fout){
    cerr << "failed to open file:" << m_FilePath << endl;
    return;
  }

  sqlite3_stmt *pStmt;
  sqlite3_prepare_v2(m_DB, querySelectData, -1, &pStmt, nullptr);
  sqlite3_bind_int(pStmt, 1, m_FileID);

  while(sqlite3_step(pStmt) == SQLITE_ROW){
    int size = sqlite3_column_bytes(pStmt, 0);
    const char* buf = static_cast<const char*>(sqlite3_column_blob(pStmt, 0));
    fout.write(buf, size);
  }
  sqlite3_finalize(pStmt);

  fout.close();
  
  return;
}

uint32_t SqliteData::GetFileID()
{
  static uint32_t id = 0;

  std::lock_guard<std::mutex> lg(m);
  return ++id;
}
