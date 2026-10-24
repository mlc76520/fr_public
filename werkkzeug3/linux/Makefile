# Native Linux build of the werkkzeug3 demo player (fr-038 theta).
# 32-bit only: the engine assumes 32-bit pointers and the x86 stdcall ABI.

SRC   := ..
DXVK  ?= $(HOME)/theta-port/dxvk-native32
OUT   := build

CXX      := g++
BASEFLAGS := -m32 -O2 -g -MMD -MP -msse2 -mfpmath=sse -fpermissive -fno-strict-aliasing -fno-delete-null-pointer-checks -fwrapv \
            -fno-exceptions -w \
            -include compat.hpp \
            -I. -I$(SRC) -I$(SRC)/materials -I$(SRC)/werkkzeug3 -I$(SRC)/base -I$(OUT)/gen \
            -I$(DXVK)/include/dxvk $(shell PKG_CONFIG_PATH=/usr/lib/pkgconfig pkg-config --cflags sdl2)
CXXFLAGS := $(BASEFLAGS) -include player_config.hpp
LDFLAGS  := -m32 -L$(DXVK)/lib -Wl,-rpath,'$$ORIGIN/lib' -Wl,-rpath,$(DXVK)/lib
LIBS     := -ldxvk_d3d9 $(shell PKG_CONFIG_PATH=/usr/lib/pkgconfig pkg-config --libs sdl2) \
            -lvorbis -logg -lm

PLAYER_SRCS := \
  _types.cpp _ogg.cpp engine.cpp kdoc.cpp mainplayer.cpp shadercodegen.cpp \
  genbitmap.cpp genblobspline.cpp geneffect.cpp geneffectdebris.cpp \
  geneffectex.cpp geneffectipp.cpp genmaterial.cpp genmesh.cpp genminmesh.cpp \
  genoverlay.cpp genscene.cpp linux/geneffectcubes_stub.cpp \
  materials/material11.cpp materials/material20.cpp \
  materials/materialdirect.cpp materials/rtmanager.cpp \
  player_demo/demo_oplist.cpp _start.cpp base/_rygdxt.cpp \
  fried/decode.cpp fried/entropy.cpp fried/pixel.cpp fried/quantize.cpp fried/transforms.cpp

PLAYER_OBJS := $(addprefix $(OUT)/obj/,$(PLAYER_SRCS:.cpp=.o))

# host tools (shader assembler, .k -> .kx exporter) use the tool configuration
TOOLFLAGS := $(BASEFLAGS) -I$(SRC)/base_console -I$(SRC)/base

SHADERCOMPILE_OBJS := $(addprefix $(OUT)/tools/,shadercompile/main.o shadercompile.o _types.o _intmath.o linux/startconsole_linux.o)

VSH := $(notdir $(wildcard $(SRC)/materials/*.vsh $(SRC)/werkkzeug3/effect_*.vsh))
PSH := $(notdir $(wildcard $(SRC)/materials/*.psh $(SRC)/werkkzeug3/effect_*.psh))
SHADER_HDRS := $(VSH:%.vsh=$(OUT)/gen/%vs.hpp) $(PSH:%.psh=$(OUT)/gen/%ps.hpp) \
               $(OUT)/gen/materials/material11vs.hpp $(OUT)/gen/materials/material11ps.hpp

all: $(OUT)/theta

shaders: $(SHADER_HDRS)

# shader headers: foo.vsh -> foovs.hpp (run from the source dir, like the VS custom build step)
$(OUT)/gen/%vs.hpp: $(OUT)/shadercompile
	@mkdir -p $(OUT)/gen
	src=$$(ls $(SRC)/materials/$*.vsh $(SRC)/werkkzeug3/$*.vsh 2>/dev/null | head -1); \
	cd $$(dirname $$src) && $(CURDIR)/$(OUT)/shadercompile --out-header $(CURDIR)/$@ $$(basename $$src)
$(OUT)/gen/%ps.hpp: $(OUT)/shadercompile
	@mkdir -p $(OUT)/gen
	src=$$(ls $(SRC)/materials/$*.psh $(SRC)/werkkzeug3/$*.psh 2>/dev/null | head -1); \
	cd $$(dirname $$src) && $(CURDIR)/$(OUT)/shadercompile --out-header $(CURDIR)/$@ $$(basename $$src)
$(OUT)/gen/materials/%.hpp: $(OUT)/gen/%.hpp
	@mkdir -p $(dir $@)
	cp $< $@

$(OUT)/theta: $(PLAYER_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OUT)/obj/%.o: $(SRC)/%.cpp | shaders
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT)/shadercompile: $(SHADERCOMPILE_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ -lm

$(OUT)/tools/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(TOOLFLAGS) -c $< -o $@

# .k -> .kx exporter: the editor's document model and operator table, no GUI
# (the GUI objects the model talks to are stand-ins without vtables, hence the
# non-virtual call patched into the extracted code)
KEXPORT_SRCS := \
  _types.cpp _intmath.cpp _ogg.cpp engine.cpp kdoc.cpp shadercodegen.cpp \
  genbitmap.cpp genblobspline.cpp geneffect.cpp geneffectdebris.cpp \
  geneffectex.cpp geneffectipp.cpp genmaterial.cpp genmesh.cpp genminmesh.cpp \
  genoverlay.cpp genscene.cpp genvector.cpp gengeloet.cpp kkriegergame.cpp \
  materials/material11.cpp materials/material20.cpp \
  materials/materialdirect.cpp materials/rtmanager.cpp \
  base/_rygdxt.cpp werkops.cpp linux/geneffectcubes_stub.cpp _util.cpp \
  fried/decode.cpp fried/encode.cpp fried/entropy.cpp fried/pixel.cpp \
  fried/quantize.cpp fried/transforms.cpp \
  linux/kexport/werkdoc_model.cpp linux/kexport/winpara_headless.cpp \
  shadercompile.cpp _wavefront.cpp _xsi.cpp _loader.cpp \
  linux/kexport/kexport.cpp
KEXPORT_OBJS := $(addprefix $(OUT)/kexport/,$(KEXPORT_SRCS:.cpp=.o)) $(OUT)/obj/_start_nomain.o
KEXPORTFLAGS := $(BASEFLAGS) -include kexport/kexport_config.hpp -Ikexport -I$(OUT)/gen/kexport

$(OUT)/gen/kexport/werkdoc_model.inc: $(SRC)/werkkzeug.cpp
	@mkdir -p $(dir $@)
	sed -n '/^\/\*\*\*   document  /,/^\/\*\*\*   File-Browser Support/p' $< | \
	  sed 's/FindResultsWin->Clear()/FindResultsWin->WinFindResults::Clear()/g' > $@

$(OUT)/gen/kexport/winpara_animops.inc: $(SRC)/winpara.cpp
	@mkdir -p $(dir $@)
	sed -n '/^const WerkOpAnimInfo AnimOpCmds\[\] = /,/^};/p' $< > $@

$(OUT)/kexport/linux/kexport/werkdoc_model.o: $(OUT)/gen/kexport/werkdoc_model.inc
$(OUT)/kexport/linux/kexport/winpara_headless.o: $(OUT)/gen/kexport/winpara_animops.inc

$(OUT)/kexport/%.o: $(SRC)/%.cpp | shaders
	@mkdir -p $(dir $@)
	$(CXX) $(KEXPORTFLAGS) -c $< -o $@

# the player's system layer, without its main()
$(OUT)/obj/_start_nomain.o: $(SRC)/_start.cpp | shaders
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -DsLINUX_NO_MAIN -c $< -o $@

$(OUT)/kexport/kexport: $(KEXPORT_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

kexport: $(OUT)/kexport/kexport

# self-contained directory: player, DXVK, theta.kx, fonts
DIST     ?= $(OUT)/dist
THETA_K  := $(SRC)/data/theta/theta_final.k
# webdings.ttf: Wine ships a free one (also inside Proton); WEBDINGS=path overrides
WEBDINGS_DIRS := $(HOME)/.steam/steam/steamapps/common /games/SteamLibrary/steamapps/common \
                 /usr/share/wine/fonts /usr/lib/wine /usr/lib64/wine

dist: $(OUT)/theta $(OUT)/kexport/kexport
	@mkdir -p $(DIST)/lib $(DIST)/fonts
	cp $(OUT)/theta $(DIST)/theta
	cp -L $(DXVK)/lib/libdxvk_d3d9.so.0 $(DIST)/lib/
	$(OUT)/kexport/kexport $(THETA_K) $(DIST)/theta.kx > /dev/null
	@w="$(WEBDINGS)"; [ -n "$$w" ] || w=$$(find $(WEBDINGS_DIRS) -name webdings.ttf -path '*wine/fonts*' 2>/dev/null | head -1); \
	if [ -n "$$w" ]; then cp "$$w" $(DIST)/fonts/webdings.ttf && echo "fonts: $$w"; \
	else echo "note: no webdings.ttf found (one symbol will be missing)"; fi
	cp README.md $(SRC)/data/theta/readme.txt $(SRC)/data/theta/LICENSE $(DIST)/ 2>/dev/null || true
	@echo "-> $(DIST)/theta"

# AppImage: the dist directory plus the 32-bit libraries distributions often
# lack. Needs appimagetool (APPIMAGETOOL=path); the host still needs 32-bit
# glibc, libstdc++, a Vulkan driver and X11/Wayland libraries (as for Steam).
APPDIR       := $(OUT)/AppDir
APPIMAGETOOL ?= appimagetool
APPLIBS      := libSDL2-2.0.so.0 libSDL3.so.0 libvorbis.so.0 libvorbisfile.so.3 libogg.so.0
APPLIBDIR    ?= /usr/lib

appimage:
	rm -rf $(APPDIR)
	$(MAKE) dist DIST=$(APPDIR)/usr/bin
	for l in $(APPLIBS); do cp -L $(APPLIBDIR)/$$l $(APPDIR)/usr/bin/lib/ || exit 1; done
	printf '%s\n' '#!/bin/sh' 'HERE="$$(dirname "$$(readlink -f "$$0")")"' \
	  'export LD_LIBRARY_PATH="$$HERE/usr/bin/lib$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH}"' \
	  'export DXVK_LOG_PATH="$${DXVK_LOG_PATH:-none}"' \
	  'exec "$$HERE/usr/bin/theta" "$$@"' > $(APPDIR)/AppRun
	chmod +x $(APPDIR)/AppRun
	printf '%s\n' '[Desktop Entry]' 'Type=Application' 'Name=fr-038: theta' \
	  'Comment=farbrausch demo (The Ultimate Meeting 2004), native Linux port' \
	  'Exec=theta' 'Icon=theta' 'Categories=AudioVideo;' 'Terminal=false' > $(APPDIR)/theta.desktop
	cp theta.png $(APPDIR)/theta.png
	ln -sf theta.png $(APPDIR)/.DirIcon
	$(APPIMAGETOOL) $(APPDIR) $(OUT)/theta-i686.AppImage

clean:
	rm -rf $(OUT)

-include $(shell find $(OUT) -name '*.d' 2>/dev/null)

.PHONY: all clean shaders kexport dist appimage
