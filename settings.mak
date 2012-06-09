
# Name of the lua-library-package (lua on gentoo and suse, lua5.1 on debian)
LUA_PKG = $(shell pkg-config --list-all | grep lua | head -n 1 | cut -d" " -f 1)

# General compilation-related defines
CC = gcc
CFLAGS = -Wall -g3 -O2 -march=native -mtune=native -pipe `pkg-config $(LUA_PKG) --cflags` -std=gnu99 -rdynamic
LDFLAGS = -lm -lrt `pkg-config $(LUA_PKG) --libs` -Wl,-O1  -lssl -lcrypto -Wl,--as-needed

# Debug builds get this define set, so the DEBUG() macro does something
dbg: CFLAGS += -DENABLE_DEBUG -fsignaling-nans

dbg: all
