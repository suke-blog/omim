#include "generator/intermediate_data.hpp"

#include <new>
#include <string>

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include "sqlite_helper.hpp"

using namespace std;

namespace
{
size_t const kFlushCount = 1024;
double const kValueOrder = 1e7;
string const kShortExtension = ".short";

// An estimation.
// OSM had around 4.1 billion nodes on 2017-11-08,
// see https://wiki.openstreetmap.org/wiki/Stats
size_t const kMaxNodesInOSM = size_t{1} << 33;

void ToLatLon(double lat, double lon, generator::cache::LatLon & ll)
{
  int64_t const lat64 = lat * kValueOrder;
  int64_t const lon64 = lon * kValueOrder;

  CHECK(
      lat64 >= std::numeric_limits<int32_t>::min() && lat64 <= std::numeric_limits<int32_t>::max(),
      ("Latitude is out of 32bit boundary:", lat64));
  CHECK(
      lon64 >= std::numeric_limits<int32_t>::min() && lon64 <= std::numeric_limits<int32_t>::max(),
      ("Longitude is out of 32bit boundary:", lon64));
  ll.m_lat = static_cast<int32_t>(lat64);
  ll.m_lon = static_cast<int32_t>(lon64);
}

bool FromLatLon(generator::cache::LatLon const & ll, double & lat, double & lon)
{
  // Assume that a valid coordinate is not (0, 0).
  if (ll.m_lat != 0.0 || ll.m_lon != 0.0)
  {
    lat = static_cast<double>(ll.m_lat) / kValueOrder;
    lon = static_cast<double>(ll.m_lon) / kValueOrder;
    return true;
  }
  lat = 0.0;
  lon = 0.0;
  return false;
}
}  // namespace

namespace generator
{
namespace cache
{
// IndexFileReader ---------------------------------------------------------------------------------
IndexFileReader::IndexFileReader(string const & name) : m_fileReader(name.c_str()) {}

void IndexFileReader::ReadAll()
{
  m_elements.clear();
  size_t fileSize = m_fileReader.Size();
  if (fileSize == 0)
    return;

  LOG_SHORT(LINFO, ("Offsets reading is started for file", m_fileReader.GetName()));
  CHECK_EQUAL(0, fileSize % sizeof(Element), ("Damaged file."));

  try
  {
    m_elements.resize(base::checked_cast<size_t>(fileSize / sizeof(Element)));
  }
  catch (std::bad_alloc const &)
  {
    LOG(LCRITICAL, ("Insufficient memory for required offset map"));
  }

  m_fileReader.Read(0, &m_elements[0], base::checked_cast<size_t>(fileSize));

  sort(m_elements.begin(), m_elements.end(), ElementComparator());

  LOG_SHORT(LINFO, ("Offsets reading is finished"));
}

bool IndexFileReader::GetValueByKey(Key key, Value & value) const
{
  auto it = lower_bound(m_elements.begin(), m_elements.end(), key, ElementComparator());
  if (it != m_elements.end() && it->first == key)
  {
    value = it->second;
    return true;
  }
  return false;
}

// IndexFileWriter ---------------------------------------------------------------------------------
IndexFileWriter::IndexFileWriter(string const & name) : m_fileWriter(name.c_str()) {}

void IndexFileWriter::WriteAll()
{
  if (m_elements.empty())
    return;

  m_fileWriter.Write(&m_elements[0], m_elements.size() * sizeof(Element));
  m_elements.clear();
}

void IndexFileWriter::Add(Key k, Value const & v)
{
  if (m_elements.size() > kFlushCount)
    WriteAll();

  m_elements.emplace_back(k, v);
}

// OSMElementCacheReader ---------------------------------------------------------------------------
OSMElementCacheReader::OSMElementCacheReader(string const & name, bool preload)
  : m_fileReader(name), m_offsets(name + OFFSET_EXT), m_name(name), m_preload(preload)
{
  if (!m_preload)
    return;
  size_t sz = m_fileReader.Size();
  m_data.resize(sz);
  m_fileReader.Read(0, m_data.data(), sz);
}

void OSMElementCacheReader::LoadOffsets() { m_offsets.ReadAll(); }

// OSMElementCacheWriter ---------------------------------------------------------------------------
OSMElementCacheWriter::OSMElementCacheWriter(string const & name, bool preload)
  : m_fileWriter(name), m_offsets(name + OFFSET_EXT), m_name(name), m_preload(preload)
{
}

void OSMElementCacheWriter::SaveOffsets() { m_offsets.WriteAll(); }

// OSMElementSqliteCacheReader ---------------------------------------------------------------------------
OSMElementSqliteCacheReader::OSMElementSqliteCacheReader(string const & name, bool preload)
  : m_name(name), m_preload(preload)
{
  // open database connection
  m_db = SqliteHelper::getInstance().getConnection(name + ".db");

  // make preapred statement
  sqlite3_prepare_v2(m_db, "SELECT data from cache_tbl WHERE id=?;", -1, &m_pStmt, nullptr);
}

OSMElementSqliteCacheReader::~OSMElementSqliteCacheReader()
{
  sqlite3_finalize(m_pStmt);
  SqliteHelper::getInstance().close(m_db);
}

void OSMElementSqliteCacheReader::LoadOffsets() { return; }

// OSMElementSqliteCacheWriter ---------------------------------------------------------------------------
OSMElementSqliteCacheWriter::OSMElementSqliteCacheWriter(string const & name, bool preload)
  : m_name(name), m_preload(preload)
{
  m_db = SqliteHelper::getInstance().getConnection(name + ".db");

  // create table
  sqlite3_exec(m_db, "CREATE TABLE cache_tbl(id INTEGER PRIMARY KEY, data BLOB);", nullptr, nullptr, nullptr);

  // make prepared statement
  sqlite3_prepare_v2(m_db, "INSERT INTO cache_tbl(id, data) VALUES(?, ?);", -1, &m_pStmt, nullptr);

  // start transaction
  sqlite3_exec(m_db, "BEGIN", nullptr, nullptr, nullptr);
}

OSMElementSqliteCacheWriter::~OSMElementSqliteCacheWriter()
{
  // commit transaction
  sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr);

  // finalize prepared statement
  sqlite3_finalize(m_pStmt);

  // close database connection
  SqliteHelper::getInstance().close(m_db);
}

void OSMElementSqliteCacheWriter::SaveOffsets() { return; }

// RawFilePointStorageMmapReader -------------------------------------------------------------------
RawFilePointStorageMmapReader::RawFilePointStorageMmapReader(string const & name)
  : m_mmapReader(name)
{
}

bool RawFilePointStorageMmapReader::GetPoint(uint64_t id, double & lat, double & lon) const
{
  LatLon ll;
  m_mmapReader.Read(id * sizeof(ll), &ll, sizeof(ll));

  bool ret = FromLatLon(ll, lat, lon);
  if (!ret)
    LOG(LERROR, ("Node with id =", id, "not found!"));
  return ret;
}

// RawFilePointStorageWriter -----------------------------------------------------------------------
RawFilePointStorageWriter::RawFilePointStorageWriter(string const & name) : m_fileWriter(name)
{
}

void RawFilePointStorageWriter::AddPoint(uint64_t id, double lat, double lon)
{
  LatLon ll;
  ToLatLon(lat, lon, ll);

  m_fileWriter.Seek(id * sizeof(ll));
  m_fileWriter.Write(&ll, sizeof(ll));

  ++m_numProcessedPoints;
}

// RawMemPointStorageReader ------------------------------------------------------------------------
RawMemPointStorageReader::RawMemPointStorageReader(string const & name)
  : m_fileReader(name), m_data(kMaxNodesInOSM)
{
  static_assert(sizeof(size_t) == 8, "This code is only for 64-bit architectures");
  m_fileReader.Read(0, m_data.data(), m_data.size() * sizeof(LatLon));
}

bool RawMemPointStorageReader::GetPoint(uint64_t id, double & lat, double & lon) const
{
  LatLon const & ll = m_data[id];
  bool ret = FromLatLon(ll, lat, lon);
  if (!ret)
    LOG(LERROR, ("Node with id =", id, "not found!"));
  return ret;
}

// RawMemPointStorageWriter ------------------------------------------------------------------------
RawMemPointStorageWriter::RawMemPointStorageWriter(string const & name)
  : m_fileWriter(name), m_data(kMaxNodesInOSM)
{
}

RawMemPointStorageWriter::~RawMemPointStorageWriter()
{
  m_fileWriter.Write(m_data.data(), m_data.size() * sizeof(LatLon));
}

void RawMemPointStorageWriter::AddPoint(uint64_t id, double lat, double lon)
{
  CHECK_LESS(id, m_data.size(),
             ("Found node with id", id, "which is bigger than the allocated cache size"));

  LatLon & ll = m_data[id];
  ToLatLon(lat, lon, ll);

  ++m_numProcessedPoints;
}

// MapFilePointStorageReader -----------------------------------------------------------------------
MapFilePointStorageReader::MapFilePointStorageReader(string const & name)
  : m_fileReader(name + kShortExtension)
{
  LOG(LINFO, ("Nodes reading is started"));

  uint64_t const count = m_fileReader.Size();

  uint64_t pos = 0;
  LatLonPos llp;
  LatLon ll;
  while (pos < count)
  {
    m_fileReader.Read(pos, &llp, sizeof(llp));
    pos += sizeof(llp);

    ll.m_lat = llp.m_lat;
    ll.m_lon = llp.m_lon;
    m_map.emplace(llp.m_pos, ll);
  }

  LOG(LINFO, ("Nodes reading is finished"));
}

bool MapFilePointStorageReader::GetPoint(uint64_t id, double & lat, double & lon) const
{
  auto const i = m_map.find(id);
  if (i == m_map.cend())
    return false;
  bool ret = FromLatLon(i->second, lat, lon);
  if (!ret)
  {
    LOG(LERROR, ("Inconsistent MapFilePointStorageReader. Node with id =", id,
                 "must exist but was not found"));
  }
  return ret;
}

// MapFilePointStorageWriter -----------------------------------------------------------------------
MapFilePointStorageWriter::MapFilePointStorageWriter(string const & name)
  : m_fileWriter(name + kShortExtension)
{
}

void MapFilePointStorageWriter::AddPoint(uint64_t id, double lat, double lon)
{
  LatLon ll;
  ToLatLon(lat, lon, ll);

  LatLonPos llp;
  llp.m_pos = id;
  llp.m_lat = ll.m_lat;
  llp.m_lon = ll.m_lon;

  m_fileWriter.Write(&llp, sizeof(llp));

  ++m_numProcessedPoints;
}

// SqlitePointStorageReader -----------------------------------------------------------------------
SqlitePointStorageReader::SqlitePointStorageReader(string const & name)
{
  LOG(LINFO, ("Nodes read from sqlite..."));

  // get database connection
  m_db = SqliteHelper::getInstance().getConnection(name + ".db");

  // make prepared statement
  sqlite3_prepare_v2(m_db, "SELECT lat, lon FROM nodes_tbl WHERE id=?;", -1, &m_pStmt, nullptr);

  LOG(LINFO, ("ready."));
}

SqlitePointStorageReader::~SqlitePointStorageReader()
{
  // finalize prepared statement
  sqlite3_finalize(m_pStmt);

  // close database connection
  SqliteHelper::getInstance().close(m_db);
}

bool SqlitePointStorageReader::GetPoint(uint64_t id, double & lat, double & lon) const
{
  bool result = false;

  // bind id
  sqlite3_bind_int64(m_pStmt, 1, id);

  // exec query
  while(sqlite3_step(m_pStmt) == SQLITE_ROW)
  {
    lat = sqlite3_column_double(m_pStmt, 0);
    lon = sqlite3_column_double(m_pStmt, 1);
    result = true;
  }

  // clear bind
  sqlite3_reset(m_pStmt);
  sqlite3_clear_bindings(m_pStmt);

  if(!result)
  {
    LOG(LERROR, ("Inconsistent SqlitePointStorageReader. Node with id =", id,
              "must exist but was not found"));
  }

  return result;
}

// SqlitePointStorageWriter -----------------------------------------------------------------------
SqlitePointStorageWriter::SqlitePointStorageWriter(string const & name)
{
  // get database connection
  m_db = SqliteHelper::getInstance().getConnection(name + ".db");

  // create table
  sqlite3_exec(m_db, "CREATE TABLE nodes_tbl(id INTEGER PRIMARY KEY, lat REAL, lon REAL);", nullptr, nullptr, nullptr);

  // make prepared statement
  sqlite3_prepare_v2(m_db, "INSERT INTO nodes_tbl(id, lat, lon) VALUES(?, ?, ?);", -1, &m_pStmt, nullptr);

  // start transaction
  sqlite3_exec(m_db, "BEGIN", nullptr, nullptr, nullptr);
}

SqlitePointStorageWriter::~SqlitePointStorageWriter()
{
  // commit transaction
  sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr);

  // finalize prepared statement
  sqlite3_finalize(m_pStmt);

  // close database connection
  SqliteHelper::getInstance().close(m_db);
}

void SqlitePointStorageWriter::AddPoint(uint64_t id, double lat, double lon)
{
  // bind values
  sqlite3_bind_int64(m_pStmt, 1, id);
  sqlite3_bind_double(m_pStmt, 2, lat);
  sqlite3_bind_double(m_pStmt, 3, lon);

  // insert
  while(sqlite3_step(m_pStmt) == SQLITE_BUSY);

  sqlite3_clear_bindings(m_pStmt);
  sqlite3_reset(m_pStmt);

  ++m_numProcessedPoints;
}

}  // namespace cache
}  // namespace generator
