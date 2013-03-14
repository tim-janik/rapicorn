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

class _BaseClass_ (object):
  class _AidaID_:
    def __init__ (self, _aidaid):
      assert isinstance (_aidaid, (int, long))
      self.id = _aidaid
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
  def __init__ (self, _aida_id):
    assert isinstance (_aida_id, _BaseClass_._AidaID_)
    self.__dict__['__aida_pyobject__'] = _aida_id.id
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
    klass = globals().get (type_name, None)
    if hasattr (klass, '_enum_lookup'):
      return klass._enum_lookup (longid)
    if not klass or not longid:
      return None
    return klass (_BaseClass_._AidaID_ (longid))
_CPY.__AIDA_pyfactory__register_callback (__AIDA_pyfactory__create_pyobject__)
