all: gameserver

include settings.mak

SUBDIRS = logging net server

.PHONY: clean debug subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

net: logging
server: net

gameserver: $(SUBDIRS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o gameserver ./logging/logging.o ./net/net.o ./net/network.o ./server/config.o ./server/entities.o ./server/entity_storage.o ./server/json_output.o ./server/luafuncs.o ./server/luastate.o ./server/main.o ./server/map.o ./server/physics.o ./server/route.o ./server/ship.o ./server/storages.o ./server/vector.o ./net/talk.o ./net/pstr.o ./net/userstuff.o ./net/dispatch.o ./server/player.o ./server/base.o

clean:
	-rm gameserver
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

debug:
	echo "Debug build"
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir dbg; \
	done
	$(CC) $(CFLAGS) $(LDFLAGS) -DENABLE_DEBUG -fsignaling-nans -o gameserver ./logging/logging.o ./net/net.o ./net/network.o ./server/config.o ./server/entities.o ./server/entity_storage.o ./server/json_output.o ./server/luafuncs.o ./server/luastate.o ./server/main.o ./server/map.o ./server/physics.o ./server/route.o ./server/ship.o ./server/storages.o ./server/vector.o ./net/talk.o ./net/pstr.o ./net/userstuff.o ./net/dispatch.o ./server/player.o ./server/base.o
