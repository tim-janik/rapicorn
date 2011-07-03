// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_APPLICATION_HH__
#define __RAPICORN_APPLICATION_HH__

#include <ui/commands.hh>

namespace Rapicorn {

class ApplicationImpl : public ApplicationIface {
  vector<Wind0wIface*>     m_wind0ws;
  void                check_primaries        ();
public:
  virtual void        init_with_x11          (const std::string &application_identifier,
                                              const StringList  &cmdline_args);
  virtual String      auto_path              (const String  &file_name,
                                              const String  &binary_path,
                                              bool           search_vpath = true);
  virtual void        auto_load              (const std::string &defs_domain,
                                              const std::string &file_name,
                                              const std::string &binary_path,
                                              const std::string &i18n_domain = "");
  virtual void        load_string            (const std::string &xml_string,
                                              const std::string &i18n_domain = "");
  virtual Wind0wIface* create_wind0w         (const std::string &wind0w_identifier,
                                              const StringList &arguments = StringList(),
                                              const StringList &env_variables = StringList());
  void                add_wind0w             (Wind0wIface &wind0w);
  bool                remove_wind0w          (Wind0wIface &wind0w);
  virtual Wind0wList  list_wind0ws           ();
  virtual ItemIface*  unique_component       (const String &path);
  virtual ItemSeq     collect_components     (const String &path);
  virtual void        test_counter_set       (int val);
  virtual void        test_counter_add       (int val);
  virtual int         test_counter_get       ();
  virtual int         test_counter_inc_fetch ();
  static void         init_with_x11          (const String       &app_ident,
                                              int                *argcp,
                                              char              **argv,
                                              const StringVector &args = StringVector());
};

extern ApplicationImpl &app;

} // Rapicorn

#endif  /* __RAPICORN_APPLICATION_HH__ */
