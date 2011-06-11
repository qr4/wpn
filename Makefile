all: gameserver

include settings.mak

SUBDIRS = logging net server

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
