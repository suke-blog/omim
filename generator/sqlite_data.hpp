#pragma once
// #include "coding/internal/file64_api.hpp"

// #include "base/base.hpp"

// #include "std/functional.hpp"
// #include "std/noncopyable.hpp"
// #include "std/string.hpp"
// #include "std/target_os.hpp"


#include <string>
#include <sqlite3.h>

using namespace std;

namespace my {

class SqliteData
{
public:
  /// @note Do not change order
  enum Op { OP_READ = 0, OP_WRITE_TRUNCATE, OP_WRITE_EXISTING, OP_APPEND };

  SqliteData(string const & filePath, Op op);
  SqliteData(string const & filePath);
  ~SqliteData();

  uint64_t Size() const;
  uint64_t Pos() const;

  void Seek(uint64_t pos);

  void Read(uint64_t pos, void * p, size_t size);
  void Write(void const * p, size_t size);

  void Flush();
  void Truncate(uint64_t sz);

  string const & GetName() const { return m_FilePath; }

private:

  sqlite3 * m_DB;
  string m_FilePath;
  uint32_t m_FileID;
  uint32_t m_FileSize;
  Op m_Op;

  sqlite3_stmt *m_pStmtSize;
  sqlite3_stmt *m_pStmtInsertData;

  static const char* queryCreateTable;
  static const char* querySize;
  static const char* queryInsertFile;
  static const char* queryInsertData;
  static const char* querySelectData;

  string GetErrorProlog() const;
  static int GetFileID();
};

// bool GetFileSize(string const & fName, uint64_t & sz);
// bool DeleteFileX(string const & fName);
// bool RenameFileX(string const & fOld, string const & fNew);

// /// Write to temp file and rename it to dest. Delete temp on failure.
// /// @param write function that writes to file with a given name, returns true on success.
// bool WriteToTempAndRenameToFile(string const & dest, function<bool(string const &)> const & write,
//                                 string const & tmp = "");

// /// @return false if copy fails. DO NOT THROWS exceptions
// bool CopyFileX(string const & fOld, string const & fNew);
// bool IsEqualFiles(string const & firstFile, string const & secondFile);
}
