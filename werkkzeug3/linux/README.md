# werkkzeug3 demo player for Linux (fr-038: theta)

Native 32-bit Linux build of the werkkzeug3 demo player, used to run
farbrausch's *fr-038: theta* (The Ultimate Meeting 2004) from its sources
(`../data/theta`). Direct3D 9 is provided by DXVK-native (Vulkan), window,
input and sound by SDL2, music by libvorbis.

## Running

    ./theta [options] [file.kx]

    -w, --window        run in a window (default: fullscreen)
    -s, --size WxH      window size (default: 1280x720)
        --aspect W:H    shape of the picture area (default 21:9; `fill` uses
                        the whole screen)
        --view W:H      field of view: the vertical one of a W:H picture
                        (default 16:9; 2:1 is the 2007 player, 4:3 the 2004 one)
        --stretch       stretch the --view picture instead of widening it
    -l, --loop          loop the demo
        --no-vsync      don't wait for vertical sync

Esc quits, F11 toggles fullscreen. The picture keeps the vertical field of
view of a `--view` shaped one and shows more on the sides when it is wider (a
shape within 3% of the screen's fills it, so 21:9 covers a 3440x1440 screen).
theta (2004) was made for a player projecting for 4:3; the 2007 player in
this repository projects for 2:1; 16:9 is in between and avoids showing the
unlit edges of the scenes on wide screens. `--aspect 4:3 --view 4:3` looks
like the original 800x600 player under Wine. Without a file name, `theta.kx` next to the
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
    make appimage   # build/theta-i686.AppImage (APPIMAGETOOL=path/to/appimagetool)

The AppImage bundles DXVK, SDL2/SDL3 and Vorbis (32-bit); the system still
needs 32-bit glibc (2.43 or later when built on Fedora 44), libstdc++, a 32-bit
Vulkan driver and X11/Wayland libraries, like 32-bit Steam games.

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

Engine fixes needed by theta (2004 content on the 2007 engine):
`GENOVER_RTSIZES` (the image post-processing render targets, disabled in
player builds as a "hack for debris") is enabled again, and effects (particles)
are sorted after the light/post-light phases like other single-pass jobs.
Views rendered into textures (the far end of the tunnels, the spider's eyes)
keep the 4:3 projection: the textures are shown on 4:3 quads, the 2:1 one
squeezed them and the tunnels' nested far ends drifted into steps.
The "full size" post-processing render targets are as large as the screen
instead of 1024x512 (most scenes are rendered into them, so above 1024 pixels
the picture was upscaled and edges turned into steps); blur offsets on them
are scaled so effects keep their size on screen.

Testing aids (environment variables):
`THETA_SHOTS=5,30,60 THETA_SHOTDIR=dir` writes PNG screenshots at the given
music times (seconds), `THETA_START=t` starts the music (and the demo) at t
seconds, `THETA_FPS=n` caps the frame rate, `THETA_DUMPTEX=dir`
and `THETA_DUMPSHADERS=dir` write the generated textures and shaders.
`kexport --dump ops.txt` lists operators, parameters and timeline events,
`--disable-event name` leaves an event out of the export.
