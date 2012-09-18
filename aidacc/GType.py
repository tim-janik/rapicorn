#!/usr/bin/env python
# Aida - Abstract Interface Definition Architecture
# Licensed GNU GPL v3 or later: http://www.gnu.org/licenses/gpl.html
"""PLIC-GType - GType string generator for PLIC

More details at http://www.testbit.eu/.
"""
import sys


def generate (namespace_list, **args):
  for ns in namespace_list:
    print "NAMESPACE:", ns.name

# control module exports
__all__ = ['generate']
