// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <stdio.h>
#include "t303-mini-server-srvt.hh"
#include "t303-mini-server-clnt.hh"

using namespace Rapicorn;
using namespace Rapicorn::Aida;


class MiniServerImpl;
typedef std::shared_ptr<MiniServerImpl> MiniServerImplP;

class MiniServerImpl : public A1::MiniServerIface {
  MainLoopP         loop_;
  ServerConnectionP connection_;
  class LoopSource : public EventSource {
    ServerConnection &connection_;
    PollFD pfd_;
  public:
    LoopSource (ServerConnection &connection) :
      connection_ (connection)
    {
      pfd_.fd = connection_.notify_fd();
      pfd_.events = PollFD::IN;
      pfd_.revents = 0;
      add_poll (&pfd_);
    }
    virtual bool prepare  (const LoopState &state, int64 *timeout_usecs_p) override { return connection_.pending(); }
    virtual bool check    (const LoopState &state) override                         { return connection_.pending(); }
    virtual bool dispatch (const LoopState &state) override                         { connection_.dispatch(); return true; }
  };
  bool              vbool_;
  int32             vi32_;
  int64             vi64t_;
  double            vf64_;
  A1::CountEnum     count_;
  String            vstr_;
  A1::Location      location_;
  A1::StringSeq     strings_;
  A1::DerivedIfaceP derived_;
  int               sensor_;
public:
  void                      changed  (const String &what)              { sig_changed.emit (what); }
  virtual bool              vbool    () const                 override { return vbool_; }
  virtual void              vbool    (bool b)                 override { vbool_ = b; changed ("vbool"); }
  virtual int32             vi32     () const                 override { return vi32_; }
  virtual void              vi32     (int32 i)                override { vi32_ = i; changed ("vi32"); }
  virtual int64             vi64t    () const                 override { return vi64t_; }
  virtual void              vi64t    (int64 i)                override { vi64t_ = i; changed ("vi64t"); }
  virtual double            vf64     () const                 override { return vf64_; }
  virtual void              vf64     (double f)               override { vf64_ = f; changed ("vf64"); }
  virtual A1::CountEnum     count    () const                 override { return count_; }
  virtual void              count    (A1::CountEnum v)        override { count_ = v; changed ("count"); }
  virtual String            vstr     () const                 override { return vstr_; }
  virtual void              vstr     (const String &s)        override { vstr_ = s; changed ("vstr"); }
  virtual A1::Location      location () const                 override { return location_; }
  virtual void              location (const A1::Location &l)  override { location_ = l; changed ("location"); }
  virtual A1::StringSeq     strings  () const                 override { return strings_; }
  virtual void              strings  (const A1::StringSeq &q) override { strings_ = q; changed ("strings"); }
  virtual A1::DerivedIfaceP derived  () const                 override { return derived_; }
  virtual void              derived  (A1::DerivedIface *d)    override { derived_ = shared_ptr_cast<A1::DerivedIface> (d); changed ("derived"); }
  MiniServerImpl () : loop_ (MainLoop::create()), vbool_ (false), sensor_ (0) {}
  bool
  bind (const String &address)
  {
    assert_return (connection_ == NULL, false);
    connection_ = ServerConnection::bind<MiniServerIface> (address, shared_ptr_cast<MiniServerIface> (this));
    if (connection_)
      loop_->add (std::make_shared<LoopSource> (*connection_.get()));
    return connection_ != NULL;
  }
  EventLoop&   loop    ()                            { return *loop_; }
  int          run     ()                            { return loop_->run(); }
  virtual void message (const String &what) override { printout ("%s\n", what); }
  virtual void quit    () override                   { loop_->quit(); } // FIXME: loop is quit before remote references can be cleared
  virtual void test_parameters () override;
};

void
MiniServerImpl::test_parameters ()
{
  // Aida::Parameter
  Parameter pf64 (*this, "vf64", &MiniServerImpl::vf64, &MiniServerImpl::vf64);
  assert (vf64() == pf64.get().get<double>());
  vf64 (-0.75); assert (vf64() == -0.75);
  assert (vf64() == pf64.get().get<double>());
  // changed()
  sensor_ = 0;
  auto increment = [this] (const String &what) {
    if (what == "vf64")
      sensor_++;
  };
  size_t handlerid = pf64.sig_changed() += increment;
  assert (sensor_ == 0);
  vf64 (99);
  assert (sensor_ == 1);
  vf64 (98);
  assert (sensor_ == 2);
  vbool (true);
  assert (sensor_ == 2);
  vbool (false);
  assert (sensor_ == 2);
  pf64.sig_changed() -= handlerid;
  vf64 (97);
  assert (sensor_ == 2);
  vf64 (96);
  assert (sensor_ == 2);
  // cleanup
  vf64 (0);
  sensor_ = 0;
}

static void
a1_server_thread (AsyncBlockingQueue<String> *notify_queue)
{
  MiniServerImplP mini_server = std::make_shared<MiniServerImpl> ();
  const bool success = mini_server->bind ("inproc://aida-test-mini-server");
  if (!success)
    {
      notify_queue->push (string_format ("%s: failed to start mini-server: %s", __func__, strerror (errno)));
      return;
    }
  mini_server->loop().exec_now ([notify_queue] () { notify_queue->push ("OK"); });
  notify_queue = NULL; // remote destroys notify_queue after receiving our message
  mini_server->run();
}

static Parameter*
parameter_vector_find (std::vector<Parameter> &params, const String &name)
{
  for (size_t i = 0; i < params.size(); i++)
    if (params[i].name() == name)
      return &params[i];
  return NULL;
}

static void
test_server (A1::MiniServerH server)
{
  Any a, b;
  // test server side parameters
  server.test_parameters();
  // create Parameter for the server properties
  std::vector<Parameter> params;
  {
    Parameter::ListVisitor av;
    server.__accept_accessor__ (av);
    const std::vector<Parameter> &cparams = av.parameters();
    for (size_t i = 0; i < cparams.size(); i++)
      params.push_back (cparams[i]);    // tests Parameter copies
  }
  assert (params.size() > 0);
  // check known properties
  Parameter *param_vbool = parameter_vector_find (params, "vbool");
  assert (param_vbool != NULL);
  Parameter *param_vi32 = parameter_vector_find (params, "vi32");
  assert (param_vi32 != NULL);
  Parameter *param_vi64t = parameter_vector_find (params, "vi64t");
  assert (param_vi64t != NULL);
  Parameter *param_vf64 = parameter_vector_find (params, "vf64");
  assert (param_vf64 != NULL);
  Parameter *param_vstr = parameter_vector_find (params, "vstr");
  assert (param_vstr != NULL);
  Parameter *param_count = parameter_vector_find (params, "count");
  assert (param_count != NULL);
  // __aida_dir__
  StringVector sv = server.__aida_dir__();
  assert (string_vector_find (sv, "vbool") == "vbool");
  assert (string_vector_find (sv, "vi32") == "vi32");
  assert (string_vector_find (sv, "vi64t") == "vi64t");
  assert (string_vector_find (sv, "vf64") == "vf64");
  assert (string_vector_find (sv, "vstr") == "vstr");
  assert (string_vector_find (sv, "count") == "count");
  // vbool
  a = param_vbool->get();
  assert (a.kind() == Aida::BOOL);
  assert (a.get<bool>() == false);
  b = server.__aida_get__ ("vbool");
  assert (a == b);
  a.set (true);
  param_vbool->set (a);
  assert (server.vbool() == true);
  b = server.__aida_get__ ("vbool");
  assert (a == b);
  b.set (false);
  server.__aida_set__ ("vbool", b);
  a = param_vbool->get();
  assert (a == b);
  assert (param_vbool->get_aux ("blurb") == "Just true or false");
  assert (param_vbool->get_aux<bool> ("default") == true);
  assert (param_vbool->get_aux ("hints") == "rw");
  // vi32
  a = param_vi32->get();
  assert (a.kind() == Aida::INT64);
  assert (a.get<int32>() == 0);
  a.set (-750);
  param_vi32->set (a);
  b = param_vi32->get();
  assert (b.get<double>() == -750);
  assert (server.vi32() == -750);
  assert (param_vi32->get_aux ("label") == "Int32 Value");
  assert (param_vi32->get_aux<int64> ("min") == -2147483648);
  assert (param_vi32->get_aux<uint64> ("max") == 2147483647);
  assert (param_vi32->get_aux<int64> ("step") == 256);
  assert (param_vi32->get_aux<int64> ("default") == 32768);
  assert (param_vi32->get_aux ("hints") == "rw");
  // vi64t
  a = param_vi64t->get();
  assert (a.kind() == Aida::INT64);
  assert (a.get<int32>() == 0);
  a.set (-750);
  param_vi64t->set (a);
  b = param_vi64t->get();
  assert (b.get<double>() == -750);
  assert (server.vi64t() == -750);
  assert (param_vi64t->get_aux ("label") == "Int64 Value");
  assert (param_vi64t->get_aux<int64> ("min") == -9223372036854775808ULL);
  assert (param_vi64t->get_aux<uint64> ("max") == 9223372036854775807);
  assert (param_vi64t->get_aux<int64> ("step") == 65536);
  assert (param_vi64t->get_aux<int64> ("default") == -65536);
  assert (param_vi64t->get_aux ("hints") == "rw");
  // vf64
  a = param_vf64->get();
  assert (a.kind() == Aida::FLOAT64);
  assert (a.get<double>() == 0);
  b = server.__aida_get__ ("vf64");
  assert (a == b);
  a.set (-0.75);
  param_vf64->set (a);
  b = param_vf64->get();
  assert (b.get<double>() == -0.75);
  assert (server.vf64() == -0.75);
  a = server.__aida_get__ ("vf64");
  assert (a == b);
  a.set (+0.25);
  server.__aida_set__ ("vf64", a);
  b = server.__aida_get__ ("vf64");
  assert (b.get<double>() == +0.25);
  assert (param_vf64->get_aux ("label") == "Float Value");
  assert (param_vf64->get_aux<double> ("min") == 0.0);
  assert (param_vf64->get_aux<double> ("max") == 1.0);
  assert (param_vf64->get_aux<double> ("step") == 0.1);
  assert (param_vf64->get_aux ("hints") == "rw");
  // vstr
  a = param_vstr->get();
  assert (a.kind() == Aida::STRING);
  assert (a.get<String>() == "");
  b = server.__aida_get__ ("vstr");
  assert (a == b);
  server.vstr ("123");
  a = param_vstr->get();
  assert (a.get<String>() == "123");
  b = server.__aida_get__ ("vstr");
  assert (a == b);
  a.set ("ERPL");
  server.__aida_set__ ("vstr", a);
  b = server.__aida_get__ ("vstr");
  assert (b.get<String>() == "ERPL");
  a.set ("ZOOT");
  param_vstr->set (a);
  assert (server.vstr() == "ZOOT");
  b = server.__aida_get__ ("vstr");
  assert (b.get<String>() == "ZOOT");
  assert (param_vstr->get_aux ("default") == "foobar");
  assert (param_vstr->get_aux ("hints") == "rw");
  // count
  server.count (A1::CountEnum::TWO);
  a = param_count->get();
  assert (a.kind() == Aida::ENUM);
  assert (a.get<A1::CountEnum>() == A1::CountEnum::TWO);
  a.set (A1::CountEnum::THREE);
  assert (a.kind() == Aida::ENUM);
  param_count->set (a);
  assert (server.count() == A1::CountEnum::THREE);
  assert (param_count->get_aux ("hints") == "rw");
  assert (param_count->get_aux<int> ("default") == 2);
  // XML serialize properties
  {
    XmlNodeP xnode = XmlNode::create_parent ("MiniServer", 0, 0, "");
    ToXmlVisitor toxml_visitor (xnode);
    server.__accept_accessor__ (toxml_visitor);
    const String xml_result = xnode->xml_string (0, true);
    assert (xml_result.find ("ZOOT") != xml_result.npos);
    server.vstr ("");
    FromXmlVisitor fromxml_visitor (xnode);
    server.__accept_accessor__ (fromxml_visitor);
    assert (server.vstr () == "ZOOT");
  }
  // INI serialize properties
  {
    IniWriter iniwriter;
    ToIniVisitor toini_visitor (iniwriter, "MiniServer.");
    server.__accept_accessor__ (toini_visitor);
    const String ini_result = iniwriter.output();
    assert (ini_result.find ("ZOOT") != ini_result.npos);
    server.vstr ("");
    IniFile inifile ("-", ini_result);
    FromIniVisitor fromini_visitor (inifile, "MiniServer.");
    server.__accept_accessor__ (fromini_visitor);
    assert (server.vstr () == "ZOOT");
  }
  // echo
  server.message ("  CHECK  MiniServer property access                                      OK");
}

static void
test_a1_server ()
{
  AsyncBlockingQueue<String> notify_queue;
  std::thread sthread (a1_server_thread, &notify_queue);
  String result = notify_queue.pop();
  if (result == "OK")
    {
      ClientConnectionP connection = ClientConnection::connect ("inproc://aida-test-mini-server");
      A1::MiniServerH server;
      if (connection)
        server = connection->remote_origin<A1::MiniServerH>();
      if (!connection)
        result = string_format ("%s: failed to connect to mini-server: %s", __func__, strerror (errno));
      if (!server)
        result = string_format ("%s: failed to talk to mini-server: %s", __func__, strerror (errno));
      else
        {
          server.message ("  CHECK  MiniServer remote call successfull                              OK");
          test_server (server);
          server.quit();
        }
      new ClientConnectionP (connection); // FIXME: need to leak this because ~ClientConnection is unsupported
    }
  if (result != "OK")
    fatal ("%s: %s", __func__, result);
  sthread.join();
}
