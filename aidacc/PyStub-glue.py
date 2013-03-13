class _BaseRecord_:
  def __init__ (self, **entries):
    self.__dict__.update (entries)
class _BaseClass_ (object):
  class _AidaID_:
    def __init__ (self, _aidaid):
      assert isinstance (_aidaid, (int, long))
      self.id = _aidaid
  def __init__ (self, _aida_id):
    assert isinstance (_aida_id, _BaseClass_._AidaID_)
    self.__dict__['__aida_pyobject__'] = _aida_id.id
