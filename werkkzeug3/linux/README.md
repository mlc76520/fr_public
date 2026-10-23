# werkkzeug3 demo player for Linux (fr-038: theta)

Native 32-bit Linux build of the werkkzeug3 demo player, used to run
farbrausch's *fr-038: theta* (The Ultimate Meeting 2004) from its sources
(`../data/theta`). Direct3D 9 is provided by DXVK-native (Vulkan), window,
input and sound by SDL2, music by libvorbis.

## Running

    ./theta [options] [file.kx]

    -w, --window        run in a window (default: fullscreen)
    -s, --size WxH      window size (default: 1280x720)
    -l, --loop          loop the demo
        --no-vsync      don't wait for vertical sync

Esc quits, F11 toggles fullscreen. Without a file name, `theta.kx` next to the
executable is used. Fonts are taken from `fonts/` next to the executable
(Windows names: `arial.ttf`, `webdings.ttf`), then from fontconfig.

## Building

Requirements (Fedora): `gcc-c++ glibc-devel.i686 libstdc++-devel.i686
sdl2-compat-devel.i686 libvorbis-devel.i686 libogg-devel.i686
vulkan-loader-devel.i686`, plus a 32-bit DXVK-native build with D3D9 and the
SDL2 backend (`DXVK=...`, default `~/theta-port/dxvk-native32`).

    make            # build/theta, the player
    make kexport    # build/kexport/kexport, the .k -> .kx exporter
    make dist       # build/dist: player + DXVK + theta.kx + fonts

`kexport input.k output.kx [--song file.ogg]` does what "File/Export" did in
the Windows editor: it compiles the editor's document model and operator
table without its GUI (`kexport/`).

## How the port works

* `compat.hpp` (forced include) maps MSVC keywords to GCC. The code assumes
  32-bit pointers and the x86 stdcall ABI (`kdoc.cpp` pushes operator
  parameters on the stack), hence `-m32`.
* MSVC inline assembly: replaced by C, GCC asm (operator calls, x87 control
  word) or MMX intrinsics that keep the original instruction sequence, so
  textures come out bit-identical (`genbitmap_mmx_linux.hpp`,
  `fried/fried_mmx_linux.hpp`).
* `_start.cpp` keeps its Direct3D code; the Win32 parts are under `#if !sLINUX`
  and replaced by `start_sdl.inl` (SDL2 window and events, SDL audio instead of
  the DirectSound thread, POSIX files, stb_image instead of OleLoadPicture,
  stb_truetype instead of GDI text).
* Compiler flags `-fno-delete-null-pointer-checks -fwrapv`: the code calls
  methods on null pointers and relies on MSVC's integer behaviour.
* Not ported: the V2 synth (x86 assembler; theta uses an .ogg), the 2006
  "cubes" effects (need the Windows-only HLSL compiler), 3D text (GDI glyph
  outlines + GLU tessellator). theta uses none of them.

Testing aid: `THETA_SHOTS=5,30,60 THETA_SHOTDIR=dir ./theta` writes PNG
screenshots at the given music times (seconds).
