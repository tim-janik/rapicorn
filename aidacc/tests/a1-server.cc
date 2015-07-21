// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include <stdio.h>
#include "components-a1-server.hh"
#include "components-a1-client.hh"

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
public:
  MiniServerImpl () : loop_ (MainLoop::create()) {}
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
  virtual void quit    ()                            { loop_->quit(); } // FIXME: loop is quit before remote references can be cleared
};

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
          server.quit();
        }
      new ClientConnectionP (connection); // FIXME: need to leak this because ~ClientConnection is unsupported
    }
  if (result != "OK")
    fatal ("%s: %s", __func__, result);
  sthread.join();
}
