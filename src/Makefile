DEBUG ?=
OPTIMIZE ?= 3
INSTALLPATH ?=
LTO ?= 0
USESTATICLIBS ?=
PUBLICRELEASE ?= 0
CHECKSYNTAX ?= 0
NOPCH ?= 0
PLATFORM ?= $(shell uname -s)
STRIP ?=
MV ?= mv

PLATFORM_PREFIX= native

override PLATFORMNATIVE= $(shell uname -s | tr '[:upper:]' '[:lower:]')
override PLATFORM:= $(shell echo $(PLATFORM) | tr '[:upper:]' '[:lower:]')

### Cygwin

ifneq (,$(findstring cygwin,$(PLATFORM)))
  override PLATFORM=mingw
  ifeq ($(shell uname -m),x86_64)
    ARCH= x86_64
  else
    ARCH= i686
  endif
endif

### Build client, plugins and chatserver by default

all: client plugins chatserver
default: all

### Setup default options

ifeq (,$(USESTATICLIBS))
ifneq (,$(findstring darwin,$(PLATFORM)))
  USESTATICLIBS=1
else
  ifeq (,$(findstring mingw,$(PLATFORM)))
    USESTATICLIBS=0
  endif
endif
endif

### Setup optimization flags

ifneq ($(OPTIMIZE),)
  override CXXFLAGS+=-O$(OPTIMIZE)
  override CFLAGS+=-O$(OPTIMIZE)
endif
ifneq ($(DEBUG),)
  override CXXFLAGS+= -g$(DEBUG)
  override CXXFLAGS+= -D__DEBUG__
endif

### Link Intel Libs statically

ifneq (,$(findstring icpc,$(CXX)))
  override LDFLAGS+= -static-intel -wd10237
endif

### ....

ifneq ($(INSTALLPATH),)
  INSTALLPATH=../..
endif

### Setup libenet flags

ENET_INCLUDE= -Ienet/include
ENET_CFLAGS= $(ENET_INCLUDE)

ifeq (,$(findstring mingw,$(PLATFORM)))
  override ENET_CFLAGS+= -fPIC $(shell ./build/enet_cflags.sh $(CC))
endif

### Setup includes

INCLUDES= -Ishared -Iengine -Ifpsgame $(ENET_INCLUDE) -Imod

### Include the source files

include objects.mk

ifeq (,$(findstring mingw,$(PLATFORM)))
  ################ GENERIC UNIX STUFF ################

  CLIENT_INCLUDES+= $(INCLUDES)
  SERVER_INCLUDES+= -DSTANDALONE $(INCLUDES)

  ifeq ($(USESTATICLIBS),1)
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libz.a
    SERVER_LIBS+= $(STATIC_LIB_DIR)/libz.a
    PLUGIN_LIBS+= $(STATIC_LIB_DIR)/libz.a
  else
    CLIENT_LIBS+= -lz
    SERVER_LIBS+= -lz
    PLUGIN_LIBS+= -lz
  endif

  CHATSERV_INCLUDES=

  ifeq (1,$(USESTATICLIBS))
    CHATSERV_LIBS+= $(STATIC_LIB_DIR)/libssl.a $(STATIC_LIB_DIR)/libcrypto.a
  else
    CHATSERV_LIBS+= -lssl -lcrypto
  endif

  ifeq (1,$(USESTATICLIBS))
    CHATSERV_INCLUDES+= -Iinclude
    CHATSERV_LIBS+= $(STATIC_LIB_DIR)/libSDL2.a
  endif

  ifeq (,$(findstring darwin,$(PLATFORM)))
    CHATSERV_LIBS+= -pthread
    CLIENT_LIBS+= -pthread
  endif

  ifeq (1,$(USESTATICLIBS))
    ifeq (,$(findstring darwin,$(PLATFORM)))
      STATIC_LIB_DIR= lib/$(PLATFORM)/$(ARCH)/$(LIBDIR)
    else
      STATIC_LIB_DIR= lib/$(PLATFORM)/$(LIBDIR)
    endif

    CHATSERV_INCLUDES+= -Iinclude/SDL2
    CLIENT_INCLUDES+= -Iinclude -Iinclude/SDL2 -Iinclude/GeoIP
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libcurl.a $(STATIC_LIB_DIR)/libGeoIP.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libssl.a $(STATIC_LIB_DIR)/libcrypto.a
  else
    CLIENT_LIBS+= -lcurl -lssl -lcrypto -lGeoIP
  endif

  MASTER_LIBS= $(SERVER_LIBS)

  ################ GENERIC UNIX STUFF END ################
endif

ifneq (,$(findstring mingw,$(PLATFORM)))
  ################ MINGW ################

  ARCH ?=
  MINGWPREFIX ?=
  MINGWCLANGPREFIX ?=

  ifeq (,$(ARCH))
    $(error ARCH must be given! i686 or x86_64)
  endif

  ifeq (,$(MINGWPREFIX))
  ifneq (,$(findstring x86_64,$(ARCH)))
    MINGWPREFIX=x86_64-w64-mingw32
  else
    MINGWPREFIX=i686-w64-mingw32
  endif
  ifneq (,$(findstring clang,$(PLATFORM)))
    ifneq (,$(findstring x86_64,$(ARCH)))
      MINGWCLANGPREFIX=x86_64-w64-mingw32-clang
    else
      MINGWCLANGPREFIX=i686-w64-mingw32-clang
    endif
  endif
  endif

  ifneq (,$(MINGWPREFIX))
    ifneq ($(MINGWCLANGPREFIX),)
      CC=$(MINGWCLANGPREFIX)
      CXX=$(MINGWCLANGPREFIX)++
    else
      CC=$(MINGWPREFIX)-gcc
      CXX=$(MINGWPREFIX)-g++
    endif
    WINDRES=$(MINGWPREFIX)-windres
  endif

  ifndef WINDRES
    WINDRES= windres
  endif

  SUBSYSTEM=-mwindows
  override LDFLAGS+= -static-libgcc -static-libstdc++

  ifneq (,$(findstring x86_64,$(ARCH)))
    WINBIN=bin64
    PLATFORMFLAG= -m64
    override WINDRES+= -F pe-x86-64
    PLUGINPLATFORMPREFIX=.64bit
  else
    WINBIN=bin
    PLATFORMFLAG= -m32
    override WINDRES+= -F pe-i386
    PLUGINPLATFORMPREFIX=
  endif

  WINLIB=lib/win/$(ARCH)/$(LIBDIR)
  WING15SDKWRAPPERDIR=plugins/g15lcd/lib/win/$(ARCH)

  override CXXFLAGS:= $(PLATFORMFLAG) $(CXXFLAGS)
  override CFLAGS:= $(PLATFORMFLAG) $(CFLAGS)
  override LDFLAGS:= $(PLATFORMFLAG) $(LDFLAGS)

  CLIENT_INCLUDES= $(INCLUDES) -Iinclude -Iinclude/SDL2 -Iinclude/GeoIP
  SERVER_INCLUDES= -DSTANDALONE $(INCLUDES) -Iinclude -Iinclude/SDL2 # (SDL for the chat server)

  SDL_DEP_LIBS= -lgdi32 -ldxguid -lole32 -lwinmm
  SDL_DEP_LIBS+= -limm32 -lversion -loleaut32

  SERVER_LIBS= $(SUBSYSTEM) -L$(WINLIB) -Wl,-Bstatic -lz -Wl,-Bdynamic -lws2_32 -lwinmm
  CHATSERV_LIBS+= -L$(WINLIB)/openssl -Wl,-Bstatic -lssl -lcrypto -Wl,-Bdynamic
  CHATSERV_LIBS+= -mconsole -Wl,-Bstatic -lSDL2 -lpthread -Wl,-Bdynamic -lws2_32 $(SDL_DEP_LIBS)
  MASTER_LIBS= -L$(WINLIB) -Wl,-Bstatic -lz -Wl,-Bdynamic -lws2_32 -lwinmm

  CLIENT_LIBS+= $(SUBSYSTEM) -L$(WINLIB)
  CLIENT_LIBS+= -Wl,-Bstatic -lcurl -lssl -lcrypto -ldl -Wl,-Bdynamic
  CLIENT_LIBS+= -lgdi32

  CLIENT_LIBS+= -Wl,-Bstatic
  CLIENT_LIBS+= -lSDL2_mixer -lvorbisfile -lvorbis -logg
  CLIENT_LIBS+= -lSDL2 -lSDL2_image -lpng16 -ljpeg -lz -lGeoIP
  CLIENT_LIBS+= -Wl,-Bdynamic
  CLIENT_LIBS+= -lopengl32 -lws2_32 $(SDL_DEP_LIBS)
  CLIENT_LIBS+= -Wl,-Bstatic
  CLIENT_LIBS+= -lpthread
  CLIENT_LIBS+= -Wl,-Bdynamic

  PLUGIN_INCLUDES= $(CLIENT_INCLUDES) -DPLUGIN

  PLUGIN_LIBS= -L$(WINLIB) -Wl,-Bstatic -lz -lSDL2 -Wl,-Bdynamic
  PLUGIN_LIBS+= -lws2_32 $(SDL_DEP_LIBS)

  # Old MinGW-w64-crt doesn't have strtok_r.
  # Referenced in libcurl.a.
  CLIENT_OBJS+= mod/quirks/mingw-w64/strtok_r.o

  ################ MINGW TARGETS  ################ 

  plugins: g15lcd hwtemp

  client: $(CLIENT_OBJS)
	$(WINDRES) -I vcpp -i vcpp/mingw.rc -J rc -o vcpp/mingw.res -O coff
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -o sauerbraten_wc.exe vcpp/mingw.res $(CLIENT_OBJS) $(CLIENT_LIBS)

  chatserver: $(CHATSERV_OBJS)
	$(WINDRES) -I vcpp -i vcpp/mingw.rc -J rc -o vcpp/mingw.res -O coff
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -o wc_chat_server.exe vcpp/mingw.res $(CHATSERV_OBJS) $(SERVER_LIBS) $(CHATSERV_LIBS)

  server: $(SERVER_OBJS)
	$(WINDRES) -I vcpp -i vcpp/mingw.rc -J rc -o vcpp/mingw.res -O coff
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -o sauer_server.exe vcpp/mingw.res $(SERVER_OBJS) $(SERVER_LIBS)

  master: $(MASTER_OBJS)
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -o sauer_master.exe $(MASTER_OBJS) $(MASTER_LIBS)

  g15lcd: plugin $(G15LCD_PLUGIN_OBJS)
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -shared -Wl,-soname,$@$(PLUGINPLATFORMPREFIX).dll -o plugins/$@$(PLUGINPLATFORMPREFIX).dll $(PLUGIN_OBJS) $(G15LCD_PLUGIN_OBJS) $(PLUGIN_LIBS) -L$(WING15SDKWRAPPERDIR) -lg15sdk

  hwtemp: plugin $(HWTEMP_PLUGIN_OBJS)
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -shared -Wl,-soname,$@$(PLUGINPLATFORMPREFIX).dll -o plugins/$@$(PLUGINPLATFORMPREFIX).dll $(PLUGIN_OBJS) $(HWTEMP_PLUGIN_OBJS) $(PLUGIN_LIBS)

  install: all
	mkdir -p $(INSTALLPATH)/$(WINBIN) $(INSTALLPATH)/plugins
	cp -p sauerbraten_wc.exe $(INSTALLPATH)/$(WINBIN)
	#cp -p sauer_server.exe $(INSTALLPATH)/$(WINBIN)
	cp -p plugins/*.dll $(INSTALLPATH)/plugins/
	ifneq ($(wildcard $(WINLIB)/*.dll),)
		cp $(WINLIB)/*.dll $(INSTALLPATH)/$(WINBIN)
	endif
	cp $(WING15SDKWRAPPERDIR)/*.dll $(INSTALLPATH)/$(WINBIN)
	ifneq (,$(STRIP))
		$(STRIP) $(INSTALLPATH)/$(WINBIN)/*.exe
		$(STRIP) $(INSTALLPATH)/plugins/*.dll
	endif

  ################ END MINGW ################ 
else
ifeq (darwin,$(PLATFORM))
  ################ OS X ################

  OSXVERSIONMIN ?= 10.5
  HAVECROSSCOMPILER ?= 0
  UNIVERSAL ?= 0
  TARGETS ?=
  NOPCH= 1

  ifeq ($(UNIVERSAL), 1)
    override TARGETS= -arch i386 -arch x86_64
  endif

  ifneq (,$(DEBUG))
    override LDFLAGS+= -Wl,-dead_strip
  endif

  ifeq (,$(findstring darwin,$(PLATFORMNATIVE)))
  ifneq (1,$(HAVECROSSCOMPILER))
    TOOLCHAINTARGET= $(shell osxcross-conf | grep "TARGET=" | head -n1 | cut -b24-)
    TOOLCHAIN= x86_64-apple-$(TOOLCHAINTARGET)-
    CXX= $(TOOLCHAIN)clang++
    CC= $(TOOLCHAIN)clang
    PLATFORM= darwin
  endif
  else
    CC= clang
    CXX= clang++
  endif

  OBJCC ?= $(CC)
  OBJCXX ?= $(CXX)

  override OSXMINFLAGS= -mmacosx-version-min=$(OSXVERSIONMIN) $(TARGETS)

  override CXXFLAGS+= $(OSXMINFLAGS)
  override CFLAGS+= $(OSXMINFLAGS)
  override OBJCFLAGS+= $(OSXMINFLAGS)
  override LDFLAGS+= $(OSXMINFLAGS)

  SDL_DEP_LIBS= -framework CoreAudio -framework AudioUnit -framework Cocoa 
  SDL_DEP_LIBS+= -framework IOKit -framework Carbon -liconv

  CHATSERV_LIBS+= $(SDL_DEP_LIBS)

  # The Apple Linker doesn't understand -Bstatic ...
  CLIENT_LIBS+= -framework OpenGL
  CLIENT_LIBS+= $(SDL_DEP_LIBS)
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libSDL2_mixer.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libSDL2.a
  CLIENT_LIBS+= -liconv
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libSDL2_image.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libvorbisfile.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libvorbis.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libogg.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libpng16.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libjpeg.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libz.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libGeoIP.a

  PLUGIN_INCLUDES= $(CLIENT_INCLUDES) -fPIC -DPLUGIN -Iinclude/GeoIP -Iinclude/osx-cpu-temp
  CLIENT_OBJS+= xcode/macutils.o xcode/main.o xcode/Launcher.o

  HWTEMP_LIBS= $(STATIC_LIB_DIR)/smc.a

  ifeq (1, $(LTO))
    HWTEMP_LIBS+= $(STATIC_LIB_DIR)/libSDL2.a -framework IOKit
  endif

  xcode/Launcher%: override OBJCFLAGS+=-Wno-deprecated-declarations

  ################ OS X targets ################

  plugins: hwtemp

  %.o: %.mm
	$(OBJCXX) $(OBJCFLAGS) $(TARGETS) $(CLIENT_INCLUDES) $(subst .o,.mm,$@) -c -o $@

  %.o: %.m
	$(OBJCC) $(OBJCFLAGS) $(TARGETS) $(CLIENT_INCLUDES) $(subst .o,.m,$@) -c -o $@

  plugins: hwtemp

  client: $(CLIENT_OBJS)
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -o wclient $(CLIENT_OBJS) $(CLIENT_LIBS)

  hwtemp: plugin $(HWTEMP_PLUGIN_OBJS)
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -dynamiclib -undefined dynamic_lookup -fPIC -Wl,-install_name,$@.dylib -o plugins/$@.dylib $(PLUGIN_OBJS) $(HWTEMP_PLUGIN_OBJS) $(PLUGIN_LIBS) $(HWTEMP_LIBS)

  ################ END OS X ################
else
  ################ UNIX ################

  ARCH ?= $(shell uname -m)
  GLIBCCOMPAT ?= 1

  ifneq (,$(findstring x86_64,$(ARCH)))
    PLATFORMFLAG= -m64
  else
    ifneq (,$(findstring i,$(ARCH)))
      PLATFORMFLAG= -m32
      ARCH= i686
    endif
  endif

  ifneq (,$(PLATFORMFLAG))
    override CXXFLAGS:= $(PLATFORMFLAG) $(CXXFLAGS)
    override CFLAGS:= $(PLATFORMFLAG) $(CFLAGS)
  endif

  ifeq ($(PLATFORM),$(PLATFORMNATIVE))
    ifeq ($(PLATFORM),linux)
      override NATIVE_LIB_PATH= -L/usr/X11R6/lib
      override NATIVE_INCLUDES= -I/usr/X11R6/include
      ifeq ($(ARCH),i686)
        override NATIVE_LIB_PATH+= -L/usr/lib32 -L/usr/lib/i386-linux-gnu
        override NATIVE_LIB_PATH+= -L/usr/lib/i386-linux-gnu/mesa
      endif
    else
      override NATIVE_LIB_PATH= -L/usr/local/lib
      override NATIVE_INCLUDES=
    endif
  else
    ifeq ($(PLATFORM),freebsd)
      override NATIVE_LIB_PATH= -Wl,-rpath,/usr/local/lib
      override NATIVE_INCLUDES=
    endif
  endif

  ifeq ($(PLATFORM),linux)
  ifeq ($(GLIBCCOMPAT),1)
    ifneq (,$(findstring icc,$(CC)))
      override COMPATCC= gcc
    else
      override COMPATCC= $(CC)
    endif
    CLIENT_OBJS+= mod/quirks/glibc/compat.o
    SERVER_OBJS+= mod/quirks/glibc/compat.o
    MASTER_OBJS+= mod/quirks/glibc/compat.o
    CHATSERV_OBJS+= mod/quirks/glibc/compat.o
    PLUGIN_OBJS+= mod/quirks/glibc/compat-plugin.o
    mod/quirks/glibc/%.o: override CC:= $(COMPATCC)
    mod/quirks/glibc/%.o: override CFLAGS= $(PLATFORMFLAG) -O2 -fomit-frame-pointer
    mod/quirks/glibc/%-plugin.o: override CFLAGS+= -fPIC
    ifeq (,$(findstring icc,$(CC)))
      override LDFLAGS+= -lm
    endif
    override LDFLAGS+= -Wl,--wrap=memcpy -Wl,--wrap=__fdelt_chk
    override LDFLAGS+= -Wl,--wrap=__exp_finite -Wl,--wrap=__expf_finite
    override LDFLAGS+= -Wl,--wrap=__log_finite -Wl,--wrap=__logf_finite
    override LDFLAGS+= -Wl,--wrap=__log10f_finite -Wl,--wrap=__pow_finite
    override LDFLAGS+= -Wl,--wrap=__powf_finite -Wl,--wrap=__acosf_finite
    override LDFLAGS+= -Wl,--wrap=__atan2f_finite -Wl,--wrap=__fmod_finite
    override LDFLAGS+= -Wl,--wrap=__acos_finite -Wl,--wrap=__asin_finite
    override LDFLAGS+= -Wl,--wrap=__log10_finite -Wl,--wrap=__atan2_finite
  endif
  endif

  ifeq ($(USESTATICLIBS),1)
    CLIENT_LIBS+= $(NATIVE_LIB_PATH)
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libSDL2_mixer.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libvorbisfile.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libvorbis.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libogg.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libSDL2.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libSDL2_image.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libpng16.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libjpeg.a
    PLUGIN_LIBS+= $(STATIC_LIB_DIR)/libSDL2.a
    CLIENT_LIBS+= -lGL -lX11 -lXext -lXrandr
    ifeq ($(PLATFORM),freebsd)
      CLIENT_LIBS+= -lvgl -liconv -lpulse -lpulse-simple
      CHATSERV_LIBS+= -liconv
    endif
    override SDL_CONFIG= -Iinclude
  else
    ifeq ($(PLATFORM),$(PLATFORMNATIVE))
      override SDL_CONFIG= `sdl2-config --cflags`
      override SDL_CONFIG_LIB= `sdl2-config --libs`
    else
      override NATIVE_LIB_PATH=
      override NATIVE_INCLUDES=
      override SDL_CONFIG_LIB=
      ifeq ($(PLATFORM),freebsd)
        override SDL_CONFIG_LIB= -lSDL2 -pthread
        override SDL_CONFIG= $(shell ./build/quirks/fbsd-cross-sdl-cpath.sh $(CXX))
      endif
   endif

   CHATSERV_INCLUDES+= $(SDL_CONFIG)
   CHATSERV_LIBS+= $(SDL_CONFIG_LIB)
   CLIENT_LIBS+= $(NATIVE_LIB_PATH) -lX11 $(SDL_CONFIG_LIB) -lSDL2_image -lSDL2_mixer -lGL
  endif

  CLIENT_INCLUDES:= $(INCLUDES) $(NATIVE_INCLUDES) $(SDL_CONFIG) $(CLIENT_INCLUDES)
  PLUGIN_INCLUDES= $(CLIENT_INCLUDES) -fPIC -DPLUGIN

  #ifneq (,$(STATIC_LIB_DIR))
    G15LCD_LIB_DIR= lib/$(PLATFORM)/$(ARCH)
    G15LCD_LIBS=  plugins/g15lcd/$(G15LCD_LIB_DIR)/g15sdkwrapper.a
    G15LCD_LIBS+= plugins/g15lcd/$(G15LCD_LIB_DIR)/libg15render.a
    G15LCD_LIBS+= plugins/g15lcd/$(G15LCD_LIB_DIR)/libg15daemon_client.a
    G15LCD_LIBS+= plugins/g15lcd/$(G15LCD_LIB_DIR)/libg15.a
  #else
  # G15LCD_LIBS+= -lg15render -lg15daemon_client -lg15
  #endif

  ifneq (,$(STATIC_LIB_DIR))
    HWTEMP_LIBS= $(STATIC_LIB_DIR)/libsensors.a
  else
    HWTEMP_LIBS= -lsensors
  endif

  ### On Linux? -> Link against -lrt and -ldl

  ifeq ($(PLATFORM),linux)
    CLIENT_LIBS+= -lrt -ldl
    CHATSERV_LIBS+= -lrt -ldl
    PLUGIN_LIBS+= -lrt -ldl
  endif

  ################ UNIX TARGETS ################

  ifeq (linux,$(PLATFORM))
    plugins: g15lcd hwtemp
  else
    plugins:
  endif

  client: $(CLIENT_OBJS)
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -o sauer_client $(CLIENT_OBJS) $(CLIENT_LIBS)

  g15lcd: plugin $(G15LCD_PLUGIN_OBJS)
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -shared -fPIC -Wl,-soname,$@.so -o plugins/$@.so $(PLUGIN_OBJS) $(G15LCD_PLUGIN_OBJS) $(PLUGIN_LIBS) $(G15LCD_LIBS)

  hwtemp: plugin $(HWTEMP_PLUGIN_OBJS)
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -shared -fPIC -Wl,-soname,$@.so -o plugins/$@.so $(PLUGIN_OBJS) $(HWTEMP_PLUGIN_OBJS) $(PLUGIN_LIBS) $(HWTEMP_LIBS)

  install: all
	cp -p sauer_client ../bin_unix/$(PLATFORM_PREFIX)_client
	#cp -p sauer_server ../bin_unix/$(PLATFORM_PREFIX)_server
  ifneq (,$(STRIP))
	$(STRIP) ../bin_unix/$(PLATFORM_PREFIX)_client
	#$(STRIP) ../bin_unix/$(PLATFORM_PREFIX)_server
  endif

  ################ END UNIX ################
endif
endif

### Check for LTO support

override LIBDIR=native

ifeq ($(LTO),1)
  ifneq (,$(findstring clang,$(CXX)))
  ifeq (linux,$(PLATFORM))
    $(shell ln -sf `which ld.gold` build/ld 2>/dev/null)
    LDGOLD=$(shell echo -B`pwd`/build)
    override LDFLAGS+=$(LDGOLD)
  endif
  endif
  LTOFLAGS:= $(shell ./build/checklto/checklto.sh $(CXX) $(LDGOLD))
  CLTOFLAGS:= $(shell ./build/checklto/checklto.sh $(CC) $(LDGOLD))
  ifneq ($(LTOFLAGS),$(CLTOFLAGS))
    $(error CC/CXX compiler vendor mismatch)
  endif

  override CXXFLAGS+= $(LTOFLAGS)
  override CFLAGS+= $(LTOFLAGS)
  override LDFLAGS+= $(LTOFLAGS)

  ifneq (,$(LTOFLAGS))
  ifneq (,$(findstring clang,$(CXX)))
    override LIBDIR=llvm-lto
    ifneq (,$(findstring mingw,$(PLATFORM)))
      # clang with MinGW target
      override LDFLAGS+= -Wl,-plugin-opt=mtriple=$(ARCH)-w64-mingw32
    else
      ifneq ($(PLATFORM),darwin)
        override LDFLAGS+= -Wl,-plugin-opt=mtriple=$(ARCH)-pc-$(PLATFORM)
      endif
    endif
  endif
  endif
endif


### Check for >= C++11

ifeq (,$(CXXSTD))
  CXXSTD= $(shell ./build/check_cxx_standard.sh $(CXX))

  ifeq (,$(CXXSTD))
    $(error a c++11 compiler is required)
  endif
else
  CXXSTD:= -std=$(CXXSTD)
endif

### Setup C/C++ flags

override CXXFLAGS+= $(CXXSTD)
override CXXFLAGS+= -Wall -Wsign-compare -Wformat-security -Wtype-limits
override CXXFLAGS+= -fsigned-char -fno-exceptions
override CFLAGS+= -Wall -Wsign-compare -Wformat-security -Wtype-limits

ifeq (,$(findstring -fsanitize=undefined,$(CXXFLAGS)))
  override CXXFLAGS+= -fno-rtti
endif

ifeq ($(PLATFORM),linux)
  override CFLAGS+= -U_FORTIFY_SOURCE
  override CXXFLAGS+= -U_FORTIFY_SOURCE
endif

################ GENERIC TARGETS ################

$(SERVER_OBJS): CXXFLAGS += $(SERVER_INCLUDES)
$(filter-out $(SERVER_OBJS),$(MASTER_OBJS)): CXXFLAGS += $(SERVER_INCLUDES)
$(CHATSERV_OBJS): CXXFLAGS += $(SERVER_INCLUDES) $(CHATSERV_INCLUDES)

mod/wcversion.h:
REVISION_MSG := $(shell ./build/get_revision.sh)
ifneq (,$(REVISION_MSG))
$(error $(REVISION_MSG))
endif

enet/%.o: override CFLAGS+= $(ENET_CFLAGS)

%.h.gch: %.h
	$(CXX) $(CXXFLAGS) -x c++-header -o $(subst .h.gch,.tmp.h.gch,$@) $(subst .h.gch,.h,$@)
	$(MV) $(subst .h.gch,.tmp.h.gch,$@) $@

%-standalone.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $(subst -standalone.o,.cpp,$@)

%-plugin.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $(subst -plugin.o,.cpp,$@)

%-plugin.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $(subst -plugin.o,.c,$@)

plugin: $(PLUGIN_OBJS)

depend:
	makedepend -Y -Ishared -Iengine -Ifpsgame -Imod $(ENET_INCLUDE) $(subst .o,.cpp,$(CLIENT_OBJS)) $(subst .o,.cpp,$(G15LCD_PLUGIN_OBJS)) $(subst .o,.cpp,$(HWTEMP_PLUGIN_OBJS))
	makedepend -a -o.h.gch -Y -Ishared -Iengine -Ifpsgame -Imod $(ENET_INCLUDE) $(subst .h.gch,.h,$(CLIENT_PCH))
	makedepend -a -o-standalone.o -Y -DSTANDALONE -Ishared -Iengine -Ifpsgame -Imod $(ENET_INCLUDE) $(subst -standalone.o,.cpp,$(SERVER_OBJS))
	makedepend -a -o-standalone.o -Y -DSTANDALONE -Ishared -Iengine -Ifpsgame -Imod $(ENET_INCLUDE) $(subst -standalone.o,.cpp,$(filter-out $(SERVER_OBJS), $(MASTER_OBJS)))

.PHONY: clean
clean:
	-$(RM) $(CLIENT_PCH) $(CLIENT_OBJS) $(CHATSERV_OBJS) $(SERVER_OBJS)
	-$(RM) $(MASTER_OBJS) $(PLUGIN_OBJS) $(ENET_OBJS)
	-$(RM) $(G15LCD_PLUGIN_OBJS) $(HWTEMP_PLUGIN_OBJS)
	-$(RM) mod/quirks/glibc/*.o xcode/*.o
	-$(RM) plugins/*.so plugins/*.dll plugins/*.dylib
	-$(RM) sauer_client sauer_server sauer_master wclient wc_chat_server *.exe
	-$(RM) mod/wcversion.h mod/hidevar.h

ifeq (,$(findstring mingw,$(PLATFORM)))
shared/cube2font.o: shared/cube2font.c
	$(CXXLINKER) $(CXXFLAGS) -c -o $@ $< `freetype-config --cflags`

cube2font: shared/cube2font.o
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -o cube2font shared/cube2font.o `freetype-config --libs` -lz -lpng

server: $(SERVER_OBJS)
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -o sauer_server $(SERVER_OBJS) $(SERVER_LIBS)

master: $(MASTER_OBJS)
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -o sauer_master $(MASTER_OBJS) $(MASTER_LIBS)

chatserver: $(CHATSERV_OBJS)
	$(CXXLINKER) $(CXXFLAGS) $(LDFLAGS) -o wc_chat_server $(CHATSERV_OBJS) $(SERVER_LIBS) $(CHATSERV_LIBS)
endif

################ GENERIC TARGETS END ################

include checksyntax.mk

### Use a pre-compiled header?

ifneq (1,$(NOPCH))
  CLIENT_PCH= shared/cube.h.gch engine/engine.h.gch fpsgame/game.h.gch
endif

$(CLIENT_OBJS): CXXFLAGS += $(CLIENT_INCLUDES)
$(filter shared/%,$(CLIENT_OBJS)): $(filter shared/%,$(CLIENT_PCH))
$(filter engine/%,$(CLIENT_OBJS)): $(filter engine/%,$(CLIENT_PCH))
$(filter fpsgame/%,$(CLIENT_OBJS)): $(filter fpsgame/%,$(CLIENT_PCH))

$(PLUGIN_OBJS): CXXFLAGS += $(PLUGIN_INCLUDES)
$(G15LCD_PLUGIN_OBJS): CXXFLAGS += $(PLUGIN_INCLUDES)
ifneq (,$(STATIC_LIB_DIR))
  $(G15LCD_PLUGIN_OBJS): CXXFLAGS += -Iplugins/g15lcd/include
endif
$(HWTEMP_PLUGIN_OBJS): CXXFLAGS += $(PLUGIN_INCLUDES) -Iplugins/hwtemp/include

engine/engine.h.gch: shared/cube.h.gch
fpsgame/game.h.gch: shared/cube.h.gch

# DO NOT DELETE

shared/crypto.o: shared/cube.h enet/include/enet/enet.h
shared/crypto.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
shared/crypto.o: enet/include/enet/enet/protocol.h
shared/crypto.o: enet/include/enet/enet/list.h
shared/crypto.o: enet/include/enet/enet/callbacks.h shared/tools.h
shared/crypto.o: shared/geom.h shared/ents.h shared/command.h
shared/crypto.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
shared/crypto.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
shared/crypto.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
shared/crypto.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
shared/crypto.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
shared/crypto.o: shared/igame.h
shared/geom.o: shared/cube.h enet/include/enet/enet.h
shared/geom.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
shared/geom.o: enet/include/enet/enet/protocol.h
shared/geom.o: enet/include/enet/enet/list.h
shared/geom.o: enet/include/enet/enet/callbacks.h shared/tools.h
shared/geom.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
shared/geom.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
shared/geom.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
shared/geom.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
shared/geom.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
shared/geom.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
shared/stream.o: shared/cube.h enet/include/enet/enet.h
shared/stream.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
shared/stream.o: enet/include/enet/enet/protocol.h
shared/stream.o: enet/include/enet/enet/list.h
shared/stream.o: enet/include/enet/enet/callbacks.h shared/tools.h
shared/stream.o: shared/geom.h shared/ents.h shared/command.h
shared/stream.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
shared/stream.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
shared/stream.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
shared/stream.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
shared/stream.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
shared/stream.o: shared/igame.h
shared/tools.o: shared/cube.h enet/include/enet/enet.h
shared/tools.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
shared/tools.o: enet/include/enet/enet/protocol.h
shared/tools.o: enet/include/enet/enet/list.h
shared/tools.o: enet/include/enet/enet/callbacks.h shared/tools.h
shared/tools.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
shared/tools.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
shared/tools.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
shared/tools.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
shared/tools.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
shared/tools.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
shared/zip.o: shared/cube.h enet/include/enet/enet.h
shared/zip.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
shared/zip.o: enet/include/enet/enet/protocol.h enet/include/enet/enet/list.h
shared/zip.o: enet/include/enet/enet/callbacks.h shared/tools.h shared/geom.h
shared/zip.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
shared/zip.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
shared/zip.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
shared/zip.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
shared/zip.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
shared/zip.o: shared/igame.h
engine/3dgui.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/3dgui.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/3dgui.o: enet/include/enet/enet/protocol.h
engine/3dgui.o: enet/include/enet/enet/list.h
engine/3dgui.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/3dgui.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
engine/3dgui.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/3dgui.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/3dgui.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/3dgui.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/3dgui.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/3dgui.o: engine/world.h engine/glexts.h engine/octa.h
engine/3dgui.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/3dgui.o: engine/model.h engine/varray.h engine/textedit.h
engine/bih.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/bih.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/bih.o: enet/include/enet/enet/protocol.h enet/include/enet/enet/list.h
engine/bih.o: enet/include/enet/enet/callbacks.h shared/tools.h shared/geom.h
engine/bih.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/bih.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
engine/bih.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
engine/bih.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
engine/bih.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/bih.o: shared/igame.h engine/world.h engine/glexts.h engine/octa.h
engine/bih.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
engine/bih.o: engine/varray.h
engine/blend.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/blend.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/blend.o: enet/include/enet/enet/protocol.h
engine/blend.o: enet/include/enet/enet/list.h
engine/blend.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/blend.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
engine/blend.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/blend.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/blend.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/blend.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/blend.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/blend.o: engine/world.h engine/glexts.h engine/octa.h
engine/blend.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/blend.o: engine/model.h engine/varray.h
engine/blob.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/blob.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/blob.o: enet/include/enet/enet/protocol.h
engine/blob.o: enet/include/enet/enet/list.h
engine/blob.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/blob.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
engine/blob.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/blob.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/blob.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/blob.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/blob.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/blob.o: engine/world.h engine/glexts.h engine/octa.h engine/lightmap.h
engine/blob.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/client.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/client.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/client.o: enet/include/enet/enet/protocol.h
engine/client.o: enet/include/enet/enet/list.h
engine/client.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/client.o: shared/geom.h shared/ents.h shared/command.h
engine/client.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/client.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/client.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/client.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/client.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/client.o: shared/igame.h engine/world.h engine/glexts.h engine/octa.h
engine/client.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/client.o: engine/model.h engine/varray.h
engine/command.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/command.o: enet/include/enet/enet/unix.h
engine/command.o: enet/include/enet/enet/types.h
engine/command.o: enet/include/enet/enet/protocol.h
engine/command.o: enet/include/enet/enet/list.h
engine/command.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/command.o: shared/geom.h shared/ents.h shared/command.h
engine/command.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/command.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/command.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/command.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/command.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/command.o: shared/igame.h engine/world.h engine/glexts.h engine/octa.h
engine/command.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/command.o: engine/model.h engine/varray.h
engine/console.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/console.o: enet/include/enet/enet/unix.h
engine/console.o: enet/include/enet/enet/types.h
engine/console.o: enet/include/enet/enet/protocol.h
engine/console.o: enet/include/enet/enet/list.h
engine/console.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/console.o: shared/geom.h shared/ents.h shared/command.h
engine/console.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/console.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/console.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/console.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/console.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/console.o: shared/igame.h engine/world.h engine/glexts.h engine/octa.h
engine/console.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/console.o: engine/model.h engine/varray.h
engine/console.o: engine/sdl2_keymap_extrakeys.h
engine/cubeloader.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/cubeloader.o: enet/include/enet/enet/unix.h
engine/cubeloader.o: enet/include/enet/enet/types.h
engine/cubeloader.o: enet/include/enet/enet/protocol.h
engine/cubeloader.o: enet/include/enet/enet/list.h
engine/cubeloader.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/cubeloader.o: shared/geom.h shared/ents.h shared/command.h
engine/cubeloader.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/cubeloader.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/cubeloader.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/cubeloader.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/cubeloader.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/cubeloader.o: shared/igame.h engine/world.h engine/glexts.h
engine/cubeloader.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/cubeloader.o: engine/texture.h engine/model.h engine/varray.h
engine/decal.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/decal.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/decal.o: enet/include/enet/enet/protocol.h
engine/decal.o: enet/include/enet/enet/list.h
engine/decal.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/decal.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
engine/decal.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/decal.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/decal.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/decal.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/decal.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/decal.o: engine/world.h engine/glexts.h engine/octa.h
engine/decal.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/decal.o: engine/model.h engine/varray.h
engine/dynlight.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/dynlight.o: enet/include/enet/enet/unix.h
engine/dynlight.o: enet/include/enet/enet/types.h
engine/dynlight.o: enet/include/enet/enet/protocol.h
engine/dynlight.o: enet/include/enet/enet/list.h
engine/dynlight.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/dynlight.o: shared/geom.h shared/ents.h shared/command.h
engine/dynlight.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/dynlight.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/dynlight.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/dynlight.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/dynlight.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/dynlight.o: shared/igame.h engine/world.h engine/glexts.h
engine/dynlight.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/dynlight.o: engine/texture.h engine/model.h engine/varray.h
engine/glare.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/glare.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/glare.o: enet/include/enet/enet/protocol.h
engine/glare.o: enet/include/enet/enet/list.h
engine/glare.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/glare.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
engine/glare.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/glare.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/glare.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/glare.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/glare.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/glare.o: engine/world.h engine/glexts.h engine/octa.h
engine/glare.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/glare.o: engine/model.h engine/varray.h engine/rendertarget.h
engine/grass.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/grass.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/grass.o: enet/include/enet/enet/protocol.h
engine/grass.o: enet/include/enet/enet/list.h
engine/grass.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/grass.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
engine/grass.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/grass.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/grass.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/grass.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/grass.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/grass.o: engine/world.h engine/glexts.h engine/octa.h
engine/grass.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/grass.o: engine/model.h engine/varray.h
engine/lightmap.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/lightmap.o: enet/include/enet/enet/unix.h
engine/lightmap.o: enet/include/enet/enet/types.h
engine/lightmap.o: enet/include/enet/enet/protocol.h
engine/lightmap.o: enet/include/enet/enet/list.h
engine/lightmap.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/lightmap.o: shared/geom.h shared/ents.h shared/command.h
engine/lightmap.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/lightmap.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/lightmap.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/lightmap.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/lightmap.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/lightmap.o: shared/igame.h engine/world.h engine/glexts.h
engine/lightmap.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/lightmap.o: engine/texture.h engine/model.h engine/varray.h
engine/main.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/main.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/main.o: enet/include/enet/enet/protocol.h
engine/main.o: enet/include/enet/enet/list.h
engine/main.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/main.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
engine/main.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/main.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/main.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/main.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/main.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/main.o: engine/world.h engine/glexts.h engine/octa.h engine/lightmap.h
engine/main.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/material.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/material.o: enet/include/enet/enet/unix.h
engine/material.o: enet/include/enet/enet/types.h
engine/material.o: enet/include/enet/enet/protocol.h
engine/material.o: enet/include/enet/enet/list.h
engine/material.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/material.o: shared/geom.h shared/ents.h shared/command.h
engine/material.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/material.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/material.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/material.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/material.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/material.o: shared/igame.h engine/world.h engine/glexts.h
engine/material.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/material.o: engine/texture.h engine/model.h engine/varray.h
engine/menus.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/menus.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/menus.o: enet/include/enet/enet/protocol.h
engine/menus.o: enet/include/enet/enet/list.h
engine/menus.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/menus.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
engine/menus.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/menus.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/menus.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/menus.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/menus.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/menus.o: engine/world.h engine/glexts.h engine/octa.h
engine/menus.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/menus.o: engine/model.h engine/varray.h
engine/movie.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/movie.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/movie.o: enet/include/enet/enet/protocol.h
engine/movie.o: enet/include/enet/enet/list.h
engine/movie.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/movie.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
engine/movie.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/movie.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/movie.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/movie.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/movie.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/movie.o: engine/world.h engine/glexts.h engine/octa.h
engine/movie.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/movie.o: engine/model.h engine/varray.h
engine/normal.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/normal.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/normal.o: enet/include/enet/enet/protocol.h
engine/normal.o: enet/include/enet/enet/list.h
engine/normal.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/normal.o: shared/geom.h shared/ents.h shared/command.h
engine/normal.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/normal.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/normal.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/normal.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/normal.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/normal.o: shared/igame.h engine/world.h engine/glexts.h engine/octa.h
engine/normal.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/normal.o: engine/model.h engine/varray.h
engine/octa.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/octa.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/octa.o: enet/include/enet/enet/protocol.h
engine/octa.o: enet/include/enet/enet/list.h
engine/octa.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/octa.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
engine/octa.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/octa.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/octa.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/octa.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/octa.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/octa.o: engine/world.h engine/glexts.h engine/octa.h engine/lightmap.h
engine/octa.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/octaedit.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/octaedit.o: enet/include/enet/enet/unix.h
engine/octaedit.o: enet/include/enet/enet/types.h
engine/octaedit.o: enet/include/enet/enet/protocol.h
engine/octaedit.o: enet/include/enet/enet/list.h
engine/octaedit.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/octaedit.o: shared/geom.h shared/ents.h shared/command.h
engine/octaedit.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/octaedit.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/octaedit.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/octaedit.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/octaedit.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/octaedit.o: shared/igame.h engine/world.h engine/glexts.h
engine/octaedit.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/octaedit.o: engine/texture.h engine/model.h engine/varray.h
engine/octarender.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/octarender.o: enet/include/enet/enet/unix.h
engine/octarender.o: enet/include/enet/enet/types.h
engine/octarender.o: enet/include/enet/enet/protocol.h
engine/octarender.o: enet/include/enet/enet/list.h
engine/octarender.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/octarender.o: shared/geom.h shared/ents.h shared/command.h
engine/octarender.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/octarender.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/octarender.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/octarender.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/octarender.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/octarender.o: shared/igame.h engine/world.h engine/glexts.h
engine/octarender.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/octarender.o: engine/texture.h engine/model.h engine/varray.h
engine/physics.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/physics.o: enet/include/enet/enet/unix.h
engine/physics.o: enet/include/enet/enet/types.h
engine/physics.o: enet/include/enet/enet/protocol.h
engine/physics.o: enet/include/enet/enet/list.h
engine/physics.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/physics.o: shared/geom.h shared/ents.h shared/command.h
engine/physics.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/physics.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/physics.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/physics.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/physics.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/physics.o: shared/igame.h engine/world.h engine/glexts.h engine/octa.h
engine/physics.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/physics.o: engine/model.h engine/varray.h engine/mpr.h
engine/pvs.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/pvs.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/pvs.o: enet/include/enet/enet/protocol.h enet/include/enet/enet/list.h
engine/pvs.o: enet/include/enet/enet/callbacks.h shared/tools.h shared/geom.h
engine/pvs.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/pvs.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
engine/pvs.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
engine/pvs.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
engine/pvs.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/pvs.o: shared/igame.h engine/world.h engine/glexts.h engine/octa.h
engine/pvs.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
engine/pvs.o: engine/varray.h
engine/rendergl.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/rendergl.o: enet/include/enet/enet/unix.h
engine/rendergl.o: enet/include/enet/enet/types.h
engine/rendergl.o: enet/include/enet/enet/protocol.h
engine/rendergl.o: enet/include/enet/enet/list.h
engine/rendergl.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/rendergl.o: shared/geom.h shared/ents.h shared/command.h
engine/rendergl.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/rendergl.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/rendergl.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/rendergl.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/rendergl.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/rendergl.o: shared/igame.h engine/world.h engine/glexts.h
engine/rendergl.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/rendergl.o: engine/texture.h engine/model.h engine/varray.h
engine/rendermodel.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/rendermodel.o: enet/include/enet/enet/unix.h
engine/rendermodel.o: enet/include/enet/enet/types.h
engine/rendermodel.o: enet/include/enet/enet/protocol.h
engine/rendermodel.o: enet/include/enet/enet/list.h
engine/rendermodel.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/rendermodel.o: shared/geom.h shared/ents.h shared/command.h
engine/rendermodel.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/rendermodel.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/rendermodel.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/rendermodel.o: mod/extinfo.h mod/demorecorder.h mod/chat.h
engine/rendermodel.o: mod/events.h mod/thread.h mod/proxy-detection.h
engine/rendermodel.o: mod/plugin.h shared/igame.h engine/world.h
engine/rendermodel.o: engine/glexts.h engine/octa.h engine/lightmap.h
engine/rendermodel.o: engine/bih.h engine/texture.h engine/model.h
engine/rendermodel.o: engine/varray.h engine/ragdoll.h engine/animmodel.h
engine/rendermodel.o: engine/vertmodel.h engine/skelmodel.h engine/md2.h
engine/rendermodel.o: engine/md3.h engine/md5.h engine/obj.h engine/smd.h
engine/rendermodel.o: engine/iqm.h
engine/renderparticles.o: engine/engine.h shared/cube.h
engine/renderparticles.o: enet/include/enet/enet.h
engine/renderparticles.o: enet/include/enet/enet/unix.h
engine/renderparticles.o: enet/include/enet/enet/types.h
engine/renderparticles.o: enet/include/enet/enet/protocol.h
engine/renderparticles.o: enet/include/enet/enet/list.h
engine/renderparticles.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/renderparticles.o: shared/geom.h shared/ents.h shared/command.h
engine/renderparticles.o: shared/iengine.h mod/compat.h mod/mod.h
engine/renderparticles.o: mod/compiler.h mod/strtool.h mod/crypto.h
engine/renderparticles.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/renderparticles.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/renderparticles.o: mod/demorecorder.h mod/chat.h mod/events.h
engine/renderparticles.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/renderparticles.o: shared/igame.h engine/world.h engine/glexts.h
engine/renderparticles.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/renderparticles.o: engine/texture.h engine/model.h engine/varray.h
engine/renderparticles.o: engine/rendertarget.h engine/depthfx.h
engine/renderparticles.o: engine/explosion.h engine/lensflare.h
engine/renderparticles.o: engine/lightning.h
engine/rendersky.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/rendersky.o: enet/include/enet/enet/unix.h
engine/rendersky.o: enet/include/enet/enet/types.h
engine/rendersky.o: enet/include/enet/enet/protocol.h
engine/rendersky.o: enet/include/enet/enet/list.h
engine/rendersky.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/rendersky.o: shared/geom.h shared/ents.h shared/command.h
engine/rendersky.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/rendersky.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/rendersky.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/rendersky.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/rendersky.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/rendersky.o: shared/igame.h engine/world.h engine/glexts.h
engine/rendersky.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/rendersky.o: engine/texture.h engine/model.h engine/varray.h
engine/rendertext.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/rendertext.o: enet/include/enet/enet/unix.h
engine/rendertext.o: enet/include/enet/enet/types.h
engine/rendertext.o: enet/include/enet/enet/protocol.h
engine/rendertext.o: enet/include/enet/enet/list.h
engine/rendertext.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/rendertext.o: shared/geom.h shared/ents.h shared/command.h
engine/rendertext.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/rendertext.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/rendertext.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/rendertext.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/rendertext.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/rendertext.o: shared/igame.h engine/world.h engine/glexts.h
engine/rendertext.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/rendertext.o: engine/texture.h engine/model.h engine/varray.h
engine/renderva.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/renderva.o: enet/include/enet/enet/unix.h
engine/renderva.o: enet/include/enet/enet/types.h
engine/renderva.o: enet/include/enet/enet/protocol.h
engine/renderva.o: enet/include/enet/enet/list.h
engine/renderva.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/renderva.o: shared/geom.h shared/ents.h shared/command.h
engine/renderva.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/renderva.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/renderva.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/renderva.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/renderva.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/renderva.o: shared/igame.h engine/world.h engine/glexts.h
engine/renderva.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/renderva.o: engine/texture.h engine/model.h engine/varray.h
engine/server.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/server.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/server.o: enet/include/enet/enet/protocol.h
engine/server.o: enet/include/enet/enet/list.h
engine/server.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/server.o: shared/geom.h shared/ents.h shared/command.h
engine/server.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/server.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/server.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/server.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/server.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/server.o: shared/igame.h engine/world.h engine/glexts.h engine/octa.h
engine/server.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/server.o: engine/model.h engine/varray.h
engine/serverbrowser.o: engine/engine.h shared/cube.h
engine/serverbrowser.o: enet/include/enet/enet.h
engine/serverbrowser.o: enet/include/enet/enet/unix.h
engine/serverbrowser.o: enet/include/enet/enet/types.h
engine/serverbrowser.o: enet/include/enet/enet/protocol.h
engine/serverbrowser.o: enet/include/enet/enet/list.h
engine/serverbrowser.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/serverbrowser.o: shared/geom.h shared/ents.h shared/command.h
engine/serverbrowser.o: shared/iengine.h mod/compat.h mod/mod.h
engine/serverbrowser.o: mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
engine/serverbrowser.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
engine/serverbrowser.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/serverbrowser.o: mod/chat.h mod/events.h mod/thread.h
engine/serverbrowser.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/serverbrowser.o: engine/world.h engine/glexts.h engine/octa.h
engine/serverbrowser.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/serverbrowser.o: engine/model.h engine/varray.h
engine/shader.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/shader.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/shader.o: enet/include/enet/enet/protocol.h
engine/shader.o: enet/include/enet/enet/list.h
engine/shader.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/shader.o: shared/geom.h shared/ents.h shared/command.h
engine/shader.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/shader.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/shader.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/shader.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/shader.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/shader.o: shared/igame.h engine/world.h engine/glexts.h engine/octa.h
engine/shader.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/shader.o: engine/model.h engine/varray.h
engine/shadowmap.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/shadowmap.o: enet/include/enet/enet/unix.h
engine/shadowmap.o: enet/include/enet/enet/types.h
engine/shadowmap.o: enet/include/enet/enet/protocol.h
engine/shadowmap.o: enet/include/enet/enet/list.h
engine/shadowmap.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/shadowmap.o: shared/geom.h shared/ents.h shared/command.h
engine/shadowmap.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/shadowmap.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/shadowmap.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/shadowmap.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/shadowmap.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/shadowmap.o: shared/igame.h engine/world.h engine/glexts.h
engine/shadowmap.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/shadowmap.o: engine/texture.h engine/model.h engine/varray.h
engine/shadowmap.o: engine/rendertarget.h
engine/sound.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/sound.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/sound.o: enet/include/enet/enet/protocol.h
engine/sound.o: enet/include/enet/enet/list.h
engine/sound.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/sound.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
engine/sound.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/sound.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/sound.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/sound.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/sound.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/sound.o: engine/world.h engine/glexts.h engine/octa.h
engine/sound.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/sound.o: engine/model.h engine/varray.h
engine/texture.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/texture.o: enet/include/enet/enet/unix.h
engine/texture.o: enet/include/enet/enet/types.h
engine/texture.o: enet/include/enet/enet/protocol.h
engine/texture.o: enet/include/enet/enet/list.h
engine/texture.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/texture.o: shared/geom.h shared/ents.h shared/command.h
engine/texture.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/texture.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/texture.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/texture.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/texture.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/texture.o: shared/igame.h engine/world.h engine/glexts.h engine/octa.h
engine/texture.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/texture.o: engine/model.h engine/varray.h engine/scale.h
engine/water.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/water.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/water.o: enet/include/enet/enet/protocol.h
engine/water.o: enet/include/enet/enet/list.h
engine/water.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/water.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
engine/water.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/water.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/water.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/water.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/water.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/water.o: engine/world.h engine/glexts.h engine/octa.h
engine/water.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/water.o: engine/model.h engine/varray.h
engine/world.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/world.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
engine/world.o: enet/include/enet/enet/protocol.h
engine/world.o: enet/include/enet/enet/list.h
engine/world.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/world.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
engine/world.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/world.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/world.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/world.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/world.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/world.o: engine/world.h engine/glexts.h engine/octa.h
engine/world.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/world.o: engine/model.h engine/varray.h
engine/worldio.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/worldio.o: enet/include/enet/enet/unix.h
engine/worldio.o: enet/include/enet/enet/types.h
engine/worldio.o: enet/include/enet/enet/protocol.h
engine/worldio.o: enet/include/enet/enet/list.h
engine/worldio.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/worldio.o: shared/geom.h shared/ents.h shared/command.h
engine/worldio.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/worldio.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/worldio.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/worldio.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/worldio.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/worldio.o: shared/igame.h engine/world.h engine/glexts.h engine/octa.h
engine/worldio.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/worldio.o: engine/model.h engine/varray.h
fpsgame/ai.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/ai.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
fpsgame/ai.o: enet/include/enet/enet/protocol.h enet/include/enet/enet/list.h
fpsgame/ai.o: enet/include/enet/enet/callbacks.h shared/tools.h shared/geom.h
fpsgame/ai.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
fpsgame/ai.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
fpsgame/ai.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
fpsgame/ai.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
fpsgame/ai.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
fpsgame/ai.o: shared/igame.h fpsgame/ai.h
fpsgame/client.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/client.o: enet/include/enet/enet/unix.h
fpsgame/client.o: enet/include/enet/enet/types.h
fpsgame/client.o: enet/include/enet/enet/protocol.h
fpsgame/client.o: enet/include/enet/enet/list.h
fpsgame/client.o: enet/include/enet/enet/callbacks.h shared/tools.h
fpsgame/client.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/client.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
fpsgame/client.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/client.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
fpsgame/client.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
fpsgame/client.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
fpsgame/client.o: shared/igame.h fpsgame/ai.h fpsgame/capture.h fpsgame/ctf.h
fpsgame/client.o: fpsgame/collect.h mod/cubescript.h
fpsgame/entities.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/entities.o: enet/include/enet/enet/unix.h
fpsgame/entities.o: enet/include/enet/enet/types.h
fpsgame/entities.o: enet/include/enet/enet/protocol.h
fpsgame/entities.o: enet/include/enet/enet/list.h
fpsgame/entities.o: enet/include/enet/enet/callbacks.h shared/tools.h
fpsgame/entities.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/entities.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
fpsgame/entities.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/entities.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
fpsgame/entities.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
fpsgame/entities.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
fpsgame/entities.o: shared/igame.h fpsgame/ai.h
fpsgame/fps.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/fps.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
fpsgame/fps.o: enet/include/enet/enet/protocol.h
fpsgame/fps.o: enet/include/enet/enet/list.h
fpsgame/fps.o: enet/include/enet/enet/callbacks.h shared/tools.h
fpsgame/fps.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
fpsgame/fps.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
fpsgame/fps.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
fpsgame/fps.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
fpsgame/fps.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
fpsgame/fps.o: mod/proxy-detection.h mod/plugin.h shared/igame.h fpsgame/ai.h
fpsgame/monster.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/monster.o: enet/include/enet/enet/unix.h
fpsgame/monster.o: enet/include/enet/enet/types.h
fpsgame/monster.o: enet/include/enet/enet/protocol.h
fpsgame/monster.o: enet/include/enet/enet/list.h
fpsgame/monster.o: enet/include/enet/enet/callbacks.h shared/tools.h
fpsgame/monster.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/monster.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
fpsgame/monster.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/monster.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
fpsgame/monster.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
fpsgame/monster.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
fpsgame/monster.o: shared/igame.h fpsgame/ai.h
fpsgame/movable.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/movable.o: enet/include/enet/enet/unix.h
fpsgame/movable.o: enet/include/enet/enet/types.h
fpsgame/movable.o: enet/include/enet/enet/protocol.h
fpsgame/movable.o: enet/include/enet/enet/list.h
fpsgame/movable.o: enet/include/enet/enet/callbacks.h shared/tools.h
fpsgame/movable.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/movable.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
fpsgame/movable.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/movable.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
fpsgame/movable.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
fpsgame/movable.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
fpsgame/movable.o: shared/igame.h fpsgame/ai.h
fpsgame/render.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/render.o: enet/include/enet/enet/unix.h
fpsgame/render.o: enet/include/enet/enet/types.h
fpsgame/render.o: enet/include/enet/enet/protocol.h
fpsgame/render.o: enet/include/enet/enet/list.h
fpsgame/render.o: enet/include/enet/enet/callbacks.h shared/tools.h
fpsgame/render.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/render.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
fpsgame/render.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/render.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
fpsgame/render.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
fpsgame/render.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
fpsgame/render.o: shared/igame.h fpsgame/ai.h
fpsgame/scoreboard.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/scoreboard.o: enet/include/enet/enet/unix.h
fpsgame/scoreboard.o: enet/include/enet/enet/types.h
fpsgame/scoreboard.o: enet/include/enet/enet/protocol.h
fpsgame/scoreboard.o: enet/include/enet/enet/list.h
fpsgame/scoreboard.o: enet/include/enet/enet/callbacks.h shared/tools.h
fpsgame/scoreboard.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/scoreboard.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
fpsgame/scoreboard.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/scoreboard.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
fpsgame/scoreboard.o: mod/extinfo.h mod/demorecorder.h mod/chat.h
fpsgame/scoreboard.o: mod/events.h mod/thread.h mod/proxy-detection.h
fpsgame/scoreboard.o: mod/plugin.h shared/igame.h fpsgame/ai.h
fpsgame/server.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/server.o: enet/include/enet/enet/unix.h
fpsgame/server.o: enet/include/enet/enet/types.h
fpsgame/server.o: enet/include/enet/enet/protocol.h
fpsgame/server.o: enet/include/enet/enet/list.h
fpsgame/server.o: enet/include/enet/enet/callbacks.h shared/tools.h
fpsgame/server.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/server.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
fpsgame/server.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/server.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
fpsgame/server.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
fpsgame/server.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
fpsgame/server.o: shared/igame.h fpsgame/ai.h fpsgame/capture.h fpsgame/ctf.h
fpsgame/server.o: fpsgame/collect.h fpsgame/aiman.h
fpsgame/waypoint.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/waypoint.o: enet/include/enet/enet/unix.h
fpsgame/waypoint.o: enet/include/enet/enet/types.h
fpsgame/waypoint.o: enet/include/enet/enet/protocol.h
fpsgame/waypoint.o: enet/include/enet/enet/list.h
fpsgame/waypoint.o: enet/include/enet/enet/callbacks.h shared/tools.h
fpsgame/waypoint.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/waypoint.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
fpsgame/waypoint.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/waypoint.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
fpsgame/waypoint.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
fpsgame/waypoint.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
fpsgame/waypoint.o: shared/igame.h fpsgame/ai.h
fpsgame/weapon.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/weapon.o: enet/include/enet/enet/unix.h
fpsgame/weapon.o: enet/include/enet/enet/types.h
fpsgame/weapon.o: enet/include/enet/enet/protocol.h
fpsgame/weapon.o: enet/include/enet/enet/list.h
fpsgame/weapon.o: enet/include/enet/enet/callbacks.h shared/tools.h
fpsgame/weapon.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/weapon.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
fpsgame/weapon.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/weapon.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
fpsgame/weapon.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
fpsgame/weapon.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
fpsgame/weapon.o: shared/igame.h fpsgame/ai.h
mod/plugin.o: shared/cube.h enet/include/enet/enet.h
mod/plugin.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
mod/plugin.o: enet/include/enet/enet/protocol.h enet/include/enet/enet/list.h
mod/plugin.o: enet/include/enet/enet/callbacks.h shared/tools.h shared/geom.h
mod/plugin.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/plugin.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
mod/plugin.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
mod/plugin.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
mod/plugin.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/plugin.o: shared/igame.h
mod/demorecorder.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/demorecorder.o: enet/include/enet/enet/unix.h
mod/demorecorder.o: enet/include/enet/enet/types.h
mod/demorecorder.o: enet/include/enet/enet/protocol.h
mod/demorecorder.o: enet/include/enet/enet/list.h
mod/demorecorder.o: enet/include/enet/enet/callbacks.h shared/tools.h
mod/demorecorder.o: shared/geom.h shared/ents.h shared/command.h
mod/demorecorder.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
mod/demorecorder.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/demorecorder.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
mod/demorecorder.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
mod/demorecorder.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/demorecorder.o: shared/igame.h fpsgame/ai.h
mod/chat.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/chat.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
mod/chat.o: enet/include/enet/enet/protocol.h enet/include/enet/enet/list.h
mod/chat.o: enet/include/enet/enet/callbacks.h shared/tools.h shared/geom.h
mod/chat.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/chat.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
mod/chat.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
mod/chat.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
mod/chat.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/chat.o: shared/igame.h fpsgame/ai.h mod/chat-ca-certs.h
mod/events.o: shared/cube.h enet/include/enet/enet.h
mod/events.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
mod/events.o: enet/include/enet/enet/protocol.h enet/include/enet/enet/list.h
mod/events.o: enet/include/enet/enet/callbacks.h shared/tools.h shared/geom.h
mod/events.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/events.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
mod/events.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
mod/events.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
mod/events.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/events.o: shared/igame.h
mod/extinfo.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/extinfo.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
mod/extinfo.o: enet/include/enet/enet/protocol.h
mod/extinfo.o: enet/include/enet/enet/list.h
mod/extinfo.o: enet/include/enet/enet/callbacks.h shared/tools.h
mod/extinfo.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
mod/extinfo.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
mod/extinfo.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
mod/extinfo.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
mod/extinfo.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
mod/extinfo.o: mod/proxy-detection.h mod/plugin.h shared/igame.h fpsgame/ai.h
mod/gamemod.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/gamemod.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
mod/gamemod.o: enet/include/enet/enet/protocol.h
mod/gamemod.o: enet/include/enet/enet/list.h
mod/gamemod.o: enet/include/enet/enet/callbacks.h shared/tools.h
mod/gamemod.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
mod/gamemod.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
mod/gamemod.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
mod/gamemod.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
mod/gamemod.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
mod/gamemod.o: mod/proxy-detection.h mod/plugin.h shared/igame.h fpsgame/ai.h
mod/geoip.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/geoip.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
mod/geoip.o: enet/include/enet/enet/protocol.h enet/include/enet/enet/list.h
mod/geoip.o: enet/include/enet/enet/callbacks.h shared/tools.h shared/geom.h
mod/geoip.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/geoip.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
mod/geoip.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
mod/geoip.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
mod/geoip.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/geoip.o: shared/igame.h fpsgame/ai.h
mod/ipignore.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/ipignore.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
mod/ipignore.o: enet/include/enet/enet/protocol.h
mod/ipignore.o: enet/include/enet/enet/list.h
mod/ipignore.o: enet/include/enet/enet/callbacks.h shared/tools.h
mod/ipignore.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
mod/ipignore.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
mod/ipignore.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
mod/ipignore.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
mod/ipignore.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
mod/ipignore.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
mod/ipignore.o: fpsgame/ai.h
mod/mod.o: shared/cube.h enet/include/enet/enet.h
mod/mod.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
mod/mod.o: enet/include/enet/enet/protocol.h enet/include/enet/enet/list.h
mod/mod.o: enet/include/enet/enet/callbacks.h shared/tools.h shared/geom.h
mod/mod.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/mod.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
mod/mod.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
mod/mod.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
mod/mod.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/mod.o: shared/igame.h last_sauer_svn_rev.h
mod/cubescript.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/cubescript.o: enet/include/enet/enet/unix.h
mod/cubescript.o: enet/include/enet/enet/types.h
mod/cubescript.o: enet/include/enet/enet/protocol.h
mod/cubescript.o: enet/include/enet/enet/list.h
mod/cubescript.o: enet/include/enet/enet/callbacks.h shared/tools.h
mod/cubescript.o: shared/geom.h shared/ents.h shared/command.h
mod/cubescript.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
mod/cubescript.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/cubescript.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
mod/cubescript.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
mod/cubescript.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/cubescript.o: shared/igame.h fpsgame/ai.h
mod/hwdisplay.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/hwdisplay.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
mod/hwdisplay.o: enet/include/enet/enet/protocol.h
mod/hwdisplay.o: enet/include/enet/enet/list.h
mod/hwdisplay.o: enet/include/enet/enet/callbacks.h shared/tools.h
mod/hwdisplay.o: shared/geom.h shared/ents.h shared/command.h
mod/hwdisplay.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
mod/hwdisplay.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/hwdisplay.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
mod/hwdisplay.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
mod/hwdisplay.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/hwdisplay.o: shared/igame.h fpsgame/ai.h
mod/playerdisplay.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/playerdisplay.o: enet/include/enet/enet/unix.h
mod/playerdisplay.o: enet/include/enet/enet/types.h
mod/playerdisplay.o: enet/include/enet/enet/protocol.h
mod/playerdisplay.o: enet/include/enet/enet/list.h
mod/playerdisplay.o: enet/include/enet/enet/callbacks.h shared/tools.h
mod/playerdisplay.o: shared/geom.h shared/ents.h shared/command.h
mod/playerdisplay.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
mod/playerdisplay.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/playerdisplay.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
mod/playerdisplay.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
mod/playerdisplay.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/playerdisplay.o: shared/igame.h fpsgame/ai.h
mod/http.o: shared/cube.h enet/include/enet/enet.h
mod/http.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
mod/http.o: enet/include/enet/enet/protocol.h enet/include/enet/enet/list.h
mod/http.o: enet/include/enet/enet/callbacks.h shared/tools.h shared/geom.h
mod/http.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/http.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
mod/http.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
mod/http.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
mod/http.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/http.o: shared/igame.h
mod/strtool.o: shared/cube.h enet/include/enet/enet.h
mod/strtool.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
mod/strtool.o: enet/include/enet/enet/protocol.h
mod/strtool.o: enet/include/enet/enet/list.h
mod/strtool.o: enet/include/enet/enet/callbacks.h shared/tools.h
mod/strtool.o: shared/geom.h shared/ents.h shared/command.h shared/iengine.h
mod/strtool.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
mod/strtool.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
mod/strtool.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
mod/strtool.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
mod/strtool.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
mod/crypto.o: shared/cube.h enet/include/enet/enet.h
mod/crypto.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
mod/crypto.o: enet/include/enet/enet/protocol.h enet/include/enet/enet/list.h
mod/crypto.o: enet/include/enet/enet/callbacks.h shared/tools.h shared/geom.h
mod/crypto.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/crypto.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
mod/crypto.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
mod/crypto.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
mod/crypto.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/crypto.o: shared/igame.h
mod/extinfo-playerpreview.o: fpsgame/game.h shared/cube.h
mod/extinfo-playerpreview.o: enet/include/enet/enet.h
mod/extinfo-playerpreview.o: enet/include/enet/enet/unix.h
mod/extinfo-playerpreview.o: enet/include/enet/enet/types.h
mod/extinfo-playerpreview.o: enet/include/enet/enet/protocol.h
mod/extinfo-playerpreview.o: enet/include/enet/enet/list.h
mod/extinfo-playerpreview.o: enet/include/enet/enet/callbacks.h
mod/extinfo-playerpreview.o: shared/tools.h shared/geom.h shared/ents.h
mod/extinfo-playerpreview.o: shared/command.h shared/iengine.h mod/compat.h
mod/extinfo-playerpreview.o: mod/mod.h mod/compiler.h mod/strtool.h
mod/extinfo-playerpreview.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/extinfo-playerpreview.o: mod/gamemod.h mod/http.h mod/geoip.h
mod/extinfo-playerpreview.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h
mod/extinfo-playerpreview.o: mod/chat.h mod/events.h mod/thread.h
mod/extinfo-playerpreview.o: mod/proxy-detection.h mod/plugin.h
mod/extinfo-playerpreview.o: shared/igame.h fpsgame/ai.h
mod/ipbuf.o: shared/cube.h enet/include/enet/enet.h
mod/ipbuf.o: enet/include/enet/enet/unix.h enet/include/enet/enet/types.h
mod/ipbuf.o: enet/include/enet/enet/protocol.h enet/include/enet/enet/list.h
mod/ipbuf.o: enet/include/enet/enet/callbacks.h shared/tools.h shared/geom.h
mod/ipbuf.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/ipbuf.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
mod/ipbuf.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
mod/ipbuf.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
mod/ipbuf.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/ipbuf.o: shared/igame.h
mod/proxy-detection.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/proxy-detection.o: enet/include/enet/enet/unix.h
mod/proxy-detection.o: enet/include/enet/enet/types.h
mod/proxy-detection.o: enet/include/enet/enet/protocol.h
mod/proxy-detection.o: enet/include/enet/enet/list.h
mod/proxy-detection.o: enet/include/enet/enet/callbacks.h shared/tools.h
mod/proxy-detection.o: shared/geom.h shared/ents.h shared/command.h
mod/proxy-detection.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
mod/proxy-detection.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/proxy-detection.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
mod/proxy-detection.o: mod/extinfo.h mod/demorecorder.h mod/chat.h
mod/proxy-detection.o: mod/events.h mod/thread.h mod/proxy-detection.h
mod/proxy-detection.o: mod/plugin.h shared/igame.h fpsgame/ai.h
plugins/g15lcd/g15lcd.o: shared/cube.h enet/include/enet/enet.h
plugins/g15lcd/g15lcd.o: enet/include/enet/enet/unix.h
plugins/g15lcd/g15lcd.o: enet/include/enet/enet/types.h
plugins/g15lcd/g15lcd.o: enet/include/enet/enet/protocol.h
plugins/g15lcd/g15lcd.o: enet/include/enet/enet/list.h
plugins/g15lcd/g15lcd.o: enet/include/enet/enet/callbacks.h shared/tools.h
plugins/g15lcd/g15lcd.o: shared/geom.h shared/ents.h shared/command.h
plugins/g15lcd/g15lcd.o: shared/iengine.h mod/compat.h mod/mod.h
plugins/g15lcd/g15lcd.o: mod/compiler.h mod/strtool.h mod/crypto.h
plugins/g15lcd/g15lcd.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
plugins/g15lcd/g15lcd.o: mod/geoip.h mod/ipignore.h mod/extinfo.h
plugins/g15lcd/g15lcd.o: mod/demorecorder.h mod/chat.h mod/events.h
plugins/g15lcd/g15lcd.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
plugins/g15lcd/g15lcd.o: shared/igame.h plugins/g15lcd/g15sdkwrapper.h
plugins/hwtemp/hwtemp.o: plugins/hwtemp/hwtemp.h shared/cube.h
plugins/hwtemp/hwtemp.o: enet/include/enet/enet.h
plugins/hwtemp/hwtemp.o: enet/include/enet/enet/unix.h
plugins/hwtemp/hwtemp.o: enet/include/enet/enet/types.h
plugins/hwtemp/hwtemp.o: enet/include/enet/enet/protocol.h
plugins/hwtemp/hwtemp.o: enet/include/enet/enet/list.h
plugins/hwtemp/hwtemp.o: enet/include/enet/enet/callbacks.h shared/tools.h
plugins/hwtemp/hwtemp.o: shared/geom.h shared/ents.h shared/command.h
plugins/hwtemp/hwtemp.o: shared/iengine.h mod/compat.h mod/mod.h
plugins/hwtemp/hwtemp.o: mod/compiler.h mod/strtool.h mod/crypto.h
plugins/hwtemp/hwtemp.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
plugins/hwtemp/hwtemp.o: mod/geoip.h mod/ipignore.h mod/extinfo.h
plugins/hwtemp/hwtemp.o: mod/demorecorder.h mod/chat.h mod/events.h
plugins/hwtemp/hwtemp.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
plugins/hwtemp/hwtemp.o: shared/igame.h

shared/cube.h.gch: enet/include/enet/enet.h enet/include/enet/enet/unix.h
shared/cube.h.gch: enet/include/enet/enet/types.h
shared/cube.h.gch: enet/include/enet/enet/protocol.h
shared/cube.h.gch: enet/include/enet/enet/list.h
shared/cube.h.gch: enet/include/enet/enet/callbacks.h shared/tools.h
shared/cube.h.gch: shared/geom.h shared/ents.h shared/command.h
shared/cube.h.gch: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
shared/cube.h.gch: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
shared/cube.h.gch: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
shared/cube.h.gch: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
shared/cube.h.gch: mod/thread.h mod/proxy-detection.h mod/plugin.h
shared/cube.h.gch: shared/igame.h
engine/engine.h.gch: shared/cube.h enet/include/enet/enet.h
engine/engine.h.gch: enet/include/enet/enet/unix.h
engine/engine.h.gch: enet/include/enet/enet/types.h
engine/engine.h.gch: enet/include/enet/enet/protocol.h
engine/engine.h.gch: enet/include/enet/enet/list.h
engine/engine.h.gch: enet/include/enet/enet/callbacks.h shared/tools.h
engine/engine.h.gch: shared/geom.h shared/ents.h shared/command.h
engine/engine.h.gch: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
engine/engine.h.gch: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/engine.h.gch: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
engine/engine.h.gch: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
engine/engine.h.gch: mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/engine.h.gch: shared/igame.h engine/world.h engine/glexts.h
engine/engine.h.gch: engine/octa.h engine/lightmap.h engine/bih.h
engine/engine.h.gch: engine/texture.h engine/model.h engine/varray.h
fpsgame/game.h.gch: shared/cube.h enet/include/enet/enet.h
fpsgame/game.h.gch: enet/include/enet/enet/unix.h
fpsgame/game.h.gch: enet/include/enet/enet/types.h
fpsgame/game.h.gch: enet/include/enet/enet/protocol.h
fpsgame/game.h.gch: enet/include/enet/enet/list.h
fpsgame/game.h.gch: enet/include/enet/enet/callbacks.h shared/tools.h
fpsgame/game.h.gch: shared/geom.h shared/ents.h shared/command.h
fpsgame/game.h.gch: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
fpsgame/game.h.gch: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/game.h.gch: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
fpsgame/game.h.gch: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
fpsgame/game.h.gch: mod/thread.h mod/proxy-detection.h mod/plugin.h
fpsgame/game.h.gch: shared/igame.h fpsgame/ai.h

mod/strtool-standalone.o: shared/cube.h enet/include/enet/enet.h
mod/strtool-standalone.o: enet/include/enet/enet/unix.h
mod/strtool-standalone.o: enet/include/enet/enet/types.h
mod/strtool-standalone.o: enet/include/enet/enet/protocol.h
mod/strtool-standalone.o: enet/include/enet/enet/list.h
mod/strtool-standalone.o: enet/include/enet/enet/callbacks.h shared/tools.h
mod/strtool-standalone.o: shared/geom.h shared/ents.h shared/command.h
mod/strtool-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
mod/strtool-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
mod/strtool-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
shared/crypto-standalone.o: shared/cube.h enet/include/enet/enet.h
shared/crypto-standalone.o: enet/include/enet/enet/unix.h
shared/crypto-standalone.o: enet/include/enet/enet/types.h
shared/crypto-standalone.o: enet/include/enet/enet/protocol.h
shared/crypto-standalone.o: enet/include/enet/enet/list.h
shared/crypto-standalone.o: enet/include/enet/enet/callbacks.h shared/tools.h
shared/crypto-standalone.o: shared/geom.h shared/ents.h shared/command.h
shared/crypto-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
shared/crypto-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
shared/crypto-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
shared/stream-standalone.o: shared/cube.h enet/include/enet/enet.h
shared/stream-standalone.o: enet/include/enet/enet/unix.h
shared/stream-standalone.o: enet/include/enet/enet/types.h
shared/stream-standalone.o: enet/include/enet/enet/protocol.h
shared/stream-standalone.o: enet/include/enet/enet/list.h
shared/stream-standalone.o: enet/include/enet/enet/callbacks.h shared/tools.h
shared/stream-standalone.o: shared/geom.h shared/ents.h shared/command.h
shared/stream-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
shared/stream-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
shared/stream-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
shared/tools-standalone.o: shared/cube.h enet/include/enet/enet.h
shared/tools-standalone.o: enet/include/enet/enet/unix.h
shared/tools-standalone.o: enet/include/enet/enet/types.h
shared/tools-standalone.o: enet/include/enet/enet/protocol.h
shared/tools-standalone.o: enet/include/enet/enet/list.h
shared/tools-standalone.o: enet/include/enet/enet/callbacks.h shared/tools.h
shared/tools-standalone.o: shared/geom.h shared/ents.h shared/command.h
shared/tools-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
shared/tools-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
shared/tools-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
engine/command-standalone.o: engine/engine.h shared/cube.h
engine/command-standalone.o: enet/include/enet/enet.h
engine/command-standalone.o: enet/include/enet/enet/unix.h
engine/command-standalone.o: enet/include/enet/enet/types.h
engine/command-standalone.o: enet/include/enet/enet/protocol.h
engine/command-standalone.o: enet/include/enet/enet/list.h
engine/command-standalone.o: enet/include/enet/enet/callbacks.h
engine/command-standalone.o: shared/tools.h shared/geom.h shared/ents.h
engine/command-standalone.o: shared/command.h shared/iengine.h mod/compat.h
engine/command-standalone.o: mod/mod.h mod/compiler.h mod/strtool.h
engine/command-standalone.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/command-standalone.o: shared/igame.h engine/world.h
engine/server-standalone.o: engine/engine.h shared/cube.h
engine/server-standalone.o: enet/include/enet/enet.h
engine/server-standalone.o: enet/include/enet/enet/unix.h
engine/server-standalone.o: enet/include/enet/enet/types.h
engine/server-standalone.o: enet/include/enet/enet/protocol.h
engine/server-standalone.o: enet/include/enet/enet/list.h
engine/server-standalone.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/server-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/server-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
engine/server-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
engine/server-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
engine/server-standalone.o: engine/world.h
engine/worldio-standalone.o: engine/engine.h shared/cube.h
engine/worldio-standalone.o: enet/include/enet/enet.h
engine/worldio-standalone.o: enet/include/enet/enet/unix.h
engine/worldio-standalone.o: enet/include/enet/enet/types.h
engine/worldio-standalone.o: enet/include/enet/enet/protocol.h
engine/worldio-standalone.o: enet/include/enet/enet/list.h
engine/worldio-standalone.o: enet/include/enet/enet/callbacks.h
engine/worldio-standalone.o: shared/tools.h shared/geom.h shared/ents.h
engine/worldio-standalone.o: shared/command.h shared/iengine.h mod/compat.h
engine/worldio-standalone.o: mod/mod.h mod/compiler.h mod/strtool.h
engine/worldio-standalone.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h
engine/worldio-standalone.o: shared/igame.h engine/world.h
fpsgame/entities-standalone.o: fpsgame/game.h shared/cube.h
fpsgame/entities-standalone.o: enet/include/enet/enet.h
fpsgame/entities-standalone.o: enet/include/enet/enet/unix.h
fpsgame/entities-standalone.o: enet/include/enet/enet/types.h
fpsgame/entities-standalone.o: enet/include/enet/enet/protocol.h
fpsgame/entities-standalone.o: enet/include/enet/enet/list.h
fpsgame/entities-standalone.o: enet/include/enet/enet/callbacks.h
fpsgame/entities-standalone.o: shared/tools.h shared/geom.h shared/ents.h
fpsgame/entities-standalone.o: shared/command.h shared/iengine.h mod/compat.h
fpsgame/entities-standalone.o: mod/mod.h mod/compiler.h mod/strtool.h
fpsgame/entities-standalone.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/entities-standalone.o: shared/igame.h fpsgame/ai.h
fpsgame/server-standalone.o: fpsgame/game.h shared/cube.h
fpsgame/server-standalone.o: enet/include/enet/enet.h
fpsgame/server-standalone.o: enet/include/enet/enet/unix.h
fpsgame/server-standalone.o: enet/include/enet/enet/types.h
fpsgame/server-standalone.o: enet/include/enet/enet/protocol.h
fpsgame/server-standalone.o: enet/include/enet/enet/list.h
fpsgame/server-standalone.o: enet/include/enet/enet/callbacks.h
fpsgame/server-standalone.o: shared/tools.h shared/geom.h shared/ents.h
fpsgame/server-standalone.o: shared/command.h shared/iengine.h mod/compat.h
fpsgame/server-standalone.o: mod/mod.h mod/compiler.h mod/strtool.h
fpsgame/server-standalone.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/server-standalone.o: shared/igame.h fpsgame/ai.h fpsgame/capture.h
fpsgame/server-standalone.o: fpsgame/ctf.h fpsgame/collect.h
fpsgame/server-standalone.o: fpsgame/extinfo.h fpsgame/aiman.h

engine/master-standalone.o: shared/cube.h enet/include/enet/enet.h
engine/master-standalone.o: enet/include/enet/enet/unix.h
engine/master-standalone.o: enet/include/enet/enet/types.h
engine/master-standalone.o: enet/include/enet/enet/protocol.h
engine/master-standalone.o: enet/include/enet/enet/list.h
engine/master-standalone.o: enet/include/enet/enet/callbacks.h shared/tools.h
engine/master-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/master-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
engine/master-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
engine/master-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
engine/master-standalone.o: enet/include/enet/time.h
