# RAID-SuperBLT ![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/RAIDModding/RAID-SuperBLT/create_build.yml)

An open-source Lua hook for RAID: World War II, designed and created for ease of use for both players and modders.

This is an unofficial continuation of the BLT and SuperBLT mod loader for PAYDAY 2, with additional features aimed at allowing things
not possible in standard Lua, such as patching XML files that are loaded directly by the engine or playing
3D sounds.

This is the developer repository and should only be used if you know what you're doing (or running GNU+Linux, as explained below). If you don't, visit the website at [SuperBLT.znix.xyz](https://superblt.znix.xyz/) for an up-to-date drag-drop install.
The Lua component of the BLT, which controls mod loading, can be found in its own repository, [GitHub: RAIDModding/RAID-SuperBLT-Lua]([https://gitlab.com/znixian/payday2-superblt-lua](https://github.com/RAIDModding/RAID-SuperBLT-Lua)).

## Download
Visit ![GitHub Latest Release](https://github.com/RAIDModding/RAID-SuperBLT/releases/latest) to get the latest stable download. 

## Documentation
Documentation for the original BLT can be found on the [GitHub Wiki](https://github.com/JamesWilko/Payday-2-BLT/wiki) for the project.

Documentation for SuperBLT can be found on the [SuperBLT Website](https://superblt.znix.xyz).

## Development

How to contribute to SuperBLT:

SuperBLT uses the CMake build system. This can generate various build files,
including GNU Makefiles and MSVC Project/Solution files, from which the program can
be built.

SuperBLT depends on two sets of libraries: those commonly available (such as OpenSSL
and ZLib) which you are expected to install via your package manager (GNU+Linux) or
by downloading and compiling (Windows).

It also has libraries that are very small projects, rarely available in package managers,
and are included as git submodules. CMake automatically adds these targets,
so you don't need to make any special efforts to use them.

### Windows

First, clone this repository and pull all required projects and repositories into one folder (Note: You **NEED** to do this, otherwise you'll get runtime and compile errors):

```
git clone --recursive https://github.com/RAIDModding/RAID-SuperBLT.git
```

You can use Visual Studio or the command line to generate the files and build SuperBLT.

For Visual Studio, select `File -> Open -> CMake` and select the top-level
`CMakeLists.txt` file.

In Visual Studio, select the configuration box (at the top of the window, which may, for
example, say `x64-Debug`) and select `x64-Debug` if it isn't already.
Select `Project->Generate Cache` and wait for it to run cmake - this may take some time.

You can now open the generated solution file in `/out/build/x64-Debug/SuperBLT.sln`

If you do not see the `out` folder, click "Show All Files" in the top bar of the Solution Explorer.

If you don't see the solution file, please ensure the configurations have `Visual Studio 16/17 Win64` selected as the cmake generator.

At this point, you can compile your project. In Visual Studio, press F7.
This will take some time as it compiles all of SuperBLT's dependencies and, finally, SuperBLT itself.

Finally, you can make RAID use your custom-built version of SBLT instead of having to copy the built
file to the RAID directory each time you change something.
Go to your `RAID World War II` directory and open PowerShell to do this. Run:

```
cmd /c mklink WSOCK32.dll <path to SBLT>\out\build\x64-Debug\WSOCK32.dll
```

For the command line, navigate to your SuperBLT folder using `cd`

Make a new directory named `build` using `mkdir build` and enter it using `cd`

Then enter the following commands:
`cmake .. -A x64 -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug`

`msbuild SuperBLT.sln /t:Build /p:Configuration=Debug`

You can symlink using the following in your `RAID World War II` directory. Run:

```
cmd /c mklink WSOCK32.dll <path to SBLT>\build\Debug\WSOCK32.dll
```

### Code Conventions
- Avoid `std::shared_ptr` and the likes unless you have a decent reason to use it. If you
need the reference counting, go ahead, but please don't use it when a regular pointer works fine.
- Don't **ever** use CRLF.
- Please ensure there is a linefeed (`\n`) as the last byte of any files you create.
- Please use `git patch.` Don't commit multiple unrelated or loosely related things in a
single commit. Likewise, please don't commit whitespace-only changes. `git blame` is a valuable
tool.
- Please run the source code using `clang-format` to ensure stuff like brace positions and whitespace
are consistent. Later, this will be put into a Continuous Integration task to mark offending
commits and test stuff like compiling in GCC.
- Please ensure your code doesn't cause any compiler warnings (not counting libraries). This is
enforced for GCC; please watch your output if you're using Visual Studio.
