// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "stock.hh"

namespace Rapicorn {

// == StockFile ==
struct StockFile {
  std::shared_ptr<IniFile> ifile_;
  String                   input_;
public:
  explicit StockFile       (Blob blob) : input_ (blob.name())          { ifile_ = std::make_shared<IniFile> (blob); }
  String   stock_element    (const String &stock_id, const String &what) { return ifile_->value_as_string (stock_id + "." + what); }
  String   dir_path        ()                                           { return Path::dirname (input_); }
  String   file_path       (const String &file)                         { return Path::join (dir_path(), file); }
  IniFile& ini_file        ()                                           { return *ifile_; }
};

// == Stock ==
static Mutex             stock_mutex;
static vector<StockFile> stock_files;

static void
init_stock_lib (const StringVector &args)
{
  const ScopedLock<Mutex> sl (stock_mutex);
  StockFile std_stock_file (Res ("@res Rapicorn/stock.ini").as<Blob>());
  if (!std_stock_file.ini_file().has_sections())
    fatal ("failed to load builtin: %s", "Rapicorn/stock.ini");
  stock_files.push_back (std_stock_file);
}
static InitHook _init_stock_lib ("core/50 Init Stock Lib", init_stock_lib);

Stock::Stock (const String &stock_id) :
  stock_id_ (stock_id)
{}

/// Retrieve and load the binary contents referred to by the "icon" attribute of @a stock_id.
String
Stock::icon() const
{
  const ScopedLock<Mutex> sl (stock_mutex);
  for (auto sf : stock_files)
    {
      String icon_source = sf.stock_element (stock_id_, "icon");
      if (!icon_source.empty())
        return "@res " + sf.file_path (icon_source);
    }
  errno = ENOENT;
  return String();
}

/// Retrieve the @a key attribute of @a stock_id as a string.
String
Stock::element (const String &key) const
{
  const ScopedLock<Mutex> sl (stock_mutex);
  for (auto sf : stock_files)
    {
      String s = sf.stock_element (stock_id_, key);
      if (!s.empty())
        return s;
    }
  return "";
}

/// Retrieve the "label" string attribute of @a stock_id.
String
Stock::label() const
{
  return element ("label");
}

/// Retrieve the "tooltip" string attribute of @a stock_id.
String
Stock::tooltip() const
{
  return element ("tooltip");
}

/// Retrieve the "accelerator" string attribute of @a stock_id.
String
Stock::accelerator() const
{
  return element ("accelerator");
}

/**
 * @class Stock
 * Stock resource retrieval facility.
 * The stock system provides a number of predefined labels, tooltips, accelerators and
 * icons out of the box, accessible via a specific identifier, the @a stock_id.
 */

} // Rapicorn
