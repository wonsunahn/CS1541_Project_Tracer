PIN_ROOT = /afs/pitt.edu/home/w/a/wahn/teaching/cs1541/pin-3.30
TARGET = ia32
DEBUG = 1

CONFIG_ROOT := $(PIN_ROOT)/source/tools/Config

include $(CONFIG_ROOT)/makefile.config
include makefile.rules
include $(TOOLS_ROOT)/Config/makefile.default.rules
