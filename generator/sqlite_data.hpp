#pragma once
// #include "coding/internal/file64_api.hpp"

#include "base/macros.hpp"

// #include "std/functional.hpp"
// #include "std/noncopyable.hpp"
// #include "std/string.hpp"
// #include "std/target_os.hpp"


#include <string>
#include <sqlite3.h>

using namespace std;


class SqliteData
{
  DISALLOW_COPY(SqliteData);

public:
  /// @note Do not change order
  enum Op { OP_READ = 0, OP_WRITE_TRUNCATE, OP_WRITE_EXISTING, OP_APPEND };

  SqliteData(SqliteData && rhs);

  explicit SqliteData(string const & filePath, Op op = OP_WRITE_TRUNCATE, bool bTruncOnClose = false);
  ~SqliteData();

  void Seek(uint64_t pos);
  uint64_t Pos() const;
  void Write(void const * p, size_t size);

  // void WritePaddingByEnd(size_t factor);
  // void WritePaddingByPos(size_t factor);

  uint64_t Size() const;
  void Flush();

  // void Reserve(uint64_t size);

  // static void DeleteFileX(std::string const & fName);

  string const & GetName() const { return m_FilePath; }

private:

  sqlite3 * m_DB;
  string m_FilePath;
  uint32_t m_FileID;
  uint32_t m_FileSize;
  Op m_Op;
  bool m_bTruncOnClose;

  sqlite3_stmt *m_pStmtSize;
  sqlite3_stmt *m_pStmtInsertData;

  static const char* queryCreateTable;
  static const char* querySize;
  static const char* queryInsertFile;
  static const char* queryInsertData;
  static const char* querySelectData;

  static uint32_t GetFileID();
};
