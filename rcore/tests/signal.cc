/* RapicornSignal
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include <rcore/testutils.hh>
#include <unistd.h>

namespace {
using namespace Rapicorn;

static uint ExtraType_deletions = 0;

struct ExtraType {
  virtual            ~ExtraType  () { ExtraType_deletions++; }
  virtual const char* message    () { return "ExtraType::message()"; }
  bool                operator== (const ExtraType &other) const { return false; }
};

struct EmitterMany {
  virtual ~EmitterMany() {}
  virtual const char* emitter_check() { return __func__; }
  Signal<EmitterMany, void ()> sig_void_0;
  Signal<EmitterMany, void (int   )> sig_void_1;
  Signal<EmitterMany, void (int   , double)> sig_void_2;
  Signal<EmitterMany, void (int   , double, int   )> sig_void_3;
  Signal<EmitterMany, void (int   , double, int   , double)> sig_void_4;
  Signal<EmitterMany, void (int   , double, int   , double, int   )> sig_void_5;
  Signal<EmitterMany, void (double, int   , double, int   , double, int   )> sig_void_6;
  Signal<EmitterMany, void (int   , double, int   , double, int   , double, int   )> sig_void_7;
  Signal<EmitterMany, void (double, int   , double, int   , double, int   , double, int   )> sig_void_8;
  Signal<EmitterMany, void (int   , double, int   , double, int   , double, int   , double, int   )> sig_void_9;
  Signal<EmitterMany, void (double, int   , double, int   , double, int   , double, int   , double, int   )> sig_void_10;
  Signal<EmitterMany, void (int   , float , int   , float , int   , float , int   , float , int   , float , int   )> sig_void_11;
  Signal<EmitterMany, void (int   , char  , int   , char  , int   , char  , int   , char  , int   , char  , int   , char  )> sig_void_12;
  Signal<EmitterMany, void (short , int   , short , int   , short , int   , short , int   , short , int   , short , int   , short )> sig_void_13;
  Signal<EmitterMany, void (int   , double, int   , double, int   , double, int   , double, int   , double, int   , double, int   , double)> sig_void_14;
  Signal<EmitterMany, void (double, int   , double, int   , double, int   , double, int   , double, int   , double, int   , double, int   , double)> sig_void_15;
  Signal<EmitterMany, void (int   , double, short , uint  , float , char  , uint8 , long  , int64 , char  , uint  , float , char  , double, uint64, short )> sig_void_16;
  Signal<EmitterMany, void (const String&, const String&, const char*)> sig_void_cref;
  Signal<EmitterMany, String ()> sig_string_0;
  Signal<EmitterMany, String (int   )> sig_string_1;
  Signal<EmitterMany, String (int   , double)> sig_string_2;
  Signal<EmitterMany, String (int   , double, int   )> sig_string_3;
  Signal<EmitterMany, String (int   , double, int   , double)> sig_string_4;
  Signal<EmitterMany, String (int   , double, int   , double, int   )> sig_string_5;
  Signal<EmitterMany, String (double, int   , double, int   , double, int   )> sig_string_6;
  Signal<EmitterMany, String (int   , double, int   , double, int   , double, int   )> sig_string_7;
  Signal<EmitterMany, String (double, int   , double, int   , double, int   , double, int   )> sig_string_8;
  Signal<EmitterMany, String (int   , double, int   , double, int   , double, int   , double, int   )> sig_string_9;
  Signal<EmitterMany, String (double, int   , double, int   , double, int   , double, int   , double, int   )> sig_string_10;
  Signal<EmitterMany, String (int   , float , int   , float , int   , float , int   , float , int   , float , int   )> sig_string_11;
  Signal<EmitterMany, String (int   , char  , int   , char  , int   , char  , int   , char  , int   , char  , int   , char  )> sig_string_12;
  Signal<EmitterMany, String (short , int   , short , int   , short , int   , short , int   , short , int   , short , int   , short )> sig_string_13;
  Signal<EmitterMany, String (int   , double, int   , double, int   , double, int   , double, int   , double, int   , double, int   , double)> sig_string_14;
  Signal<EmitterMany, String (double, int   , double, int   , double, int   , double, int   , double, int   , double, int   , double, int   , double)> sig_string_15;
  Signal<EmitterMany, String (int   , double, short , uint  , float , char  , uint8 , long  , int64 , char  , uint  , float , char  , double, uint64, short )> sig_string_16;
  EmitterMany() :
    sig_void_0 (*this), sig_void_1 (*this), sig_void_2 (*this), sig_void_3 (*this),
    sig_void_4 (*this), sig_void_5 (*this), sig_void_6 (*this), sig_void_7 (*this),
    sig_void_8 (*this), sig_void_9 (*this), sig_void_10 (*this), sig_void_11 (*this),
    sig_void_12 (*this), sig_void_13 (*this), sig_void_14 (*this), sig_void_15 (*this), sig_void_16 (*this),
    sig_void_cref (*this),
    sig_string_0 (*this), sig_string_1 (*this), sig_string_2 (*this), sig_string_3 (*this),
    sig_string_4 (*this), sig_string_5 (*this), sig_string_6 (*this), sig_string_7 (*this),
    sig_string_8 (*this), sig_string_9 (*this), sig_string_10 (*this), sig_string_11 (*this),
    sig_string_12 (*this), sig_string_13 (*this), sig_string_14 (*this), sig_string_15 (*this), sig_string_16 (*this)
  {}
  static String test_string_14_emitter_data (EmitterMany &em,int a1, double a2, int a3, double a4, int a5, double a6, int a7, double a8, int a9,
                                             double a10, int a11, double a12, int a13, double a14, ExtraType x)
  {
    TINFO ("  callback: %s (%s, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %s);\n",
           __func__, em.emitter_check(), a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, x.message());
    return __func__;
  }
  static String test_string_14_data (int a1, double a2, int a3, double a4, int a5, double a6, int a7, double a8, int a9,
                                     double a10, int a11, double a12, int a13, double a14, ExtraType x)
  {
    TINFO ("  callback: %s (%d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %s);\n",
           __func__, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, x.message());
    return __func__;
  }
  static String test_string_14_refdata (int a1, double a2, int a3, double a4, int a5, double a6, int a7, double a8, int a9,
                                        double a10, int a11, double a12, int a13, double a14, ExtraType &xref)
  {
    TINFO ("  callback: %s (%d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %s);\n",
           __func__, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, xref.message());
    return __func__;
  }
  void do_void_cref (const String &s, const String &t, const char *c) { assert (s != t && s == c); }
  void testme ()
  {
    TSTART ("Signals/Multi-arg");
    sig_void_0.emit ();
    sig_void_1.emit (1);
    sig_void_2.emit (1, 2);
    sig_void_3.emit (1, 2, 3);
    sig_void_4.emit (1, 2, 3, 4);
    sig_void_5.emit (1, 2, 3, 4, 5);
    sig_void_6.emit (1, 2, 3, 4, 5, 6);
    sig_void_7.emit (1, 2, 3, 4, 5, 6, 7);
    sig_void_8.emit (1, 2, 3, 4, 5, 6, 7, 8);
    sig_void_9.emit (1, 2, 3, 4, 5, 6, 7, 8, 9);
    sig_void_10.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    sig_void_11.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
    sig_void_12.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
    sig_void_13.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);
    sig_void_14.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14);
    sig_void_15.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    sig_void_16.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    sig_void_cref.emit ("foo", "A const String reference", "foo");
    sig_void_cref += slot (*this, &EmitterMany::do_void_cref);
    ExtraType xt, &xref = xt;
    sig_string_14 += slot (test_string_14_emitter_data, xt);
    sig_string_14 += slot (test_string_14_data, xt);
    sig_string_14 += slot (test_string_14_refdata, xref);
    String results;
    results += sig_string_0.emit ();
    results += sig_string_1.emit (1);
    results += sig_string_2.emit (1, 2);
    results += sig_string_3.emit (1, 2, 3);
    results += sig_string_4.emit (1, 2, 3, 4);
    results += sig_string_5.emit (1, 2, 3, 4, 5);
    results += sig_string_6.emit (1, 2, 3, 4, 5, 6);
    results += sig_string_7.emit (1, 2, 3, 4, 5, 6, 7);
    results += sig_string_8.emit (1, 2, 3, 4, 5, 6, 7, 8);
    results += sig_string_9.emit (1, 2, 3, 4, 5, 6, 7, 8, 9);
    results += sig_string_10.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    results += sig_string_11.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
    results += sig_string_12.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
    results += sig_string_13.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);
    results += sig_string_14.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14);
    results += sig_string_15.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    results += sig_string_16.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    TINFO ("32 signals emitted: %s\n", results.c_str());
    TDONE();
  }
  void ref()   {}
  void unref() {}
};

struct Emitter3 {
  // Signal3<Emitter3, String, int, String, float, Signal::CollectorSum<String> > sig_mixed;
  Signal<Emitter3, String (int, String, float), Signals::CollectorSum<String> > sig_mixed;
  // Signal3<Emitter3, void,   int, String, float> sig_void_mixed;
  Signal<Emitter3, void (int, String, float)> sig_void_mixed;
  Emitter3() : sig_mixed (*this), sig_void_mixed (*this) {}
  void
  test_emissions()
  {
    TINFO ("Emitter3.emit()\n");
    String s = sig_mixed.emit (7, "seven.seven", 7.7);
    TINFO ("Emitter3: result=%s\n", s.c_str());
    TINFO ("Emitter3.emit() (void)\n");
    sig_void_mixed.emit (3, "three.three", 3.3);
    TINFO ("Emitter3: done.\n");
  }
  void ref() {}
  void unref() {}
};

struct Connection3 {
  static String mixed_func (int i, String s, float f)
  {
    TASSERT (i == 7 && s == "seven.seven" && f == (float) 7.7);
    TINFO ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
    return __func__;
  }
  static String mixed_efunc      (Emitter3 &obj, int i, String s, float f)
  {
    TASSERT (i == 7 && s == "seven.seven" && f == (float) 7.7);
    TINFO ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
    return __func__;
  }
  static void   void_mixed_func (int i, String s, float f)
  {
    TASSERT (i == 3 && s == "three.three" && f == (float) 3.3);
    TINFO ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
  }
  static void   void_mixed_efunc (Emitter3 &obj, int i, String s, float f)
  {
    TASSERT (i == 3 && s == "three.three" && f == (float) 3.3);
    TINFO ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
  }
  String string_callback (int i, String s, float f)
  {
    TINFO ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
    return __func__;
  }
  String string_emitter_callback (Emitter3 &emitter, int i, String s, float f)
  {
    TINFO ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
    return __func__;
  }
  void void_callback (int i, String s, float f)
  {
    TINFO ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
  }
  void void_emitter_callback (Emitter3 &emitter, int i, String s, float f)
  {
    TINFO ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
  }
  void test_signal (Emitter3 &e3)
  {
    TSTART ("Signals/Mixed emissions");
    e3.sig_mixed += mixed_efunc;
    e3.sig_mixed += mixed_func;
    e3.sig_mixed += slot (*this, &Connection3::string_emitter_callback);
    e3.sig_mixed += slot (*this, &Connection3::string_callback);
    e3.sig_void_mixed += void_mixed_efunc;
    e3.sig_void_mixed += void_mixed_func;
    e3.sig_void_mixed += slot (*this, &Connection3::void_emitter_callback);
    e3.sig_void_mixed += slot (*this, &Connection3::void_callback);
    e3.test_emissions();
    TDONE();
  }
};

//bool
//operator== (const std::auto_ptr<ExtraType> &a, const std::auto_ptr<ExtraType> &b)
//{ return a.get() == b.get(); } // needed for signal data

static uint emission_counter = 0;

void
shared_ptr_callback (int i, String s, float f, std::shared_ptr<ExtraType> data)
{
  emission_counter++;
  TINFO ("  callback: %s (%d, %s, %f, %s);\n", __func__, i, s.c_str(), f, data->message());
}

struct TemporaryObject : public virtual Deletable {
  String msg, msg2;
  TemporaryObject() :
    msg ("TemporaryObject"), msg2 ("Blub")
  {}
  String string_callback (int i, String s, float f)
  {
    emission_counter++;
    TINFO ("  callback: %s (%d, %s, %f); [%s]\n", __func__, i, s.c_str(), f, msg.c_str());
    TASSERT (msg2 == "Blub");
    return __func__;
  }
  String string_emitter_callback (Emitter3 &emitter, int i, String s, float f)
  {
    emission_counter++;
    TINFO ("  callback: %s (%d, %s, %f); [%s]\n", __func__, i, s.c_str(), f, msg.c_str());
    TASSERT (msg2 == "Blub");
    return __func__;
  }
  void void_callback (int i, String s, float f)
  {
    emission_counter++;
    TINFO ("  callback: %s (%d, %s, %f); [%s]\n", __func__, i, s.c_str(), f, msg2.c_str());
    TASSERT (msg2 == "Blub");
  }
  void shared_ptr_method (int i, String s, float f, std::shared_ptr<ExtraType> data)
  {
    emission_counter++;
    TINFO ("  callback: %s (%d, %s, %f, %s); [%s]\n", __func__, i, s.c_str(), f,
            data->message(), msg2.c_str());
    TASSERT (msg2 == "Blub");
  }
  void void_emitter_callback (Emitter3 &emitter, int i, String s, float f)
  {
    emission_counter++;
    TINFO ("  callback: %s (%d, %s, %f); [%s]\n", __func__, i, s.c_str(), f, msg2.c_str());
    TASSERT (msg2 == "Blub");
  }
  void never_ever_call_me (int i, String s, float f)
  {
    TASSERT (1 == 0);
  }
  static void
  test_temporary_object (Emitter3 &e3)
  {
    TSTART ("Signals/Temporary object");
    uint ac = emission_counter;
    uint edata_deletions;

    // Signal destructor tests
    {
      TemporaryObject tobj;
      e3.sig_mixed += slot (tobj, &TemporaryObject::string_emitter_callback);
      e3.sig_void_mixed += slot (tobj, &TemporaryObject::never_ever_call_me);
      e3.sig_mixed += slot (tobj, &TemporaryObject::string_callback);
      {
        TemporaryObject tmp2;
        e3.sig_void_mixed += slot (tmp2, &TemporaryObject::never_ever_call_me);
      } // auto-disconnect through ~TemporaryObject
      {
        Emitter3 tmp_emitter;
        tmp_emitter.sig_mixed += slot (tobj, &TemporaryObject::string_emitter_callback);
        tmp_emitter.sig_mixed += slot (tobj, &TemporaryObject::string_callback);
        tmp_emitter.sig_void_mixed += slot (tobj, &TemporaryObject::never_ever_call_me);
        TemporaryObject tobj2;
        tmp_emitter.sig_void_mixed += slot (tobj2, &TemporaryObject::never_ever_call_me);
        tmp_emitter.sig_void_mixed += slot (tobj2, &TemporaryObject::never_ever_call_me);
        tmp_emitter.sig_void_mixed -= slot (tobj2, &TemporaryObject::never_ever_call_me);
      } // removes all slots from tmp_emitter
      e3.sig_void_mixed -= slot (tobj, &TemporaryObject::never_ever_call_me);
      TASSERT (ac == emission_counter);
      e3.test_emissions();
      TASSERT (ac < emission_counter);
      e3.sig_void_mixed += slot (tobj, &TemporaryObject::void_emitter_callback);
      e3.sig_void_mixed += slot (tobj, &TemporaryObject::void_callback);
      ac = emission_counter;
      e3.test_emissions();
      TASSERT (ac < emission_counter);
      e3.sig_void_mixed += slot (tobj, &TemporaryObject::never_ever_call_me);
    } // calls TemporaryObject->Deletable::invoke_destruction_hooks()

    // check shared_ptr<ExtraType> destruction
    {
      edata_deletions = ExtraType_deletions;
      std::shared_ptr<ExtraType> sp (new ExtraType()); // ExtraType will be deleted at scope end
      TASSERT (edata_deletions == ExtraType_deletions);
    } // ~shared_ptr<ExtraType> -> ~ExtraType
    TASSERT (edata_deletions + 1 == ExtraType_deletions);

    // check ExtraType destruction upon disconnection
    {
      TemporaryObject tobj;
      edata_deletions = ExtraType_deletions;
      Signals::ConId cid;
      {
        std::shared_ptr<ExtraType> sp (new ExtraType());
        TASSERT (edata_deletions == ExtraType_deletions); // undestructed
        cid = e3.sig_void_mixed.connect (slot (tobj, &TemporaryObject::shared_ptr_method, sp));
        TASSERT (edata_deletions == ExtraType_deletions); // undestructed
      } // ~shared_ptr<ExtraType>
      TASSERT (edata_deletions == ExtraType_deletions); // undestructed
      ac = emission_counter;
      e3.test_emissions();
      TASSERT (ac < emission_counter);
      TASSERT (edata_deletions == ExtraType_deletions); // undestructed across emission
      uint dels = e3.sig_void_mixed.disconnect (cid); // ~ConId -> ~ExtraType
      TASSERT (dels == 1);
      TASSERT (edata_deletions + 1 == ExtraType_deletions); // destructed
    }

    // check signal connection destruction upon TemporaryObject death (emitter remains)
    {
      TemporaryObject tobj;
      edata_deletions = ExtraType_deletions;
      {
        std::shared_ptr<ExtraType> sp (new ExtraType());
        TASSERT (edata_deletions == ExtraType_deletions); // undestructed
        e3.sig_void_mixed += slot (tobj, &TemporaryObject::shared_ptr_method, sp);
      } // ~shared_ptr<ExtraType>
      TASSERT (edata_deletions == ExtraType_deletions); // undestructed
      ac = emission_counter;
      e3.test_emissions();
      TASSERT (ac < emission_counter);
      TASSERT (edata_deletions == ExtraType_deletions); // undestructed
      ac = emission_counter;
      e3.test_emissions();
      TASSERT (ac < emission_counter);
      TASSERT (edata_deletions == ExtraType_deletions); // undestructed
    } // implicit disconnection through ~TemporaryObject -> ~ExtraType
    TASSERT (edata_deletions + 1 == ExtraType_deletions);

    // check shared_ptr<ExtraType> for function slots
    {
      edata_deletions = ExtraType_deletions;
      Signals::ConId cid;
      {
        std::shared_ptr<ExtraType> sp (new ExtraType());
        TASSERT (edata_deletions == ExtraType_deletions); // undestructed
        cid = e3.sig_void_mixed.connect (slot (shared_ptr_callback, sp));
      } // ~shared_ptr<ExtraType>
      TASSERT (edata_deletions == ExtraType_deletions); // undestructed
      ac = emission_counter;
      e3.test_emissions();
      TASSERT (ac < emission_counter);
      TASSERT (edata_deletions == ExtraType_deletions); // undestructed
      ac = emission_counter;
      e3.test_emissions();
      TASSERT (ac < emission_counter);
      TASSERT (edata_deletions == ExtraType_deletions); // undestructed
      uint dels = e3.sig_void_mixed.disconnect (cid); // ~ConId -> ~ExtraType
      TASSERT (dels == 1);
      TASSERT (edata_deletions + 1 == ExtraType_deletions);
      // check bogus disconnection
      dels = e3.sig_void_mixed.disconnect (cid); // junk cid
      TASSERT (dels == 0); // none found
    }

    // check proper handler disconnections
    ac = emission_counter;
    e3.test_emissions(); // no increment since all handlers got disconnected
    TASSERT (ac == emission_counter);
    TDONE();
  }
};

static int tst_counter = 0;
static int
increment_tst_counter (void)
{
  tst_counter++;
  return 47;
}

static void
test_slot_trampoline ()
{
  int r;
  TASSERT (tst_counter == 0);
  const Signals::Slot3<void, int, double, char> &s3 = slot ((void (*) (int, double, char)) increment_tst_counter);
  Signals::Trampoline3<void, int, double, char> *t3 = s3.get_trampoline();
  (*t3) (2, 3, 4);
  TASSERT (tst_counter == 1);
  const Signals::Slot0<int> &s0 = slot ((int (*) ()) increment_tst_counter);
  Signals::Trampoline0<int> *t0 = s0.get_trampoline();
  r = (*t0) ();
  TASSERT (tst_counter == 2);
  TASSERT (r == 47);
  const Signals::Slot9<int,char,char,char,double,double,double,int,int,void*> &s9 =
    slot ((int (*) (char,char,char,double,double,double,int,int,void*)) increment_tst_counter);
  Signals::Trampoline9<int,char,char,char,double,double,double,int,int,void*> *t9 = s9.get_trampoline();
  r = (*t9) (1, 1, 1, .1, .1, .1, 999, 999, NULL);
  TASSERT (tst_counter == 3);
  TASSERT (r == 47);
}
REGISTER_TEST ("Signals/Slots & Trampolines", test_slot_trampoline);

static void
test_various()
{
  TDONE();

  Emitter3 e3;
  TemporaryObject::test_temporary_object (e3);

  Connection3 c3;
  Connection3 c3b;
  c3.test_signal (e3);
  c3b.test_signal (e3);

  EmitterMany many1, many2;
  many1.testme();
  many2.testme();

  TSTART ("Signals/Boilerplate2");
}
REGISTER_TEST ("Signals/Boilerplate1", test_various);

struct TestConnectionCounter {
  void  ref()   { /* dummy for signal emission */ }
  void  unref() { /* dummy for signal emission */ }
  int   connection_count;
  void  connections_changed (bool hasconnections) { connection_count += hasconnections ? +1 : -1; }
  // signal declaration
  typedef Signal<TestConnectionCounter, int64 (float, char), CollectorDefault<float>,
                 SignalConnectionRelay<TestConnectionCounter> > TestSignal;
  TestSignal   test_signal;
  // signal connection proxy
  typedef SignalProxy<TestConnectionCounter, int64 (float, char)> TestSignalProxy;
  TestSignalProxy proxy1, proxy2;
  /*ctro*/        TestConnectionCounter() :
    connection_count (0), test_signal (*this), proxy1 (test_signal), proxy2 (test_signal)
  {}
  // signal handlers
  int64        dummy_handler1 (float f, char c) { return int (f) + c; }
  static int64 dummy_handler2 (float f, char c) { return int (f) * c; }
  void
  test_signal_connections()
  {
    TASSERT (connection_count == 0);

    test_signal += slot (*this, &TestConnectionCounter::dummy_handler1);
    TASSERT (connection_count == 1);

    int64 result = test_signal.emit (2.1, 'a');
    TASSERT (result == 2 + 'a');

    test_signal += dummy_handler2;
    TASSERT (connection_count == 1);

    result = test_signal.emit (3.7, 'b');
    TASSERT (result == 3 * 'b');

    test_signal -= slot (*this, &TestConnectionCounter::dummy_handler1);
    TASSERT (connection_count == 1);

    result = test_signal.emit (1, '@');
    TASSERT (result == '@');

    test_signal -= dummy_handler2;
    TASSERT (connection_count == 0);

    result = test_signal.emit (0, 0);
    TASSERT (result == 0);
  }
  void
  test_proxy_connections()
  {
    TASSERT (connection_count == 0);
    uint id = proxy1.connect (slot (*this, &TestConnectionCounter::dummy_handler1));
    TASSERT (connection_count == 1);
    test_signal += dummy_handler2;
    TASSERT (connection_count == 1);

    int64 result = test_signal.emit (5.5, 'p');
    TASSERT (result == 5 * 'p'); // collector returns dummy_handler2's result

    bool success = proxy2.disconnect (id);
    TASSERT (success == true);
    TASSERT (connection_count == 1);
    proxy1 -= dummy_handler2;
    TASSERT (connection_count == 0);
    proxy1 += dummy_handler2;
    TASSERT (connection_count == 1);

    proxy1.divorce();
    proxy1 -= dummy_handler2;   // ignored
    TASSERT (connection_count == 1);
    proxy2 -= dummy_handler2;
    TASSERT (connection_count == 0);
  }
  static void
  test_connection_counter ()
  {
    TestConnectionCounter tcc;
    tcc.test_signal.listener (tcc, &TestConnectionCounter::connections_changed);
    tcc.test_signal_connections();
    tcc.test_proxy_connections();
  }
};
REGISTER_TEST ("Signals/Connection Counter", TestConnectionCounter::test_connection_counter);

struct TestCollectorVector {
  void  ref()   { /* dummy for signal emission */ }
  void  unref() { /* dummy for signal emission */ }
  Signal<TestCollectorVector, int (void), CollectorVector<int> > signal_int_farm;
  TestCollectorVector() : signal_int_farm (*this) {}
  static int handler1   (void)  { return 1; }
  static int handler42  (void)  { return 42; }
  static int handler777 (void)  { return 777; }
  void
  run_test ()
  {
    signal_int_farm += handler777;
    signal_int_farm += handler42;
    signal_int_farm += handler1;
    signal_int_farm += handler42;
    signal_int_farm += handler777;
    vector<int> results = signal_int_farm.emit();
    const int reference_array[] = { 777, 42, 1, 42, 777, };
    const vector<int> reference = vector_from_array (reference_array);
    TASSERT (results == reference);
  }
  static void run () { TestCollectorVector self; self.run_test(); }
};
REGISTER_TEST ("Signals/Test Vector Collector", TestCollectorVector::run);

} // anon
