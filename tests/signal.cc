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

extern "C" int
main (int   argc,
      char *argv[])
{
  SignalTest signal_test;
  signal_test.basic_signal_tests();
  signal_test.member_pointer_tests();
  return 0;
}

} // anon
