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
  override ENET_CFLAGS+= $(shell ./build/enet_cflags.sh $(CC))
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
    CHATSERV_INCLUDES+= -Ilib/include
    CHATSERV_LIBS+= $(STATIC_LIB_DIR)/libSDL.a
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

    CHATSERV_INCLUDES+= -Ilib/include/SDL1
    CLIENT_INCLUDES+= -Ilib/include -Ilib/include/SDL1
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libcurl.a $(STATIC_LIB_DIR)/libmaxminddb.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libssl.a $(STATIC_LIB_DIR)/libcrypto.a
  else
    CLIENT_LIBS+= -lcurl -lssl -lcrypto -lmaxminddb
  endif

  MASTER_LIBS= $(SERVER_LIBS)

  ################ GENERIC UNIX STUFF END ################
endif

ifneq (,$(findstring mingw,$(PLATFORM)))
  ################ MINGW ################

  ARCH ?=
  MINGWPREFIX ?=
  MINGWCLANGPREFIX ?=

  PLUGIN_LIBS= plugins/exports.def

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

  CLIENT_INCLUDES= $(INCLUDES) -Ilib/include -Ilib/include/SDL1
  SERVER_INCLUDES= -DSTANDALONE $(INCLUDES) -Ilib/include -Ilib/include/SDL1

  SDL_DEP_LIBS= -lgdi32 -ldxguid -lole32 -lwinmm
  SDL_DEP_LIBS+= -limm32 -lversion -loleaut32

  SERVER_LIBS= $(SUBSYSTEM) -L$(WINLIB) -Wl,-Bstatic -lz -Wl,-Bdynamic -lws2_32 -lwinmm
  CHATSERV_LIBS+= -L$(WINLIB)/openssl -Wl,-Bstatic -lssl -lcrypto -Wl,-Bdynamic
  CHATSERV_LIBS+= -mconsole -Wl,-Bstatic -lSDL -lpthread -Wl,-Bdynamic -lws2_32 $(SDL_DEP_LIBS)
  MASTER_LIBS= -L$(WINLIB) -Wl,-Bstatic -lz -Wl,-Bdynamic -lws2_32 -lwinmm

  CLIENT_LIBS+= $(SUBSYSTEM) -L$(WINLIB)
  CLIENT_LIBS+= -Wl,-Bstatic -lmaxminddb -lcurl -lssl -lcrypto -ldl -Wl,-Bdynamic
  CLIENT_LIBS+= -lgdi32

  CLIENT_LIBS+= -Wl,-Bstatic
  CLIENT_LIBS+= -lSDL_mixer -lvorbisfile -lvorbis -logg
  CLIENT_LIBS+= -lSDL -lSDL_image -lpng16 -ljpeg -lz
  CLIENT_LIBS+= -Wl,-Bdynamic
  CLIENT_LIBS+= -lopengl32 -lws2_32 $(SDL_DEP_LIBS)
  CLIENT_LIBS+= -Wl,-Bstatic
  CLIENT_LIBS+= -lpthread
  CLIENT_LIBS+= -Wl,-Bdynamic

  PLUGIN_INCLUDES= $(CLIENT_INCLUDES) -DPLUGIN
  PLUGIN_LIBS+= -L$(WINLIB) -Wl,-Bstatic -lz -lSDL -Wl,-Bdynamic
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

  PLUGIN_LIBS= -Wl,-exported_symbols_list,plugins/exports.ld64

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
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libSDL_mixer.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libSDL.a
  CLIENT_LIBS+= -liconv
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libSDL_image.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libvorbisfile.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libvorbis.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libogg.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libSDL.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libSDL_image.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libpng16.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libjpeg.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libz.a
  CLIENT_LIBS+= $(STATIC_LIB_DIR)/libmaxminddb.a

  PLUGIN_INCLUDES= $(CLIENT_INCLUDES) -fPIC -DPLUGIN -Ilib/include/osx-cpu-temp
  CLIENT_OBJS+= xcode/macutils.o xcode/main.o xcode/Launcher.o

  HWTEMP_LIBS= $(STATIC_LIB_DIR)/smc.a

  ifeq (1, $(LTO))
    HWTEMP_LIBS+= $(STATIC_LIB_DIR)/libSDL.a -framework IOKit
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

  PLUGIN_LIBS= -Wl,--version-script,plugins/exports.ld

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
    CLIENT_LIBS+= -lGL -lX11 -lXext -lXrandr
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libSDL_mixer.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libvorbisfile.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libvorbis.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libogg.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libSDL.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libSDL_image.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libpng16.a
    CLIENT_LIBS+= $(STATIC_LIB_DIR)/libjpeg.a
    PLUGIN_LIBS+= $(STATIC_LIB_DIR)/libSDL.a
    ifeq ($(PLATFORM),freebsd)
      CLIENT_LIBS+= -lvgl -liconv -lpulse -lpulse-simple
    endif
    override SDL_CONFIG= -Ilib/include
  else
    ifeq ($(PLATFORM),$(PLATFORMNATIVE))
      override SDL_CONFIG= `sdl-config --cflags`
      override SDL_CONFIG_LIB= `sdl-config --libs`
    else
      override NATIVE_LIB_PATH=
      override NATIVE_INCLUDES=
      override SDL_CONFIG_LIB=
      ifeq ($(PLATFORM),freebsd)
        override SDL_CONFIG_LIB= -lSDL -pthread
        override SDL_CONFIG= $(shell ./build/quirks/fbsd-cross-sdl-cpath.sh $(CXX))
      endif
   endif

   CHATSERV_INCLUDES+= $(SDL_CONFIG)
   CHATSERV_LIBS+= $(SDL_CONFIG_LIB)
   CLIENT_LIBS+= $(NATIVE_LIB_PATH) -lX11 $(SDL_CONFIG_LIB) -lSDL_image -lSDL_mixer -lGL
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
shared/crypto.o: enet/include/enet/unix.h enet/include/enet/types.h
shared/crypto.o: enet/include/enet/protocol.h enet/include/enet/list.h
shared/crypto.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
shared/crypto.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
shared/crypto.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
shared/crypto.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
shared/crypto.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
shared/crypto.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
shared/crypto.o: mod/plugin.h shared/igame.h
shared/geom.o: shared/cube.h enet/include/enet/enet.h
shared/geom.o: enet/include/enet/unix.h enet/include/enet/types.h
shared/geom.o: enet/include/enet/protocol.h enet/include/enet/list.h
shared/geom.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
shared/geom.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
shared/geom.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
shared/geom.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
shared/geom.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
shared/geom.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
shared/geom.o: mod/plugin.h shared/igame.h
shared/stream.o: shared/cube.h enet/include/enet/enet.h
shared/stream.o: enet/include/enet/unix.h enet/include/enet/types.h
shared/stream.o: enet/include/enet/protocol.h enet/include/enet/list.h
shared/stream.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
shared/stream.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
shared/stream.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
shared/stream.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
shared/stream.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
shared/stream.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
shared/stream.o: mod/plugin.h shared/igame.h
shared/tools.o: shared/cube.h enet/include/enet/enet.h
shared/tools.o: enet/include/enet/unix.h enet/include/enet/types.h
shared/tools.o: enet/include/enet/protocol.h enet/include/enet/list.h
shared/tools.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
shared/tools.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
shared/tools.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
shared/tools.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
shared/tools.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
shared/tools.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
shared/tools.o: mod/plugin.h shared/igame.h
shared/zip.o: shared/cube.h enet/include/enet/enet.h enet/include/enet/unix.h
shared/zip.o: enet/include/enet/types.h enet/include/enet/protocol.h
shared/zip.o: enet/include/enet/list.h enet/include/enet/callbacks.h
shared/zip.o: shared/tools.h shared/geom.h shared/ents.h shared/command.h
shared/zip.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
shared/zip.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
shared/zip.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
shared/zip.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
shared/zip.o: mod/thread.h mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/3dgui.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/3dgui.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/3dgui.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/3dgui.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/3dgui.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/3dgui.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/3dgui.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/3dgui.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/3dgui.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/3dgui.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/3dgui.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/3dgui.o: engine/model.h engine/varray.h engine/textedit.h
engine/bih.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/bih.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/bih.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/bih.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/bih.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/bih.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
engine/bih.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
engine/bih.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
engine/bih.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/bih.o: shared/igame.h engine/world.h engine/glexts.h engine/octa.h
engine/bih.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
engine/bih.o: engine/varray.h
engine/blend.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/blend.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/blend.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/blend.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/blend.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/blend.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/blend.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/blend.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/blend.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/blend.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/blend.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/blend.o: engine/model.h engine/varray.h
engine/blob.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/blob.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/blob.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/blob.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/blob.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/blob.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/blob.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/blob.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/blob.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/blob.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/blob.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/blob.o: engine/model.h engine/varray.h
engine/client.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/client.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/client.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/client.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/client.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/client.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/client.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/client.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/client.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/client.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/client.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/client.o: engine/texture.h engine/model.h engine/varray.h
engine/command.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/command.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/command.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/command.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/command.o: shared/ents.h shared/command.h shared/iengine.h
engine/command.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/command.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/command.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/command.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/command.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/command.o: engine/world.h engine/glexts.h engine/octa.h
engine/command.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/command.o: engine/model.h engine/varray.h
engine/console.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/console.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/console.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/console.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/console.o: shared/ents.h shared/command.h shared/iengine.h
engine/console.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/console.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/console.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/console.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/console.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/console.o: engine/world.h engine/glexts.h engine/octa.h
engine/console.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/console.o: engine/model.h engine/varray.h
engine/cubeloader.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/cubeloader.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/cubeloader.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/cubeloader.o: enet/include/enet/callbacks.h shared/tools.h
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
engine/decal.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/decal.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/decal.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/decal.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/decal.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/decal.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/decal.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/decal.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/decal.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/decal.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/decal.o: engine/model.h engine/varray.h
engine/dynlight.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/dynlight.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/dynlight.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/dynlight.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/dynlight.o: shared/ents.h shared/command.h shared/iengine.h
engine/dynlight.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/dynlight.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/dynlight.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/dynlight.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/dynlight.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/dynlight.o: engine/world.h engine/glexts.h engine/octa.h
engine/dynlight.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/dynlight.o: engine/model.h engine/varray.h
engine/glare.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/glare.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/glare.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/glare.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/glare.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/glare.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/glare.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/glare.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/glare.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/glare.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/glare.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/glare.o: engine/model.h engine/varray.h engine/rendertarget.h
engine/grass.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/grass.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/grass.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/grass.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/grass.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/grass.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/grass.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/grass.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/grass.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/grass.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/grass.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/grass.o: engine/model.h engine/varray.h
engine/lightmap.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/lightmap.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/lightmap.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/lightmap.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/lightmap.o: shared/ents.h shared/command.h shared/iengine.h
engine/lightmap.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/lightmap.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/lightmap.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/lightmap.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/lightmap.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/lightmap.o: engine/world.h engine/glexts.h engine/octa.h
engine/lightmap.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/lightmap.o: engine/model.h engine/varray.h
engine/main.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/main.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/main.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/main.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/main.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/main.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/main.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/main.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/main.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/main.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/main.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/main.o: engine/model.h engine/varray.h
engine/material.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/material.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/material.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/material.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/material.o: shared/ents.h shared/command.h shared/iengine.h
engine/material.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/material.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/material.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/material.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/material.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/material.o: engine/world.h engine/glexts.h engine/octa.h
engine/material.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/material.o: engine/model.h engine/varray.h
engine/menus.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/menus.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/menus.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/menus.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/menus.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/menus.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/menus.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/menus.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/menus.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/menus.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/menus.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/menus.o: engine/model.h engine/varray.h
engine/movie.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/movie.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/movie.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/movie.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/movie.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/movie.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/movie.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/movie.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/movie.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/movie.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/movie.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/movie.o: engine/model.h engine/varray.h
engine/normal.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/normal.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/normal.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/normal.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/normal.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/normal.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/normal.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/normal.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/normal.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/normal.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/normal.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/normal.o: engine/texture.h engine/model.h engine/varray.h
engine/octa.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/octa.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/octa.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/octa.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/octa.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/octa.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/octa.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/octa.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/octa.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/octa.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/octa.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/octa.o: engine/model.h engine/varray.h
engine/octaedit.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/octaedit.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/octaedit.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/octaedit.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/octaedit.o: shared/ents.h shared/command.h shared/iengine.h
engine/octaedit.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/octaedit.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/octaedit.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/octaedit.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/octaedit.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/octaedit.o: engine/world.h engine/glexts.h engine/octa.h
engine/octaedit.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/octaedit.o: engine/model.h engine/varray.h
engine/octarender.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/octarender.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/octarender.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/octarender.o: enet/include/enet/callbacks.h shared/tools.h
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
engine/physics.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/physics.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/physics.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/physics.o: shared/ents.h shared/command.h shared/iengine.h
engine/physics.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/physics.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/physics.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/physics.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/physics.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/physics.o: engine/world.h engine/glexts.h engine/octa.h
engine/physics.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/physics.o: engine/model.h engine/varray.h engine/mpr.h
engine/pvs.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/pvs.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/pvs.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/pvs.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/pvs.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/pvs.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
engine/pvs.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
engine/pvs.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
engine/pvs.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
engine/pvs.o: shared/igame.h engine/world.h engine/glexts.h engine/octa.h
engine/pvs.o: engine/lightmap.h engine/bih.h engine/texture.h engine/model.h
engine/pvs.o: engine/varray.h
engine/rendergl.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/rendergl.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/rendergl.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/rendergl.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/rendergl.o: shared/ents.h shared/command.h shared/iengine.h
engine/rendergl.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/rendergl.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/rendergl.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/rendergl.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/rendergl.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/rendergl.o: engine/world.h engine/glexts.h engine/octa.h
engine/rendergl.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/rendergl.o: engine/model.h engine/varray.h
engine/rendermodel.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/rendermodel.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/rendermodel.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/rendermodel.o: enet/include/enet/callbacks.h shared/tools.h
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
engine/renderparticles.o: enet/include/enet/enet.h enet/include/enet/unix.h
engine/renderparticles.o: enet/include/enet/types.h
engine/renderparticles.o: enet/include/enet/protocol.h
engine/renderparticles.o: enet/include/enet/list.h
engine/renderparticles.o: enet/include/enet/callbacks.h shared/tools.h
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
engine/rendersky.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/rendersky.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/rendersky.o: enet/include/enet/callbacks.h shared/tools.h
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
engine/rendertext.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/rendertext.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/rendertext.o: enet/include/enet/callbacks.h shared/tools.h
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
engine/renderva.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/renderva.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/renderva.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/renderva.o: shared/ents.h shared/command.h shared/iengine.h
engine/renderva.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/renderva.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/renderva.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/renderva.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/renderva.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/renderva.o: engine/world.h engine/glexts.h engine/octa.h
engine/renderva.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/renderva.o: engine/model.h engine/varray.h
engine/server.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/server.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/server.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/server.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/server.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/server.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/server.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/server.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/server.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/server.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/server.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/server.o: engine/texture.h engine/model.h engine/varray.h
engine/serverbrowser.o: engine/engine.h shared/cube.h
engine/serverbrowser.o: enet/include/enet/enet.h enet/include/enet/unix.h
engine/serverbrowser.o: enet/include/enet/types.h
engine/serverbrowser.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/serverbrowser.o: enet/include/enet/callbacks.h shared/tools.h
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
engine/shader.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/shader.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/shader.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/shader.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/shader.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/shader.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/shader.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/shader.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/shader.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/shader.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/shader.o: engine/texture.h engine/model.h engine/varray.h
engine/shadowmap.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/shadowmap.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/shadowmap.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/shadowmap.o: enet/include/enet/callbacks.h shared/tools.h
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
engine/sound.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/sound.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/sound.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/sound.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/sound.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/sound.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/sound.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/sound.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/sound.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/sound.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/sound.o: engine/model.h engine/varray.h
engine/texture.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/texture.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/texture.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/texture.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/texture.o: shared/ents.h shared/command.h shared/iengine.h
engine/texture.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/texture.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/texture.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/texture.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/texture.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/texture.o: engine/world.h engine/glexts.h engine/octa.h
engine/texture.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/texture.o: engine/model.h engine/varray.h engine/scale.h
engine/water.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/water.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/water.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/water.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/water.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/water.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/water.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/water.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/water.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/water.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/water.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/water.o: engine/model.h engine/varray.h
engine/world.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/world.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/world.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/world.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/world.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
engine/world.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
engine/world.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
engine/world.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
engine/world.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
engine/world.o: mod/plugin.h shared/igame.h engine/world.h engine/glexts.h
engine/world.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/world.o: engine/model.h engine/varray.h
engine/worldio.o: engine/engine.h shared/cube.h enet/include/enet/enet.h
engine/worldio.o: enet/include/enet/unix.h enet/include/enet/types.h
engine/worldio.o: enet/include/enet/protocol.h enet/include/enet/list.h
engine/worldio.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
engine/worldio.o: shared/ents.h shared/command.h shared/iengine.h
engine/worldio.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
engine/worldio.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
engine/worldio.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
engine/worldio.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
engine/worldio.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
engine/worldio.o: engine/world.h engine/glexts.h engine/octa.h
engine/worldio.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/worldio.o: engine/model.h engine/varray.h
fpsgame/ai.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/ai.o: enet/include/enet/unix.h enet/include/enet/types.h
fpsgame/ai.o: enet/include/enet/protocol.h enet/include/enet/list.h
fpsgame/ai.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
fpsgame/ai.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
fpsgame/ai.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
fpsgame/ai.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
fpsgame/ai.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
fpsgame/ai.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
fpsgame/ai.o: shared/igame.h fpsgame/ai.h
fpsgame/client.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/client.o: enet/include/enet/unix.h enet/include/enet/types.h
fpsgame/client.o: enet/include/enet/protocol.h enet/include/enet/list.h
fpsgame/client.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
fpsgame/client.o: shared/ents.h shared/command.h shared/iengine.h
fpsgame/client.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
fpsgame/client.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
fpsgame/client.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
fpsgame/client.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
fpsgame/client.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
fpsgame/client.o: fpsgame/ai.h fpsgame/capture.h fpsgame/ctf.h
fpsgame/client.o: fpsgame/collect.h mod/cubescript.h
fpsgame/entities.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/entities.o: enet/include/enet/unix.h enet/include/enet/types.h
fpsgame/entities.o: enet/include/enet/protocol.h enet/include/enet/list.h
fpsgame/entities.o: enet/include/enet/callbacks.h shared/tools.h
fpsgame/entities.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/entities.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
fpsgame/entities.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/entities.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
fpsgame/entities.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
fpsgame/entities.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
fpsgame/entities.o: shared/igame.h fpsgame/ai.h
fpsgame/fps.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/fps.o: enet/include/enet/unix.h enet/include/enet/types.h
fpsgame/fps.o: enet/include/enet/protocol.h enet/include/enet/list.h
fpsgame/fps.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
fpsgame/fps.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
fpsgame/fps.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
fpsgame/fps.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
fpsgame/fps.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
fpsgame/fps.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
fpsgame/fps.o: mod/plugin.h shared/igame.h fpsgame/ai.h
fpsgame/monster.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/monster.o: enet/include/enet/unix.h enet/include/enet/types.h
fpsgame/monster.o: enet/include/enet/protocol.h enet/include/enet/list.h
fpsgame/monster.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
fpsgame/monster.o: shared/ents.h shared/command.h shared/iengine.h
fpsgame/monster.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
fpsgame/monster.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
fpsgame/monster.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
fpsgame/monster.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
fpsgame/monster.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
fpsgame/monster.o: fpsgame/ai.h
fpsgame/movable.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/movable.o: enet/include/enet/unix.h enet/include/enet/types.h
fpsgame/movable.o: enet/include/enet/protocol.h enet/include/enet/list.h
fpsgame/movable.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
fpsgame/movable.o: shared/ents.h shared/command.h shared/iengine.h
fpsgame/movable.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
fpsgame/movable.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
fpsgame/movable.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
fpsgame/movable.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
fpsgame/movable.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
fpsgame/movable.o: fpsgame/ai.h
fpsgame/render.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/render.o: enet/include/enet/unix.h enet/include/enet/types.h
fpsgame/render.o: enet/include/enet/protocol.h enet/include/enet/list.h
fpsgame/render.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
fpsgame/render.o: shared/ents.h shared/command.h shared/iengine.h
fpsgame/render.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
fpsgame/render.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
fpsgame/render.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
fpsgame/render.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
fpsgame/render.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
fpsgame/render.o: fpsgame/ai.h
fpsgame/scoreboard.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/scoreboard.o: enet/include/enet/unix.h enet/include/enet/types.h
fpsgame/scoreboard.o: enet/include/enet/protocol.h enet/include/enet/list.h
fpsgame/scoreboard.o: enet/include/enet/callbacks.h shared/tools.h
fpsgame/scoreboard.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/scoreboard.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
fpsgame/scoreboard.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/scoreboard.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
fpsgame/scoreboard.o: mod/extinfo.h mod/demorecorder.h mod/chat.h
fpsgame/scoreboard.o: mod/events.h mod/thread.h mod/proxy-detection.h
fpsgame/scoreboard.o: mod/plugin.h shared/igame.h fpsgame/ai.h
fpsgame/server.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/server.o: enet/include/enet/unix.h enet/include/enet/types.h
fpsgame/server.o: enet/include/enet/protocol.h enet/include/enet/list.h
fpsgame/server.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
fpsgame/server.o: shared/ents.h shared/command.h shared/iengine.h
fpsgame/server.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
fpsgame/server.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
fpsgame/server.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
fpsgame/server.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
fpsgame/server.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
fpsgame/server.o: fpsgame/ai.h fpsgame/capture.h fpsgame/ctf.h
fpsgame/server.o: fpsgame/collect.h fpsgame/aiman.h
fpsgame/waypoint.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/waypoint.o: enet/include/enet/unix.h enet/include/enet/types.h
fpsgame/waypoint.o: enet/include/enet/protocol.h enet/include/enet/list.h
fpsgame/waypoint.o: enet/include/enet/callbacks.h shared/tools.h
fpsgame/waypoint.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/waypoint.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
fpsgame/waypoint.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/waypoint.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
fpsgame/waypoint.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
fpsgame/waypoint.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
fpsgame/waypoint.o: shared/igame.h fpsgame/ai.h
fpsgame/weapon.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
fpsgame/weapon.o: enet/include/enet/unix.h enet/include/enet/types.h
fpsgame/weapon.o: enet/include/enet/protocol.h enet/include/enet/list.h
fpsgame/weapon.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
fpsgame/weapon.o: shared/ents.h shared/command.h shared/iengine.h
fpsgame/weapon.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
fpsgame/weapon.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
fpsgame/weapon.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
fpsgame/weapon.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
fpsgame/weapon.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
fpsgame/weapon.o: fpsgame/ai.h
mod/plugin.o: shared/cube.h enet/include/enet/enet.h enet/include/enet/unix.h
mod/plugin.o: enet/include/enet/types.h enet/include/enet/protocol.h
mod/plugin.o: enet/include/enet/list.h enet/include/enet/callbacks.h
mod/plugin.o: shared/tools.h shared/geom.h shared/ents.h shared/command.h
mod/plugin.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
mod/plugin.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/plugin.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
mod/plugin.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
mod/plugin.o: mod/thread.h mod/proxy-detection.h mod/plugin.h shared/igame.h
mod/demorecorder.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/demorecorder.o: enet/include/enet/unix.h enet/include/enet/types.h
mod/demorecorder.o: enet/include/enet/protocol.h enet/include/enet/list.h
mod/demorecorder.o: enet/include/enet/callbacks.h shared/tools.h
mod/demorecorder.o: shared/geom.h shared/ents.h shared/command.h
mod/demorecorder.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
mod/demorecorder.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/demorecorder.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
mod/demorecorder.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
mod/demorecorder.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/demorecorder.o: shared/igame.h fpsgame/ai.h
mod/chat.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/chat.o: enet/include/enet/unix.h enet/include/enet/types.h
mod/chat.o: enet/include/enet/protocol.h enet/include/enet/list.h
mod/chat.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
mod/chat.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/chat.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
mod/chat.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
mod/chat.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
mod/chat.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/chat.o: shared/igame.h fpsgame/ai.h mod/chat-ca-certs.h
mod/events.o: shared/cube.h enet/include/enet/enet.h enet/include/enet/unix.h
mod/events.o: enet/include/enet/types.h enet/include/enet/protocol.h
mod/events.o: enet/include/enet/list.h enet/include/enet/callbacks.h
mod/events.o: shared/tools.h shared/geom.h shared/ents.h shared/command.h
mod/events.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
mod/events.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/events.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
mod/events.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
mod/events.o: mod/thread.h mod/proxy-detection.h mod/plugin.h shared/igame.h
mod/extinfo.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/extinfo.o: enet/include/enet/unix.h enet/include/enet/types.h
mod/extinfo.o: enet/include/enet/protocol.h enet/include/enet/list.h
mod/extinfo.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
mod/extinfo.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/extinfo.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
mod/extinfo.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
mod/extinfo.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
mod/extinfo.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
mod/extinfo.o: mod/plugin.h shared/igame.h fpsgame/ai.h
mod/gamemod.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/gamemod.o: enet/include/enet/unix.h enet/include/enet/types.h
mod/gamemod.o: enet/include/enet/protocol.h enet/include/enet/list.h
mod/gamemod.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
mod/gamemod.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/gamemod.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
mod/gamemod.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
mod/gamemod.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
mod/gamemod.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
mod/gamemod.o: mod/plugin.h shared/igame.h fpsgame/ai.h
mod/geoip.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/geoip.o: enet/include/enet/unix.h enet/include/enet/types.h
mod/geoip.o: enet/include/enet/protocol.h enet/include/enet/list.h
mod/geoip.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
mod/geoip.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/geoip.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
mod/geoip.o: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
mod/geoip.o: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
mod/geoip.o: mod/events.h mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/geoip.o: shared/igame.h fpsgame/ai.h
mod/ipignore.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/ipignore.o: enet/include/enet/unix.h enet/include/enet/types.h
mod/ipignore.o: enet/include/enet/protocol.h enet/include/enet/list.h
mod/ipignore.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
mod/ipignore.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/ipignore.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
mod/ipignore.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
mod/ipignore.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
mod/ipignore.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
mod/ipignore.o: mod/plugin.h shared/igame.h fpsgame/ai.h
mod/mod.o: shared/cube.h enet/include/enet/enet.h enet/include/enet/unix.h
mod/mod.o: enet/include/enet/types.h enet/include/enet/protocol.h
mod/mod.o: enet/include/enet/list.h enet/include/enet/callbacks.h
mod/mod.o: shared/tools.h shared/geom.h shared/ents.h shared/command.h
mod/mod.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
mod/mod.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/mod.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
mod/mod.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
mod/mod.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
mod/mod.o: last_sauer_svn_rev.h
mod/cubescript.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/cubescript.o: enet/include/enet/unix.h enet/include/enet/types.h
mod/cubescript.o: enet/include/enet/protocol.h enet/include/enet/list.h
mod/cubescript.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
mod/cubescript.o: shared/ents.h shared/command.h shared/iengine.h
mod/cubescript.o: mod/compat.h mod/mod.h mod/compiler.h mod/strtool.h
mod/cubescript.o: mod/crypto.h mod/ipbuf.h mod/wcversion.h mod/gamemod.h
mod/cubescript.o: mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
mod/cubescript.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
mod/cubescript.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
mod/cubescript.o: fpsgame/ai.h
mod/hwdisplay.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/hwdisplay.o: enet/include/enet/unix.h enet/include/enet/types.h
mod/hwdisplay.o: enet/include/enet/protocol.h enet/include/enet/list.h
mod/hwdisplay.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
mod/hwdisplay.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/hwdisplay.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
mod/hwdisplay.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
mod/hwdisplay.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
mod/hwdisplay.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
mod/hwdisplay.o: mod/plugin.h shared/igame.h fpsgame/ai.h
mod/playerdisplay.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/playerdisplay.o: enet/include/enet/unix.h enet/include/enet/types.h
mod/playerdisplay.o: enet/include/enet/protocol.h enet/include/enet/list.h
mod/playerdisplay.o: enet/include/enet/callbacks.h shared/tools.h
mod/playerdisplay.o: shared/geom.h shared/ents.h shared/command.h
mod/playerdisplay.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
mod/playerdisplay.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/playerdisplay.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
mod/playerdisplay.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
mod/playerdisplay.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
mod/playerdisplay.o: shared/igame.h fpsgame/ai.h
mod/http.o: shared/cube.h enet/include/enet/enet.h enet/include/enet/unix.h
mod/http.o: enet/include/enet/types.h enet/include/enet/protocol.h
mod/http.o: enet/include/enet/list.h enet/include/enet/callbacks.h
mod/http.o: shared/tools.h shared/geom.h shared/ents.h shared/command.h
mod/http.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
mod/http.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/http.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h mod/extinfo.h
mod/http.o: mod/demorecorder.h mod/chat.h mod/events.h mod/thread.h
mod/http.o: mod/proxy-detection.h mod/plugin.h shared/igame.h
mod/strtool.o: shared/cube.h enet/include/enet/enet.h
mod/strtool.o: enet/include/enet/unix.h enet/include/enet/types.h
mod/strtool.o: enet/include/enet/protocol.h enet/include/enet/list.h
mod/strtool.o: enet/include/enet/callbacks.h shared/tools.h shared/geom.h
mod/strtool.o: shared/ents.h shared/command.h shared/iengine.h mod/compat.h
mod/strtool.o: mod/mod.h mod/compiler.h mod/strtool.h mod/crypto.h
mod/strtool.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
mod/strtool.o: mod/geoip.h mod/ipignore.h mod/extinfo.h mod/demorecorder.h
mod/strtool.o: mod/chat.h mod/events.h mod/thread.h mod/proxy-detection.h
mod/strtool.o: mod/plugin.h shared/igame.h
mod/crypto.o: shared/cube.h enet/include/enet/enet.h enet/include/enet/unix.h
mod/crypto.o: enet/include/enet/types.h enet/include/enet/protocol.h
mod/crypto.o: enet/include/enet/list.h enet/include/enet/callbacks.h
mod/crypto.o: shared/tools.h shared/geom.h shared/ents.h shared/command.h
mod/crypto.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
mod/crypto.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/crypto.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
mod/crypto.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
mod/crypto.o: mod/thread.h mod/proxy-detection.h mod/plugin.h shared/igame.h
mod/extinfo-playerpreview.o: fpsgame/game.h shared/cube.h
mod/extinfo-playerpreview.o: enet/include/enet/enet.h
mod/extinfo-playerpreview.o: enet/include/enet/unix.h
mod/extinfo-playerpreview.o: enet/include/enet/types.h
mod/extinfo-playerpreview.o: enet/include/enet/protocol.h
mod/extinfo-playerpreview.o: enet/include/enet/list.h
mod/extinfo-playerpreview.o: enet/include/enet/callbacks.h shared/tools.h
mod/extinfo-playerpreview.o: shared/geom.h shared/ents.h shared/command.h
mod/extinfo-playerpreview.o: shared/iengine.h mod/compat.h mod/mod.h
mod/extinfo-playerpreview.o: mod/compiler.h mod/strtool.h mod/crypto.h
mod/extinfo-playerpreview.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h
mod/extinfo-playerpreview.o: mod/http.h mod/geoip.h mod/ipignore.h
mod/extinfo-playerpreview.o: mod/extinfo.h mod/demorecorder.h mod/chat.h
mod/extinfo-playerpreview.o: mod/events.h mod/thread.h mod/proxy-detection.h
mod/extinfo-playerpreview.o: mod/plugin.h shared/igame.h fpsgame/ai.h
mod/ipbuf.o: shared/cube.h enet/include/enet/enet.h enet/include/enet/unix.h
mod/ipbuf.o: enet/include/enet/types.h enet/include/enet/protocol.h
mod/ipbuf.o: enet/include/enet/list.h enet/include/enet/callbacks.h
mod/ipbuf.o: shared/tools.h shared/geom.h shared/ents.h shared/command.h
mod/ipbuf.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
mod/ipbuf.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/ipbuf.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
mod/ipbuf.o: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
mod/ipbuf.o: mod/thread.h mod/proxy-detection.h mod/plugin.h shared/igame.h
mod/proxy-detection.o: fpsgame/game.h shared/cube.h enet/include/enet/enet.h
mod/proxy-detection.o: enet/include/enet/unix.h enet/include/enet/types.h
mod/proxy-detection.o: enet/include/enet/protocol.h enet/include/enet/list.h
mod/proxy-detection.o: enet/include/enet/callbacks.h shared/tools.h
mod/proxy-detection.o: shared/geom.h shared/ents.h shared/command.h
mod/proxy-detection.o: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
mod/proxy-detection.o: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
mod/proxy-detection.o: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
mod/proxy-detection.o: mod/extinfo.h mod/demorecorder.h mod/chat.h
mod/proxy-detection.o: mod/events.h mod/thread.h mod/proxy-detection.h
mod/proxy-detection.o: mod/plugin.h shared/igame.h fpsgame/ai.h
plugins/g15lcd/g15lcd.o: shared/cube.h enet/include/enet/enet.h
plugins/g15lcd/g15lcd.o: enet/include/enet/unix.h enet/include/enet/types.h
plugins/g15lcd/g15lcd.o: enet/include/enet/protocol.h
plugins/g15lcd/g15lcd.o: enet/include/enet/list.h
plugins/g15lcd/g15lcd.o: enet/include/enet/callbacks.h shared/tools.h
plugins/g15lcd/g15lcd.o: shared/geom.h shared/ents.h shared/command.h
plugins/g15lcd/g15lcd.o: shared/iengine.h mod/compat.h mod/mod.h
plugins/g15lcd/g15lcd.o: mod/compiler.h mod/strtool.h mod/crypto.h
plugins/g15lcd/g15lcd.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
plugins/g15lcd/g15lcd.o: mod/geoip.h mod/ipignore.h mod/extinfo.h
plugins/g15lcd/g15lcd.o: mod/demorecorder.h mod/chat.h mod/events.h
plugins/g15lcd/g15lcd.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
plugins/g15lcd/g15lcd.o: shared/igame.h plugins/g15lcd/g15sdkwrapper.h
plugins/hwtemp/hwtemp.o: plugins/hwtemp/hwtemp.h shared/cube.h
plugins/hwtemp/hwtemp.o: enet/include/enet/enet.h enet/include/enet/unix.h
plugins/hwtemp/hwtemp.o: enet/include/enet/types.h
plugins/hwtemp/hwtemp.o: enet/include/enet/protocol.h
plugins/hwtemp/hwtemp.o: enet/include/enet/list.h
plugins/hwtemp/hwtemp.o: enet/include/enet/callbacks.h shared/tools.h
plugins/hwtemp/hwtemp.o: shared/geom.h shared/ents.h shared/command.h
plugins/hwtemp/hwtemp.o: shared/iengine.h mod/compat.h mod/mod.h
plugins/hwtemp/hwtemp.o: mod/compiler.h mod/strtool.h mod/crypto.h
plugins/hwtemp/hwtemp.o: mod/ipbuf.h mod/wcversion.h mod/gamemod.h mod/http.h
plugins/hwtemp/hwtemp.o: mod/geoip.h mod/ipignore.h mod/extinfo.h
plugins/hwtemp/hwtemp.o: mod/demorecorder.h mod/chat.h mod/events.h
plugins/hwtemp/hwtemp.o: mod/thread.h mod/proxy-detection.h mod/plugin.h
plugins/hwtemp/hwtemp.o: shared/igame.h

shared/cube.h.gch: enet/include/enet/enet.h enet/include/enet/unix.h
shared/cube.h.gch: enet/include/enet/types.h enet/include/enet/protocol.h
shared/cube.h.gch: enet/include/enet/list.h enet/include/enet/callbacks.h
shared/cube.h.gch: shared/tools.h shared/geom.h shared/ents.h
shared/cube.h.gch: shared/command.h shared/iengine.h mod/compat.h mod/mod.h
shared/cube.h.gch: mod/compiler.h mod/strtool.h mod/crypto.h mod/ipbuf.h
shared/cube.h.gch: mod/wcversion.h mod/gamemod.h mod/http.h mod/geoip.h
shared/cube.h.gch: mod/ipignore.h mod/extinfo.h mod/demorecorder.h mod/chat.h
shared/cube.h.gch: mod/events.h mod/thread.h mod/proxy-detection.h
shared/cube.h.gch: mod/plugin.h shared/igame.h
engine/engine.h.gch: shared/cube.h enet/include/enet/enet.h
engine/engine.h.gch: enet/include/enet/unix.h enet/include/enet/types.h
engine/engine.h.gch: enet/include/enet/protocol.h enet/include/enet/list.h
engine/engine.h.gch: enet/include/enet/callbacks.h shared/tools.h
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
fpsgame/game.h.gch: enet/include/enet/unix.h enet/include/enet/types.h
fpsgame/game.h.gch: enet/include/enet/protocol.h enet/include/enet/list.h
fpsgame/game.h.gch: enet/include/enet/callbacks.h shared/tools.h
fpsgame/game.h.gch: shared/geom.h shared/ents.h shared/command.h
fpsgame/game.h.gch: shared/iengine.h mod/compat.h mod/mod.h mod/compiler.h
fpsgame/game.h.gch: mod/strtool.h mod/crypto.h mod/ipbuf.h mod/wcversion.h
fpsgame/game.h.gch: mod/gamemod.h mod/http.h mod/geoip.h mod/ipignore.h
fpsgame/game.h.gch: mod/extinfo.h mod/demorecorder.h mod/chat.h mod/events.h
fpsgame/game.h.gch: mod/thread.h mod/proxy-detection.h mod/plugin.h
fpsgame/game.h.gch: shared/igame.h fpsgame/ai.h

mod/strtool-standalone.o: shared/cube.h enet/include/enet/enet.h
mod/strtool-standalone.o: enet/include/enet/unix.h enet/include/enet/types.h
mod/strtool-standalone.o: enet/include/enet/protocol.h
mod/strtool-standalone.o: enet/include/enet/list.h
mod/strtool-standalone.o: enet/include/enet/callbacks.h shared/tools.h
mod/strtool-standalone.o: shared/geom.h shared/ents.h shared/command.h
mod/strtool-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
mod/strtool-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
mod/strtool-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
shared/crypto-standalone.o: shared/cube.h enet/include/enet/enet.h
shared/crypto-standalone.o: enet/include/enet/unix.h
shared/crypto-standalone.o: enet/include/enet/types.h
shared/crypto-standalone.o: enet/include/enet/protocol.h
shared/crypto-standalone.o: enet/include/enet/list.h
shared/crypto-standalone.o: enet/include/enet/callbacks.h shared/tools.h
shared/crypto-standalone.o: shared/geom.h shared/ents.h shared/command.h
shared/crypto-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
shared/crypto-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
shared/crypto-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
shared/stream-standalone.o: shared/cube.h enet/include/enet/enet.h
shared/stream-standalone.o: enet/include/enet/unix.h
shared/stream-standalone.o: enet/include/enet/types.h
shared/stream-standalone.o: enet/include/enet/protocol.h
shared/stream-standalone.o: enet/include/enet/list.h
shared/stream-standalone.o: enet/include/enet/callbacks.h shared/tools.h
shared/stream-standalone.o: shared/geom.h shared/ents.h shared/command.h
shared/stream-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
shared/stream-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
shared/stream-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
shared/tools-standalone.o: shared/cube.h enet/include/enet/enet.h
shared/tools-standalone.o: enet/include/enet/unix.h enet/include/enet/types.h
shared/tools-standalone.o: enet/include/enet/protocol.h
shared/tools-standalone.o: enet/include/enet/list.h
shared/tools-standalone.o: enet/include/enet/callbacks.h shared/tools.h
shared/tools-standalone.o: shared/geom.h shared/ents.h shared/command.h
shared/tools-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
shared/tools-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
shared/tools-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
engine/command-standalone.o: engine/engine.h shared/cube.h
engine/command-standalone.o: enet/include/enet/enet.h
engine/command-standalone.o: enet/include/enet/unix.h
engine/command-standalone.o: enet/include/enet/types.h
engine/command-standalone.o: enet/include/enet/protocol.h
engine/command-standalone.o: enet/include/enet/list.h
engine/command-standalone.o: enet/include/enet/callbacks.h shared/tools.h
engine/command-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/command-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
engine/command-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
engine/command-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
engine/command-standalone.o: engine/world.h
engine/server-standalone.o: engine/engine.h shared/cube.h
engine/server-standalone.o: enet/include/enet/enet.h enet/include/enet/unix.h
engine/server-standalone.o: enet/include/enet/types.h
engine/server-standalone.o: enet/include/enet/protocol.h
engine/server-standalone.o: enet/include/enet/list.h
engine/server-standalone.o: enet/include/enet/callbacks.h shared/tools.h
engine/server-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/server-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
engine/server-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
engine/server-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
engine/server-standalone.o: engine/world.h
engine/worldio-standalone.o: engine/engine.h shared/cube.h
engine/worldio-standalone.o: enet/include/enet/enet.h
engine/worldio-standalone.o: enet/include/enet/unix.h
engine/worldio-standalone.o: enet/include/enet/types.h
engine/worldio-standalone.o: enet/include/enet/protocol.h
engine/worldio-standalone.o: enet/include/enet/list.h
engine/worldio-standalone.o: enet/include/enet/callbacks.h shared/tools.h
engine/worldio-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/worldio-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
engine/worldio-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
engine/worldio-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
engine/worldio-standalone.o: engine/world.h
fpsgame/entities-standalone.o: fpsgame/game.h shared/cube.h
fpsgame/entities-standalone.o: enet/include/enet/enet.h
fpsgame/entities-standalone.o: enet/include/enet/unix.h
fpsgame/entities-standalone.o: enet/include/enet/types.h
fpsgame/entities-standalone.o: enet/include/enet/protocol.h
fpsgame/entities-standalone.o: enet/include/enet/list.h
fpsgame/entities-standalone.o: enet/include/enet/callbacks.h shared/tools.h
fpsgame/entities-standalone.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/entities-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
fpsgame/entities-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
fpsgame/entities-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
fpsgame/entities-standalone.o: fpsgame/ai.h
fpsgame/server-standalone.o: fpsgame/game.h shared/cube.h
fpsgame/server-standalone.o: enet/include/enet/enet.h
fpsgame/server-standalone.o: enet/include/enet/unix.h
fpsgame/server-standalone.o: enet/include/enet/types.h
fpsgame/server-standalone.o: enet/include/enet/protocol.h
fpsgame/server-standalone.o: enet/include/enet/list.h
fpsgame/server-standalone.o: enet/include/enet/callbacks.h shared/tools.h
fpsgame/server-standalone.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/server-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
fpsgame/server-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
fpsgame/server-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
fpsgame/server-standalone.o: fpsgame/ai.h fpsgame/capture.h fpsgame/ctf.h
fpsgame/server-standalone.o: fpsgame/collect.h fpsgame/extinfo.h
fpsgame/server-standalone.o: fpsgame/aiman.h

engine/master-standalone.o: shared/cube.h enet/include/enet/enet.h
engine/master-standalone.o: enet/include/enet/unix.h
engine/master-standalone.o: enet/include/enet/types.h
engine/master-standalone.o: enet/include/enet/protocol.h
engine/master-standalone.o: enet/include/enet/list.h
engine/master-standalone.o: enet/include/enet/callbacks.h shared/tools.h
engine/master-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/master-standalone.o: shared/iengine.h mod/compat.h mod/mod.h
engine/master-standalone.o: mod/compiler.h mod/strtool.h mod/crypto.h
engine/master-standalone.o: mod/ipbuf.h mod/wcversion.h shared/igame.h
engine/master-standalone.o: enet/include/enet/time.h
