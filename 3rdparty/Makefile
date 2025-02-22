# (c) 2018-2024 Thomas BERNARD
#
# Makefile to build GrafX2 library dependencies.
#
# Variables :
# - API : sdl(default), sdl2, win32
# - WIN32CROSS
#
# most useful targets :
# - download : download .tar.gz files from the WWW
# - all (default) : download and build...
# - clean : remove built libraries
# - clean_archives : remove downloaded .tar.gz files
#
# examples :
# $ make WIN32CROSS=1 API=sdl2
# $ make download
# $ make libtiff

API ?= sdl

PLATFORM = $(shell uname)

SDL = SDL-1.2.15
SDLARCH = $(SDL).tar.gz
SDLSIG = $(SDLARCH).sig
SDLURL = https://www.libsdl.org/release/$(SDLARCH)
SDLSIGURL = $(SDLURL).sig
SDLPATCHES = SDL-1.2.15-patch-src_video_x11_SDL_x11sym.h \
             SDL-1.2.15-quartzvideo.patch \
             SDL-1.2.15-atari-timer.patch
SDLDEVEL = SDL-devel-1.2.15-mingw32.tar.gz
SDLDEVELURL = https://www.libsdl.org/release/$(SDLDEVEL)
SDLDEVELPATCH = SDL-1.2.15.patch
SDLDEVELSHA256 = d51eedfe7e07893d6c93a2d761c6ccc91d04b5f68a2ecabdbef83b7a1fef9cde
SDLIMAGE = SDL_image-1.2.12
SDLIMAGEARCH = $(SDLIMAGE).tar.gz
SDLIMAGEURL = https://www.libsdl.org/projects/SDL_image/release/$(SDLIMAGEARCH)
SDLIMAGESHA256 = 0b90722984561004de84847744d566809dbb9daf732a9e503b91a1b5a84e5699
SDLIMAGEPATCHES = SDL_image-1.2.12.XCF_infinite_loop.patch \
                  SDL_image-1.2.12.XCF_v11_64bits_offsets.patch \
                  SDL_image-1.2.12.XCF-fixversion.patch \
                  SDL_image_XCF_v11_load_level.patch \
                  SDL_image-1.2.12-png1.6.patch \
                  SDL_image-1.2.12-png_const_colorp.patch \
                  SDL_image-1.2.12-png_const_fix.patch \
                  SDL_image-1.2.12-configure_update.patch \
                  SDL_image-1.2.12-configure_libpng.patch
SDLTTF=SDL_ttf-2.0.11
SDLTTFARCH=$(SDLTTF).tar.gz
SDLTTFURL=https://www.libsdl.org/projects/SDL_ttf/release/$(SDLTTFARCH)
SDLTTFSHA256 = 724cd895ecf4da319a3ef164892b72078bd92632a5d812111261cde248ebcdb7
# we cannot take the latest version automatically because SDL2DEVEL is not signed :(
SDL2VER = 2.30.2
LATESTSDL2VER = $(shell curl -s -S http://www.libsdl.org/ | grep current | grep stable | sed 's/.*version \([0-9.]*\).*/\1/' )
SDL2 = SDL2-$(SDL2VER)
SDL2ARCH = $(SDL2).tar.gz
SDL2URL = https://www.libsdl.org/release/$(SDL2ARCH)
SDL2SIG = $(SDL2ARCH).sig
SDL2SIGURL = $(SDL2URL).sig
SDL2PATCHES =
ifeq ($(SDL2VER),2.0.12)
SDL2PATCHES += SDL2-NSOSVersion.patch
endif
SDL2DEVEL = SDL2-devel-$(SDL2VER)-mingw.tar.gz
SDL2DEVELURL = https://www.libsdl.org/release/$(SDL2DEVEL)
SDL2DEVELPATCH = SDL2-devel.patch
SDL2DEVELSHA256 = f2a1759450cd85416beec9ba355c243ae5f145dd9f9c502ec3d14186dd5d941a
SDL2IMAGEVER = $(LATESTSDL2IMAGEVER)
SDL2IMAGE = SDL2_image-$(SDL2IMAGEVER)
SDL2IMAGEARCH = $(SDL2IMAGE).tar.gz
SDL2IMAGESIG = $(SDL2IMAGEARCH).sig
SDL2IMAGEURL = https://github.com/libsdl-org/SDL_image/releases/download/release-$(SDL2IMAGEVER)/$(SDL2IMAGEARCH)
SDL2IMAGESIGURL = $(SDL2IMAGEURL).sig
LATESTSDL2IMAGEVER := $(shell curl -s -S https://api.github.com/repos/libsdl-org/SDL_image/releases | jq -r '.[] | select (.prerelease == false and .draft == false) | .name' | head -n1)
SDL2TTFVER = $(LATESTSDL2TTFVER)
SDL2TTF = SDL2_ttf-$(SDL2TTFVER)
SDL2TTFARCH = $(SDL2TTF).tar.gz
SDL2TTFSIG = $(SDL2TTFARCH).sig
SDL2TTFURL = https://github.com/libsdl-org/SDL_ttf/releases/download/release-$(SDL2TTFVER)/$(SDL2TTFARCH)
SDL2TTFSIGURL = $(SDL2TTFURL).sig
LATESTSDL2TTFVER := $(shell curl -s -S https://api.github.com/repos/libsdl-org/SDL_ttf/tags | jq -r .[0].name|sed 's/.*-\([0-9.]*\)/\1/')
LIBPNGVER = 1.6.43
LIBPNG = libpng-$(LIBPNGVER)
LIBPNGARCH = $(LIBPNG).tar.gz
LIBPNGURL = https://download.sourceforge.net/libpng/$(LIBPNGARCH)
LIBPNGURLALT = http://sourceforge.mirrorservice.org/l/project/project/li/libpng/libpng16/$(LIBPNGVER)/$(LIBPNGARCH)
LIBPNGSHA256 = e804e465d4b109b5ad285a8fb71f0dd3f74f0068f91ce3cdfde618180c174925
LATESTLIBPNGVER = $(shell curl -s -S http://www.libpng.org/pub/png/libpng.html | grep "current public release" | sed 's/.*libpng \([0-9.]*\).*/\1/' )
JPEGVER = 9f
LATESTJPEGVER = $(shell curl -s -S http://www.ijg.org/ |grep "current version" | sed 's/.*release \([0-9]*[a-z]\) .*/\1/' )
JPEGDIR = jpeg-$(JPEGVER)
JPEGARCH = jpegsrc.v$(JPEGVER).tar.gz
JPEGURL = https://www.ijg.org/files/$(JPEGARCH)
# http://www.simplesystems.org/libtiff/
# https://gitlab.com/libtiff/libtiff
LIBTIFFVER = $(LATESTLIBTIFFVER)
LATESTLIBTIFFVER := $(shell curl -s -S http://www.simplesystems.org/libtiff/ |grep -A1 Latest|tail -n1|sed 's/.*>v\([0-9.]*\).*/\1/')
LIBTIFF = tiff-$(LIBTIFFVER)
LIBTIFFARCH = $(LIBTIFF).tar.gz
LIBTIFFURL = https://download.osgeo.org/libtiff/$(LIBTIFFARCH)
LIBTIFFURLALT = https://fossies.org/linux/misc/$(LIBTIFFARCH)
LIBTIFFVERMAJOR = $(shell echo $(LIBTIFFVER) | cut -d. -f1)
LIBTIFFVERMINOR = $(shell echo $(LIBTIFFVER) | cut -d. -f2)
ifeq ($(shell if [ $(LIBTIFFVERMAJOR) -lt 4 ] || [ \( $(LIBTIFFVERMAJOR) -eq 4 \) -a \( $(LIBTIFFVERMINOR) -lt 3 \) ] ; then echo true ; fi),true)
# this patch is needed up to version 4.2.0
LIBTIFFPATCHES = tiff-uint64_long_long.patch
endif
# additionnal variables for the generic rules to work :
TIFFARCH = $(LIBTIFFARCH)
TIFFSIG = $(TIFFARCH).sig
TIFFURL = $(LIBTIFFURL)
TIFFSIGURL = $(TIFFURL).sig
TIFFURLALT = $(LIBTIFFURLALT)
TIFFPATCHES = $(LIBTIFFPATCHES)
ZLIBVER=$(LATESTZLIBVER)
LATESTZLIBVER := $(shell curl -s -S https://www.zlib.net/ChangeLog.txt |grep "Changes in" |head -n1|sed 's/Changes in \([0-9.]*\).*/\1/')
ZLIB=zlib-$(ZLIBVER)
ZLIBARCH=$(ZLIB).tar.gz
ZLIBSIG=$(ZLIBARCH).asc
ZLIBURL=https://www.zlib.net/$(ZLIBARCH)
ZLIBSIGURL=$(ZLIBURL).asc
ZLIBURLALT=http://downloads.sourceforge.net/project/libpng/zlib/$(ZLIBVER)/$(ZLIBARCH)
FREETYPEVER=$(LATESTFREETYPEVER)
FREETYPE=freetype-$(FREETYPEVER)
FREETYPEARCH=$(FREETYPE).tar.gz
FREETYPESIG=$(FREETYPEARCH).sig
FREETYPEURL=https://download.savannah.gnu.org/releases/freetype/$(FREETYPEARCH)
FREETYPEURLALT=https://sourceforge.net/projects/freetype/files/freetype2/$(FREETYPEVER)/$(FREETYPEARCH)
FREETYPESIGURL=$(FREETYPEURL).sig
FREETYPESIGURLALT=$(FREETYPEURLALT).sig
LATESTFREETYPEVER := $(shell curl -s -S -I "https://sourceforge.net/projects/freetype/files/latest/download" |grep -i '^location:' | sed 's:.*/\([0-9.]*\)/.*:\1:' )
LUAVER=5.3.6
LUA=lua-$(LUAVER)
LUAARCH=$(LUA).tar.gz
LUAURL=https://www.lua.org/ftp/$(LUAARCH)
LUAURLALT=https://www.tecgraf.puc-rio.br/lua/mirror/ftp/$(LUAARCH)
LUASHA256=fc5fd69bb8736323f026672b1b7235da613d7177e72558893a0bdcd320466d60
LUAPATCHES = lua-atari-log2.patch \
             lua-atari-inttype.patch
LATESTLUAVER = $(shell curl -s -S https://www.lua.org/download.html | grep -A1 "current release is" | tail -n1 | sed 's/.*lua-\([0-9.]*\)\.tar.*/\1/' )
LATESTLUA53VER = $(shell curl -s -S https://www.lua.org/versions.html |grep 5[.]3[.] | head -n 1 | sed 's/.*\(5[0-9.]*\).*/\1/' )
# https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-0.6.1.tar.gz
RECOILVER=6.4.4
LATESTRECOILVER = $(shell curl -s -S -I https://sourceforge.net/projects/recoil/files/latest/download|grep -i "^location:"|sed 's:.*/\([0-9.]*\)/.*:\1:' )
RECOIL=recoil-$(RECOILVER)
RECOILARCH=$(RECOIL).tar.gz
# https://downloads.sourceforge.net/project/recoil/recoil/4.3.0/recoil-4.3.0.tar.gz
RECOILURL=https://downloads.sourceforge.net/project/recoil/recoil/$(RECOILVER)/$(RECOILARCH)
RECOILURLALT=http://nanard.free.fr/grafx2/$(RECOILARCH)
RECOILSHA256=06b89b90980f8c8dc732221e58771f63e663362fc3853fecc309723de6f0b1d9
#https://github.com/redcode/6502/releases/download/v0.1/6502-v0.1.tar.xz
REDCODE6502=6502-v0.1
REDCODE6502ARCH=6502-v0.1.tar.xz
REDCODE6502URL=https://github.com/redcode/6502/releases/download/v0.1/$(REDCODE6502ARCH)
REDCODE6502SHA256=bba50f327163d40b1014f5affc8042137f37b200dae50a4f0fe56b5769b3940f
REDCODE6502PATCHES = 6502-illegal-opcode.patch

ARCHIVES = $(addprefix archives/, $(SDLARCH) $(SDLIMAGEARCH) $(SDLTTFARCH) \
             $(SDL2ARCH) $(SDL2IMAGEARCH) $(SDL2TTFARCH) $(LIBPNGARCH) \
             $(JPEGARCH) $(LIBTIFFARCH) $(ZLIBARCH) $(FREETYPEARCH) \
             $(LUAARCH) $(RECOILARCH))

PLATFORMDIR =
PREFIX = $(PWD)/usr$(PLATFORMDIR)

MKDIR = mkdir -p
CP = cp -v
TAR = $(shell which tar)
GPG = gpg
SHA256CMD = $(shell SHASUM=`which shasum 2> /dev/null` ; if [ "$$?" = "0" ] && [ -x "$$SHASUM" ] ; \
                    then echo "(\"$$SHASUM\" -a 256 | cut -f1 -d' ')" ; \
                    else OPENSSL=`which openssl` ; if [ "$$?" = "0" ] && [ -x "$$OPENSSL" ] ; \
                    then echo "(\"$$OPENSSL\" sha256 -hex | sed 's/^.* //')" ; \
                    else echo "echo 'WARNING: no sha256 digest tool' >&2 ; echo $(SHA256)" ; \
                    fi ; fi )
# you need either wget or curl to download the files
# wget -nv option is used to avoid messing the output when using "make -j3"
GETURL = $(shell WGET=`which wget` ; if [ "$$?" = "0" ] && [ -x "$$WGET" ] ; \
                 then echo "$$WGET -nv" ; \
                 else echo 'curl -s -w "%{url_effective} downloaded to %{filename_effective} in %{time_total} seconds\\n" -R -O -L --max-time 120' ; fi )

# default to gcc compiler
ifeq (default,$(origin CC))
  CC = gcc
endif

BUILD_CC := $(CC)
STRIP = strip
OBJDUMP = objdump

# There is no uname under windows, but we can guess we are there with the COMSPEC env.var
# Windows specific
ifdef ComSpec
# sometimes, it is ComSpec, sometimes it is COMSPEC
COMSPEC = $(ComSpec)
endif
ifdef COMSPEC
WIN32 = 1
PLATFORMDIR = -win32
RANLIB = ranlib
LDFLAGS += -static-libgcc
endif

ifdef WIN32CROSS
WIN32 = 1
PLATFORMDIR = -win32
CROSS_CC ?= $(shell which i686-w64-mingw32-gcc || which mingw32-gcc)
CROSS_CXX ?= $(shell which i686-w64-mingw32-g++ || which mingw32-g++)
CROSS_AR ?= $(shell which i686-w64-mingw32-ar || which mingw32-ar)
CROSS_RANLIB ?= $(shell which i686-w64-mingw32-ranlib || which mingw32-ranlib)
CROSS_STRIP ?= $(shell which i686-w64-mingw32-strip || which mingw32-strip)
CROSS_OBJDUMP ?= $(shell which i686-w64-mingw32-objdump || which mingw32-objdump)
CROSS_LDFLAGS += -static-libgcc
CC = $(CROSS_CC)
CXX = $(CROSS_CXX)
AR = $(CROSS_AR)
RANLIB = $(CROSS_RANLIB)
STRIP = $(CROSS_STRIP)
OBJDUMP = $(CROSS_OBJDUMP)
CFLAGS = $(CROSS_CFLAGS)
LDFLAGS = $(CROSS_LDFLAGS)
endif

ifdef ATARICROSS
ATARI = 1
PLATFORMDIR = -atari
ARCH ?= m68k
MCPU ?= -m68020-60
CROSS_CC ?= $(ARCH)-atari-mint-gcc
CROSS_AR ?= $(ARCH)-atari-mint-ar
CROSS_RANLIB ?= $(ARCH)-atari-mint-ranlib
CROSS_STRIP ?= $(ARCH)-atari-mint-strip
CROSS_OBJDUMP ?= $(ARCH)-atari-mint-objdump
CC = $(CROSS_CC)
AR = $(CROSS_AR)
RANLIB = $(CROSS_RANLIB)
STRIP = $(CROSS_STRIP)
OBJDUMP = $(CROSS_OBJDUMP)
endif

HOST = $(shell $(CC) -dumpmachine)
#HOST = i686-pc-mingw32
BUILD = $(shell $(BUILD_CC) -dumpmachine)
DATE = $(shell date -R)

.PHONY:	all clean clean_archives clean_all download libs libpng \
        libsdl libsdl_image libsdl_ttf \
        libsdl2 libsdl2_image libsdl2_ttf \
        libjpeg libtiff zlib freetype lua recoil \
        checkversions importgpgkeys

all:	libs

checkversions:	checkversion-libpng checkversion-freetype \
	checkversion-lua checkversion-jpeg checkversion-libtiff \
	checkversion-zlib checkversion-recoil \
	checkversion-sdl2 checkversion-sdl2ttf checkversion-sdl2image

clean_all:	clean clean_archives

clean:
	$(RM) -r $(PREFIX) $(LIBPNG) $(ZLIB) $(SDLIMAGE) $(JPEGDIR) $(LIBTIFF)
	$(RM) -r $(SDLTTF) $(FREETYPE) $(SDL) $(LUA) $(RECOIL)
	$(RM) -r $(SDL2) $(SDL2IMAGE) $(SDL2TTF)

clean_archives:
	$(RM) -r archives

download:	$(ARCHIVES)

ifeq ($(API), sdl)
libs:	libpng libsdl libsdl_image libsdl_ttf lua
else
ifeq ($(API), sdl2)
libs:	libpng libsdl2 libsdl2_image libsdl2_ttf lua
else
libs:	libpng lua libtiff
endif
endif
libsdl:	$(PREFIX)/lib/libSDLmain.a
libsdl_image:	$(PREFIX)/lib/libSDL_image.a
libsdl_ttf:	$(PREFIX)/lib/libSDL_ttf.a
libsdl2:	$(PREFIX)/lib/libSDL2main.a
libsdl2_image:	$(PREFIX)/lib/libSDL2_image.a
libsdl2_ttf:	$(PREFIX)/lib/libSDL2_ttf.a
libjpeg:	$(PREFIX)/lib/libjpeg.a
libpng:	$(PREFIX)/lib/libpng.a
libtiff:	$(PREFIX)/lib/libtiff.a
zlib:	$(PREFIX)/lib/libz.a
freetype:	$(PREFIX)/lib/libfreetype.a
lua:	$(PREFIX)/lib/liblua.a

$(PREFIX)/lib/liblua.a:	$(LUA)/.ok
	cd $(LUA) && $(MAKE) clean
ifdef WIN32
	#cd $(LUA) && $(MAKE) PLAT=mingw CC=$(CC) RANLIB=$(RANLIB)
	cd $(LUA)/src && $(MAKE) "CC=$(CC)" "LUA_A=lua53.dll" "LUA_T=lua.exe" \
	"AR=$(CC) -shared -o" "RANLIB=$(STRIP) --strip-unneeded" \
	"SYSCFLAGS=-DLUA_BUILD_AS_DLL" "SYSLIBS=" "SYSLDFLAGS=-s" lua.exe
	cd $(LUA)/src && $(MAKE) "CC=$(CC)" "RANLIB=$(RANLIB)" "LUAC_T=luac.exe" luac.exe
	cd $(LUA) && $(MAKE) install PLAT=mingw INSTALL_TOP=$(PREFIX) TO_BIN="lua.exe luac.exe"
	$(MKDIR) ../bin && for f in $(LUA)/src/lua*.dll ; do \
	    $(CP) $$f ../bin ; \
	    $(STRIP) ../bin/`basename $$f` ; \
	  done
	echo "The Windows distribution of Grafx2 is linked with Lua v$(LUAVER)" > ../doc/README-lua.txt
else
ifdef ATARI
	cd $(LUA) && $(MAKE) PLAT=generic CC=$(CC) RANLIB=$(RANLIB) AR="$(AR) rcs"
	cd $(LUA) && $(MAKE) install PLAT=generic INSTALL_TOP=$(PREFIX)
else
ifeq ($(PLATFORM), FreeBSD)
	cd $(LUA) && $(MAKE) freebsd install INSTALL_TOP=$(PREFIX)
else
ifeq ($(PLATFORM), Darwin)
	cd $(LUA) && $(MAKE) macosx install INSTALL_TOP=$(PREFIX)
else
ifeq ($(PLATFORM), Linux)
	cd $(LUA) && $(MAKE) linux install INSTALL_TOP=$(PREFIX)
else
	cd $(LUA) && $(MAKE) posix install INSTALL_TOP=$(PREFIX)
endif
endif
endif
endif
	echo "Lua v$(LUAVER)" > ../doc/README-lua.txt
endif
	grep LUA_COPYRIGHT $(LUA)/src/lua.h | cut -d'"' -f 2 >> ../doc/README-lua.txt
	echo "" >> ../doc/README-lua.txt
	echo "License : http://www.lua.org/license.html" >> ../doc/README-lua.txt
	# extract license from readme.html
	awk '/BLOCKQUOTE/{flag=1-flag;next}flag' $(LUA)/doc/readme.html | grep -v '<P>' | tail -n +2 >> ../doc/README-lua.txt

ifdef WIN32
$(PREFIX)/lib/libSDL2main.a:	archives/$(SDL2DEVEL)
	$(TAR) xzf $<
	cd $(SDL2) && patch -p1 < ../$(SDL2DEVELPATCH)
	$(MKDIR) $(PREFIX)
	cd $(SDL2) && $(MAKE) install-package arch=i686-w64-mingw32 prefix=$(PREFIX)
	$(MKDIR) ../bin && $(CP) $(PREFIX)/bin/SDL2.dll ../bin && $(STRIP) ../bin/SDL2.dll
	$(OBJDUMP) -p $(PREFIX)/bin/SDL2.dll | grep 'DLL Name'
	echo "The following file:" > ../doc/README-SDL2.txt
	echo "" >> ../doc/README-SDL2.txt
	echo "	SDL2.dll" >> ../doc/README-SDL2.txt
	echo "" >> ../doc/README-SDL2.txt
	echo "is the runtime environment for the SDL library." >> ../doc/README-SDL2.txt
	tail --lines +3 $(SDL2)/README-SDL.txt >> ../doc/README-SDL2.txt
else
SDL2DISABLE = --disable-video-wayland --disable-audio
ifeq ($(PLATFORM), Darwin)
SDL2DISABLE += --disable-video-x11
endif

$(PREFIX)/lib/libSDL2main.a:	$(SDL2)/.ok
	$(MKDIR) $(SDL2)/build$(PLATFORMDIR)
	cd $(SDL2)/build$(PLATFORMDIR) && ../configure --prefix=$(PREFIX) --host=$(HOST) \
	  $(SDL2DISABLE)
	cd $(SDL2)/build$(PLATFORMDIR) && $(MAKE) && $(MAKE) install
	$(CP) $(SDL2)/README-SDL.txt ../doc/README-SDL2.txt
endif

$(PREFIX)/lib/libSDL2_image.a:	$(PREFIX)/lib/libjpeg.a
$(PREFIX)/lib/libSDL2_image.a:	$(PREFIX)/lib/libtiff.a
$(PREFIX)/lib/libSDL2_image.a:	$(PREFIX)/lib/libpng.a
$(PREFIX)/lib/libSDL2_image.a:	$(PREFIX)/lib/libSDL2main.a

$(PREFIX)/lib/libSDL2_image.a:	$(SDL2IMAGE)/.ok
	$(MKDIR) $(SDL2IMAGE)/build$(PLATFORMDIR)
	cd $(SDL2IMAGE)/build$(PLATFORMDIR) && PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig \
	  CC="$(CC) $(LDFLAGS)" CPPFLAGS=-I$(PREFIX)/include LDFLAGS="-L$(PREFIX)/lib" \
	  ../configure --prefix=$(PREFIX) --host=$(HOST) --disable-sdltest \
	  --disable-webp --disable-imageio \
	  --disable-jpg-shared --disable-png-shared --disable-tif-shared
	cd $(SDL2IMAGE)/build$(PLATFORMDIR) && $(MAKE) && $(MAKE) install
ifdef WIN32
	$(MKDIR) ../bin && $(CP) $(PREFIX)/bin/SDL2_image.dll ../bin && $(STRIP) ../bin/SDL2_image.dll
	$(OBJDUMP) -p $(PREFIX)/bin/SDL2_image.dll | grep 'DLL Name'
endif
	echo "$(SDL2IMAGE)" > ../doc/README-SDL2_image.txt
	echo "" >> ../doc/README-SDL2_image.txt
	echo "dependencies :" >> ../doc/README-SDL2_image.txt
	echo " - $(LIBPNG)" >> ../doc/README-SDL2_image.txt
	echo " - http://www.ijg.org/ JPEG lib $(JPEGVER)" >> ../doc/README-SDL2_image.txt
	echo " - $(LIBTIFF)" >> ../doc/README-SDL2_image.txt
	echo "" >> ../doc/README-SDL2_image.txt
	echo "License :" >> ../doc/README-SDL2_image.txt
	cat $(SDL2IMAGE)/COPYING.txt | tr -d "\r" >> ../doc/README-SDL2_image.txt

$(PREFIX)/lib/libSDL2_ttf.a:	$(PREFIX)/lib/libfreetype.a
$(PREFIX)/lib/libSDL2_ttf.a:	$(PREFIX)/lib/libSDL2main.a

$(PREFIX)/lib/libSDL2_ttf.a:	$(SDL2TTF)/.ok
	$(MKDIR) $(SDL2TTF)/build$(PLATFORMDIR)
	cd $(SDL2TTF)/build$(PLATFORMDIR) && PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig \
	  CC="$(CC) $(LDFLAGS)" CXX="$(CXX) $(LDFLAGS)" \
	  CPPFLAGS=-I$(PREFIX)/include LDFLAGS="-L$(PREFIX)/lib $(LDFLAGS)" \
	  ../configure --prefix=$(PREFIX) --with-sdl-prefix=$(PREFIX) \
	  --with-freetype-prefix=$(PREFIX) --with-ft-prefix=$(PREFIX) --host=$(HOST)
	cd $(SDL2TTF)/build$(PLATFORMDIR) && $(MAKE)
	cd $(SDL2TTF)/build$(PLATFORMDIR) && $(MAKE) install
ifdef WIN32
	$(OBJDUMP) -p $(PREFIX)/bin/SDL2_ttf.dll | grep 'DLL Name'
	$(MKDIR) ../bin && $(CP) $(PREFIX)/bin/SDL2_ttf.dll ../bin && $(STRIP) ../bin/SDL2_ttf.dll
endif
	echo "$(SDL2TTF)" > ../doc/README-SDL2_ttf.txt
	echo "" >> ../doc/README-SDL2_ttf.txt
	echo "dependencies :" >> ../doc/README-SDL2_ttf.txt
	echo " - $(FREETYPE)" >> ../doc/README-SDL2_ttf.txt
	echo "" >> ../doc/README-SDL2_ttf.txt
	echo "License :" >> ../doc/README-SDL2_ttf.txt
	if [ -f "$(SDL2TTF)/COPYING.txt" ] ; then cat "$(SDL2TTF)/COPYING.txt" | tr -d "\r" >> ../doc/README-SDL2_ttf.txt ; fi
	if [ -f "$(SDL2TTF)/LICENSE.txt" ] ; then cat "$(SDL2TTF)/LICENSE.txt" | tr -d "\r" >> ../doc/README-SDL2_ttf.txt ; fi

ifdef WIN32
$(PREFIX)/lib/libSDLmain.a:	archives/$(SDLDEVEL)
	$(TAR) xzf $<
	find $(SDL) -name \._\* -exec rm -v {} \;
	patch -p0 < $(SDLDEVELPATCH)
	$(MKDIR) $(PREFIX)
	cd $(SDL) && CROSS_PATH=$(PREFIX) $(MAKE) cross
	$(MKDIR) ../bin && $(CP) $(PREFIX)/bin/SDL.dll ../bin && $(STRIP) ../bin/SDL.dll
	echo "The following file:" > ../doc/README-SDL.txt
	echo "" >> ../doc/README-SDL.txt
	echo "	SDL.dll" >> ../doc/README-SDL.txt
	echo "" >> ../doc/README-SDL.txt
	echo "is the runtime environment for the SDL library." >> ../doc/README-SDL.txt
	tail --lines +3 $(SDL)/README-SDL.txt >> ../doc/README-SDL.txt
else
SDLDISABLE = --disable-joystick --disable-audio
ifdef ATARI
SDLDISABLE += --disable-video-opengl --disable-threads
endif
ifeq ($(PLATFORM), Darwin)
SDLDISABLE += --without-x
# the following trick was stolen from macports
ifeq ($(shell if [ "`uname -r | cut -f1 -d. `" = "8" ] && [ "`sysctl -n hw.vectorunit`" = "1" ] ; then echo "true" ; fi ), true)
SDLCFLAGS += -faltivec
endif
endif

$(PREFIX)/lib/libSDLmain.a:	$(SDL)/.ok
	$(MKDIR) $(SDL)/build$(PLATFORMDIR)
	cd $(SDL)/build$(PLATFORMDIR) && CFLAGS=$(SDLCFLAGS) \
	  ../configure --host=$(HOST) --prefix=$(PREFIX) \
	  $(SDLDISABLE)
	cd $(SDL)/build$(PLATFORMDIR) && $(MAKE)
	cd $(SDL)/build$(PLATFORMDIR) && $(MAKE) install
ifdef ATARI
	$(RM) $(PREFIX)/include/SDL/SDL_opengl.h
endif
	$(CP) $(SDL)/README-SDL.txt ../doc/README-SDL.txt
endif

ifndef WIN32
ifeq ($(PLATFORM), FreeBSD)
ADDLDFLAGS = -L/usr/local/lib
endif
endif

$(PREFIX)/lib/libSDL_image.a:	$(PREFIX)/lib/libjpeg.a
$(PREFIX)/lib/libSDL_image.a:	$(PREFIX)/lib/libtiff.a
$(PREFIX)/lib/libSDL_image.a:	$(PREFIX)/lib/libpng.a
$(PREFIX)/lib/libSDL_image.a:	$(PREFIX)/lib/libSDLmain.a

$(PREFIX)/lib/libSDL_image.a:	$(SDLIMAGE)/.ok
	$(MKDIR) $(SDLIMAGE)/build$(PLATFORMDIR)
	touch $(SDLIMAGE)/*.m4
	touch $(SDLIMAGE)/configure
	touch $(SDLIMAGE)/Makefile.in
ifneq ($(LDFLAGS),)
	cd $(SDLIMAGE)/build$(PLATFORMDIR) && CC="$(CC) $(LDFLAGS)" CPPFLAGS=-I$(PREFIX)/include \
	  LDFLAGS="-L$(PREFIX)/lib $(ADDLDFLAGS)" \
	  PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig \
	  ../configure --prefix=$(PREFIX) --with-sdl-prefix=$(PREFIX) \
	  --host=$(HOST) --build=$(BUILD) \
	  --disable-webp --disable-imageio --disable-sdltest \
	  --disable-jpg-shared --disable-png-shared --disable-tif-shared \
	  --disable-dependency-tracking
else
	cd $(SDLIMAGE)/build$(PLATFORMDIR) && CPPFLAGS=-I$(PREFIX)/include \
	  LDFLAGS="-L$(PREFIX)/lib $(ADDLDFLAGS)" \
	  PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig \
	  ../configure --prefix=$(PREFIX) --with-sdl-prefix=$(PREFIX) \
	  --host=$(HOST) --build=$(BUILD) \
	  --disable-webp --disable-imageio --disable-sdltest \
	  --disable-jpg-shared --disable-png-shared --disable-tif-shared \
	  --disable-dependency-tracking
endif
	cd $(SDLIMAGE)/build$(PLATFORMDIR) && $(MAKE)
	cd $(SDLIMAGE)/build$(PLATFORMDIR) && $(MAKE) install
ifdef WIN32
	$(MKDIR) ../bin && $(CP) $(PREFIX)/bin/SDL_image.dll ../bin && $(STRIP) ../bin/SDL_image.dll
endif
	echo "$(SDLIMAGE)" > ../doc/README-SDL_image.txt
	echo "" >> ../doc/README-SDL_image.txt
	echo "dependencies :" >> ../doc/README-SDL_image.txt
	echo " - $(LIBPNG)" >> ../doc/README-SDL_image.txt
	echo " - http://www.ijg.org/ JPEG lib $(JPEGVER)" >> ../doc/README-SDL_image.txt
	echo " - $(LIBTIFF)" >> ../doc/README-SDL_image.txt
	echo "" >> ../doc/README-SDL_image.txt
	echo "License :" >> ../doc/README-SDL_image.txt
	cat $(SDLIMAGE)/COPYING >> ../doc/README-SDL_image.txt

$(PREFIX)/lib/libSDL_ttf.a:	$(PREFIX)/lib/libfreetype.a
$(PREFIX)/lib/libSDL_ttf.a:	$(PREFIX)/lib/libSDLmain.a

$(PREFIX)/lib/libSDL_ttf.a:	$(SDLTTF)/.ok
	$(MKDIR) $(SDLTTF)/build$(PLATFORMDIR)
ifdef WIN32
	$(CP) $(PREFIX)/bin/SDL.dll $(SDLTTF)/build$(PLATFORMDIR)
endif
	cd $(SDLTTF)/build$(PLATFORMDIR) && PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig \
	  CC=$(CC) CPPFLAGS=-I$(PREFIX)/include LDFLAGS="-L$(PREFIX)/lib $(LDFLAGS) $(ADDLDFLAGS)" \
	  ../configure --prefix=$(PREFIX) --with-sdl-prefix=$(PREFIX) --with-freetype-prefix=$(PREFIX) \
	  --host=$(HOST)
ifdef ATARI
	sed -i.bak 's/-O2/-O0/' $(SDLTTF)/build$(PLATFORMDIR)/Makefile
endif
	cd $(SDLTTF)/build$(PLATFORMDIR) && $(MAKE)
	cd $(SDLTTF)/build$(PLATFORMDIR) && $(MAKE) install
ifdef WIN32
	$(MKDIR) ../bin && $(CP) $(PREFIX)/bin/SDL_ttf.dll ../bin && $(STRIP) ../bin/SDL_ttf.dll
endif
	echo "$(SDLTTF)" > ../doc/README-SDL_ttf.txt
	echo "" >> ../doc/README-SDL_ttf.txt
	echo "dependencies :" >> ../doc/README-SDL_ttf.txt
	echo " - $(FREETYPE)" >> ../doc/README-SDL_ttf.txt
	echo "" >> ../doc/README-SDL_ttf.txt
	echo "License :" >> ../doc/README-SDL_ttf.txt
	cat $(SDLTTF)/COPYING >> ../doc/README-SDL_ttf.txt

$(PREFIX)/lib/libfreetype.a:	$(PREFIX)/lib/libpng.a

$(PREFIX)/lib/libfreetype.a:	$(FREETYPE)/.ok
	$(MKDIR) $(FREETYPE)/build$(PLATFORMDIR)
	cd $(FREETYPE)/build$(PLATFORMDIR) && ../configure --build=$(BUILD) --host=$(HOST) \
	  --prefix=$(PREFIX) --enable-freetype-config \
	  PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig CC="$(CC) $(LDFLAGS)"
	cd $(FREETYPE)/build$(PLATFORMDIR) && $(MAKE) && $(MAKE) install
ifdef WIN32
	$(MKDIR) ../bin && for f in $(PREFIX)/bin/libfreetype*.dll ; do \
	    $(CP) $$f ../bin ; \
	    $(STRIP) ../bin/`basename $$f` ; \
	  done
endif
	echo "$(FREETYPE)" > ../doc/README-freetype.txt
	echo "" >> ../doc/README-freetype.txt
	echo "License :" >> ../doc/README-freetype.txt
	echo "" >> ../doc/README-freetype.txt
	cat $(FREETYPE)/docs/GPLv2.TXT >> ../doc/README-freetype.txt

$(PREFIX)/lib/libjpeg.a:	$(JPEGDIR)/.ok
	$(MKDIR) $(JPEGDIR)/build$(PLATFORMDIR)
	cd $(JPEGDIR)/build$(PLATFORMDIR) && CC="$(CC) $(LDFLAGS)" \
	  ../configure --prefix=$(PREFIX) --host=$(HOST)
	cd $(JPEGDIR)/build$(PLATFORMDIR) && $(MAKE)
	cd $(JPEGDIR)/build$(PLATFORMDIR) && $(MAKE) install
ifdef WIN32
	$(MKDIR) ../bin && for f in $(PREFIX)/bin/libjpeg*.dll ; do \
	    $(OBJDUMP) -p $$f | grep 'DLL Name' ; \
	    $(CP) $$f ../bin ; \
	    $(STRIP) ../bin/`basename $$f` ; \
	  done
endif
	$(CP) $(JPEGDIR)/README ../doc/README-jpeg.txt

# libjpeg archive name and directory name are not the same
# so the generic rule won't work
$(JPEGDIR)/.ok:	archives/$(JPEGARCH)
	$(TAR) xzf $<
	touch $@

$(PREFIX)/lib/libtiff.a:	$(PREFIX)/lib/libjpeg.a
$(PREFIX)/lib/libtiff.a:	$(PREFIX)/lib/libz.a

$(PREFIX)/lib/libtiff.a:	$(LIBTIFF)/.ok
	$(MKDIR) $(LIBTIFF)/_build$(PLATFORMDIR)
	cd $(LIBTIFF)/_build$(PLATFORMDIR) && CC="$(CC) $(LDFLAGS)" \
	  ../configure --prefix=$(PREFIX) \
	  --host=$(HOST) --build=$(BUILD) \
	  --with-zlib-include-dir=$(PREFIX)/include --with-zlib-lib-dir=$(PREFIX)/lib \
	  --with-jpeg-include-dir=$(PREFIX)/include --with-jpeg-lib-dir=$(PREFIX)/lib \
	  --enable-silent-rules \
	  --disable-cxx --disable-webp --disable-lzma --disable-zstd
	cd $(LIBTIFF)/_build$(PLATFORMDIR) && $(MAKE)
	cd $(LIBTIFF)/_build$(PLATFORMDIR) && $(MAKE) install
ifdef WIN32
	$(MKDIR) ../bin && for f in $(PREFIX)/bin/libtiff*.dll ; do \
	    $(CP) $$f ../bin ; \
	    $(STRIP) ../bin/`basename $$f` ; \
	  done
endif
	echo "$(LIBTIFF)" > ../doc/README-tiff.txt
	echo "" >> ../doc/README-tiff.txt
	echo "License :" >> ../doc/README-tiff.txt
	echo "" >> ../doc/README-tiff.txt
	if [ -f $(LIBTIFF)/COPYRIGHT ] ; then \
	  cat $(LIBTIFF)/COPYRIGHT >> ../doc/README-tiff.txt ; \
	else \
	  tail -n +3 $(LIBTIFF)/LICENSE.md >> ../doc/README-tiff.txt ; \
	fi

$(PREFIX)/lib/libpng.a:	$(PREFIX)/lib/libz.a

$(PREFIX)/lib/libpng.a:	$(LIBPNG)/.ok
	$(MKDIR) $(LIBPNG)/build$(PLATFORMDIR)
	cd $(LIBPNG)/build$(PLATFORMDIR) && CC="$(CC) $(LDFLAGS)" CPPFLAGS="-I$(PREFIX)/include" LDFLAGS="-L$(PREFIX)/lib" \
	  ../configure --prefix=$(PREFIX) --host=$(HOST) --enable-silent-rules \
	  --disable-dependency-tracking
	cd $(LIBPNG)/build$(PLATFORMDIR) && $(MAKE) && $(MAKE) install
ifdef WIN32
	$(MKDIR) ../bin && for f in $(PREFIX)/bin/libpng*.dll ; do \
	    $(CP) $$f ../bin ; \
	    $(STRIP) ../bin/`basename $$f` ; \
	  done
endif
	echo "$(LIBPNG)" > ../doc/README-libpng.txt
	echo "" >> ../doc/README-libpng.txt
	echo "License :" >> ../doc/README-libpng.txt
	echo "" >> ../doc/README-libpng.txt
	cat $(LIBPNG)/LICENSE >> ../doc/README-libpng.txt

$(PREFIX)/lib/libz.a:	$(ZLIB)/.ok
	if [ "$(ZLIB)" = "zlib-1.2.12" ]; then cd $(ZLIB) ; curl https://github.com/madler/zlib/commit/05796d3d8d5546cf1b4dfe2cd72ab746afae505d.patch | patch -p1 -N || true ; fi
ifdef WIN32
	cd $(ZLIB) && $(MAKE) -fwin32/Makefile.gcc PREFIX=$(shell echo $(CC) | sed 's/^\(.*\)gcc/\1/') LOC="$(LDFLAGS) $(CFLAGS)"
	cd $(ZLIB) && INCLUDE_PATH=$(PREFIX)/include LIBRARY_PATH=$(PREFIX)/lib BINARY_PATH=$(PREFIX)/bin \
	  $(MAKE) install -fwin32/Makefile.gcc SHARED_MODE=1
	$(MKDIR) ../bin && for f in $(PREFIX)/bin/zlib*.dll ; do \
	    $(CP) $$f ../bin ; \
	    $(STRIP) ../bin/`basename $$f` ; \
	  done
else
	$(MKDIR) $(ZLIB)/build$(PLATFORMDIR)
	# configure [--const] [--zprefix] [--prefix=PREFIX]  [--eprefix=EXPREFIX]
	#  [--static] [--64] [--libdir=LIBDIR] [--sharedlibdir=LIBDIR]
	#  [--includedir=INCLUDEDIR] [--archs="-arch i386 -arch x86_64"]
	cd $(ZLIB)/build$(PLATFORMDIR) && CC=$(CC) AR=$(AR) RANLIB=$(RANLIB) \
	  ../configure --prefix=$(PREFIX)
ifdef ATARICROSS
	cd $(ZLIB)/build$(PLATFORMDIR) && $(MAKE)
else
	cd $(ZLIB)/build$(PLATFORMDIR) && $(MAKE) test
endif
	cd $(ZLIB)/build$(PLATFORMDIR) && $(MAKE) install
endif
	$(CP) $(ZLIB)/README ../doc/README-zlib1.txt

recoil:	$(RECOIL)/.ok
	$(CP) $(RECOIL)/README ../doc/README-recoil.txt

6502:	6502/.ok
	$(CP) 6502/README ../doc/README-6502.txt

6502/.ok:	archives/$(REDCODE6502ARCH)
	$(TAR) xJf $<
	cd $(@D) ; for p in $(REDCODE6502PATCHES) ; do echo "applying $$p" ; patch -p1 < ../$$p ; done
	touch $@

importgpgkeys:	gpgkeys/imported.ok

gpgkeys/imported.ok:	$(filter-out gpgkeys/imported.ok,$(wildcard gpgkeys/*))
	$(GPG) --import $^
	touch $@

# generic rule to unpack tarball and apply patches
%/.ok:	archives/%.tar.gz
	$(TAR) xzf $<
	cd $(@D) ; for p in $($(shell echo $* | cut -d- -f1 | tr a-z A-Z | tr -d _)PATCHES) ; do echo "applying $$p" ; patch -p1 < ../$$p ; done
	touch $@

# the following archive names won't work with the generic rule
archives/$(JPEGARCH):
	@$(MKDIR) $(@D)
	cd $(@D) && $(GETURL) $(JPEGURL)
	@echo $(JPEGURL) > JPEG.source

archives/$(REDCODE6502ARCH):
	@$(MKDIR) $(@D)
	cd $(@D) && $(GETURL) $(REDCODE6502URL)
	@[ "`$(SHA256CMD) < $@`" = "$(REDCODE6502SHA256)" ] || ( $(RM) $@ && echo "$@ SHA256 mismatch !" && false )
	@echo $(REDCODE6502URL) > 6502.source

# generic rule to download tarballs
archives/%.tar.gz:
	@$(MKDIR) $(@D)
	$(eval BASE := $(shell echo $* | sed 's/-devel/devel/' | cut -d- -f1 | tr a-z A-Z | tr -d _))
	$(eval URL = $($(BASE)URL))
	$(eval URLALT = $($(BASE)URLALT))
	$(eval SHA256 = $($(BASE)SHA256))
	$(eval SIG = $($(BASE)SIG))
	$(eval SIGURL = $($(BASE)SIGURL))
	$(eval SIGURLALT = $($(BASE)SIGURLALT))
	@if [ -z "$(SIGURL)" ] && [ -z "$(SHA256)" ] ; then echo "Need either signature or hash to check the archive $@" ; false ; fi
	@echo "$*: fetching $(URL) (or $(URLALT)) $(SIG)"
	@cd $(@D) && ( $(GETURL) $(URL) || ( [ -n "$(URLALT)" ] && $(GETURL) $(URLALT) ) )
	@[ -z "$(SIGURL)" ] || ( cd $(@D) && ( $(GETURL) $(SIGURL) || ( [ -n "$(SIGURLALT)" ] && $(GETURL) $(SIGURLALT) ) ) && $(GPG) --verify $(SIG) ) || ( $(RM) $@ && false )
	@[ -z "$(SHA256)" ] || [ "`$(SHA256CMD) < $@`" = "$(SHA256)" ] || ( $(RM) $@ && echo "$@ SHA256 mismatch !" && false )
	@echo $(URL) > $(BASE).source

# generic rule to check package version
checkversion-%:
	@echo "checking $* version"
	$(eval PKG := $(shell echo $* | tr a-z A-Z))
	$(eval LATEST := $(LATEST$(PKG)VER))
	@if [ -z "$(LATEST)" ] ; then \
	  echo "failed to retrieve latest version of $*" ; \
	  touch check-version-failed ; \
	elif [ "$($(PKG)VER)" != "$(LATEST)" ] ; then \
	  echo "$* $($(PKG)VER) is outdated, latest version is $(LATEST)" ; \
	  if [ "$*" = "lua" ] && [ "$($(PKG)VER)" = "$(LATESTLUA53VER)" ] ; then \
	    echo "But it is the last lua 5.3 release." ; \
	  else \
	    touch check-version-failed ; \
	  fi ; \
	fi
