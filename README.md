# Quiver

A cross-platform pseudo-3D game engine written in C++ using SFML and Box2D

## Important Note!

Quiver is far from finished. It's not even ready for its first 'release' yet. You definitely shouldn't seriously attempt to make a game with it.

## Getting Started

You will need:

- [Premake 5](https://premake.github.io/download.html)
- [SFML 2.4.2](https://www.sfml-dev.org/download/sfml/2.4.2/)

Clone the Quiver repository. 

Run Premake in the root directory (where you'll find `premake5.lua`), specifying where to find the SFML header and library files using the `--sfmlinc` and `--sfmllib` parameters. In this example, Quiver has been cloned to `C:/Dev/Quiver`, the correct version of SFML 2.4.2 has been unzipped to `C:/SFML-2.4.2`, and I am generating a Visual Studio 2017 workspace:

```
PS C:\Dev\Quiver> premake5 vs2017 --sfmlinc=C:/SFML-2.4.2/include/ --sfmllib=C:/SFML-2.4.2/lib/
```

Premake generates the Visual Studio 2017 files, sticking them in the PremakeGenerated folder:

```
Generated PremakeGenerated/Quiver.sln...
Generated PremakeGenerated/Box2D.vcxproj...
Generated PremakeGenerated/Box2D.vcxproj.user...
Generated PremakeGenerated/Box2D.vcxproj.filters...
Generated PremakeGenerated/ImGui-SFML.vcxproj...
Generated PremakeGenerated/ImGui-SFML.vcxproj.user...
Generated PremakeGenerated/Quiver.vcxproj...
Generated PremakeGenerated/Quiver.vcxproj.user...
Generated PremakeGenerated/Quiver.vcxproj.filters...
Generated PremakeGenerated/QuiverTests.vcxproj...
Generated PremakeGenerated/QuiverTests.vcxproj.user...
Generated PremakeGenerated/QuiverApp.vcxproj...
Generated PremakeGenerated/QuiverApp.vcxproj.user...
```

You should now be able to build Quiver using Visual Studio. Hopefully it's possible to adapt these instructions to different build systems and platforms!

## Overview

The workspace is divided into a few projects:

- **Box2D** - A mostly-unmodified clone of the popular open-source 2D [physics engine](https://github.com/erincatto/Box2D), built as a static library.
- **ImGui-SFML** - A static library containing [ImGui](https://github.com/ocornut/imgui) compiled alongside [SFML bindings](https://github.com/eliasdaler/imgui-sfml)
- **Quiver** - The core of Quiver, containing all the engine code. Compiled as a static library.
- **QuiverTests** - A suite of [Catch](https://github.com/philsquared/Catch) unit tests for the Quiver library.
- **QuiverApp** - The most basic possible Quiver executable, provided as a starting point for new games. 

## Contributing

As mentioned before, attempting to make a game with Quiver would be folly. That doesn't mean you shouldn't muck about with it. Feel free to poke me with questions, feature requests or bugs using [issues](https://github.com/rachelnertia/Quiver/issues).

## Projects

- **Quarrel** - An abandoned(?) first-person shooter, available [here](https://github.com/rachelnertia/Quarrel).
