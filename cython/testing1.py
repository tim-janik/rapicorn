# Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
import Rapicorn
# python -ic "import sys, os; sys.path.insert (0, os.path.abspath ('.libs')) ; import Rapicorn"

# test MainLoop and EventLoop object identities
ml = Rapicorn.MainLoop()
sl = ml.create_slave()
sm = sl.main_loop()
m3 = Rapicorn.MainLoop()
assert ml == ml and sl == sl and sm == sm and m3 == m3
assert ml != sl and sl != ml and sm != sl and sl != sm
assert ml == sm and sm == ml and ml != m3 and m3 != ml
assert ml.main_loop() == ml  and m3.main_loop() == m3

# all done
print '  %-6s' % 'CHECK', '%-67s' % __file__, 'OK'
