SUBDIRS = logging net server
CC = gcc
LUA_PKG = $(shell pkg-config --list-all | grep lua | head -n 1 | cut -d" " -f 1)
CFLAGS = -Wall -g3 -O2 -march=native -mtune=native -pipe `pkg-config $(LUA_PKG) --cflags` -std=gnu99 -lm -lrt `pkg-config $(LUA_PKG) --libs` -Wl,-O1 -Wl,--as-needed

all: gameserver

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

net: logging
server: net

gameserver: $(SUBDIRS)
	$(CC) $(CFLAGS) -o gameserver ./logging/logging.o ./net/net.o ./net/network.o ./server/config.o ./server/entities.o ./server/entity_storage.o ./server/json_output.o ./server/luafuncs.o ./server/luastate.o ./server/main.o ./server/map.o ./server/physics.o ./server/route.o ./server/ship.o ./server/storages.o ./server/vector.o

clean:
	-rm gameserver
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
