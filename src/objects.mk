ENET_OBJS= \
    enet/callbacks.o enet/host.o enet/list.o enet/packet.o enet/peer.o \
    enet/protocol.o enet/unix.o enet/win32.o

CLIENT_MOD_OBJS= \
    mod/plugin.o mod/demorecorder.o mod/chat.o mod/events.o \
    mod/extinfo.o mod/gamemod.o mod/geoip.o mod/ipignore.o mod/mod.o \
    mod/cubescript.o mod/hwdisplay.o mod/playerdisplay.o mod/http.o \
    mod/http-builtin-cert.o mod/strtool.o mod/crypto.o \
    mod/extinfo-playerpreview.o mod/ipbuf.o mod/proxy-detection.o \
    mod/geoip-compat.o mod/neutral-player-names.o

CLIENT_OBJS= \
    shared/crypto.o shared/geom.o  shared/stream.o  shared/tools.o \
    shared/zip.o  engine/3dgui.o engine/bih.o engine/blend.o \
    engine/blob.o engine/client.o engine/command.o engine/console.o \
    engine/cubeloader.o engine/decal.o engine/dynlight.o engine/glare.o \
    engine/grass.o engine/lightmap.o engine/main.o engine/material.o \
    engine/menus.o engine/movie.o engine/normal.o engine/octa.o \
    engine/octaedit.o engine/octarender.o engine/physics.o engine/pvs.o \
    engine/rendergl.o engine/rendermodel.o engine/renderparticles.o \
    engine/rendersky.o engine/rendertext.o engine/renderva.o \
    engine/server.o  engine/serverbrowser.o engine/shader.o \
    engine/shadowmap.o engine/sound.o engine/texture.o engine/water.o \
    engine/world.o engine/worldio.o fpsgame/ai.o fpsgame/client.o \
    fpsgame/entities.o fpsgame/fps.o fpsgame/monster.o fpsgame/movable.o \
    fpsgame/render.o fpsgame/scoreboard.o fpsgame/server.o fpsgame/waypoint.o \
    fpsgame/weapon.o $(CLIENT_MOD_OBJS)

CHATSERV_OBJS= \
    mod/chatserver/chatserver.o mod/strtool-standalone.o \
    shared/crypto-standalone.o shared/stream-standalone.o \
    shared/tools-standalone.o  engine/command-standalone.o 

SERVER_OBJS= \
    mod/strtool-standalone.o \
    shared/crypto-standalone.o shared/stream-standalone.o \
    shared/tools-standalone.o engine/command-standalone.o \
    engine/server-standalone.o engine/worldio-standalone.o \
    fpsgame/entities-standalone.o fpsgame/server-standalone.o

MASTER_OBJS= \
    mod/strtool-standalone.o \
    shared/crypto-standalone.o shared/stream-standalone.o \
    shared/tools-standalone.o engine/command-standalone.o \
    engine/master-standalone.o

PLUGIN_OBJS= \
    mod/strtool-plugin.o shared/stream-plugin.o \
    shared/tools-plugin.o shared/zip-plugin.o

G15LCD_PLUGIN_OBJS= \
    plugins/g15lcd/g15lcd.o

HWTEMP_PLUGIN_OBJS= \
    plugins/hwtemp/hwtemp.o plugins/hwtemp/linux.o \
    plugins/hwtemp/osx.o plugins/hwtemp/windows.o

CLIENT_OBJS+= $(ENET_OBJS)
SERVER_OBJS+= $(ENET_OBJS)
MASTER_OBJS+= $(ENET_OBJS)
CHATSERV_OBJS+= $(ENET_OBJS)

ifneq (,$(findstring mingw,$(PLATFORM)))
 # try to dynamically use the client's
 # enet library on non windows platforms
 PLUGIN_OBJS+= $(ENET_OBJS)
endif

