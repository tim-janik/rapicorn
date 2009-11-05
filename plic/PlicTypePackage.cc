/* PlicTypePackage.cc - PLIC TypePackage Parser
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

/* --- main types --- */
struct TypeRegistry;    //** Registry for TypePackage streams
struct TypeNamespace;   //** Handle for TypePackage namespace
struct TypeInfo;        //** Handle for all types

/* --- config --- */
#ifndef TRACE_ERRORS
#define TRACE_ERRORS 1  // adds debugging info to funcs and error strings
#endif
#ifndef ALLOW_EMPTY_NAMESPACES
#define ALLOW_EMPTY_NAMESPACES 1
#endif

/* --- auxillary types --- */
typedef unsigned char uint8;
typedef unsigned int  uint;
typedef uint8         byte;
typedef std::string   String;
using std::vector;
using std::map;

/* --- auxillalry macros --- */
#define EXPR2STR(x)             #x
#define STRINGIFY(macroarg)     EXPR2STR (macroarg) // must use macro indirection to expand __LINE__
#define STRLINE                 STRINGIFY (__LINE__)
#define TRACABLESTR(s)          String (String (s) + String (TRACE_ERRORS ? " #" STRLINE "" : ""))
#define return_if(cond,string)  do { if (cond) return TRACABLESTR (string); } while (0)
#define error(...)              do { fputs ("ERROR: ", stderr); fprintf (stderr, __VA_ARGS__); fputs ("\n", stderr); abort(); } while (0)
#define error_if(cond,string)   do { if (cond) error ("%s", TRACABLESTR (string).c_str()); } while (0)
#define DEBUG(...)              do { if (TRACE_ERRORS) fprintf (stderr, __VA_ARGS__); } while (0)
#define MAXPOINTER              ((const byte*) 0xffffffffffffffffULL)
#define ERRPREMATURE            "premature end"
#define ERRINVALNSTABLE         "invalid namespace table"

/* --- auxillary functions --- */
static inline String
parse_int (const byte **dp,
           const byte  *boundary,
           uint        *ui)
{
  return_if (*dp + 4 > boundary, ERRPREMATURE);
  const byte *us = *dp, u0 = us[0], u1 = us[1], u2 = us[2], u3 = us[3];
  *dp += 4;
  if (u0 >= 128 || u1 >= 128 || u2 >= 128 || u3 >= 128)
    {
      *ui = (u3 & 0x7f) + ((u2 & 0x7f) >> 1) + ((u1 & 0x7f) >> 2) + ((u0 & 0x7f) >> 3);
      return "";
    }
  else if ((u0 >= '0' && u0 <= '9') || u0 == '.'  || u0 == '\n')
    {
      uint i = 0;
      if (u0 >= '0' && u0 <= '9')
        i = u0 - '0';
      if (u1 >= '0' && u1 <= '9')
        i = u1 - '0' + i * 10;
      if (u2 >= '0' && u2 <= '9')
        i = u2 - '0' + i * 10;
      if (u3 >= '0' && u3 <= '9')
        i = u3 - '0' + i * 10;
      *ui = i;
      return "";
    }
  DEBUG ("INT? %c%c%c%c\n", u0, u1, u2, u3);
  return "invalid integer";
}

static inline String
parse_string (const byte **dp,
              const byte  *boundary,
              String      *result)
{
  uint len = 0;
  String err = parse_int (dp, boundary, &len);
  return_if (err != "", err);
  uint dlen = (len + 3) / 4 * 4;
  return_if (*dp + dlen > boundary, ERRPREMATURE);
  if (result)
    *result = String ((const char*) *dp, len);
  *dp += dlen;
  return "";
}

static String
parse_strings (uint                 count,
               const byte         **dp,
               const byte          *boundary,
               vector<const byte*> *strlist)
{
  // parse @count consecutive strings: llll StringOfLengthllll
  for (uint i = 0; i < count; i++)
    {
      strlist->push_back (*dp);
      String err = parse_string (dp, boundary, NULL); // skip ahead
      return_if (err != "", err);
    }
  return "";
}

/* --- type info --- */
struct TypeInfo {
  uint                storage;
  const byte         *namep;    // pointer  to [ llll TypeName ]
  vector<const byte*> auxdata;  // pointers to [ llll AuxEntry ]
  const byte         *rtypep;   // __Ts: pointer to [ llll TypeReference ]
  uint                nnnn;     // type specific count
  const byte         *tdata;    // start of data after nnnn
  uint                tlength;
  String              name ();
  uint                n_aux_strings ();
  const char*         aux_string (uint nth, uint *length);
  String              parse_type_info (uint        length,
                                       const byte *data);
  TypeInfo() :
    storage (0), namep (NULL), rtypep (NULL),
    nnnn (0), tdata (NULL), tlength (0)
  {}
};

/* TypeInfo Layout:
 * - __?                        // storage type
 * - llll TypeName              // type or field name
 * - _Ts: llll TypeReference    // non-fundamental type references
 * - kkkk                       // number of aux entries
 * - [ llll AuxEntry ]*         // kkkk times aux entry strings
 * - _Es: nnnn                  // number of values for choice types
 * - _Es: [ llll Ident llll Label llll Blurb ]+
 * - __c: nnnn                  // number of prerequisites for interface types
 * - __c: [ llll PrerequisiteName ]+
 * - _Ra: nnnn                  // number of fields for record types
 * - _Ra: [ _TYPE_INFO_ ]+
 * - _Qa: _TYPE_INFO_           // field info for sequence types
 */

String
TypeInfo::parse_type_info (uint        length,
                           const byte *data)
{
  error_if (namep != NULL, "type info reparsing attempted");
  const byte *dp = data, *boundary = dp + length;
  // parse \n__? storage type
  return_if (dp + 4 > boundary, ERRPREMATURE);
  const bool storageok = dp[0] == '\n' && dp[1] == '_' &&
                         ((dp[2] == '_' && strchr ("idsac", dp[3]) != 0) ||
                          (strchr ("ET", dp[2]) && dp[3] == 's') ||
                          (strchr ("QR", dp[2]) && dp[3] == 'a'));
  return_if (!storageok, "invalid type format");
  storage = dp[3] + (dp[2] == '_' ? 0 : dp[2]) * 256;
  dp += 4;
  // store and validate (skip) type name
  const byte *typenamep = dp;
  String err = parse_string (&dp, boundary, NULL);
  return_if (err != "", err);
  namep = typenamep;
  // parse number of aux entries
  uint auxcount = 0;
  err = parse_int (&dp, boundary, &auxcount);
  return_if (err != "", err);
  // parse aux entries
  err = parse_strings (auxcount, &dp, boundary, &auxdata);
  return_if (err != "", err);
  // parse type specific counter
  if (storage == 'R' * 256 + 'a' || // record
      storage == 'E' * 256 + 's' || // enum
      storage == 'c')               // class
    {
      err = parse_int (&dp, boundary, &nnnn);
      return_if (err != "", err);
    }
  // store type data pointer
  if (storage == 'T' * 256 + 's' || // type reference
      storage == 'E' * 256 + 's' || // enum
      storage == 'R' * 256 + 'a' || // record
      storage == 'Q' * 256 + 'a' || // sequence
      storage == 'c')               // class
    {
      tdata = dp;
      tlength = boundary - dp;
    }
  return "";
}

String
TypeInfo::name ()
{
  const byte *dp = namep;
  String result, err = parse_string (&dp, MAXPOINTER /*prevalidated*/, &result);
  error_if (err != "", err);
  return result;
}

uint
TypeInfo::n_aux_strings ()
{
  return auxdata.size();
}

const char*
TypeInfo::aux_string (uint  nth,
                      uint *lengthp)
{
  if (nth >= auxdata.size())
    {
      *lengthp = 0;
      return NULL;
    }
  const byte *dp = auxdata[nth];
  String err = parse_int (&dp, MAXPOINTER /*prevalidated*/, lengthp);
  error_if (err != "", err);
  return (const char*) dp;
}

/* --- type namespace --- */
struct TypeNamespace {
  const byte         *m_fullnamep;
  const byte         *m_type_table;
  String              fullname ();
  vector<TypeInfo>    list_types ();
  String              parse_namespace (uint        data_length,
                                       const byte *data);
  TypeNamespace() :
    m_fullnamep (NULL),
    m_type_table (NULL)
  {}
};

/* Type Namespace Layout:
 * - llll NamespaceName         // full namespace name for type lookups
 * - nnnn                       // number of types, offset fixpoint
 * - jjj1 jjj2 ... jjjn         // index of types 1..n
 * - jjjT                       // tail index, points after last type
 * - [ __TYPE_INFO__ ]+
 */

String
TypeNamespace::parse_namespace (uint        data_length,
                                const byte *data)
{
  error_if (m_fullnamep || m_type_table, "namespace reparsing attempted");
  const byte *dp = data, *boundary = dp + data_length;
  // parse namespace name
  String err = parse_string (&dp, boundary, NULL);
  return_if (err != "", err);
  m_fullnamep = data;
  // save pointer to type table
  const byte *type_tablep = dp;
  // parse number of types
  uint ntypes = 0;
  err = parse_int (&dp, boundary, &ntypes);
  return_if (err != "", err);
  if (!ALLOW_EMPTY_NAMESPACES)
    return_if (ntypes < 1, "empty namespace");
  // validate tail pointer
  dp += ntypes * 4;
  return_if (dp + 4 > boundary, ERRPREMATURE);
  uint tailoffset = 0;
  err = parse_int (&dp, boundary, &tailoffset);
  return_if (err != "", err);
  return_if (type_tablep - data + tailoffset != data_length, "invalid type table");
  // store type table
  m_type_table = type_tablep;
  return "";
}

vector<TypeInfo>
TypeNamespace::list_types ()
{
  vector<TypeInfo> type_vector;
  const byte *dp = m_type_table;
  // number of types
  uint ntypes = 0;
  String err = parse_int (&dp, dp + 4, // table size is prevalidated
                          &ntypes);
  error_if (err != "", err);
  const byte *table_boundary = dp + (ntypes + 1) * 4;
  // parse tail pointer (prevalidated)
  const byte *tp = dp + 4 * ntypes;
  uint tailoffset = 0;
  err = parse_int (&tp, tp + 4, &tailoffset);
  error_if (err != "", err);
  // parse first offset
  uint offset = 0;
  err = parse_int (&dp, table_boundary, &offset);
  error_if (err != "", err);
  for (uint i = 0; i < ntypes; i++)
    {
      // parse next offset, extract length
      uint next_offset = 0;
      err = parse_int (&dp, table_boundary, &next_offset);
      error_if (err != "", err);
      // avoid overflows
      error_if (next_offset <= offset, "0-length namespace");
      error_if (next_offset > tailoffset, ERRINVALNSTABLE);
      // parse type info
      TypeInfo ti;
      err = ti.parse_type_info (next_offset - offset, m_type_table + offset);
      error_if (err != "", err);
      type_vector.push_back (ti);
      offset = next_offset;
    }
  return type_vector;
}

String
TypeNamespace::fullname ()
{
  const byte *dp = m_fullnamep;
  String result, err = parse_string (&dp, MAXPOINTER /*prevalidated*/, &result);
  error_if (err != "", err);
  return result;
}

/* --- type registry --- */
struct TypeRegistry {
  vector<const uint8*>      m_namespace_tables;
  vector<TypeNamespace>     list_namespaces ();
  String                    register_type_package (uint         data_length,
                                                   const uint8 *data);
};

/* Type Package Layout:
 * - PlicTypePkg_01\r\n         // magic
 * - llll TypePackageName       // not used for type lookups
 * - nnnn                       // number of namespaces, offset fixpoint
 * - jjj1 jjj2 ... jjjn         // index of namespace 1..n
 * - jjjT                       // tail index, points after last namespace
 * - [ __NAMESPACE__ ]+
 */

String
TypeRegistry::register_type_package (uint         data_length,
                                     const uint8 *data)
{
  const byte *dp = data, *boundary = dp + data_length;
  // parse type package magic
  return_if (dp + 12 > boundary, "unknown package format");
  return_if (memcmp (dp, "PlicTypePkg_", 12) != 0, "unknown package format");
  dp += 12;
  // parse 'TPKG01\r\n' magic
  return_if (dp + 4 > boundary, "unknown package format");
  return_if (memcmp (dp, "01\r\n", 4) != 0, "unknown package format");
  dp += 4;
  // parse package name
  String package_name;
  String err = parse_string (&dp, boundary, &package_name);
  return_if (err != "", err);
  // save pointer to namespace table
  const byte *namespace_tablep = dp;
  // parse number of namespaces
  uint nnamespaces = 0;
  err = parse_int (&dp, boundary, &nnamespaces);
  return_if (err != "", err);
  return_if (nnamespaces < 1, "empty type package");
  // validate tail pointer
  dp += nnamespaces * 4;
  return_if (dp + 4 > boundary, ERRPREMATURE);
  uint tailoffset = 0;
  err = parse_int (&dp, boundary, &tailoffset);
  return_if (err != "", err);
  return_if (namespace_tablep - data + tailoffset != data_length, ERRINVALNSTABLE);
  // register namespace table
  m_namespace_tables.push_back (namespace_tablep);
  return "";
}

vector<TypeNamespace>
TypeRegistry::list_namespaces ()
{
  vector<TypeNamespace> namespace_vector;
  for (uint j = 0; j < m_namespace_tables.size(); j++)
    {
      const byte *dp = m_namespace_tables[j];
      // parse number of namespaces
      uint nnamespaces = 0;
      String err = parse_int (&dp, dp + 4, // table size is prevalidated
                              &nnamespaces);
      error_if (err != "", err);
      const byte *table_boundary = dp + (nnamespaces + 1) * 4;
      // parse tail pointer (prevalidated)
      const byte *tp = dp + 4 * nnamespaces;
      uint tailoffset = 0;
      err = parse_int (&tp, tp + 4, &tailoffset);
      error_if (err != "", err);
      // parse first offset
      uint offset = 0;
      err = parse_int (&dp, table_boundary, &offset);
      error_if (err != "", err);
      for (uint i = 0; i < nnamespaces; i++)
        {
          // parse next offset, extract length
          uint next_offset = 0;
          err = parse_int (&dp, table_boundary, &next_offset);
          error_if (err != "", err);
          // avoid overflows
          error_if (next_offset <= offset, "0-length namespace");
          error_if (next_offset > tailoffset, ERRINVALNSTABLE);
          // parse namespace
          TypeNamespace tns;
          err = tns.parse_namespace (next_offset - offset, m_namespace_tables[j] + offset);
          error_if (err != "", err);
          namespace_vector.push_back (tns);
          offset = next_offset;
        }
    }
  return namespace_vector;
}
