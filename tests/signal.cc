/* BirnetSignal
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <rapicorn/rapicorn.hh>

#define STRFUNC BIRNET_PRETTY_FUNCTION

namespace {
using namespace Rapicorn;
using Rapicorn::uint;

#if 0
struct Test3 {
  void
  vids (int i, double d, char *s)
  {
    printf ("Test3::%s: this=%p int=%d double=%f str=%s\n", __func__, this, i, d, s);
  }
  int
  iids (int i, double d, char *s)
  {
    printf ("Test3::%s: this=%p int=%d double=%f str=%s\n", __func__, this, i, d, s);
    return 0;
  }
};

class SignalTest {
public:
  void ref()   { /* mandatory function for signal owners */ printf ("++emission\n"); }
  void unref() { /* mandatory function for signal owners */ printf ("--emission\n"); }
  void member (int i);
  //Signal2<int,int,double> sig_iid;
  static int  ifunc3 (int a, double b, char *s) { printf ("%s: int=%d double=%f str=%s\n", __func__, a, b, s); return 2; }
  static void vfunc3 (int a, double b, char *s) { printf ("%s: int=%d double=%f str=%s\n", __func__, a, b, s); }
  enum DummyEnum { DummyEVal };
  static int  ifunc9 (int a, double b, char c, float d, SignalTest *e, long f, short g, long long h, DummyEnum i) { printf("%s\n", __func__); return 0; }
  static std::string stringdot() { return "."; }
  bool bool0() { printf("%s: return 0;\n", __func__); return 0; }
  bool bool1() { printf("%s: return 1;\n", __func__); return 1; }
  void
  basic_signal_tests()
  {
    Test3 test3;
    int result;
    
    Closure3<void,int,double,char*> *cf3v = new ClosureF3<void,int,double,char*> (vfunc3);
    Closure3<void,int,double,char*> *cm3v = new ClosureM3<Test3,void,int,double,char*> (&test3, &Test3::vids);
    Slot3<void,int,double,char*> sf3v = slot (vfunc3);
    Slot3<void,int,double,char*> sm3v = slot (&test3, &Test3::vids);
    Signal3<void,int,double,char*> sig_vids (this);
    sig_vids.connect (cf3v);
    sig_vids.connect (cm3v);
    sig_vids.connect (sf3v);
    sig_vids.connect (sm3v);
    sig_vids.connect (vfunc3);
    sig_vids.connect (&test3, &Test3::vids);
    sig_vids += vfunc3;
    sig_vids += slot (&test3, &Test3::vids);
    sig_vids += cf3v;
    // result = sig_vids.emit (3, 4.9, "huhu");
    sig_vids.emit (3, 4.9, "*HaHa!*");
    
    Closure3<int,int,double,char*> *cf3i = new ClosureF3<int,int,double,char*> (ifunc3);
    Closure3<int,int,double,char*> *cm3i = new ClosureM3<Test3,int,int,double,char*> (&test3, &Test3::iids);
    Slot3<int,int,double,char*> sf3i = slot (ifunc3);
    Slot3<int,int,double,char*> sm3i = slot (&test3, &Test3::iids);
    Signal3<int,int,double,char*,AccumulatorSum> sig_iids (this);
    sig_iids.connect (cf3i);
    sig_iids.connect (cm3i);
    sig_iids.connect (sf3i);
    sig_iids.connect (sm3i);
    sig_iids.connect (ifunc3);
    sig_iids.connect (&test3, &Test3::iids);
    result = sig_iids.emit (3, 4.9, "huhu");
    printf ("sig_iids.emit()=%d\n", result);
    
    Signal0<float,AccumulatorPass0> sig_flt (this);
    float flt = sig_flt.emit();
    printf ("sig_flt.emit()=%f\n", flt);
    
    Signal0<std::string,AccumulatorSum> sig_str (this);
    sig_str.connect (stringdot);
    sig_str.connect (stringdot);
    sig_str.connect (stringdot);
    std::string str = sig_str.emit();
    printf ("sig_str.emit()=%s\n", str.c_str());
    
    Signal0<bool,AccumulatorPass0> bsig (this);
    bsig += slot (this, &SignalTest::bool0);
    bsig += slot (this, &SignalTest::bool0);
    bsig += slot (this, &SignalTest::bool1);
    bsig += slot (this, &SignalTest::bool1); // not executed
    printf ("bsig.emit()=%d\n", bsig.emit());
    
    // no matching emit(): sig_vid.emit (3, "xc");
    //Slot2<int,int,double>  s2 = slot (ifoo2);
    //Slot2<int,int,double>  s2cp = s2;
    //sig_iid.connect (s2);
    //sig_iid.connect (slot (ifoo2));
    //sig_iid.connect (ifoo2);
    slot (ifunc9);
    Signal9<int,int,double,char,float,SignalTest*,long,short,long long,DummyEnum> sig_i9 (this);
    sig_i9.connect (ifunc9);
    sig_i9.connect (ifunc9);
    sig_i9.connect (ifunc9);
    result = sig_i9.emit (1000000, 2.3e-280, -33, 3.3, NULL, 234235, 32765, 219LL, DummyEVal);
    printf ("sig_i9.emit()=%d\n", result);
  }
  void
  member_pointer_tests()
  {
    class Foo : Deletable {
      void privfoo(int i) { printf ("%s: i=%d this=%p\n", STRFUNC, i, this); }
    protected:
      void protfoo(int i) { printf ("%s: i=%d this=%p\n", STRFUNC, i, this); }
    public:
      void pubfoo(int i)  { printf ("%s: i=%d this=%p\n", STRFUNC, i, this); }
      virtual
      void virtfoo(int i) { printf ("%s: i=%d this=%p\n", STRFUNC, i, this); }
    };
    Foo f;
    union { void (Foo::*member) (int); void *pointer; } u;
    
    void (Foo::*member) (int) = &Foo::pubfoo;
    printf ("Foo::*member=%p\n", (u.member = member, u.pointer));
    (f.*member) (1);
    printf ("Foo::*member=%p\n", (u.member = member, u.pointer));
    (((Foo*)0)->*member) (2); // works
    member = &Foo::virtfoo;
    printf ("Foo::*member=%p\n", (u.member = member, u.pointer));
    ((&f)->*member) (3);
    //printf ("Foo::*member=%p\n", member);
    //(((Foo*)0)->*member) (4); // segfaults
    member = NULL;
    printf ("Foo::*member=%p\n", (u.member = member, u.pointer));
    
    //typedef void (*Func) (Foo*,int);
    //Func m2 = (Func) (f.* &Foo::pubfoo);
  }
};
#endif

struct ExtraType {
  virtual const char* message() { return "ExtraType::message()"; }
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
    sig_string_0 (*this), sig_string_1 (*this), sig_string_2 (*this), sig_string_3 (*this),
    sig_string_4 (*this), sig_string_5 (*this), sig_string_6 (*this), sig_string_7 (*this),
    sig_string_8 (*this), sig_string_9 (*this), sig_string_10 (*this), sig_string_11 (*this),
    sig_string_12 (*this), sig_string_13 (*this), sig_string_14 (*this), sig_string_15 (*this), sig_string_16 (*this)
  {}
  static String test_string_14_emitter_data (EmitterMany &em,int a1, double a2, int a3, double a4, int a5, double a6, int a7, double a8, int a9,
                                             double a10, int a11, double a12, int a13, double a14, ExtraType x)
  {
    printf ("  callback: %s (%s, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %s);\n",
            __func__, em.emitter_check(), a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, x.message());
    return __func__;
  }
  static String test_string_14_data (int a1, double a2, int a3, double a4, int a5, double a6, int a7, double a8, int a9,
                                     double a10, int a11, double a12, int a13, double a14, ExtraType x)
  {
    printf ("  callback: %s (%d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %s);\n",
            __func__, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, x.message());
    return __func__;
  }
  void testme ()
  {
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
    ExtraType xt;
    sig_string_14 += slot (test_string_14_emitter_data, xt);
    sig_string_14 += slot (test_string_14_data, xt);
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
    printf ("32 signals emitted: %s\n", results.c_str());
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
    printf ("Emitter3.emit()\n");
    String s = sig_mixed.emit (7, "seven.seven", 7.7);
    printf ("Emitter3: result=%s\n", s.c_str());
    printf ("Emitter3.emit() (void)\n");
    sig_void_mixed.emit (3, "three.three", 3.3);
    printf ("Emitter3: done.\n");
  }
  void ref() {}
  void unref() {}
};

struct Connection3 {
  static String mixed_func (int i, String s, float f)
  {
    assert (i == 7 && s == "seven.seven" && f == (float) 7.7);
    printf ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
    return __func__;
  }
  static String mixed_efunc      (Emitter3 &obj, int i, String s, float f)
  {
    assert (i == 7 && s == "seven.seven" && f == (float) 7.7);
    printf ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
    return __func__;
  }
  static void   void_mixed_func (int i, String s, float f)
  {
    assert (i == 3 && s == "three.three" && f == (float) 3.3);
    printf ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
  }
  static void   void_mixed_efunc (Emitter3 &obj, int i, String s, float f)
  {
    assert (i == 3 && s == "three.three" && f == (float) 3.3);
    printf ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
  }
  String string_callback (int i, String s, float f)
  {
    printf ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
    return __func__;
  }
  String string_emitter_callback (Emitter3 &emitter, int i, String s, float f)
  {
    printf ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
    return __func__;
  }
  void void_callback (int i, String s, float f)
  {
    printf ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
  }
  void void_emitter_callback (Emitter3 &emitter, int i, String s, float f)
  {
    printf ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
  }
  void test_signal (Emitter3 &e3)
  {
    e3.sig_mixed += mixed_efunc;
    e3.sig_mixed += mixed_func;
    e3.sig_mixed += slot (*this, &Connection3::string_emitter_callback);
    e3.sig_mixed += slot (*this, &Connection3::string_callback);
    e3.sig_void_mixed += void_mixed_efunc;
    e3.sig_void_mixed += void_mixed_func;
    e3.sig_void_mixed += slot (*this, &Connection3::void_emitter_callback);
    e3.sig_void_mixed += slot (*this, &Connection3::void_callback);
    e3.test_emissions();
  }
};

extern "C" int
main (int   argc,
      char *argv[])
{
  printf ("TEST: %s:\n", basename (argv[0]));
#if 0
  SignalTest signal_test;
  signal_test.basic_signal_tests();
  signal_test.member_pointer_tests();
#endif
  Connection3 c3;
  Emitter3 e3;
  c3.test_signal (e3);
  EmitterMany many;
  many.testme();
  return 0;
}

} // anon
