def __AIDA__open_namespace__ (name, _file):
  import sys, imp
  if sys.modules.has_key (name):
    return sys.modules[name]
  m = imp.new_module (name)
  m.__file__ = _file
  m.__doc__ = "AIDA IDL module %s (%s)" % (name, _file)
  sys.modules[m.__name__] = m
  return m

class __AIDA_Enum__ (long):
  def __new__ (_class, value, ident, doc = None):
    if ident and _class.enum_values.has_key (ident):
      raise KeyError (ident)
    instance = long.__new__ (_class, value)
    if ident: # track named enums (non-anonymous)
      instance.name = ident
      _class.enum_values[ident] = instance
      vlist = _class._valuedict.get (value, [])
      if not vlist:
        _class._valuedict[value] = vlist
      vlist += [ instance ]
    if doc:
      instance.__doc__ = doc
    return instance
  def __repr__ (self):
    if hasattr (self, 'name'):
      return '<enum %s.%s value %s>' % (self.__class__.__name__, self.name, repr (long (self)))
    else:
      return '<enum %s.%u>' % (self.__class__.__name__, self)
  def __str__ (self):
    return self.name if hasattr (self, 'name') else str (long (self))
  @classmethod
  def _enum_lookup (_class, value):
    try:
      return _class.index (value)
    except:
      return _class (value, None) # anonymous enum
  @classmethod
  def index (_class, value):
    return _class._valuedict[value][0]
  @classmethod
  def get (_class, key, default = None):
    return _class.enum_values.get (key, default)

class _BaseRecord_:
  def __init__ (self, **entries):
    self.__dict__.update (entries)

class _BaseClass_ (_CPY.PyRemoteHandle):
  class __Signal__:
    def __init__ (self, object, pyconnect):
      self.__object = object
      self.__pyconnect = pyconnect
    def connect (self, _callable):
      return self.__pyconnect (self.__object, _callable, 0)
    def disconnect (self, connection_id):
      return self.__pyconnect (self.__object, None, connection_id)
    def __iadd__ (self, _callable):
      return self.connect (_callable)
    def __isub__ (self, connection_id):
      return self.disconnect (connection_id)
  def __init__ (self):
    pass
  def __getattr__ (self, name):
    try:
      getter = getattr (self.__class__, '__pygetter__%s__' % name)
    except AttributeError:
      try:
        getter = getattr (self.__class__, '__pysignal__%s__' % name)
      except AttributeError:
        raise AttributeError ("class %s has no attribute '%s'" % (self.__class__.__name__, name))
    return getter (self)
  def __setattr__ (self, name, value):
    try:
      setter = getattr (self.__class__, '__pysetter__%s__' % name)
    except AttributeError:
      try:
        getattr (self.__class__, '__pysignal__%s__' % name)
        return None # ignore setting of signals to allow __iadd__ and __isub__
      except AttributeError:
        raise AttributeError ("class %s has no attribute '%s'" % (self.__class__.__name__, name))
    return setter (self, value)

def __AIDA_pyfactory__create_pyobject__ (type_name, longid):
  def resolve (obj, path):
    parts = path.split ('.')
    parts.reverse()
    if parts:
      element = parts.pop()
      obj = obj[element]                # obj is a dict initially
    while parts:
      element = parts.pop()
      obj = getattr (obj, element)      # nested obj are module/object instances
    return obj
  try:
    klass = resolve (globals(), type_name)
  except:
    raise NameError ("undefined identifier '%s'" % type_name)
  if hasattr (klass, '_enum_lookup'):
    return klass._enum_lookup (longid)
  if not klass:
    raise NameError ("undefined identifier '%s'" % type_name)
  return klass ()
_CPY.__AIDA_pyfactory__register_callback (__AIDA_pyfactory__create_pyobject__)
_CPY.__AIDA_BaseRecord__ = _BaseRecord_
