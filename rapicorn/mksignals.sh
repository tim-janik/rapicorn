#!/bin/bash

SOURCE="$1"
[ -z "$SOURCE" ] && { echo "usage: $0 <inputfile>"; exit 1; }

# Signal0
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\20/g;
	s/\(, \)\?typename A1, typename A2, typename A3//g;
	s/\(, \)\?A1, A2, A3//g;
	s/\(, \)\?a1, a2, a3//g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3//g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3//g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)//g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3//g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal1
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\21/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1/g;
	s/\(, \)\?A1, A2, A3/\1A1/g;
	s/\(, \)\?a1, a2, a3/\1a1/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal2
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\22/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1, emission.m_a2/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal3
cat <$SOURCE

# Signal4
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\24/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1, emission.m_a2, emission.m_a3, emission.m_a4/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal5
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\25/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1, emission.m_a2, emission.m_a3, emission.m_a4, emission.m_a5/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal6
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\26/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1, emission.m_a2, emission.m_a3, emission.m_a4, emission.m_a5, emission.m_a6/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal7
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\27/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1, emission.m_a2, emission.m_a3, emission.m_a4, emission.m_a5, emission.m_a6, emission.m_a7/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal8
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\28/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1, emission.m_a2, emission.m_a3, emission.m_a4, emission.m_a5, emission.m_a6, emission.m_a7, emission.m_a8/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal9
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\29/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1, emission.m_a2, emission.m_a3, emission.m_a4, emission.m_a5, emission.m_a6, emission.m_a7, emission.m_a8, emission.m_a9/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal10
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\210/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10;/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1, emission.m_a2, emission.m_a3, emission.m_a4, emission.m_a5, emission.m_a6, emission.m_a7, emission.m_a8, emission.m_a9, emission.m_a10/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal11
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\211/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10; A11 m_a11/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10), m_a11 (a11)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1, emission.m_a2, emission.m_a3, emission.m_a4, emission.m_a5, emission.m_a6, emission.m_a7, emission.m_a8, emission.m_a9, emission.m_a10, emission.m_a11/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal12
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\212/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10; A11 m_a11; A12 m_a12/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10), m_a11 (a11), m_a12 (a12)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1, emission.m_a2, emission.m_a3, emission.m_a4, emission.m_a5, emission.m_a6, emission.m_a7, emission.m_a8, emission.m_a9, emission.m_a10, emission.m_a11, emission.m_a12/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal13
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\213/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10; A11 m_a11; A12 m_a12; A13 m_a13/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10), m_a11 (a11), m_a12 (a12), m_a13 (a13)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1, emission.m_a2, emission.m_a3, emission.m_a4, emission.m_a5, emission.m_a6, emission.m_a7, emission.m_a8, emission.m_a9, emission.m_a10, emission.m_a11, emission.m_a12, emission.m_a13/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal14
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\214/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13, A14 a14/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10; A11 m_a11; A12 m_a12; A13 m_a13; A14 m_a14/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10), m_a11 (a11), m_a12 (a12), m_a13 (a13), m_a14 (a14)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1, emission.m_a2, emission.m_a3, emission.m_a4, emission.m_a5, emission.m_a6, emission.m_a7, emission.m_a8, emission.m_a9, emission.m_a10, emission.m_a11, emission.m_a12, emission.m_a13, emission.m_a14/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal15
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\215/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13, A14 a14, A15 a15/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10; A11 m_a11; A12 m_a12; A13 m_a13; A14 m_a14; A15 m_a15/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10), m_a11 (a11), m_a12 (a12), m_a13 (a13), m_a14 (a14), m_a15 (a15)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1, emission.m_a2, emission.m_a3, emission.m_a4, emission.m_a5, emission.m_a6, emission.m_a7, emission.m_a8, emission.m_a9, emission.m_a10, emission.m_a11, emission.m_a12, emission.m_a13, emission.m_a14, emission.m_a15/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# Signal16
cat >xgen-signals.sed <<EOT
	s/\b\(Handler\|Slot\|Signal\|Emission\|SignalEmittable\)\([A-Z]*\)3\b/\1\216/g;
	s/\(, \)\?typename A1, typename A2, typename A3/\1typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15, typename A16/g;
	s/\(, \)\?A1, A2, A3/\1A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16/g;
	s/\(, \)\?a1, a2, a3/\1a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16/g;
	s/\(, \)\?A1 a1, A2 a2, A3 a3/\1A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13, A14 a14, A15 a15, A16 a16/g;
	s/\(; \)\?A1 m_a1; A2 m_a2; A3 m_a3/\1A1 m_a1; A2 m_a2; A3 m_a3; A4 m_a4; A5 m_a5; A6 m_a6; A7 m_a7; A8 m_a8; A9 m_a9; A10 m_a10; A11 m_a11; A12 m_a12; A13 m_a13; A14 m_a14; A15 m_a15; A16 m_a16/g;
	s/\(, \)\?m_a1 (a1), m_a2 (a2), m_a3 (a3)/\1m_a1 (a1), m_a2 (a2), m_a3 (a3), m_a4 (a4), m_a5 (a5), m_a6 (a6), m_a7 (a7), m_a8 (a8), m_a9 (a9), m_a10 (a10), m_a11 (a11), m_a12 (a12), m_a13 (a13), m_a14 (a14), m_a15 (a15), m_a16 (a16)/g;
	s/\(, \)\?emission.m_a1, emission.m_a2, emission.m_a3/\1emission.m_a1, emission.m_a2, emission.m_a3, emission.m_a4, emission.m_a5, emission.m_a6, emission.m_a7, emission.m_a8, emission.m_a9, emission.m_a10, emission.m_a11, emission.m_a12, emission.m_a13, emission.m_a14, emission.m_a15, emission.m_a16/g;
EOT
sed -f xgen-signals.sed <$SOURCE

# cleanup
rm -f xgen-signals.sed
