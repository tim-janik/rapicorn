// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <ui/widget.hh> // unguarded, because widget.hh includes commands.hh

#ifndef __RAPICORN_COMMANDS_HH__
#define __RAPICORN_COMMANDS_HH__

namespace Rapicorn {

struct Command {
  const String ident;
  const String blurb;
  bool         needs_arg;
  explicit     Command (const char *cident, const char *cblurb, bool has_arg);
  virtual     ~Command();
  virtual bool exec (ObjectImpl *obj, const StringSeq &args) = 0;
};
typedef std::shared_ptr<Command> CommandP;

struct CommandList : vector<CommandP> {
  CommandList () {}
  template<typename Array>
  CommandList (Array &a, const CommandList &chain = CommandList())
  {
    const size_t array_length = sizeof (a) / sizeof (a[0]);
    for (size_t i = 0; i < array_length; i++)
      push_back (a[i]);
    for (auto cmd : chain)
      push_back (cmd);
  }
};

#define RAPICORN_MakeNamedCommand(Type, cident, blurb, method, data)   \
  create_command (&Type::method, cident, blurb, data)

/* --- command implementations --- */
/* command with data and arg string */
template<class Class, class Data>
struct CommandDataArg : Command {
  typedef bool (Class::*CommandMethod) (Data data, const StringSeq &args);
  bool (Class::*command_method) (Data data, const StringSeq &args);
  Data data;
  CommandDataArg (bool (Class::*method) (Data, const String&),
                  const char *cident, const char *cblurb, const Data &method_data);
  virtual bool exec (ObjectImpl *obj, const StringSeq &args);
};
template<class Class, class Data> inline CommandP
create_command (bool (Class::*method) (Data, const String&),
                const char *ident, const char *blurb, const Data &method_data)
{ return std::make_shared<CommandDataArg<Class,Data>> (method, ident, blurb, method_data); }

/* command with data */
template<class Class, class Data>
struct CommandData : Command {
  typedef bool (Class::*CommandMethod) (Data data);
  bool (Class::*command_method) (Data data);
  Data data;
  CommandData (bool (Class::*method) (Data),
               const char *cident, const char *cblurb, const Data &method_data);
  virtual bool exec (ObjectImpl *obj, const StringSeq&);
};
template<class Class, class Data> inline CommandP
create_command (bool (Class::*method) (Data),
                const char *ident, const char *blurb, const Data &method_data)
{ return std::make_shared<CommandData<Class,Data>> (method, ident, blurb, method_data); }

/* command with arg string */
template<class Class>
struct CommandArg: Command {
  typedef bool (Class::*CommandMethod) (const StringSeq &args);
  bool (Class::*command_method) (const StringSeq &args);
  CommandArg (bool (Class::*method) (const String&),
              const char *cident, const char *cblurb);
  virtual bool exec (ObjectImpl *obj, const StringSeq &args);
};
template<class Class> inline CommandP
create_command (bool (Class::*method) (const String&),
                const char *ident, const char *blurb)
{ return std::make_shared<CommandArg<Class>> (method, ident, blurb); }

/* --- implementations --- */
/* command with data and arg string */
template<class Class, typename Data>
CommandDataArg<Class,Data>::CommandDataArg (bool (Class::*method) (Data, const String&),
                                            const char *cident, const char *cblurb, const Data &method_data) :
  Command (cident, cblurb, true),
  command_method (method),
  data (method_data)
{}
template<class Class, typename Data> bool
CommandDataArg<Class,Data>::exec (ObjectImpl *obj, const StringSeq &args)
{
  Class *instance = dynamic_cast<Class*> (obj);
  if (!instance)
    fatal ("Rapicorn::Command: invalid command object: %s", obj->typeid_name().c_str());
  return (instance->*command_method) (data, args);
}

/* command arg string */
template<class Class>
CommandArg<Class>::CommandArg (bool (Class::*method) (const String&),
                               const char *cident, const char *cblurb) :
  Command (cident, cblurb, true),
  command_method (method)
{}
template<class Class> bool
CommandArg<Class>::exec (ObjectImpl *obj, const StringSeq &args)
{
  Class *instance = dynamic_cast<Class*> (obj);
  if (!instance)
    fatal ("Rapicorn::Command: invalid command object: %s", obj->typeid_name().c_str());
  return (instance->*command_method) (args);
}

/* command with data */
template<class Class, typename Data>
CommandData<Class,Data>::CommandData (bool (Class::*method) (Data),
                                      const char *cident, const char *cblurb, const Data &method_data) :
  Command (cident, cblurb, false),
  command_method (method),
  data (method_data)
{}
template<class Class, typename Data> bool
CommandData<Class,Data>::exec (ObjectImpl *obj, const StringSeq&)
{
  Class *instance = dynamic_cast<Class*> (obj);
  if (!instance)
    fatal ("Rapicorn::Command: invalid command object: %s", obj->typeid_name().c_str());
  return (instance->*command_method) (data);
}

} // Rapicorn

#endif  /* __RAPICORN_COMMANDS_HH__ */
