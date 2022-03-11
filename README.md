# bsnes-plus with NWAccess

Implements [emulator-networkaccess](https://github.com/usb2snes/emulator-networkaccess) protocol to interact with auto-splitters and trackers through [QUsb2Snes](https://github.com/Skarsnik/QUsb2snes).\
See also [client library](https://github.com/black-sliver/EmuNWAccess-qt) and [test client](https://github.com/black-sliver/EmuNWAccessTest).

Original README below

# bsnes-plus

bsnes-plus (or bsnes+) is a fork of bsnes (based on bsnes-classic) intended to
introduce some new features and improvements, mostly aimed at debugging.

## What's new

- Improved debugger UI with register editing
- Redesigned memory editor and breakpoint editor
- Improved handling of address mirroring for breakpoints (extends to the entire address space, not just RAM)
- Real-time code and data highlighting in memory editor, with fast searching for known code/data locations and unexplored regions
- Cartridge ROM and RAM views in memory editor for mapper-agnostic analysis
- Enhanced VRAM, sprite, and tilemap viewing
- SA-1 disassembly and debugging
- SA-1 bus and BW-RAM viewing and (partial) usage logging
- Super FX disassembly and debugging
- Super FX bus viewing and usage logging

Non-debugging features:

- Satellaview / BS-X support
- SPC file dumping
- SPC output visualizer (keyboards & peak meters)
- IPS and BPS soft patching
- Multiple emulation improvements backported from bsnes/higan (mostly via bsnes-classic)

## Building on Windows

- [Get mingw-w64](http://mingw-w64.yaxm.org/doku.php/download) (make sure toolchain supports 64-bit builds)
- Initialize the bsnes-plus-ext-qt submodule in git
- Run `mingw32-make`

## Building on OS X

```
brew install qt@5
make
```

Unfortunately due to complexity with homebrew's mixed inclusion of paths using symlinks and absolute paths in the dylib files of qt, the build is sensitive to the specific qt@5 package version installed and its installed absolute path. Currently the Makefiles assume qt 5.15.2_1 but this can be overridden by specifying the Makefile variable `qtabspath` which should point at the Cellar folder for the qt@5 installation.

## Building on Linux / other *nix

As there is no ``configure`` step, make sure necessary Qt5/X11 packages are installed. On a Debian/Ubuntu system, it would require a command like:

```
sudo apt install qt5-default qtbase5-dev-tools libxv-dev libsdl1.2-dev libao-dev libopenal-dev g++
```

Afterwards, run ``make`` and if everything works out correctly you will find the output binary in the ``out/`` directory.

The snesfilter, snesreader, and supergameboy plugins can all be built by running make (or mingw32-make) after you've configured your environment to build bsnes itself.
After building, just copy the .dll, .so, or .dylib files into the same directory as bsnes itself.

bsnes v073 and its derivatives are licensed under the GPL v2; see *Help > License ...* for more information.

## Contributors

See *Help > Documentation ...* for a list of authors.
