# Licensed GNU GPLv3 or later: http://www.gnu.org/licenses/gpl.html
"""AidaCxxStub - Aida C++ Code Generator

More details at http://www.rapicorn.org/
"""
import Decls, GenUtils, re

clienthh_boilerplate = r"""
// --- ClientHH Boilerplate ---
#include <rapicorn-core.hh>
"""

serverhh_boilerplate = r"""
// --- ServerHH Boilerplate ---
#include <rapicorn-core.hh>
"""

serverhh_testcode = r"""
namespace Rapicorn { namespace Aida {
class TestServerBase : public virtual PropertyHostInterface {
public:
  explicit             TestServerBase ()            {}
  virtual             ~TestServerBase ()            {}
  virtual uint64_t     _orbid         () const      { return uint64_t (this); }
};
} } // Rapicorn::Aida
"""

rapicornsignal_boilerplate = r"""
#include <rapicorn-core.hh> // for rcore/signal.hh
using Rapicorn::Signals::slot;
"""

servercc_testcode = r"""
#ifndef AIDA_CONNECTION
#define AIDA_CONNECTION()       (*(Rapicorn::Aida::ServerConnection*)NULL)
#endif // !AIDA_CONNECTION
"""

gencc_boilerplate = r"""
// --- ClientCC/ServerCC Boilerplate ---
#include <string>
#include <vector>
#include <stdexcept>
#ifndef __AIDA_GENERIC_CC_BOILERPLATE__
#define __AIDA_GENERIC_CC_BOILERPLATE__

#define AIDA_CHECK(cond,errmsg) do { if (cond) break; throw std::runtime_error (std::string ("AIDA-ERROR: ") + errmsg); } while (0)

namespace { // Anon
using Rapicorn::Aida::uint64_t;

namespace __AIDA_Local__ {
using namespace Rapicorn::Aida;
static __attribute__ ((__format__ (__printf__, 1, 2), unused)) FieldBuffer*
error (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  error_vprintf (format, args);
  va_end (args);
  return NULL;
}
} } // Anon::__AIDA_Local__
#endif // __AIDA_GENERIC_CC_BOILERPLATE__
"""

servercc_boilerplate = r"""
namespace { // Anon
namespace __AIDA_Local__ {
// types
typedef ServerConnection::MethodRegistry MethodRegistry;
typedef ServerConnection::MethodEntry MethodEntry;
// objects
template<class Object> static inline Object* id2obj (uint64_t oid)
{
  const ptrdiff_t addr = AIDA_CONNECTION().orbid2instance (oid);
  $AIDA_iface_base$ *instance = reinterpret_cast<$AIDA_iface_base$*> (addr);
  return dynamic_cast<Object*> (instance);
}
static inline uint64_t obj2id  ($AIDA_iface_base$ *obj)
{ return AIDA_CONNECTION().instance2orbid (reinterpret_cast<ptrdiff_t> (obj)); }
template<class Object> inline Object* smh2obj (const SmartHandle &sh)
{ uint64_t orbid = sh._orbid(); return (orbid & 0) ? NULL : id2obj<Object> (orbid); }
template<class SMH> static inline SMH obj2smh ($AIDA_iface_base$ *self)
{
  const uint64_t orbid = obj2id (self);
  SMH target;
  const uint64_t input[2] = { orbid, target._orbid() };
  Rapicorn::Aida::ObjectBroker::dup_handle (input, target);
  return target;
}
// messages
static inline void post_msg (FieldBuffer *fb) { ObjectBroker::post_msg (fb); }
static inline void add_header1_discon (FieldBuffer &fb, uint64_t orbid, uint64_t h, uint64_t l)
{ fb.add_header1 (Rapicorn::Aida::MSGID_DISCON, ObjectBroker::connection_id_from_orbid (orbid), h, l); }
static inline void add_header1_event  (FieldBuffer &fb, uint64_t orbid, uint64_t h, uint64_t l)
{ fb.add_header1 (Rapicorn::Aida::MSGID_EVENT, ObjectBroker::connection_id_from_orbid (orbid), h, l); }
static inline FieldBuffer* new_result (FieldReader &fbr, uint64_t h, uint64_t l, uint32_t n = 1)
{ return ObjectBroker::renew_into_result (fbr, ObjectBroker::receiver_connection_id (fbr.field_buffer()->first_id()), h, l, n); }

} } // Anon::__AIDA_Local__
"""

clientcc_boilerplate = r"""
#ifndef AIDA_CONNECTION
#define AIDA_CONNECTION()       (*(Rapicorn::Aida::ClientConnection*)NULL)
#endif // !AIDA_CONNECTION
namespace { // Anon
namespace __AIDA_Local__ {
static FieldBuffer*  invoke (FieldBuffer *fb) { return AIDA_CONNECTION().call_remote (fb); } // async remote call, transfers memory
static bool          signal_disconnect (uint64_t signal_handler_id) { return AIDA_CONNECTION().signal_disconnect (signal_handler_id); }
static uint64_t      signal_connect    (uint64_t hhi, uint64_t hlo, const SmartHandle &sh, SignalEmitHandler seh, void *data)
                                       { return AIDA_CONNECTION().signal_connect (hhi, hlo,
                                         sh._orbid(), AIDA_CONNECTION().connection_id(), seh, data); }
static inline uint64_t smh2id (const SmartHandle &h) { return h._orbid(); }
template<class SMH> SMH smh2cast (const SmartHandle &handle) {
  const uint64_t orbid = __AIDA_Local__::smh2id (handle);
  SMH target;
  const uint64_t input[2] = { orbid, target._orbid() };
  Rapicorn::Aida::ObjectBroker::dup_handle (input, target);
  return target;
}
static inline void add_header2 (FieldBuffer &fb, const SmartHandle &sh, uint64_t h, uint64_t l) {
  fb.add_header2 (Rapicorn::Aida::MSGID_TWOWAY, ObjectBroker::connection_id_from_handle (sh),
                  AIDA_CONNECTION().connection_id(), h, l); }
static inline void add_header1 (FieldBuffer &fb, const SmartHandle &sh, uint64_t h, uint64_t l)
{ fb.add_header1 (Rapicorn::Aida::MSGID_ONEWAY, ObjectBroker::connection_id_from_handle (sh), h, l); }

} } // Anon::__AIDA_Local__
"""

def reindent (prefix, lines):
  return re.compile (r'^', re.M).sub (prefix, lines.rstrip())

I_prefix_postfix = ('', '_Interface')

class G4STUB: pass    # generate stub classes (smart handles)
class G4SERVANT: pass    # generate servants classes (interfaces)

class Generator:
  def __init__ (self):
    self.ntab = 26
    self.namespaces = []
    self.insertions = {}
    self.cppguard = ''
    self.ns_aida = None
    self.gen_inclusions = []
    self.skip_symbols = set()
    self.test_iface_base = 'Rapicorn::Aida::TestServerBase'
    self.iface_base = self.test_iface_base
    self.property_list = 'Rapicorn::Aida::PropertyList'
    self.gen_mode = None
    self.object_impl = None # ('impl', ('', 'Impl'))
  def Iwrap (self, name):
    cc = name.rfind ('::')
    if cc >= 0:
      ns, base = name[:cc+2], name[cc+2:]
    else:
      ns, base = '', name
    return ns + I_prefix_postfix[0] + base + I_prefix_postfix[1]
  def type2cpp (self, type_node):
    tstorage = type_node.storage
    if tstorage == Decls.VOID:          return 'void'
    if tstorage == Decls.BOOL:          return 'bool'
    if tstorage == Decls.INT32:         return 'int'
    if tstorage == Decls.INT64:         return 'Rapicorn::Aida::int64_t'
    if tstorage == Decls.FLOAT64:       return 'double'
    if tstorage == Decls.STRING:        return 'std::string'
    if tstorage == Decls.ANY:           return 'Rapicorn::Aida::Any'
    fullnsname = '::'.join (self.type_relative_namespaces (type_node) + [ type_node.name ])
    return fullnsname
  def C4server (self, type_node):
    tname = self.type2cpp (type_node)
    if type_node.storage == Decls.INTERFACE:
      return self.Iwrap (tname)                         # construct servant class interface name
    return tname
  def C4client (self, type_node):
    tname = self.type2cpp (type_node)
    if type_node.storage == Decls.INTERFACE:
      return tname + 'Handle'                           # construct client class SmartHandle
    return tname
  def C (self, type_node, mode = None):                 # construct Class name
    mode = mode or self.gen_mode
    if mode == G4SERVANT:
      return self.C4server (type_node)
    else: # G4STUB
      return self.C4client (type_node)
  def R (self, type_node):                              # construct Return type
    tname = self.C (type_node)
    if self.gen_mode == G4SERVANT and type_node.storage == Decls.INTERFACE:
      tname += '*'
    return tname
  def M (self, type_node):                              # construct Member type
    if self.gen_mode == G4STUB and type_node.storage == Decls.INTERFACE:
      classH = self.C4client (type_node) # smart handle class name
      classC = self.C4server (type_node) # servant class name
      return 'Rapicorn::Aida::SmartMember<%s>' % classH # classC
    else:
      return self.R (type_node)
  def V (self, ident, type_node, f_delta = -999999):    # construct Variable
    s = ''
    s += self.C (type_node)
    s = self.F (s, f_delta)
    if self.gen_mode == G4SERVANT and type_node.storage == Decls.INTERFACE:
      s += '*'
    else:
      s += ' '
    return s + ident
  def A (self, ident, type_node, defaultinit = None):   # construct call Argument
    constref = type_node.storage in (Decls.STRING, Decls.SEQUENCE, Decls.RECORD, Decls.ANY)
    needsref = constref or type_node.storage == Decls.INTERFACE
    s = self.C (type_node)                      # const {Obj} &foo = 3
    s += ' ' if ident else ''                   # const Obj{ }&foo = 3
    if constref:
      s = 'const ' + s                          # {const }Obj &foo = 3
    if needsref:
      s += '&'                                  # const Obj {&}foo = 3
    s += ident                                  # const Obj &{foo} = 3
    if defaultinit != None:
      if type_node.storage == Decls.ENUM:
        s += ' = %s (%s)' % (self.C (type_node), defaultinit)  # { = 3}
      elif type_node.storage in (Decls.SEQUENCE, Decls.RECORD):
        s += ' = %s()' % self.C (type_node)                    # { = 3}
      elif type_node.storage == Decls.INTERFACE:
        s += ' = *(%s*) NULL' % self.C (type_node)             # { = 3}
      else:
        s += ' = %s' % defaultinit                             # { = 3}
    return s
  def Args (self, ftype, prefix, argindent = 2):        # construct list of call Arguments
    l = []
    for a in ftype.args:
      l += [ self.A (prefix + a[0], a[1]) ]
    return (',\n' + argindent * ' ').join (l)
  def U (self, ident, type_node):                       # construct function call argument Use
    s = '*' if type_node.storage == Decls.INTERFACE and self.gen_mode == G4SERVANT else ''
    return s + ident
  def F (self, string, delta = 0):                      # Format string to tab stop
    return string + ' ' * max (1, self.ntab + delta - len (string))
  def tab_stop (self, n):
    self.ntab = n
  def close_inner_namespace (self):
    current_namespace = self.namespaces.pop()
    return '} // %s\n' % current_namespace.name if current_namespace.name else ''
  def open_inner_namespace (self, namespace):
    self.namespaces += [ namespace ]
    current_namespace = self.namespaces[-1]
    return '\nnamespace %s {\n' % current_namespace.name if current_namespace.name else ''
  def open_namespace (self, typeinfo):
    s = ''
    newspaces = []
    if type (typeinfo) == tuple:
      newspaces = list (typeinfo)
    elif typeinfo:
      newspaces = typeinfo.list_namespaces()
      # s += '// ' + str ([n.name for n in newspaces]) + '\n'
    while len (self.namespaces) > len (newspaces):
      s += self.close_inner_namespace()
    while self.namespaces and newspaces[0:len (self.namespaces)] != self.namespaces:
      s += self.close_inner_namespace()
    for n in newspaces[len (self.namespaces):]:
      s += self.open_inner_namespace (n)
    return s
  def type_relative_namespaces (self, type_node):
    tnsl = type_node.list_namespaces() # type namespace list
    # remove common prefix with global namespace list
    for n in self.namespaces:
      if tnsl and tnsl[0] == n:
        tnsl = tnsl[1:]
      else:
        break
    namespace_names = [d.name for d in tnsl if d.name]
    return namespace_names
  def namespaced_identifier (self, ident):
    names = [d.name for d in self.namespaces if d.name]
    if ident:
      names += [ ident ]
    return '::'.join (names)
  def mkzero (self, type):
    if type.storage == Decls.STRING:
      return '""'
    if type.storage == Decls.ENUM:
      return self.C (type) + ' (0)'
    if type.storage in (Decls.RECORD, Decls.SEQUENCE, Decls.ANY):
      return self.C (type) + '()'
    return '0'
  def generate_recseq_decl (self, type_info):
    s = '\n'
    s += self.generate_shortdoc (type_info)     # doxygen IDL snippet
    if type_info.storage == Decls.SEQUENCE:
      fl = type_info.elements
      s += 'struct ' + self.C (type_info) + ' : public std::vector<' + self.M (fl[1]) + '>\n'
      s += '{\n'
    else:
      s += 'struct %s\n' % self.C (type_info)
      s += '{\n'
    if type_info.storage == Decls.RECORD:
      fieldlist = type_info.fields
      for fl in fieldlist:
        s += '  ' + self.F (self.M (fl[1])) + fl[0] + ';\n'
    elif type_info.storage == Decls.SEQUENCE:
      s += '  typedef std::vector<' + self.M (fl[1]) + '> Sequence;\n'
      s += '  reference append_back(); ///< Append data at the end, returns write reference to data.\n'
    if type_info.storage == Decls.RECORD:
      s += '  ' + self.F ('inline') + '%s () {' % self.C (type_info) # ctor
      for fl in fieldlist:
        if fl[1].storage in (Decls.BOOL, Decls.INT32, Decls.INT64, Decls.FLOAT64, Decls.ENUM):
          s += " %s = %s;" % (fl[0], self.mkzero (fl[1]))
      s += ' }\n'
    s += self.insertion_text ('class_scope:' + type_info.name)
    s += '};\n'
    if type_info.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += 'void operator<<= (Rapicorn::Aida::FieldBuffer&, const %s&);\n' % self.C (type_info)
      s += 'void operator>>= (Rapicorn::Aida::FieldReader&, %s&);\n' % self.C (type_info)
    return s
  def generate_proto_add_args (self, fb, type_info, aprefix, arg_info_list, apostfix):
    s = ''
    for arg_it in arg_info_list:
      ident, type_node = arg_it
      ident = aprefix + ident + apostfix
      s += '  %s <<= %s;\n' % (fb, ident)
    return s
  def generate_proto_pop_args (self, fbr, type_info, aprefix, arg_info_list, apostfix = ''):
    s = ''
    for arg_it in arg_info_list:
      ident, type_node = arg_it
      ident = aprefix + ident + apostfix
      s += '  %s >>= %s;\n' % (fbr, ident)
    return s
  def generate_record_impl (self, type_info):
    s = ''
    s += 'inline void __attribute__ ((used))\n'
    s += 'operator<<= (Rapicorn::Aida::FieldBuffer &dst, const %s &self)\n{\n' % self.C (type_info)
    s += '  Rapicorn::Aida::FieldBuffer &fb = dst.add_rec (%u);\n' % len (type_info.fields)
    s += self.generate_proto_add_args ('fb', type_info, 'self.', type_info.fields, '')
    s += '}\n'
    s += 'inline void __attribute__ ((used))\n'
    s += 'operator>>= (Rapicorn::Aida::FieldReader &src, %s &self)\n{\n' % self.C (type_info)
    s += '  Rapicorn::Aida::FieldReader fbr (src.pop_rec());\n'
    s += '  if (fbr.remaining() < %u) return;\n' % len (type_info.fields)
    s += self.generate_proto_pop_args ('fbr', type_info, 'self.', type_info.fields)
    s += '}\n'
    return s
  def generate_sequence_impl (self, type_info):
    s = ''
    el = type_info.elements
    s += 'inline void __attribute__ ((used))\n'
    s += 'operator<<= (Rapicorn::Aida::FieldBuffer &dst, const %s &self)\n{\n' % self.C (type_info)
    s += '  const size_t len = self.size();\n'
    s += '  Rapicorn::Aida::FieldBuffer &fb = dst.add_seq (len);\n'
    s += '  for (size_t k = 0; k < len; k++) {\n'
    s += reindent ('  ', self.generate_proto_add_args ('fb', type_info, '',
                                                       [('self', type_info.elements[1])],
                                                       '[k]')) + '\n'
    s += '  }\n'
    s += '}\n'
    s += 'inline void __attribute__ ((used))\n'
    s += 'operator>>= (Rapicorn::Aida::FieldReader &src, %s &self)\n{\n' % self.C (type_info)
    s += '  Rapicorn::Aida::FieldReader fbr (src.pop_seq());\n'
    s += '  const size_t len = fbr.remaining();\n'
    if el[1].storage == Decls.INTERFACE:
      s += '  self.reserve (len);\n'
    else:
      s += '  self.resize (len);\n'
    s += '  for (size_t k = 0; k < len; k++) {\n'
    s += '    fbr >>= self[k];\n'
    s += '  }\n'
    s += '}\n'
    s += '%s::reference\n' % self.C (type_info)
    s += '%s::append_back()\n{\n' % self.C (type_info)
    s += '  resize (size() + 1);\n'
    s += '  return back();\n'
    s += '}\n'
    return s
  def generate_enum_impl (self, type_info):
    s = '\n'
    ns, nm = self.namespaced_identifier (None), type_info.name
    s += 'static Rapicorn::Init _Rapicorn_Aida__INIT__%s_ ([]() {\n' % nm
    s += '  static const Rapicorn::Aida::EnumInfo::Value enum_values[] = {\n'
    for opt in type_info.options:
      (ident, label, blurb, number) = opt
      s += '    RAPICORN_AIDA_ENUM_INFO_VALUE (%s),\n' % ident
    s += '  };\n'
    s += '  Rapicorn::Aida::EnumInfo::enlist ("%s", "%s", enum_values);\n' % (ns, nm)
    s += '});\n'
    return s
  def generate_enum_info_specialization (self, type_info):
    s = '\n'
    ns, nm = '::'.join (self.type_relative_namespaces (type_info)), type_info.name
    s += 'template<> inline EnumInfo enum_info<%s::%s>() { return EnumInfo::from_nsid ("%s", "%s"); }\n' % (ns, nm, ns, nm)
    return s
  def digest2cbytes (self, digest):
    return '0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL' % digest
  def method_digest (self, method_info):
    return self.digest2cbytes (method_info.type_hash())
  def class_digest (self, class_info):
    return self.digest2cbytes (class_info.type_hash())
  def list_types_digest (self, class_info):
    return self.digest2cbytes (class_info.twoway_hash ('__AIDA__list_types # internal-method'))
  def setter_digest (self, class_info, fident, ftype):
    setter_hash = class_info.property_hash ((fident, ftype), True)
    return self.digest2cbytes (setter_hash)
  def getter_digest (self, class_info, fident, ftype):
    getter_hash = class_info.property_hash ((fident, ftype), False)
    return self.digest2cbytes (getter_hash)
  def class_ancestry (self, type_info):
    def deep_ancestors (type_info):
      l = [ type_info ]
      for a in type_info.prerequisites:
        l += deep_ancestors (a)
      return l
    def make_list_uniq (lst):
      r, q = [], set()
      for e in lst:
        if not e in q:
          q.add (e)
          r += [ e ]
      return r
    return make_list_uniq (deep_ancestors (type_info))
  def inherit_reduce (self, type_list):
    def hasancestor (child, parent):
      if child == parent:
        return True
      for childpre in child.prerequisites:
        if hasancestor (childpre, parent):
          return True
    reduced = []
    while type_list:
      p = type_list.pop()
      skip = 0
      for c in type_list + reduced:
        if hasancestor (c, p):
          skip = 1
          break
      if not skip:
        reduced = [ p ] + reduced
    return reduced
  def interface_class_ancestors (self, type_info):
    l = []
    for pr in type_info.prerequisites:
      l += [ pr ]
    l = self.inherit_reduce (l)
    return l
  def interface_class_inheritance (self, type_info):
    aida_smarthandle, ddc = 'Rapicorn::Aida::SmartHandle', False
    l = self.interface_class_ancestors (type_info)
    l = [self.C (pr) for pr in l] # types -> names
    if self.gen_mode == G4SERVANT:
      if l:
        heritage = 'public virtual'
      else:
        l = [self.iface_base]
        heritage = 'public virtual'
    else:
      if l:
        heritage = 'public'
      else:
        l, ddc = [aida_smarthandle], True
        heritage = 'public virtual'
    if self.gen_mode == G4SERVANT:
      cl = []
    else:
      cl = l if l == [aida_smarthandle] else [aida_smarthandle] + l
    return (l, heritage, cl, ddc) # prerequisites, heritage type, constructor args, direct-SH-descendant
  def generate_interface_class (self, type_info):
    s, classC, classH = '\n', self.C (type_info), self.C4client (type_info)
    # declare
    s += self.generate_shortdoc (type_info)     # doxygen IDL snippet
    s += 'class %s' % classC
    # inherit
    precls, heritage, cl, ddc = self.interface_class_inheritance (type_info)
    s += ' : ' + heritage + ' %s' % (', ' + heritage + ' ').join (precls) + '\n'
    s += '{\n'
    if self.gen_mode == G4STUB:
      s += '  ' + self.F ('static %s' % classC) + '_cast (Rapicorn::Aida::SmartHandle&, const Rapicorn::Aida::TypeHashList&);\n'
      s += '  ' + self.F ('static const Rapicorn::Aida::TypeHash&') + '_type ();\n'
    # constructors
    s += 'protected:\n'
    if self.gen_mode == G4SERVANT:
      s += '  explicit ' + self.F ('') + '%s ();\n' % self.C (type_info) # ctor
      s += '  virtual ' + self.F ('/*Des*/') + '~%s () = 0;\n' % self.C (type_info) # dtor
    s += 'public:\n'
    if self.gen_mode == G4SERVANT:
      s += '  virtual ' + self.F ('void') + '_list_types (Rapicorn::Aida::TypeHashList&) const;\n'
      if self.property_list:
        s += '  virtual ' + self.F ('const ' + self.property_list + '&') + '_property_list ();\n'
    else: # G4STUB
      aliasfix = '__attribute__ ((noinline))' # work around bogus strict-aliasing warning in g++-4.4.5
      s += '  ' + self.F ('const Rapicorn::Aida::TypeHashList') + '_down_cast_types();\n'
      s += '  template<class SmartHandle>\n'
      s += '  ' + self.F ('static %s' % classH) + 'down_cast (SmartHandle smh) '
      s += '{ return smh._is_null() ? %s() : _cast (smh, smh._down_cast_types()); }\n' % classH
      s += '  ' + self.F ('explicit') + '%s ();\n' % classH # ctor
      #s += '  ' + self.F ('inline') + '%s (const %s &src)' % (classH, classH) # copy ctor
      #s += ' : ' + ' (src), '.join (cl) + ' (src) {}\n'
    # properties
    il = 0
    if type_info.fields:
      il = max (len (fl[0]) for fl in type_info.fields)
    for fl in type_info.fields:
      s += self.generate_property_prototype (fl[0], fl[1], il)
    # signals
    if self.gen_mode == G4SERVANT:
      for sg in type_info.signals:
        s += '  ' + self.generate_server_signal_typedef (sg, type_info)
      for sg in type_info.signals:
        s += '  ' + self.generate_signal_typename (sg, type_info) + ' sig_%s;\n' % sg.name
    else: # G4STUB
      for sg in type_info.signals:
        s += self.generate_client_signal_decl (sg, type_info)
    # methods
    il = 0
    if type_info.methods:
      il = max (len (m.name) for m in type_info.methods)
      il = max (il, len (self.C (type_info)))
    for m in type_info.methods:
      s += self.generate_method_decl (m, il)
    # impl()
    if self.gen_mode == G4SERVANT and self.object_impl:
      impl_method, ppwrap = self.object_impl
      implname = ppwrap[0] + type_info.name + ppwrap[1]
      s += '  inline %s&       impl () ' % implname
      s += '{ %s *_impl = dynamic_cast<%s*> (this); if (!_impl) throw std::bad_cast(); return *_impl; }\n' % (implname, implname)
      s += '  inline const %s& impl () const { return impl (const_cast<%s*> (this)); }\n' % (implname, self.C (type_info))
    s += self.insertion_text ('class_scope:' + type_info.name)
    s += '};\n'
    if self.gen_mode == G4SERVANT:
      s += 'void operator<<= (Rapicorn::Aida::FieldBuffer&, %s&);\n' % self.C (type_info)
      s += 'void operator<<= (Rapicorn::Aida::FieldBuffer&, %s*);\n' % self.C (type_info)
      s += 'void operator>>= (Rapicorn::Aida::FieldReader&, %s*&);\n' % self.C (type_info)
    else: # G4STUB
      s += 'void operator<<= (Rapicorn::Aida::FieldBuffer&, const %s&);\n' % self.C (type_info)
      s += 'void operator>>= (Rapicorn::Aida::FieldReader&, %s&);\n' % self.C (type_info)
    # conversion operators
    if self.gen_mode == G4SERVANT:       # ->* _servant and ->* _handle operators
      s += '%s* operator->* (%s &sh, Rapicorn::Aida::_ServantType);\n' % (classC, classH)
      s += '%s operator->* (%s *obj, Rapicorn::Aida::_HandleType);\n' % (classH, classC)
    else: # self.gen_mode == G4STUB
      s += '//' + self.F ('inline', -9)
      s += 'operator _UnspecifiedBool () const ' # return non-NULL pointer to member on true
      s += '{ return _is_null() ? NULL : _unspecified_bool_true(); }\n' # avoids auto-bool conversions on: float (*this)
    # typedef alias
    if self.gen_mode == G4STUB:
      s += self.generate_shortalias (type_info)
    return s
  def generate_shortdoc (self, type_info):      # doxygen snippets
    classC = self.C (type_info) # class name
    xkind = 'servant' if self.gen_mode == G4SERVANT else 'stub'
    s  = '/** @interface %s\n' % type_info.name
    s += ' * See also the corresponding C++ %s class %s. */\n' % (xkind, classC)
    s += '/// See also the corresponding IDL class %s.\n' % type_info.name
    return s
  def generate_shortalias (self, type_info):
    assert self.gen_mode == G4STUB
    s = ''
    alias = self.type2cpp (type_info) + 'H'
    s += 'typedef %s %s;' % (self.C (type_info), alias)
    s += ' ///< Convenience alias for the IDL type %s.\n' % type_info.name
    return s
  def generate_method_decl (self, functype, pad):
    s = '  '
    if self.gen_mode == G4SERVANT:
      s += 'virtual '
    s += self.F (self.R (functype.rtype))
    s += functype.name
    s += ' ' * max (0, pad - len (functype.name))
    s += ' ('
    argindent = len (s)
    l = []
    for a in functype.args:
      l += [ self.A (a[0], a[1], a[2]) ]
    s += (',\n' + argindent * ' ').join (l)
    s += ')'
    if self.gen_mode == G4SERVANT and functype.pure:
      s += ' = 0'
    s += ';\n'
    return s
  def generate_client_class_methods (self, class_info):
    s, classH = '', self.C4client (class_info)
    classH2 = (classH, classH)
    precls, heritage, cl, ddc = self.interface_class_inheritance (class_info)
    s += '%s::%s ()' % classH2 # ctor
    s += '\n{}\n'
    s += 'void\n'
    s += 'operator<<= (Rapicorn::Aida::FieldBuffer &fb, const %s &handle)\n{\n' % classH
    s += '  fb.add_object (__AIDA_Local__::smh2id (handle));\n'
    s += '}\n'
    s += 'void\n'
    s += 'operator>>= (Rapicorn::Aida::FieldReader &fbr, %s &handle)\n{\n' % classH
    s += '  Rapicorn::Aida::ObjectBroker::pop_handle (fbr, handle);\n'
    s += '}\n'
    s += 'const Rapicorn::Aida::TypeHash&\n'
    s += '%s::_type()\n{\n' % classH
    s += '  static const Rapicorn::Aida::TypeHash type_hash = Rapicorn::Aida::TypeHash (%s);\n' % self.class_digest (class_info)
    s += '  return type_hash;\n'
    s += '}\n'
    s += '%s\n%s::_cast (Rapicorn::Aida::SmartHandle &other, const Rapicorn::Aida::TypeHashList &types)\n{\n' % classH2 # similar to ctor
    s += '  size_t i; const Rapicorn::Aida::TypeHash &mine = _type();\n'
    s += '  for (i = 0; i < types.size(); i++)\n'
    s += '    if (mine == types[i])\n'
    s += '      return __AIDA_Local__::smh2cast<%s> (other);\n' % classH
    s += '  return %s();\n' % classH
    s += '}\n'
    s += 'const Rapicorn::Aida::TypeHashList\n'
    s += '%s::_down_cast_types()\n{\n' % classH
    s += '  Rapicorn::Aida::FieldBuffer &fb = *Rapicorn::Aida::FieldBuffer::_new (3 + 1);\n' # header + self
    s += '  __AIDA_Local__::add_header2 (fb, *this, %s);\n' % self.list_types_digest (class_info)
    s += self.generate_proto_add_args ('fb', class_info, '', [('(*this)', class_info)], '')
    s += '  Rapicorn::Aida::FieldBuffer *fr = __AIDA_Local__::invoke (&fb);\n' # deletes fb
    s += '  AIDA_CHECK (fr != NULL, "missing result from 2-way call");\n'
    s += '  Rapicorn::Aida::FieldReader frr (*fr);\n'
    s += '  frr.skip_header();\n'
    s += '  size_t len;\n'
    s += '  frr >>= len;\n'
    s += '  AIDA_CHECK (frr.remaining() == len * 2, "result truncated");\n'
    s += '  Rapicorn::Aida::TypeHashList thl;\n'
    s += '  Rapicorn::Aida::TypeHash thash;\n'
    s += '  for (size_t i = 0; i < len; i++) {\n'
    s += '    frr >>= thash;\n'
    s += '    thl.push_back (thash);\n'
    s += '  }\n'
    s += '  delete fr;\n'
    s += '  return thl;\n'
    s += '}\n'
    return s
  def generate_server_class_methods (self, class_info):
    assert self.gen_mode == G4SERVANT
    s, classC, classH = '\n', self.C (class_info), self.C4client (class_info)
    s += '%s::%s ()' % (classC, classC) # ctor
    l = [] # constructor agument list
    for sg in class_info.signals:
      l += ['sig_%s (*this)' % sg.name]
    if l:
      s += ' :\n  ' + ', '.join (l)
    s += '\n{}\n'
    s += '%s::~%s ()\n{}\n' % (classC, classC) # dtor
    s += 'void\n'
    s += 'operator<<= (Rapicorn::Aida::FieldBuffer &fb, %s &obj)\n{\n' % classC
    s += '  fb.add_object (__AIDA_Local__::obj2id (&obj));\n'
    s += '}\n'
    s += 'void\n'
    s += 'operator<<= (Rapicorn::Aida::FieldBuffer &fb, %s *obj)\n{\n' % classC
    s += '  fb.add_object (__AIDA_Local__::obj2id (obj));\n'
    s += '}\n'
    s += 'void\n'
    s += 'operator>>= (Rapicorn::Aida::FieldReader &fbr, %s* &obj)\n{\n' % classC
    s += '  obj = __AIDA_Local__::id2obj<%s> (fbr.pop_object());\n' % classC
    s += '}\n'
    s += '%s*\noperator->* (%s &sh, Rapicorn::Aida::_ServantType)\n{\n' % (classC, classH)
    s += '  return __AIDA_Local__::smh2obj<%s> (sh);\n' % classC
    s += '}\n'
    s += '%s\noperator->* (%s *obj, Rapicorn::Aida::_HandleType)\n{\n' % (classH, classC)
    s += '  return __AIDA_Local__::obj2smh<%s> (obj);\n' % classH
    s += '}\n'
    s += 'void\n'
    s += '%s::_list_types (Rapicorn::Aida::TypeHashList &thl) const\n{\n' % classC
    ancestors = self.class_ancestry (class_info)
    ancestors.reverse()
    for an in ancestors:
      s += '  thl.push_back (Rapicorn::Aida::TypeHash (%s)); // %s\n' % (self.class_digest (an), an.name)
    s += '}\n'
    return s
  def generate_server_property_list (self, class_info):
    if not self.property_list:
      return ''
    assert self.gen_mode == G4SERVANT
    s, classC, constPList = '', self.C (class_info), 'const ' + self.property_list
    s += constPList + '&\n' + classC + '::_property_list ()\n{\n'
    s += '  static ' + self.property_list + '::Property *properties[] = {\n'
    for fl in class_info.fields:
      cmmt = '' if fl[1].auxdata.has_key ('label') else '// '
      label, blurb = fl[1].auxdata.get ('label', '"' + fl[0] + '"'), fl[1].auxdata.get ('blurb', '""')
      dflags = fl[1].auxdata.get ('hints', '""')
      s += '    ' + cmmt + 'RAPICORN_AIDA_PROPERTY (%s, %s, %s, %s, %s),\n' % (classC, fl[0], label, blurb, dflags)
    s += '  };\n'
    precls, heritage, cl, ddc = self.interface_class_inheritance (class_info)
    calls = [cl + '::_property_list()' for cl in precls]
    s += '  static ' + constPList + ' property_list (properties, %s);\n' % (', ').join (calls)
    s += '  return property_list;\n'
    s += '}\n'
    return s
  def generate_client_method_stub (self, class_info, mtype):
    s = ''
    hasret = mtype.rtype.storage != Decls.VOID
    # prototype
    s += self.C (mtype.rtype) + '\n'
    q = '%s::%s (' % (self.C (class_info), mtype.name)
    s += q + self.Args (mtype, 'arg_', len (q)) + ')\n{\n'
    # vars, procedure
    s += '  Rapicorn::Aida::FieldBuffer &fb = *Rapicorn::Aida::FieldBuffer::_new (3 + 1 + %u), *fr = NULL;\n' % len (mtype.args) # header + self + args
    if hasret:  s += '  __AIDA_Local__::add_header2 (fb, *this, %s);\n' % self.method_digest (mtype)
    else:       s += '  __AIDA_Local__::add_header1 (fb, *this, %s);\n' % self.method_digest (mtype)
    # marshal args
    s += self.generate_proto_add_args ('fb', class_info, '', [('(*this)', class_info)], '')
    ident_type_args = [('arg_' + a[0], a[1]) for a in mtype.args]
    s += self.generate_proto_add_args ('fb', class_info, '', ident_type_args, '')
    # call out
    s += '  fr = __AIDA_Local__::invoke (&fb);\n' # deletes fb
    # unmarshal return
    if hasret:
      rarg = ('retval', mtype.rtype)
      s += '  Rapicorn::Aida::FieldReader frr (*fr);\n'
      s += '  frr.skip_header();\n'
      s += '  ' + self.V (rarg[0], rarg[1]) + ';\n'
      s += self.generate_proto_pop_args ('frr', class_info, '', [rarg], '')
      s += '  delete fr;\n'
      s += '  return retval;\n'
    else:
      s += '  if (AIDA_UNLIKELY (fr != NULL)) delete fr;\n' # FIXME: check return error
    s += '}\n'
    return s
  def generate_server_method_stub (self, class_info, mtype, reglines):
    assert self.gen_mode == G4SERVANT
    s = ''
    dispatcher_name = '__AIDA_call__%s__%s' % (class_info.name, mtype.name)
    reglines += [ (self.method_digest (mtype), self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Rapicorn::Aida::FieldBuffer*\n'
    s += dispatcher_name + ' (Rapicorn::Aida::FieldReader &fbr)\n'
    s += '{\n'
    s += '  if (fbr.remaining() != 3 + 1 + %u) return __AIDA_Local__::error ("invalid number of arguments");\n' % len (mtype.args)
    # fetch self
    s += '  %s *self;\n' % self.C (class_info)
    s += '  fbr.skip_header();\n'
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  AIDA_CHECK (self, "self must be non-NULL");\n'
    # fetch args
    for a in mtype.args:
      s += '  ' + self.V ('arg_' + a[0], a[1]) + ';\n'
      s += self.generate_proto_pop_args ('fbr', class_info, 'arg_', [(a[0], a[1])])
    # return var
    s += '  '
    hasret = mtype.rtype.storage != Decls.VOID
    if hasret:
      s += self.V ('', mtype.rtype) + 'rval = '
    # call out
    s += 'self->' + mtype.name + ' ('
    s += ', '.join (self.U ('arg_' + a[0], a[1]) for a in mtype.args)
    s += ');\n'
    # store return value
    if hasret:
      s += '  Rapicorn::Aida::FieldBuffer &rb = *__AIDA_Local__::new_result (fbr, %s);\n' % self.method_digest (mtype)
      rval = 'rval'
      s += self.generate_proto_add_args ('rb', class_info, '', [(rval, mtype.rtype)], '')
      s += '  return &rb;\n'
    else:
      s += '  return NULL;\n'
    # done
    s += '}\n'
    return s
  def generate_property_prototype (self, fident, ftype, pad = 0):
    s, v, v0, ptr = '', '', '', ''
    if self.gen_mode == G4SERVANT:
      v, v0, ptr = 'virtual ', ' = 0', '*'
    tname = self.C (ftype)
    pid = fident + ' ' * max (0, pad - len (fident))
    if ftype.storage in (Decls.BOOL, Decls.INT32, Decls.INT64, Decls.FLOAT64, Decls.ENUM):
      s += '  ' + v + self.F (tname)  + pid + ' () const%s;\n' % v0
      s += '  ' + v + self.F ('void') + pid + ' (' + tname + ')%s;\n' % v0
    elif ftype.storage in (Decls.STRING, Decls.RECORD, Decls.SEQUENCE, Decls.ANY):
      s += '  ' + v + self.F (tname)  + pid + ' () const%s;\n' % v0
      s += '  ' + v + self.F ('void') + pid + ' (const ' + tname + '&)%s;\n' % v0
    elif ftype.storage == Decls.INTERFACE:
      s += '  ' + v + self.F (tname + ptr)  + pid + ' () const%s;\n' % v0
      s += '  ' + v + self.F ('void') + pid + ' (' + tname + ptr + ')%s;\n' % v0
    return s
  def generate_client_property_stub (self, class_info, fident, ftype):
    s = ''
    tname = self.C (ftype)
    # getter prototype
    s += tname + '\n'
    q = '%s::%s (' % (self.C (class_info), fident)
    s += q + ') const\n{\n'
    s += '  Rapicorn::Aida::FieldBuffer &fb = *Rapicorn::Aida::FieldBuffer::_new (3 + 1), *fr = NULL;\n'
    s += '  __AIDA_Local__::add_header2 (fb, *this, %s);\n' % self.getter_digest (class_info, fident, ftype)
    s += self.generate_proto_add_args ('fb', class_info, '', [('(*this)', class_info)], '')
    s += '  fr = __AIDA_Local__::invoke (&fb);\n' # deletes fb
    if 1: # hasret
      rarg = ('retval', ftype)
      s += '  Rapicorn::Aida::FieldReader frr (*fr);\n'
      s += '  frr.skip_header();\n'
      s += '  ' + self.V (rarg[0], rarg[1]) + ';\n'
      s += self.generate_proto_pop_args ('frr', class_info, '', [rarg], '')
      s += '  delete fr;\n'
      s += '  return retval;\n'
    s += '}\n'
    # setter prototype
    s += 'void\n'
    if ftype.storage in (Decls.STRING, Decls.RECORD, Decls.SEQUENCE, Decls.ANY):
      s += q + 'const ' + tname + ' &value)\n{\n'
    else:
      s += q + tname + ' value)\n{\n'
    s += '  Rapicorn::Aida::FieldBuffer &fb = *Rapicorn::Aida::FieldBuffer::_new (3 + 1 + 1), *fr = NULL;\n' # header + self + value
    s += '  __AIDA_Local__::add_header1 (fb, *this, %s);\n' % self.setter_digest (class_info, fident, ftype)
    s += self.generate_proto_add_args ('fb', class_info, '', [('(*this)', class_info)], '')
    ident_type_args = [('value', ftype)]
    s += self.generate_proto_add_args ('fb', class_info, '', ident_type_args, '')
    s += '  fr = __AIDA_Local__::invoke (&fb);\n' # deletes fb
    s += '  if (fr) delete fr;\n'
    s += '}\n'
    return s
  def generate_server_property_setter (self, class_info, fident, ftype, reglines):
    assert self.gen_mode == G4SERVANT
    s = ''
    dispatcher_name = '__AIDA_set__%s__%s' % (class_info.name, fident)
    setter_hash = self.setter_digest (class_info, fident, ftype)
    reglines += [ (setter_hash, self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Rapicorn::Aida::FieldBuffer*\n'
    s += dispatcher_name + ' (Rapicorn::Aida::FieldReader &fbr)\n'
    s += '{\n'
    s += '  if (fbr.remaining() != 3 + 1 + 1) return __AIDA_Local__::error ("invalid number of arguments");\n'
    # fetch self
    s += '  %s *self;\n' % self.C (class_info)
    s += '  fbr.skip_header();\n'
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  AIDA_CHECK (self, "self must be non-NULL");\n'
    # fetch property
    s += '  ' + self.V ('arg_' + fident, ftype) + ';\n'
    s += self.generate_proto_pop_args ('fbr', class_info, 'arg_', [(fident, ftype)])
    ref = '&' if ftype.storage == Decls.INTERFACE else ''
    # call out
    s += '  self->' + fident + ' (' + ref + self.U ('arg_' + fident, ftype) + ');\n'
    s += '  return NULL;\n'
    s += '}\n'
    return s
  def generate_server_property_getter (self, class_info, fident, ftype, reglines):
    assert self.gen_mode == G4SERVANT
    s = ''
    dispatcher_name = '__AIDA_get__%s__%s' % (class_info.name, fident)
    getter_hash = self.getter_digest (class_info, fident, ftype)
    reglines += [ (getter_hash, self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Rapicorn::Aida::FieldBuffer*\n'
    s += dispatcher_name + ' (Rapicorn::Aida::FieldReader &fbr)\n'
    s += '{\n'
    s += '  if (fbr.remaining() != 3 + 1) return __AIDA_Local__::error ("invalid number of arguments");\n'
    # fetch self
    s += '  %s *self;\n' % self.C (class_info)
    s += '  fbr.skip_header();\n'
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  AIDA_CHECK (self, "self must be non-NULL");\n'
    # return var
    s += '  '
    s += self.V ('', ftype) + 'rval = '
    # call out
    s += 'self->' + fident + ' ();\n'
    # store return value
    s += '  Rapicorn::Aida::FieldBuffer &rb = *__AIDA_Local__::new_result (fbr, %s);\n' % getter_hash
    rval = 'rval'
    s += self.generate_proto_add_args ('rb', class_info, '', [(rval, ftype)], '')
    s += '  return &rb;\n'
    s += '}\n'
    return s
  def generate_server_list_types (self, class_info, reglines):
    assert self.gen_mode == G4SERVANT
    s = ''
    dispatcher_name = '__AIDA_types__%s' % class_info.name
    digest = self.list_types_digest (class_info)
    reglines += [ (digest, self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Rapicorn::Aida::FieldBuffer*\n'
    s += dispatcher_name + ' (Rapicorn::Aida::FieldReader &fbr)\n'
    s += '{\n'
    s += '  if (fbr.remaining() != 3 + 1) return __AIDA_Local__::error ("invalid number of arguments");\n'
    s += '  %s *self;\n' % self.C (class_info)  # fetch self
    s += '  fbr.skip_header();\n'
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  AIDA_CHECK (self, "self must be non-NULL");\n'
    s += '  Rapicorn::Aida::TypeHashList thl;\n'
    s += '  self->_list_types (thl);\n'
    # return: length (typehi, typelo)*length
    s += '  Rapicorn::Aida::FieldBuffer &rb = *__AIDA_Local__::new_result (fbr, %s, 1 + 2 * thl.size());\n' % digest
    s += '  rb <<= Rapicorn::Aida::int64_t (thl.size());\n'
    s += '  for (size_t i = 0; i < thl.size(); i++)\n'
    s += '    rb <<= thl[i];\n'
    s += '  return &rb;\n'
    s += '}\n'
    return s
  def generate_signal_typename (self, functype, ctype, prefix = 'Signal'):
    return '%s_%s' % (prefix, functype.name)
  def generate_signal_signature_tuple (self, functype, funcname = ''):
    s, r = '', self.R (functype.rtype)
    if funcname:
      s += '%s ' % funcname
    s += '('
    l = []
    for a in functype.args:
      l += [ self.A (a[0], a[1]) ]
    s += ', '.join (l)
    s += ')'
    return (r, s)
  def generate_client_signal_decl (self, functype, ctype):
    assert self.gen_mode == G4STUB
    s, cbtname, u64 = '', self.generate_signal_typename (functype, ctype, 'Callback'), 'Rapicorn::Aida::uint64_t'
    sigret, sigfunc = self.generate_signal_signature_tuple (functype, cbtname)
    s += '  ' + self.F ('typedef ' + sigret) + sigfunc + ';\n'
    s += '  ' + self.F (u64 + ' ') + 'sig_' + functype.name + ' (const std::function<%s> &callback);\n' % cbtname
    s += '  ' + self.F ('bool ') + 'sig_' + functype.name + ' (%s signal_id); ///< Disconnect Signal\n' % u64
    return s
  def generate_client_signal_def (self, class_info, functype):
    assert self.gen_mode == G4STUB
    s, cbtname, classH = '', self.generate_signal_typename (functype, class_info, 'Callback'), self.C4client (class_info)
    (sigret, sigfunc), u64 = self.generate_signal_signature_tuple (functype, cbtname), 'Rapicorn::Aida::uint64_t'
    emitfunc = '__AIDA_emit__%s__%s' % (classH, functype.name)
    s += 'static Rapicorn::Aida::FieldBuffer*\n%s ' % emitfunc
    s += '(Rapicorn::Aida::ClientConnection &aida_con, const Rapicorn::Aida::FieldBuffer *sfb, void *data)\n{\n'
    s += '  auto fptr = (const std::function<%s::%s>*) data;\n' % (classH, cbtname)
    s += '  if (AIDA_UNLIKELY (!sfb)) { delete fptr; return NULL; }\n'
    s += '  Rapicorn::Aida::field_buffer_emit_signal (*sfb, *fptr);\n'
    s += '  return NULL; // no support for remote signal returns atm\n'
    s += '}\n'
    s += u64 + '\n%s::sig_%s (const std::function<%s> &func)\n{\n' % (classH, functype.name, cbtname)
    s += '  void *fptr = new std::function<%s> (func);\n' % cbtname
    s += '  %s id = __AIDA_Local__::signal_connect (%s, *this, %s, fptr);\n' % (u64, self.method_digest (functype), emitfunc)
    s += '  return id;\n}\n'
    s += 'bool\n%s::sig_%s (%s signal_id)\n{\n' % (classH, functype.name, u64)
    s += '  const bool r = __AIDA_Local__::signal_disconnect (signal_id);\n'
    s += '  return r;\n}\n'
    return s
  def generate_server_signal_typedef (self, functype, ctype, prefix = '', ancestor = ''):
    s, signame = '', self.generate_signal_typename (functype, ctype)
    cpp_rtype = self.R (functype.rtype)
    s += 'typedef Rapicorn::Signals::Signal<%s, %s (' % (self.C (ctype), cpp_rtype)
    l = []
    for a in functype.args:
      l += [ self.A (a[0], a[1]) ]
    s += ', '.join (l)
    s += ')'
    if functype.rtype.collector != 'void' or ancestor:
      colname = functype.rtype.collector if functype.rtype.collector != 'void' else 'Default'
      s += ', Rapicorn::Signals::Collector' + colname.capitalize() + '<' + cpp_rtype + '>'
      if ancestor:
        s += ', ' + ancestor
      else:
        s += ' '
    s += '> ' + prefix + signame + ';\n'
    return s
  def generate_server_signal_dispatcher (self, class_info, stype, reglines):
    assert self.gen_mode == G4SERVANT
    s = ''
    dispatcher_name = '__AIDA_signal__%s__%s' % (class_info.name, stype.name)
    digest = self.method_digest (stype)
    reglines += [ (digest, self.namespaced_identifier (dispatcher_name)) ]
    closure_class = '__AIDA_Closure__%s__%s' % (class_info.name, stype.name)
    s += 'class %s {\n' % closure_class
    s += '  Rapicorn::Aida::ServerConnection &m_connection; Rapicorn::Aida::uint64_t m_handler;\n'
    s += 'public:\n'
    s += '  typedef std::shared_ptr<%s> SharedPtr;\n' % closure_class
    s += '  %s (Rapicorn::Aida::ServerConnection &conn, Rapicorn::Aida::uint64_t h) : m_connection (conn), m_handler (h) {}\n' % closure_class # ctor
    s += '  ~%s()\n' % closure_class # dtor
    s += '  {\n'
    s += '    Rapicorn::Aida::FieldBuffer &fb = *Rapicorn::Aida::FieldBuffer::_new (3 + 1);\n' # header + handler
    s += '    __AIDA_Local__::add_header1_discon (fb, m_handler, %s);\n' % digest
    s += '    fb <<= m_handler;\n'
    s += '    __AIDA_Local__::post_msg (&fb);\n' # deletes fb
    s += '  }\n'
    cpp_rtype = self.R (stype.rtype)
    s += '  static %s\n' % cpp_rtype
    s += '  handler ('
    s += self.Args (stype, 'arg_', 11) + (',\n           ' if stype.args else '')
    s += 'SharedPtr sp)\n  {\n'
    s += '    Rapicorn::Aida::FieldBuffer &fb = *Rapicorn::Aida::FieldBuffer::_new (3 + 1 + %u);\n' % len (stype.args) # header + handler + args
    s += '    __AIDA_Local__::add_header1_event (fb, sp->m_handler, %s);\n' % digest
    s += '    fb <<= sp->m_handler;\n'
    ident_type_args = [('arg_' + a[0], a[1]) for a in stype.args] # marshaller args
    args2fb = self.generate_proto_add_args ('fb', class_info, '', ident_type_args, '')
    if args2fb:
      s += reindent ('  ', args2fb) + '\n'
    s += '    __AIDA_Local__::post_msg (&fb);\n' # deletes fb
    if stype.rtype.storage != Decls.VOID:
      s += '    return %s;\n' % self.mkzero (stype.rtype)
    s += '  }\n'
    s += '};\n'
    s += 'static Rapicorn::Aida::FieldBuffer*\n'
    s += dispatcher_name + ' (Rapicorn::Aida::FieldReader &fbr)\n'
    s += '{\n'
    s += '  if (fbr.remaining() != 3 + 1 + 2) return __AIDA_Local__::error ("invalid number of arguments");\n'
    s += '  %s *self;\n' % self.C (class_info)
    s += '  fbr.skip_header();\n'
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  AIDA_CHECK (self, "self must be non-NULL");\n'
    s += '  Rapicorn::Aida::uint64_t handler_id, signal_connection, cid = 0;\n'
    s += '  fbr >>= handler_id;\n'
    s += '  fbr >>= signal_connection;\n'
    s += '  if (signal_connection) self->sig_%s.disconnect (signal_connection);\n' % stype.name
    s += '  if (handler_id) {\n'
    s += '    %s::SharedPtr sp (new %s (AIDA_CONNECTION(), handler_id));\n' % (closure_class, closure_class)
    s += '    cid = self->sig_%s.connect (slot (sp->handler, sp)); }\n' % stype.name
    s += '  Rapicorn::Aida::FieldBuffer &rb = *__AIDA_Local__::new_result (fbr, %s);\n' % digest
    s += '  rb <<= cid;\n'
    s += '  return &rb;\n'
    s += '}\n'
    return s
  def generate_server_method_registry (self, reglines):
    s = ''
    if len (reglines) == 0:
      return '// Skipping empty MethodRegistry\n'
    s += 'static const __AIDA_Local__::MethodEntry _aida_stub_entries[] = {\n'
    for dispatcher in reglines:
      cdigest, dispatcher_name = dispatcher
      s += '  { ' + cdigest + ', '
      s += dispatcher_name + ', },\n'
    s += '};\n'
    s += 'static __AIDA_Local__::MethodRegistry _aida_stub_registry (_aida_stub_entries);\n'
    return s
  def generate_virtual_method_skel (self, functype, type_info):
    assert self.gen_mode == G4SERVANT
    s = ''
    if functype.pure:
      return s
    absname = self.C (type_info) + '::' + functype.name
    if absname in self.skip_symbols:
      return ''
    sret = self.R (functype.rtype)
    sret += '\n'
    absname += ' ('
    argindent = len (absname)
    s += '\n' + sret + absname
    l = []
    for a in functype.args:
      l += [ self.A (a[0], a[1]) ]
    s += (',\n' + argindent * ' ').join (l)
    s += ')\n{\n'
    if functype.rtype.storage == Decls.VOID:
      pass
    else:
      s += '  return %s;\n' % self.mkzero (functype.rtype)
    s += '}\n'
    return s
  def generate_interface_skel (self, type_info):
    s = ''
    for m in type_info.methods:
      s += self.generate_virtual_method_skel (m, type_info)
    return s
  def generate_enum_decl (self, type_info):
    s = '\n'
    nm = type_info.name
    l = []
    s += 'enum %s {\n' % type_info.name
    for opt in type_info.options:
      (ident, label, blurb, number) = opt
      s += '  %s = %s,' % (ident, number)
      if blurb:
        s += ' // %s' % re.sub ('\n', ' ', blurb)
      s += '\n'
    s += '};\n'
    s += 'inline void operator<<= (Rapicorn::Aida::FieldBuffer &fb,  %s  e) ' % nm
    s += '{ fb <<= Rapicorn::Aida::EnumValue (e); }\n'
    s += 'inline void operator>>= (Rapicorn::Aida::FieldReader &frr, %s &e) ' % nm
    s += '{ e = %s (frr.pop_evalue()); }\n' % nm
    if type_info.combinable: # enum as flags
      s += 'inline %s  operator&  (%s  s1, %s s2) { return %s (s1 & Rapicorn::Aida::uint64_t (s2)); }\n' % (nm, nm, nm, nm)
      s += 'inline %s& operator&= (%s &s1, %s s2) { s1 = s1 & s2; return s1; }\n' % (nm, nm, nm)
      s += 'inline %s  operator|  (%s  s1, %s s2) { return %s (s1 | Rapicorn::Aida::uint64_t (s2)); }\n' % (nm, nm, nm, nm)
      s += 'inline %s& operator|= (%s &s1, %s s2) { s1 = s1 | s2; return s1; }\n' % (nm, nm, nm)
    return s
  def insertion_text (self, key):
    text = self.insertions.get (key, '')
    lstrip = re.compile ('^\n*')
    rstrip = re.compile ('\n*$')
    text = lstrip.sub ('', text)
    text = rstrip.sub ('', text)
    if text:
      ind = '  ' if key.startswith ('class_scope:') else '' # inner class indent
      return ind + '// ' + key + ':\n' + text + '\n'
    else:
      return ''
  def insertion_file (self, filename):
    f = open (filename, 'r')
    key = None
    for line in f:
      m = (re.match ('(includes):\s*(//.*)?$', line) or
           re.match ('(class_scope:\w+):\s*(//.*)?$', line) or
           re.match ('(global_scope):\s*(//.*)?$', line))
      if not m:
        m = re.match ('(IGNORE):\s*(//.*)?$', line)
      if m:
        key = m.group (1)
        continue
      if key:
        block = self.insertions.get (key, '')
        block += line
        self.insertions[key] = block
  def symbol_file (self, filename):
    f = open (filename)
    txt = f.read()
    f.close()
    import re
    w = re.findall (r'(\b[a-zA-Z_][a-zA-Z_0-9$:]*)(?:\()', txt)
    self.skip_symbols.update (set (w))
  def generate_impl_types (self, implementation_types):
    def text_expand (txt):
      txt = txt.replace ('$AIDA_iface_base$', self.iface_base)
      return txt
    self.gen_mode = G4SERVANT if self.gen_serverhh or self.gen_servercc else G4STUB
    s = '// --- Generated by AidaCxxStub ---\n'
    # CPP guard
    if self.cppguard:
      s += '#ifndef %s\n#define %s\n\n' % (self.cppguard, self.cppguard)
    # inclusions
    if self.gen_inclusions:
      s += '\n// --- Custom Includes ---\n'
    if self.gen_inclusions and (self.gen_clientcc or self.gen_servercc):
      s += '#ifndef __AIDA_UTILITIES_HH__\n'
    for i in self.gen_inclusions:
      s += '#include %s\n' % i
    if self.gen_inclusions and (self.gen_clientcc or self.gen_servercc):
      s += '#endif\n'
    s += self.insertion_text ('includes')
    if self.gen_clienthh:
      s += clienthh_boilerplate
    if self.gen_serverhh:
      s += serverhh_boilerplate
      if self.iface_base == self.test_iface_base:
        s += serverhh_testcode
      if self.gen_rapicornsignals:
        s += rapicornsignal_boilerplate
    if self.gen_servercc and self.iface_base == self.test_iface_base:
      s += servercc_testcode + '\n'
    if self.gen_servercc:
      s += gencc_boilerplate + '\n' + text_expand (servercc_boilerplate) + '\n'
    if self.gen_clientcc:
      s += gencc_boilerplate + '\n' + clientcc_boilerplate + '\n'
    self.tab_stop (30)
    s += self.open_namespace (None)
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # generate client/server decls
    if self.gen_clienthh or self.gen_serverhh:
      self.gen_mode = G4SERVANT if self.gen_serverhh else G4STUB
      s += '\n// --- Interfaces (class declarations) ---\n'
      spc_enums = []
      for tp in types:
        if tp.is_forward:
          s += self.open_namespace (tp) + '\n'
          s += 'class %s;\n' % self.C (tp)
        elif tp.typedef_origin:
          s += self.open_namespace (tp) + '\n'
          s += 'typedef %s %s;\n' % (self.C (tp.typedef_origin), tp.name)
        elif tp.storage in (Decls.RECORD, Decls.SEQUENCE) and self.gen_mode == G4STUB:
          s += self.open_namespace (tp)
          s += self.generate_recseq_decl (tp)
        elif tp.storage == Decls.ENUM and self.gen_mode == G4STUB:
          s += self.open_namespace (tp)
          s += self.generate_enum_decl (tp)
          spc_enums += [ tp ]
        elif tp.storage == Decls.INTERFACE:
          s += self.open_namespace (tp)
          s += self.generate_interface_class (tp)     # Class smart handle
      if spc_enums:
        s += self.open_namespace (self.ns_aida)
        for tp in spc_enums:
          s += self.generate_enum_info_specialization (tp)
      s += self.open_namespace (None)
    # generate client/server impls
    if self.gen_clientcc or self.gen_servercc:
      self.gen_mode = G4SERVANT if self.gen_servercc else G4STUB
      s += '\n// --- Implementations ---\n'
      for tp in types:
        if tp.typedef_origin or tp.is_forward:
          continue
        if tp.storage == Decls.RECORD and self.gen_mode == G4STUB:
          s += self.open_namespace (tp)
          s += self.generate_record_impl (tp)
        elif tp.storage == Decls.SEQUENCE and self.gen_mode == G4STUB:
          s += self.open_namespace (tp)
          s += self.generate_sequence_impl (tp)
        elif tp.storage == Decls.ENUM and self.gen_mode == G4STUB:
          s += self.open_namespace (tp)
          s += self.generate_enum_impl (tp)
        elif tp.storage == Decls.INTERFACE:
          if self.gen_servercc:
            s += self.open_namespace (tp)
            s += self.generate_server_class_methods (tp)
            s += self.generate_server_property_list (tp)
          if self.gen_clientcc:
            s += self.open_namespace (tp)
            for sg in tp.signals:
              s += self.generate_client_signal_def (tp, sg)
            s += self.generate_client_class_methods (tp)
            for fl in tp.fields:
              s += self.generate_client_property_stub (tp, fl[0], fl[1])
            for m in tp.methods:
              s += self.generate_client_method_stub (tp, m)
    # generate unmarshalling server calls
    if self.gen_servercc:
      self.gen_mode = G4SERVANT
      s += '\n// --- Method Dispatchers & Registry ---\n'
      reglines = []
      for tp in types:
        if tp.typedef_origin or tp.is_forward:
          continue
        s += self.open_namespace (tp)
        if tp.storage == Decls.INTERFACE:
          s += self.generate_server_list_types (tp, reglines)
          for fl in tp.fields:
            s += self.generate_server_property_getter (tp, fl[0], fl[1], reglines)
            s += self.generate_server_property_setter (tp, fl[0], fl[1], reglines)
          for m in tp.methods:
            s += self.generate_server_method_stub (tp, m, reglines)
          for sg in tp.signals:
            s += self.generate_server_signal_dispatcher (tp, sg, reglines)
          s += '\n'
      s += self.generate_server_method_registry (reglines) + '\n'
      s += self.open_namespace (None)
    # generate interface method skeletons
    if self.gen_server_skel:
      s += self.open_namespace (None)
      self.gen_mode = G4SERVANT
      s += '\n// --- Interface Skeletons ---\n'
      for tp in types:
        if tp.typedef_origin or tp.is_forward:
          continue
        elif tp.storage == Decls.INTERFACE:
          s += self.generate_interface_skel (tp)
    s += self.open_namespace (None) # close all namespaces
    s += '\n'
    s += self.insertion_text ('global_scope')
    # CPP guard
    if self.cppguard:
      s += '#endif /* %s */\n' % self.cppguard
    return s

def error (msg):
  import sys
  print >>sys.stderr, sys.argv[0] + ":", msg
  sys.exit (127)

def generate (namespace_list, **args):
  import sys, tempfile, os
  global I_prefix_postfix
  config = {}
  config.update (args)
  gg = Generator()
  all = config['backend-options'] == []
  gg.gen_serverhh = all or 'serverhh' in config['backend-options']
  gg.gen_servercc = all or 'servercc' in config['backend-options']
  gg.gen_server_skel = 'server-skel' in config['backend-options']
  gg.gen_clienthh = all or 'clienthh' in config['backend-options']
  gg.gen_clientcc = all or 'clientcc' in config['backend-options']
  gg.gen_rapicornsignals = all or 'RapicornSignal' in config['backend-options']
  gg.gen_inclusions = config['inclusions']
  for opt in config['backend-options']:
    if opt.startswith ('cppguard='):
      gg.cppguard += opt[9:]
    if opt.startswith ('iface-postfix='):
      I_prefix_postfix = (I_prefix_postfix[0], opt[14:])
    if opt.startswith ('iface-prefix='):
      I_prefix_postfix = (opt[13:], I_prefix_postfix[1])
    if opt.startswith ('iface-base='):
      gg.iface_base = opt[11:]
    if opt.startswith ('property-list=') and opt[14:].lower() in ('0', 'no', 'none', 'false'):
      gg.property_list = ""
  for ifile in config['insertions']:
    gg.insertion_file (ifile)
  for ssfile in config['skip-skels']:
    gg.symbol_file (ssfile)
  ns_rapicorn = Decls.Namespace ('Rapicorn', None, [])
  gg.ns_aida = ( ns_rapicorn, Decls.Namespace ('Aida', ns_rapicorn, []) ) # Rapicorn::Aida namespace tuple for open_namespace()
  textstring = gg.generate_impl_types (config['implementation_types']) # namespace_list
  outname = config.get ('output', '-')
  if outname != '-':
    fout = open (outname, 'w')
    fout.write (textstring)
    fout.close()
  else:
    print textstring,

# control module exports
__all__ = ['generate']
