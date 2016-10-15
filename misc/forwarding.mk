# forward make $(MAKECMDGOALS) to the project root

forwarding_mk_topdir ::= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)

MAKECMDGOALS ?= all
$(sort ${MAKECMDGOALS}):
	@$(MAKE) -C $(forwarding_mk_topdir) $@
