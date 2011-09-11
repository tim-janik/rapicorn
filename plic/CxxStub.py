# Licensed GNU GPLv3 or later: http://www.gnu.org/licenses/gpl.html
"""PLIC-CxxStub - C++ RPC Glue Code Generator

More details at http://www.testbit.eu/PLIC
"""
import Decls, GenUtils, re

clienthh_boilerplate = r"""
// --- ClientHH Boilerplate ---
#include <plic/runtime.hh>
"""

serverhh_boilerplate = r"""
// --- ServerHH Boilerplate ---
#include <plic/runtime.hh>
"""

rapicornsignal_boilerplate = r"""
#include <rapicorn-core.hh> // for rcore/rapicornsignal.hh
using Rapicorn::Signals::slot;
"""

gencc_boilerplate = r"""
// --- ClientCC/ServerCC Boilerplate ---
#include <string>
#include <vector>
#include <stdexcept>
#ifndef __PLIC_GENERIC_CC_BOILERPLATE__
#define __PLIC_GENERIC_CC_BOILERPLATE__

#define PLIC_CHECK(cond,errmsg) do { if (cond) break; throw std::runtime_error (std::string ("PLIC-ERROR: ") + errmsg); } while (0)

namespace { // Anonymous
using Plic::uint64_t;

static __attribute__ ((__format__ (__printf__, 1, 2), unused))
Plic::FieldBuffer* plic$_error (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  Plic::error_vprintf (format, args);
  va_end (args);
  return NULL;
}

} // Anonymous
#endif // __PLIC_GENERIC_CC_BOILERPLATE__
"""

servercc_boilerplate = r"""
#ifndef PLIC_CONNECTION
#define PLIC_CONNECTION()       (*(Plic::Connection*)NULL)
template<class O> O*  connection_id2object (Plic::uint64_t oid) { return dynamic_cast<O*> (reinterpret_cast<Plic::SimpleServer*> (oid)); }
inline Plic::uint64_t connection_object2id (const Plic::SimpleServer *obj) { return reinterpret_cast<ptrdiff_t> (obj); }
inline Plic::uint64_t connection_object2id (const Plic::SimpleServer &obj) { return connection_object2id (&obj); }
#endif // !PLIC_CONNECTION
"""

clientcc_boilerplate = r"""
#ifndef PLIC_CONNECTION
#define PLIC_CONNECTION()       (*(Plic::Connection*)NULL)
Plic::uint64_t       connection_handle2id  (const Plic::SmartHandle &h) { return h._rpc_id(); }
static inline void   connection_context4id (Plic::uint64_t ipcid, Plic::NonCopyable *ctx) {}
template<class C> C* connection_id2context (Plic::uint64_t oid) { return (C*) NULL; }
#endif // !PLIC_CONNECTION
"""

identifiers = {
  'downcast' :          'downcast',
  'cast_types' :        'cast_types',
};

def reindent (prefix, lines):
  return re.compile (r'^', re.M).sub (prefix, lines.rstrip())

I_prefix_postfix = ('', '_Interface')

class G4CLIENT: pass    # generate client side classes (smart handles)
class G4SERVER: pass    # generate server side classes (interfaces)

class Generator:
  def __init__ (self):
    self.ntab = 26
    self.namespaces = []
    self.insertions = {}
    self.cppguard = ''
    self.gen_inclusions = []
    self.skip_symbols = set()
    self.skip_classes = []
    self._iface_base = 'Plic::SimpleServer'
    self.gen_mode = None
    self.gen_shortalias = False
    self.object_impl = None # ('impl', ('', 'Impl'))
  def Iwrap (self, name):
    cc = name.rfind ('::')
    if cc >= 0:
      ns, base = name[:cc+2], name[cc+2:]
    else:
      ns, base = '', name
    return ns + I_prefix_postfix[0] + base + I_prefix_postfix[1]
  def type2cpp (self, type_node):
    typename = type_node.name
    if typename == 'any': return 'Plic::Any'
    if typename == 'float': return 'double'
    if typename == 'string': return 'std::string'
    fullnsname = '::'.join (self.type_relative_namespaces (type_node) + [ type_node.name ])
    return fullnsname
  def H (self, name):                                   # construct client class Handle
    return name + '_SmartHandle'
  def C4server (self, type_node):
    tname = self.type2cpp (type_node)
    if type_node.storage in (Decls.SEQUENCE, Decls.RECORD):
      return tname + "Impl"     # FIXME
    elif type_node.storage == Decls.INTERFACE:
      return self.Iwrap (tname)
    else:
      return tname
  def C4client (self, type_node):
    tname = self.type2cpp (type_node)
    if type_node.storage in (Decls.SEQUENCE, Decls.RECORD):
      return tname + "_Handle"  # FIXME
    elif type_node.storage == Decls.INTERFACE:
      return self.H (tname)
    return tname
  def C (self, type_node, mode = None):                 # construct Class name
    mode = mode or self.gen_mode
    if mode == G4SERVER:
      return self.C4server (type_node)
    else: # G4CLIENT
      return self.C4client (type_node)
  def R (self, type_node):                              # construct Return type
    tname = self.C (type_node)
    if self.gen_mode == G4SERVER and type_node.storage == Decls.INTERFACE:
      tname += '*'
    return tname
  def V (self, ident, type_node, f_delta = -999999):    # construct Variable
    s = ''
    s += self.C (type_node)
    s = self.F (s, f_delta)
    if self.gen_mode == G4SERVER and type_node.storage == Decls.INTERFACE:
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
    s = '*' if type_node.storage == Decls.INTERFACE and self.gen_mode == G4SERVER else ''
    return s + ident
  def F (self, string, delta = 0):                      # Format string to tab stop
    return string + ' ' * max (1, self.ntab + delta - len (string))
  def tab_stop (self, n):
    self.ntab = n
  def close_inner_namespace (self):
    return '} // %s\n' % self.namespaces.pop().name
  def open_inner_namespace (self, namespace):
    self.namespaces += [ namespace ]
    return '\nnamespace %s {\n' % self.namespaces[-1].name
  def open_namespace (self, type):
    s = ''
    newspaces = []
    if type:
      newspaces = type.list_namespaces()
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
    namespace_names = [d.name for d in tnsl]
    return namespace_names
  def namespaced_identifier (self, ident):
    return '::'.join ([d.name for d in self.namespaces] + [ident])
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
    if type_info.storage == Decls.SEQUENCE:
      fl = type_info.elements
      s += 'struct ' + self.C (type_info) + ' : public std::vector<' + self.R (fl[1]) + '> {\n'
    else:
      s += 'struct %s {\n' % self.C (type_info)
    if type_info.storage == Decls.RECORD:
      fieldlist = type_info.fields
      for fl in fieldlist:
        s += '  ' + self.F (self.R (fl[1])) + fl[0] + ';\n'
    elif type_info.storage == Decls.SEQUENCE:
      s += '  typedef std::vector<' + self.R (fl[1]) + '> Sequence;\n'
    if type_info.storage == Decls.RECORD:
      s += '  ' + self.F ('inline') + '%s () {' % self.C (type_info) # ctor
      for fl in fieldlist:
        if fl[1].storage in (Decls.INT, Decls.FLOAT, Decls.ENUM):
          s += " %s = %s;" % (fl[0], self.mkzero (fl[1]))
      s += ' }\n'
    s += self.insertion_text ('class_scope:' + type_info.name)
    s += '};\n'
    if type_info.storage in (Decls.RECORD, Decls.SEQUENCE):
      s += 'Plic::FieldBuffer& operator<< (Plic::FieldBuffer&, const %s&);\n' % self.C (type_info)
      s += 'Plic::FieldReader& operator>> (Plic::FieldReader&, %s&);\n' % self.C (type_info)
    s += self.generate_shortalias (type_info)   # typedef alias
    return s
  def generate_proto_add_args (self, fb, type_info, aprefix, arg_info_list, apostfix):
    s = ''
    for arg_it in arg_info_list:
      ident, type_node = arg_it
      ident = aprefix + ident + apostfix
      s += '  %s << %s;\n' % (fb, ident)
    return s
  def generate_proto_pop_args (self, fbr, type_info, aprefix, arg_info_list, apostfix = ''):
    s = ''
    for arg_it in arg_info_list:
      ident, type_node = arg_it
      ident = aprefix + ident + apostfix
      s += '  %s >> %s;\n' % (fbr, ident)
    return s
  def generate_record_impl (self, type_info):
    s = ''
    s += 'inline Plic::FieldBuffer& __attribute__ ((used))\n'
    s += 'operator<< (Plic::FieldBuffer &dst, const %s &self)\n{\n' % self.C (type_info)
    s += '  Plic::FieldBuffer &fb = dst.add_rec (%u);\n' % len (type_info.fields)
    s += self.generate_proto_add_args ('fb', type_info, 'self.', type_info.fields, '')
    s += '  return dst;\n'
    s += '}\n'
    s += 'inline Plic::FieldReader& __attribute__ ((used))\n'
    s += 'operator>> (Plic::FieldReader &src, %s &self)\n{\n' % self.C (type_info)
    s += '  Plic::FieldReader fbr (src.pop_rec());\n'
    s += '  if (fbr.remaining() < %u) return src;\n' % len (type_info.fields)
    s += self.generate_proto_pop_args ('fbr', type_info, 'self.', type_info.fields)
    s += '  return src;\n'
    s += '}\n'
    return s
  def generate_sequence_impl (self, type_info):
    s = ''
    el = type_info.elements
    s += 'inline Plic::FieldBuffer& __attribute__ ((used))\n'
    s += 'operator<< (Plic::FieldBuffer &dst, const %s &self)\n{\n' % self.C (type_info)
    s += '  const size_t len = self.size();\n'
    s += '  Plic::FieldBuffer &fb = dst.add_seq (len);\n'
    s += '  for (size_t k = 0; k < len; k++) {\n'
    s += reindent ('  ', self.generate_proto_add_args ('fb', type_info, '',
                                                       [('self', type_info.elements[1])],
                                                       '[k]')) + '\n'
    s += '  }\n'
    s += '  return dst;\n'
    s += '}\n'
    s += 'inline Plic::FieldReader& __attribute__ ((used))\n'
    s += 'operator>> (Plic::FieldReader &src, %s &self)\n{\n' % self.C (type_info)
    s += '  Plic::FieldReader fbr (src.pop_seq());\n'
    s += '  const size_t len = fbr.remaining();\n'
    if el[1].storage == Decls.INTERFACE:
      s += '  self.reserve (len);\n'
    else:
      s += '  self.resize (len);\n'
    s += '  for (size_t k = 0; k < len; k++) {\n'
    s += '    fbr >> self[k];\n'
    s += '  }\n'
    s += '  return src;\n'
    s += '}\n'
    return s
  def digest2cbytes (self, digest):
    return '0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL' % digest
  def method_digest (self, method_info):
    return self.digest2cbytes (method_info.type_hash())
  def class_digest (self, class_info):
    return self.digest2cbytes (class_info.type_hash())
  def list_types_digest (self, class_info):
    return self.digest2cbytes (class_info.twoway_hash ('Plic:list_types()'))
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
    plic_smarthandle = 'Plic::SmartHandle'
    l = self.interface_class_ancestors (type_info)
    l = [self.C (pr) for pr in l] # types -> names
    if   self.gen_mode == G4SERVER and l:
      heritage = 'public virtual'
    elif self.gen_mode == G4SERVER and not l:
      l = [self._iface_base]
      heritage = 'public virtual'
    elif self.gen_mode == G4CLIENT and l:
      heritage = 'public'
    elif self.gen_mode == G4CLIENT and not l:
      l = [plic_smarthandle]
      heritage = 'public virtual'
    if self.gen_mode == G4CLIENT:
      cl = l if l == [plic_smarthandle] else [plic_smarthandle] + l
    else:
      cl = []
    return (l, heritage, cl) # prerequisite list, heritage type, constructor arg list)
  def generate_interface_class (self, type_info):
    s, classC = '\n', self.C (type_info) # class names
    # declare
    s += 'class %s' % classC
    # inherit
    l, heritage, cl = self.interface_class_inheritance (type_info)
    s += ' : ' + heritage + ' %s' % (', ' + heritage + ' ').join (l) + '\n'
    s += '  /// See also the corresponding IDL class %s.\n' % type_info.name # doxygen
    s += '{\n'
    if self.gen_mode == G4CLIENT:
      s += '  ' + self.F ('static %s' % classC) + '_cast (Plic::SmartHandle&, const Plic::TypeHashList&);\n'
      s += '  ' + self.F ('static const Plic::TypeHash&') + '_type ();\n'
    # constructors
    s += 'protected:\n'
    if self.gen_mode == G4SERVER:
      s += '  explicit ' + self.F ('') + '%s ();\n' % self.C (type_info) # ctor
      s += '  virtual ' + self.F ('/*Des*/') + '~%s () = 0;\n' % self.C (type_info) # dtor
    else: # G4CLIENT
      for sg in type_info.signals:
        s += '  ' + self.generate_signal_proxy_typedef (sg, type_info)
    s += 'public:\n'
    if self.gen_mode == G4SERVER:
      s += '  virtual ' + self.F ('void') + '_list_types (Plic::TypeHashList&) const;\n'
    else: # G4CLIENT
      classH = self.H (type_info.name) # smart handle class name
      aliasfix = '__attribute__ ((noinline))' # work around bogus strict-aliasing warning in g++-4.4.5
      s += '  template<class C>\n'
      s += '  ' + self.F ('static %s' % classH) + identifiers['downcast'] + ' (C c) ' # ctor
      s += '{ return _cast (c, c.%s()); }\n' % identifiers['cast_types']
      s += '  ' + self.F ('const Plic::TypeHashList&') + identifiers['cast_types'] + ' ();\n'
      s += '  ' + self.F ('explicit') + '%s ();\n' % classH # ctor
      #s += '  ' + self.F ('inline') + '%s (const %s &src)' % (classH, classH) # copy ctor
      #s += ' : ' + ' (src), '.join (cl) + ' (src) {}\n'
    # properties
    il = 0
    if type_info.fields:
      il = max (len (fl[0]) for fl in type_info.fields)
    for fl in type_info.fields:
      s += self.generate_property (fl[0], fl[1], il)
    # signals
    if self.gen_mode == G4SERVER:
      for sg in type_info.signals:
        s += '  ' + self.generate_signal_typedef (sg, type_info)
      for sg in type_info.signals:
        s += '  ' + self.generate_signal_typename (sg, type_info) + ' sig_%s;\n' % sg.name
    else: # G4CLIENT
      for sg in type_info.signals:
        s += self.generate_signal_accessor_def (sg, type_info)
    # methods
    il = 0
    if type_info.methods:
      il = max (len (m.name) for m in type_info.methods)
      il = max (il, len (self.C (type_info)))
    for m in type_info.methods:
      s += self.generate_method_decl (m, il)
    # specials (operators)
    if self.gen_mode == G4CLIENT:
      s += '//' + self.F ('inline', -9)
      s += 'operator _UnspecifiedBool () const ' # return non-NULL pointer to member on true
      s += '{ return _is_null() ? NULL : _unspecified_bool_true(); }\n' # avoids auto-bool conversions on: float (*this)
    if self.gen_mode == G4SERVER and self.object_impl:
      impl_method, ppwrap = self.object_impl
      implname = ppwrap[0] + type_info.name + ppwrap[1]
      s += '  inline %s&       impl () ' % implname
      s += '{ %s *_impl = dynamic_cast<%s*> (this); if (!_impl) throw std::bad_cast(); return *_impl; }\n' % (implname, implname)
      s += '  inline const %s& impl () const { return impl (const_cast<%s*> (this)); }\n' % (implname, self.C (type_info))
    s += self.insertion_text ('class_scope:' + type_info.name)
    s += '};\n'
    if self.gen_mode == G4SERVER:
      s += 'Plic::FieldBuffer& operator<< (Plic::FieldBuffer&, %s&);\n' % self.C (type_info)
      s += 'Plic::FieldBuffer& operator<< (Plic::FieldBuffer&, %s*);\n' % self.C (type_info)
      s += 'Plic::FieldReader& operator>> (Plic::FieldReader&, %s*&);\n' % self.C (type_info)
    else: # G4CLIENT
      s += 'Plic::FieldBuffer& operator<< (Plic::FieldBuffer&, const %s&);\n' % self.C (type_info)
      s += 'Plic::FieldReader& operator>> (Plic::FieldReader&, %s&);\n' % self.C (type_info)
    s += self.generate_shortalias (type_info)   # typedef alias
    return s
  def generate_shortalias (self, type_info):
    classC = self.C (type_info) # class names
    if not self.gen_shortalias: return ''
    s  = '/** @interface %s\n' % type_info.name # doxygen
    s += ' * See also the corresponding C++ class %s. */\n' % classC
    if type_info.storage == Decls.INTERFACE and self.gen_mode == G4SERVER:
      s += '// '
    s += 'typedef %s %s;' % (self.C (type_info), self.type2cpp (type_info))
    s += ' ///< Convenience alias for the IDL type %s.\n' % type_info.name
    if self.gen_sharedimpls:
      # shared client & server impl
      s += 'typedef %s %s; ///< Convenience alias for the IDL type %s.\n' % \
          (self.C (type_info), self.C4server (type_info), type_info.name)
    return s
  def generate_method_decl (self, functype, pad):
    s = '  '
    if self.gen_mode == G4SERVER:
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
    if self.gen_mode == G4SERVER and functype.pure:
      s += ' = 0'
    s += ';\n'
    return s
  def generate_client_class_context (self, class_info):
    s, classH, classC = '\n', self.H (class_info.name), class_info.name + '_Context$' # class names
    s += '// === %s ===\n' % class_info.name
    s += 'static inline void ref   (%s&) {} // dummy stub for Signal<>.emit\n' % classH
    s += 'static inline void unref (%s&) {} // dummy stub for Signal<>.emit\n' % classH
    s += 'struct %s : public Plic::NonCopyable {\n' % classC    # context class
    s += '  struct SmartHandle$ : public %s {\n' % classH       # derive smart handle for copy-ctor initialization
    s += '    SmartHandle$ (Plic::uint64_t ipcid) : Plic::SmartHandle (ipcid) {}\n'
    s += '  } handle$;\n'
    for sg in class_info.signals:
      s += self.generate_client_class_context_event_handler_def (class_info, class_info, sg)
    s += '  %s (Plic::uint64_t ipcid) :\n' % classC             # ctor
    s += '    handle$ (ipcid)'
    for sg in class_info.signals:
      s += ',\n    %s (handle$)' % sg.name
    s += ',\n    m_cached_types (NULL)\n  {}\n'
    s += '  Plic::TypeHashList *m_cached_types;\n'
    s += '  const Plic::TypeHashList& list_types ();\n'
    s += '};\n'
    s += 'const Plic::TypeHashList&\n'
    s += '%s::list_types ()\n{\n' % classC
    s += '  if (!m_cached_types) {\n'
    s += '    Plic::FieldBuffer &fb = *Plic::FieldBuffer::_new (2 + 1);\n' # msgid self
    s += '    fb.add_msgid (%s);\n' % self.list_types_digest (class_info)
    s += '  ' + self.generate_proto_add_args ('fb', class_info, '', [('handle$', class_info)], '')
    s += '    Plic::FieldBuffer *fr = PLIC_CONNECTION().call_remote (&fb); // deletes fb\n'
    s += '    PLIC_CHECK (fr != NULL, "missing result from 2-way call");\n'
    s += '    Plic::FieldReader frr (*fr);\n'
    s += '    frr.skip_msgid(); // FIXME: msgid for return?\n' # FIXME: check errors
    s += '    size_t len;\n'
    s += '    frr >> len;\n'
    s += '    PLIC_CHECK (frr.remaining() == len * 2, "result truncated");\n'
    s += '    Plic::TypeHashList *thv = new Plic::TypeHashList();\n'
    s += '    Plic::TypeHash thash;\n'
    s += '    for (size_t i = 0; i < len; i++) {\n'
    s += '      frr >> thash;\n'
    s += '      thv->push_back (thash);\n'
    s += '    }\n'
    s += '    delete fr;\n'
    s += '    if (!Plic::atomic_ptr_cas (&m_cached_types, (Plic::TypeHashList*) NULL, thv))\n'
    s += '      delete thv;\n'
    s += '  }\n'
    s += '  return *m_cached_types;\n'
    s += '}\n'
    for sg in class_info.signals:
      s += self.generate_client_class_context_event_handler_methods (class_info, class_info, sg)
    return s
  def generate_client_class_context_event_handler_def (self, derived_info, class_info, sg):
    s, classH, signame = '', self.H (class_info.name), self.generate_signal_typename (sg, class_info)
    sigE = self.generate_event_handler_typename (sg, class_info)
    # s += '  %s %s;\n' % (signame, sg.name)                                  # signal member
    s += '  struct %s : Plic::Connection::EventHandler {\n' % sigE            # signal EventHandler
    relay = 'Rapicorn::Signals::SignalConnectionRelay<%s> ' % sigE
    s += '    ' + self.generate_signal_proxy_typedef (sg, class_info, 'Proxy')
    s += '    ' + self.generate_signal_typedef (sg, class_info, 'Relay', relay) # signal typedefs
    s += '    Plic::uint64_t m_handler_id, m_connection_id;\n'
    s += '    Relay%s rsignal;\n' % signame     # relay signal
    s += '    Proxy%s psignal;\n' % signame     # proxy signal
    s += '    %s (%s &handle) :\n' % (sigE, classH)
    s += '      m_handler_id (0), m_connection_id (0), rsignal (handle), psignal (rsignal)\n'
    s += '    { rsignal.listener (*this, &%s::connections_changed); }\n' % sigE
    s += '    ~%s ()\n' % sigE
    s += '    { if (m_handler_id) PLIC_CONNECTION().delete_event_handler (m_handler_id), m_handler_id = 0; }\n'
    s += '    void                       connections_changed (bool hasconnections);\n'
    s += '    virtual Plic::FieldBuffer* handle_event        (Plic::FieldBuffer &fb);\n'
    s += '  } %s;\n' % sg.name
    return s
  def generate_client_class_context_event_handler_methods (self, derived_info, class_info, sg):
    s, classC, signame = '', derived_info.name + '_Context$', self.generate_signal_typename (sg, class_info)
    sigE = self.generate_event_handler_typename (sg, class_info)
    s += 'void\n'
    s += '%s::%s::connections_changed (bool hasconnections)\n' % (classC, sigE)
    s += '{\n'
    s += '  Plic::FieldBuffer &fb = *Plic::FieldBuffer::_new (2 + 1 + 2);\n' # msgid self ConId ClosureId
    s += '  fb.add_msgid (%s);\n' % self.method_digest (sg)
    s += self.generate_proto_add_args ('fb', class_info, '', [('*rsignal.emitter()', class_info)], '')
    s += '  if (hasconnections) {\n'
    s += '    if (!m_handler_id)              // signal connected\n'
    s += '      m_handler_id = PLIC_CONNECTION().register_event_handler (this);\n' # FIXME: badly broken, memory alloc!
    s += '    fb << m_handler_id;    // handler connection request\n'
    s += '    fb << 0;               // no disconnection\n'
    s += '  } else {                          // signal disconnected\n'
    s += '    if (m_handler_id)\n'
    s += '      ; // FIXME: deletion! PLIC_CONNECTION().delete_event_handler (m_handler_id), m_handler_id = 0;\n'
    s += '    fb << 0;               // no handler connection\n'
    s += '    fb << m_connection_id; // disconnection request\n'
    s += '    m_connection_id = 0;\n'
    s += '  }\n'
    s += '  Plic::FieldBuffer *fr = PLIC_CONNECTION().call_remote (&fb); // deletes fb\n'
    s += '  if (fr) { // FIXME: assert that fr is a non-NULL FieldBuffer with result message\n'
    s += '    Plic::FieldReader frr (*fr);\n'
    s += '    frr.skip_msgid(); // FIXME: msgid for return?\n' # FIXME: check errors
    s += '    if (frr.remaining() && m_handler_id)\n'
    s += '      frr >> m_connection_id;\n'
    s += '    delete fr;\n'
    s += '  }\n'
    s += '}\n'
    s += 'Plic::FieldBuffer*\n'
    s += '%s::%s::handle_event (Plic::FieldBuffer &fb)\n' % (classC, sigE)
    s += '{\n'
    s += '  Plic::FieldReader fbr (fb);\n'
    s += '  fbr.skip_msgid(); // FIXME: check msgid\n'
    s += '  fbr.skip();       // skip m_handler_id\n'
    s += '  if (fbr.remaining() != %u) return plic$_error ("invalid number of arguments");\n' % len (sg.args)
    for a in sg.args:                                 # fetch args
      s += '  ' + self.V ('arg_' + a[0], a[1]) + ';\n'
      s += self.generate_proto_pop_args ('fbr', class_info, 'arg_', [(a[0], a[1])])
    s += '  '                                         # return var
    hasret = sg.rtype.storage != Decls.VOID
    if hasret:
      s += self.V ('', sg.rtype) + 'rval = '
    s += 'rsignal.emit ('                             # call out
    s += ', '.join (self.U ('arg_' + a[0], a[1]) for a in sg.args)
    s += ');\n'
    if hasret:                                        # store return value
      s += '  Plic::FieldBuffer &rb = *Plic::FieldBuffer::new_result();\n'
      s += self.generate_proto_add_args ('rb', class_info, '', [('rval', sg.rtype)], '')
      s += '  return &rb;\n'
    else:
      s += '  return NULL;\n'
    s += '}\n'
    return s;
  def generate_client_class_methods (self, class_info):
    s, classH, classC = '', self.H (class_info.name), class_info.name + '_Context$' # class names
    classH2 = (classH, classH)
    l, heritage, cl = self.interface_class_inheritance (class_info)
    s += '%s::%s ()' % classH2 # ctor
    s += '\n{}\n'
    s += 'Plic::FieldBuffer&\n'
    s += 'operator<< (Plic::FieldBuffer &fb, const %s &handle)\n{\n' % classH
    s += '  fb.add_object (connection_handle2id (handle));\n'
    s += '  return fb;\n'
    s += '}\n'
    s += 'Plic::FieldReader&\n'
    s += 'operator>> (Plic::FieldReader &fbr, %s &handle)\n{\n' % classH
    s += '  const Plic::uint64_t ipcid = fbr.pop_object();\n'
    s += '  handle = PLIC_ISLIKELY (ipcid) ? connection_id2context<%s> (ipcid)->handle$ : %s();\n' % (classC, classH)
    s += '  return fbr;\n'
    s += '}\n'
    s += 'const Plic::TypeHash&\n'
    s += '%s::_type()\n{\n' % classH
    s += '  static const Plic::TypeHash type_hash = Plic::TypeHash (%s);\n' % self.class_digest (class_info)
    s += '  return type_hash;\n'
    s += '}\n'
    s += '%s\n%s::_cast (Plic::SmartHandle &other, const Plic::TypeHashList &types)\n{\n' % classH2 # similar to ctor
    s += '  size_t i; const Plic::TypeHash &mine = _type();\n'
    s += '  for (i = 0; i < types.size(); i++)\n'
    s += '    if (mine == types[i])\n'
    s += '      return connection_id2context<%s> (connection_handle2id (other))->handle$;\n' % classC
    s += '  return %s();\n' % classH
    s += '}\n'
    s += 'const Plic::TypeHashList&\n'
    s += '%s::%s()\n{\n' % (classH, identifiers['cast_types'])
    s += '  static Plic::TypeHashList notypes;\n'
    s += '  const Plic::uint64_t ipcid = connection_handle2id (*this);\n'
    s += '  if (PLIC_UNLIKELY (!ipcid)) return notypes; // null handle\n'
    s += '  return connection_id2context<%s> (ipcid)->list_types();\n' % classC
    s += '}\n'
    for sg in class_info.signals:
      signame = self.generate_signal_typename (sg, class_info)
      s += '%s::%s&\n' % (classH, signame)
      s += '%s::sig_%s ()\n{\n' % (classH, sg.name)
      s += '  return connection_id2context<%s> (connection_handle2id (*this))->%s.psignal;\n' % (classC, sg.name)
      s += '}\n'
    return s
  def generate_server_class_methods (self, class_info):
    assert self.gen_mode == G4SERVER
    s, classC = '\n', self.C (class_info) # class names
    s += '%s::%s ()' % (classC, classC) # ctor
    l = [] # constructor agument list
    for sg in class_info.signals:
      l += ['sig_%s (*this)' % sg.name]
    if l:
      s += ' :\n  ' + ', '.join (l)
    s += '\n{}\n'
    s += '%s::~%s () {}\n' % (classC, classC) # dtor
    s += 'Plic::FieldBuffer&\n'
    s += 'operator<< (Plic::FieldBuffer &fb, %s &obj)\n{\n' % classC
    s += '  fb.add_object (connection_object2id (&obj));\n'
    s += '  return fb;\n'
    s += '}\n'
    s += 'Plic::FieldBuffer&\n'
    s += 'operator<< (Plic::FieldBuffer &fb, %s *obj)\n{\n' % classC
    s += '  fb.add_object (connection_object2id (obj));\n'
    s += '  return fb;\n'
    s += '}\n'
    s += 'Plic::FieldReader&\n'
    s += 'operator>> (Plic::FieldReader &fbr, %s* &obj)\n{\n' % classC
    s += '  obj = connection_id2object<%s> (fbr.pop_object());\n' % classC
    s += '  return fbr;\n'
    s += '}\n'
    s += 'void\n'
    s += '%s::_list_types (Plic::TypeHashList &thl) const\n{\n' % classC
    ancestors = self.class_ancestry (class_info)
    ancestors.reverse()
    for an in ancestors:
      s += '  thl.push_back (Plic::TypeHash (%s)); // %s\n' % (self.class_digest (an), an.name)
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
    s += '  Plic::FieldBuffer &fb = *Plic::FieldBuffer::_new (2 + 1 + %u), *fr = NULL;\n' % len (mtype.args) # msgid self args
    s += '  fb.add_msgid (%s); // msgid\n' % self.method_digest (mtype)
    # marshal args
    s += self.generate_proto_add_args ('fb', class_info, '', [('(*this)', class_info)], '')
    ident_type_args = [('arg_' + a[0], a[1]) for a in mtype.args]
    s += self.generate_proto_add_args ('fb', class_info, '', ident_type_args, '')
    # call out
    s += '  fr = PLIC_CONNECTION().call_remote (&fb); // deletes fb\n'
    # unmarshal return
    if hasret:
      rarg = ('retval', mtype.rtype)
      s += '  Plic::FieldReader frr (*fr);\n'
      s += '  frr.skip_msgid(); // FIXME: check msgid\n'
      # FIXME: check return error and return type
      s += '  ' + self.V (rarg[0], rarg[1]) + ';\n'
      s += self.generate_proto_pop_args ('frr', class_info, '', [rarg], '')
      s += '  delete fr;\n'
      s += '  return retval;\n'
    s += '}\n'
    return s
  def generate_server_method_stub (self, class_info, mtype, reglines):
    assert self.gen_mode == G4SERVER
    s = ''
    dispatcher_name = '_$caller__%s__%s' % (class_info.name, mtype.name)
    reglines += [ (self.method_digest (mtype), self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Plic::FieldBuffer*\n'
    s += dispatcher_name + ' (Plic::FieldReader &fbr)\n'
    s += '{\n'
    s += '  if (fbr.remaining() != 1 + %u) return plic$_error ("invalid number of arguments");\n' % len (mtype.args)
    # fetch self
    s += '  %s *self;\n' % self.C (class_info)
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  PLIC_CHECK (self, "self must be non-NULL");\n'
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
      s += '  Plic::FieldBuffer &rb = *Plic::FieldBuffer::new_result();\n'
      rval = 'rval'
      s += self.generate_proto_add_args ('rb', class_info, '', [(rval, mtype.rtype)], '')
      s += '  return &rb;\n'
    else:
      s += '  return NULL;\n'
    # done
    s += '}\n'
    return s
  def generate_property (self, fident, ftype, pad = 0):
    s, v, v0, ptr = '', '', '', ''
    if self.gen_mode == G4SERVER:
      v, v0, ptr = 'virtual ', ' = 0', '*'
    tname = self.C (ftype)
    pid = fident + ' ' * max (0, pad - len (fident))
    if ftype.storage in (Decls.INT, Decls.FLOAT, Decls.ENUM):
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
    s += '  Plic::FieldBuffer &fb = *Plic::FieldBuffer::_new (2 + 1), *fr = NULL;\n' # msgid self
    s += '  fb.add_msgid (%s);\n' % self.getter_digest (class_info, fident, ftype)
    s += self.generate_proto_add_args ('fb', class_info, '', [('(*this)', class_info)], '')
    s += '  fr = PLIC_CONNECTION().call_remote (&fb); // deletes fb\n'
    if 1: # hasret
      rarg = ('retval', ftype)
      s += '  Plic::FieldReader frr (*fr);\n'
      s += '  frr.skip_msgid(); // FIXME: check msgid\n'
      # FIXME: check return error and return type
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
    s += '  Plic::FieldBuffer &fb = *Plic::FieldBuffer::_new (2 + 1 + 1), *fr = NULL;\n' # msgid self value
    s += '  fb.add_msgid (%s); // msgid\n' % self.setter_digest (class_info, fident, ftype)
    s += self.generate_proto_add_args ('fb', class_info, '', [('(*this)', class_info)], '')
    ident_type_args = [('value', ftype)]
    s += self.generate_proto_add_args ('fb', class_info, '', ident_type_args, '')
    s += '  fr = PLIC_CONNECTION().call_remote (&fb); // deletes fb\n'
    s += '  if (fr) delete fr;\n' # FIXME: check return error
    s += '}\n'
    return s
  def generate_server_property_setter (self, class_info, fident, ftype, reglines):
    assert self.gen_mode == G4SERVER
    s = ''
    dispatcher_name = '_$setter__%s__%s' % (class_info.name, fident)
    setter_hash = self.setter_digest (class_info, fident, ftype)
    reglines += [ (setter_hash, self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Plic::FieldBuffer*\n'
    s += dispatcher_name + ' (Plic::FieldReader &fbr)\n'
    s += '{\n'
    s += '  if (fbr.remaining() != 1 + 1) return plic$_error ("invalid number of arguments");\n'
    # fetch self
    s += '  %s *self;\n' % self.C (class_info)
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  PLIC_CHECK (self, "self must be non-NULL");\n'
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
    assert self.gen_mode == G4SERVER
    s = ''
    dispatcher_name = '_$getter__%s__%s' % (class_info.name, fident)
    getter_hash = self.getter_digest (class_info, fident, ftype)
    reglines += [ (getter_hash, self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Plic::FieldBuffer*\n'
    s += dispatcher_name + ' (Plic::FieldReader &fbr)\n'
    s += '{\n'
    s += '  if (fbr.remaining() != 1) return plic$_error ("invalid number of arguments");\n'
    # fetch self
    s += '  %s *self;\n' % self.C (class_info)
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  PLIC_CHECK (self, "self must be non-NULL");\n'
    # return var
    s += '  '
    s += self.V ('', ftype) + 'rval = '
    # call out
    s += 'self->' + fident + ' ();\n'
    # store return value
    s += '  Plic::FieldBuffer &rb = *Plic::FieldBuffer::new_result();\n'
    rval = 'rval'
    s += self.generate_proto_add_args ('rb', class_info, '', [(rval, ftype)], '')
    s += '  return &rb;\n'
    s += '}\n'
    return s
  def generate_server_list_types (self, class_info, reglines):
    assert self.gen_mode == G4SERVER
    s = ''
    dispatcher_name = '_$lsttyp__%s__' % class_info.name
    reglines += [ (self.list_types_digest (class_info), self.namespaced_identifier (dispatcher_name)) ]
    s += 'static Plic::FieldBuffer*\n'
    s += dispatcher_name + ' (Plic::FieldReader &fbr)\n'
    s += '{\n'
    s += '  if (fbr.remaining() != 1) return plic$_error ("invalid number of arguments");\n'
    s += '  %s *self;\n' % self.C (class_info)  # fetch self
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  PLIC_CHECK (self, "self must be non-NULL");\n'
    s += '  Plic::TypeHashList thl;\n'
    s += '  self->_list_types (thl);\n'
    s += '  Plic::FieldBuffer &rb = *Plic::FieldBuffer::new_result (1 + 2 * thl.size());\n' # store return value
    s += '  rb << Plic::int64_t (thl.size());\n'
    s += '  for (size_t i = 0; i < thl.size(); i++)\n'
    s += '    rb << thl[i];\n'
    s += '  return &rb;\n'
    s += '}\n'
    return s
  def generate_signal_proxy_typename (self, functype, ctype):
    return 'Signal_%s' % functype.name # 'Proxy_%s'
  def generate_signal_typename (self, functype, ctype):
    return 'Signal_%s' % functype.name
  def generate_event_handler_typename (self, functype, ctype):
    return '__EventHandler__%s' % functype.name
  def generate_signal_proxy_typedef (self, functype, ctype, prefix = ''):
    assert self.gen_mode == G4CLIENT
    proxyname = self.generate_signal_proxy_typename (functype, ctype)
    cpp_rtype = self.R (functype.rtype)
    s = 'typedef Rapicorn::Signals::SignalProxy<%s, %s (' % (self.C (ctype), cpp_rtype)
    l = []
    for a in functype.args:
      l += [ self.A (a[0], a[1]) ]
    s += ', '.join (l)
    s += ')'
    s += '> ' + prefix + proxyname + ';\n'
    return s
  def generate_signal_accessor_def (self, functype, ctype):
    assert self.gen_mode == G4CLIENT
    s, signame = '', self.generate_signal_typename (functype, ctype)
    s = '  ' + self.F (signame + '&') + 'sig_' + functype.name + '();\n'
    return s
  def generate_signal_typedef (self, functype, ctype, prefix = '', ancestor = ''):
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
    assert self.gen_mode == G4SERVER
    s = ''
    dispatcher_name = '_$sigcon__%s__%s' % (class_info.name, stype.name)
    reglines += [ (self.method_digest (stype), self.namespaced_identifier (dispatcher_name)) ]
    closure_class = '_$Closure__%s__%s' % (class_info.name, stype.name)
    s += 'class %s {\n' % closure_class
    s += '  Plic::Connection &m_connection; Plic::uint64_t m_handler;\n'
    s += 'public:\n'
    s += '  typedef Plic::shared_ptr<%s> SharedPtr;\n' % closure_class
    s += '  %s (Plic::Connection &conn, Plic::uint64_t h) : m_connection (conn), m_handler (h) {}\n' % closure_class # ctor
    s += '  ~%s()\n' % closure_class # dtor
    s += '  {\n'
    s += '    Plic::FieldBuffer &fb = *Plic::FieldBuffer::_new (2 + 1);\n' # msgid handler
    s += '    fb.add_msgid (Plic::MSGID_DISCON, 0); // FIXME: 0\n' # self.method_digest (stype)
    s += '    fb << m_handler;\n'
    s += '    m_connection.send_event (&fb); // deletes fb\n'
    s += '  }\n'
    cpp_rtype = self.R (stype.rtype)
    s += '  static %s\n' % cpp_rtype
    s += '  handler ('
    s += self.Args (stype, 'arg_', 11) + (',\n           ' if stype.args else '')
    s += 'SharedPtr sp)\n  {\n'
    s += '    Plic::FieldBuffer &fb = *Plic::FieldBuffer::_new (2 + 1 + %u);\n' % len (stype.args) # msgid handler args
    s += '    fb.add_msgid (Plic::MSGID_EVENT, 0); // FIXME: 0\n' # self.method_digest (stype)
    s += '    fb << sp->m_handler;\n'
    ident_type_args = [('arg_' + a[0], a[1]) for a in stype.args] # marshaller args
    args2fb = self.generate_proto_add_args ('fb', class_info, '', ident_type_args, '')
    if args2fb:
      s += reindent ('  ', args2fb) + '\n'
    s += '    sp->m_connection.send_event (&fb); // deletes fb\n'
    if stype.rtype.storage != Decls.VOID:
      s += '    return %s;\n' % self.mkzero (stype.rtype)
    s += '  }\n'
    s += '};\n'
    s += 'static Plic::FieldBuffer*\n'
    s += dispatcher_name + ' (Plic::FieldReader &fbr)\n'
    s += '{\n'
    s += '  if (fbr.remaining() != 1 + 2) return plic$_error ("invalid number of arguments");\n'
    s += '  %s *self;\n' % self.C (class_info)
    s += self.generate_proto_pop_args ('fbr', class_info, '', [('self', class_info)])
    s += '  PLIC_CHECK (self, "self must be non-NULL");\n'
    s += '  Plic::uint64_t handler_id, con_id, cid = 0;\n'
    s += '  fbr >> handler_id;\n'
    s += '  fbr >> con_id;\n'
    s += '  if (con_id) self->sig_%s.disconnect (con_id);\n' % stype.name
    s += '  if (handler_id) {\n'
    s += '    %s::SharedPtr sp (new %s (PLIC_CONNECTION(), handler_id));\n' % (closure_class, closure_class)
    s += '    cid = self->sig_%s.connect (slot (sp->handler, sp)); }\n' % stype.name
    s += '  Plic::FieldBuffer &rb = *Plic::FieldBuffer::new_result();\n'
    s += '  rb << cid;\n'
    s += '  return &rb;\n'
    s += '}\n'
    return s
  def generate_server_method_registry (self, reglines):
    s = ''
    s += 'static const Plic::Connection::MethodEntry _plic_stub_entries[] = {\n'
    for dispatcher in reglines:
      cdigest, dispatcher_name = dispatcher
      s += '  { ' + cdigest + ', '
      s += dispatcher_name + ', },\n'
    s += '};\n'
    s += 'static Plic::Connection::MethodRegistry _plic_stub_registry (_plic_stub_entries);\n'
    return s
  def generate_virtual_method_skel (self, functype, type_info):
    assert self.gen_mode == G4SERVER
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
    l = []
    s += 'enum %s {\n' % type_info.name
    for opt in type_info.options:
      (ident, label, blurb, number) = opt
      s += '  %s = %s,' % (ident, number)
      if blurb:
        s += ' // %s' % re.sub ('\n', ' ', blurb)
      s += '\n'
    s += '};\n'
    s += 'inline Plic::FieldBuffer& operator<< (Plic::FieldBuffer &fb,  %s &e) ' % type_info.name
    s += '{ fb << Plic::EnumValue (e); return fb; }\n'
    s += 'inline Plic::FieldReader& operator>> (Plic::FieldReader &frr, %s &e) ' % type_info.name
    s += '{ e = %s (frr.pop_evalue()); return frr; }\n' % type_info.name
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
           re.match ('(filtered_class_hh:\w+):\s*(//.*)?$', line) or
           re.match ('(filtered_class_cc:\w+):\s*(//.*)?$', line) or
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
    self.gen_mode = G4SERVER if self.gen_serverhh or self.gen_servercc else G4CLIENT
    s = '/* --- Generated by PLIC-CxxStub --- */\n'
    # CPP guard
    if self.cppguard:
      s += '#ifndef %s\n#define %s\n\n' % (self.cppguard, self.cppguard)
    # inclusions
    if self.gen_inclusions:
      s += '\n// --- Custom Includes ---\n'
    if self.gen_inclusions and (self.gen_clientcc or self.gen_servercc):
      s += '#ifndef __PLIC_UTILITIES_HH__\n'
    for i in self.gen_inclusions:
      s += '#include %s\n' % i
    if self.gen_inclusions and (self.gen_clientcc or self.gen_servercc):
      s += '#endif\n'
    s += self.insertion_text ('includes')
    if self.gen_clienthh:
      s += clienthh_boilerplate
      if self.gen_rapicornsignals:
        s += rapicornsignal_boilerplate
    if self.gen_serverhh:
      s += serverhh_boilerplate
      if self.gen_rapicornsignals:
        s += rapicornsignal_boilerplate
    if self.gen_clientcc:
      s += gencc_boilerplate + '\n' + clientcc_boilerplate + '\n'
    if self.gen_servercc:
      s += gencc_boilerplate + '\n' + servercc_boilerplate + '\n'
    self.tab_stop (30)
    s += self.open_namespace (None)
    # collect impl types
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
    # generate client/server decls
    if self.gen_clienthh or self.gen_serverhh:
      self.gen_mode = G4SERVER if self.gen_serverhh else G4CLIENT
      s += '\n// --- Interfaces (class declarations) ---\n'
      for tp in types:
        if tp.is_forward:
          s += self.open_namespace (tp) + '\n'
          if self.gen_serverhh:
            s += 'class %s;\n' % self.C (tp)    # G4SERVER
            self.gen_mode = G4SERVER
          elif self.gen_clienthh:
            s += 'class %s;\n' % self.C (tp)    # G4CLIENT
        elif tp.typedef_origin:
          s += self.open_namespace (tp) + '\n'
          s += 'typedef %s %s;\n' % (self.C (tp.typedef_origin), tp.name)
        elif tp.storage in (Decls.RECORD, Decls.SEQUENCE):
          if self.gen_serverhh:
            s += self.open_namespace (tp)
            s += self.generate_recseq_decl (tp)
          if self.gen_clienthh:
            s += self.open_namespace (tp)
            s += self.generate_recseq_decl (tp)
        elif tp.storage == Decls.ENUM:
          s += self.open_namespace (tp)
          s += self.generate_enum_decl (tp)
        elif tp.storage == Decls.INTERFACE:
          if self.gen_clienthh and not self.gen_serverhh:
            s += self.open_namespace (tp)
            s += self.generate_interface_class (tp)     # Class smart handle
          if self.gen_serverhh:
            if tp.name in self.skip_classes:
              s += self.open_namespace (None) # close all namespaces
              s += '\n'
              s += self.insertion_text ('filtered_class_hh:' + tp.name)
            else:
              s += self.open_namespace (tp)
              s += self.generate_interface_class (tp)   # Class_Interface server base
      s += self.open_namespace (None)
    # generate client/server impls
    if self.gen_clientcc or self.gen_servercc:
      self.gen_mode = G4SERVER if self.gen_servercc else G4CLIENT
      s += '\n// --- Implementations ---\n'
      for tp in types:
        if tp.typedef_origin or tp.is_forward:
          continue
        if tp.storage == Decls.RECORD:
          s += self.open_namespace (tp)
          s += self.generate_record_impl (tp)
        elif tp.storage == Decls.SEQUENCE:
          s += self.open_namespace (tp)
          s += self.generate_sequence_impl (tp)
        elif tp.storage == Decls.INTERFACE:
          if self.gen_servercc:
            if tp.name in self.skip_classes:
              s += self.open_namespace (None) # close all namespaces
              s += self.insertion_text ('filtered_class_cc:' + tp.name)
            else:
              s += self.open_namespace (tp)
              s += self.generate_server_class_methods (tp)
          if self.gen_clientcc:
            s += self.open_namespace (tp)
            s += self.generate_client_class_context (tp)
            s += self.generate_client_class_methods (tp)
            for fl in tp.fields:
              s += self.generate_client_property_stub (tp, fl[0], fl[1])
            for m in tp.methods:
              s += self.generate_client_method_stub (tp, m)
    # generate unmarshalling server calls
    if self.gen_servercc:
      self.gen_mode = G4SERVER
      s += '\n// --- Method Dispatchers & Registry ---\n'
      reglines = []
      for tp in types:
        if tp.typedef_origin or tp.is_forward:
          continue
        s += self.open_namespace (tp)
        if tp.storage == Decls.INTERFACE and not tp.name in self.skip_classes:
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
      self.gen_mode = G4SERVER
      s += '\n// --- Interface Skeletons ---\n'
      for tp in types:
        if tp.typedef_origin or tp.is_forward:
          continue
        elif tp.storage == Decls.INTERFACE and not tp.name in self.skip_classes:
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
  gg.gen_shortalias = all or 'shortalias' in config['backend-options']
  gg.gen_sharedimpls = all or 'sharedimpls' in config['backend-options']
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
      gg._iface_base = opt[11:]
    if opt.startswith ('filter-out='):
      gg.skip_classes += opt[11:].split (',')
  for ifile in config['insertions']:
    gg.insertion_file (ifile)
  for ssfile in config['skip-skels']:
    gg.symbol_file (ssfile)
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
