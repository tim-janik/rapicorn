class __Enum__ (long):
  def __new__ (_class, value, ident, doc = None):
    if _class.enum_values.has_key (ident):
      raise KeyError (ident)
    instance = long.__new__ (_class, value)
    instance.__name__ = ident
    if doc:
      instance.__doc__ = doc
    _class.enum_values[ident] = instance
    vlist = _class._valuedict.get (value, [])
    if not vlist:
      _class._valuedict[value] = vlist
    vlist += [ instance ]
    return instance
  def __repr__ (self):
    return '<enum %s.%s value %s>' % (self.__class__.__name__, self.__name__, repr (long (self)))
  def __str__ (self):
    return self.__name__
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
