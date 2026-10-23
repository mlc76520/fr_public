# Native Linux build of the werkkzeug3 demo player (fr-038 theta).
# 32-bit only: the engine assumes 32-bit pointers and the x86 stdcall ABI.

SRC   := ..
DXVK  ?= $(HOME)/theta-port/dxvk-native32
OUT   := build

CXX      := g++
BASEFLAGS := -m32 -O2 -g -MMD -MP -msse2 -mfpmath=sse -fpermissive -fno-strict-aliasing \
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

clean:
	rm -rf $(OUT)

-include $(shell find $(OUT) -name '*.d' 2>/dev/null)

.PHONY: all clean shaders
