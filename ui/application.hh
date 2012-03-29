// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_APPLICATION_HH__
#define __RAPICORN_APPLICATION_HH__

#include <ui/commands.hh>

namespace Rapicorn {

class ApplicationImpl : public ApplicationIface {
  vector<WindowIface*>     m_windows;
public:
  virtual String      auto_path              (const String  &file_name,
                                              const String  &binary_path,
                                              bool           search_vpath = true);
  virtual StringList  auto_load              (const std::string &defs_domain,
                                              const std::string &file_name,
                                              const std::string &binary_path,
                                              const std::string &i18n_domain = "");
  virtual void        load_string            (const std::string &xml_string,
                                              const std::string &i18n_domain = "");
  virtual WindowIface* create_window         (const std::string &window_identifier,
                                              const StringList &arguments = StringList(),
                                              const StringList &env_variables = StringList());
  void                add_window             (WindowIface &window);
  bool                remove_window          (WindowIface &window);
  virtual WindowIface*query_window           (const String &selector);
  virtual WindowList  query_windows          (const String &selector);
  virtual WindowList  list_windows           ();
  virtual ListModelRelayIface* create_list_model_relay (int                n_columns,
                                                        const std::string &name = "");
  virtual void        test_counter_set       (int val);
  virtual void        test_counter_add       (int val);
  virtual int         test_counter_get       ();
  virtual int         test_counter_inc_fetch ();
  void                lost_primaries         ();
  static ApplicationImpl& the                ();
};

} // Rapicorn

#endif  /* __RAPICORN_APPLICATION_HH__ */
