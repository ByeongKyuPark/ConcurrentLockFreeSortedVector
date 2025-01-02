# 1. detect the os
UNAME_S := $(shell uname -s)

# 2. include the appropriate Makefile
ifeq ($(UNAME_S),Darwin) # system name for macOS
include Makefile.mac
else
include Makefile.win
endif
