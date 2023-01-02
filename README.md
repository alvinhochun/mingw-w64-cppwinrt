C++/WinRT on MinGW-w64
======================

[C++/WinRT][cppwinrt] is a C++ binding (officially "language projection") for the WinRT[^1] APIs. Originally developed by Kenny Kerr, it has become a part of the official Windows SDK and the recommended way to consume WinRT APIs from C++. See also the [official introduction to C++/WinRT][intro] and the [WinRT API reference][apiref].

C++/WinRT was made with MSVC in mind, but a series of changes to make it compatible with GCC and Clang (also libstdc++ and libc++ respectively) has been merged upstream, making it usable with various MinGW-w64 toolchains (with some limitations). This is a collection of documentation and example code intended to help you understand how to use this binding with a MinGW-w64 toolchain.

> **Note:** If you are using MSVC and Visual Studio, please use the [official NuGet package][nuget] instead.

> ⚠️ **Warning: The information here is work in progress and experimental.**

[^1]: Windows Runtime, not to be confused with the operating system that was called "Windows RT".

[cppwinrt]: https://github.com/microsoft/cppwinrt
[intro]: https://learn.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/
[apiref]: https://learn.microsoft.com/en-us/uwp/api/
[nuget]: https://www.nuget.org/packages/Microsoft.Windows.CppWinRT/


Status
------

* ✅ Calling simple WinRT APIs should work.
* ✅ Some test cases of the upstream test suite pass with MinGW-w64.
* ❌ XAML UI has not been explored but likely doesn't work out of the box.
* ❌ Authoring WinRT components is currently impossible (missing tooling to produce WinMD from IDL.)
* ⚠️ Consuming custom WinRT components may work but has not been tested.
* ⚠️ Anything involving coroutine needs more testing.
* ⚠️ Some untested functionality may break in various ways.


Usage Guide
-----------

### Toolchain

C++/WinRT requires C++20 for its coroutine support. (C++/WinRT officially supports C++17 but only with MSVC-specific coroutine extensions.) You will need a rather recent compiler. You should also try to get the latest MinGW-w64 runtime and headers.

Compilers:

* LLVM/Clang:
    * ✅ LLVM 15 is recommended.
    * 🔲 LLVM 14 is untested.
    * ⚠️ LLVM 13 does not work out of the box due to missing a floating point `std::to_chars` implementation in libc++, but can be worked around by disabling the `winrt::to_hstring(float)` and `winrt::to_hstring(double)` overloads (https://github.com/microsoft/cppwinrt/pull/1257). It also contains only experimental coroutine support (the stabilized implementation is only available since libc++ 14) so anything involving coroutines should be considered unstable and to be avoided.
    * ⚠️ Earlier versions are untested.
* GCC:
    * ✅ GCC 12 in MSYS2 has been tested to work.
    * 🔲 GCC 11 is untested.
    * ⚠️ GCC 10 is missing a floating point `std::to_chars` implementation in libc++ (same as LLVM 13).
    * ❌ GCC 9 and earlier do not support coroutine.

Tested toolchains:

* [llvm-mingw](https://github.com/mstorsjo/llvm-mingw)
* [MSYS2](https://www.msys2.org/)

### Using cppwinrt with Prebuilt Headers

Headers are built on [GitHub Actions][GHA] workflow runs and made available as build artifacts.

[GHA]: https://github.com/alvinhochun/mingw-w64-cppwinrt/actions

### Build System Integration

This project provides both CMake config file and pkg-config file in the releases for using the headers with build systems such as CMake and Autotools. To see how these integrations work, you can check the [examples](examples/). Better documentation may be added in the future.

(I am still figuring this out, so details are subject to change.)

<!--
### Package Managers

* A cppwinrt package is available in MSYS2: https://packages.msys2.org/base/mingw-w64-cppwinrt
-->

### Building Upstream cppwinrt Directly

If you want to use the upstream cppwinrt tool directly and without build system integration, please refer to [Using Upstream cppwinrt](docs/using-upstream-cppwinrt.md).


<!--
Examples
--------

Examples are provided in the [`examples/`](examples/) directory.
-->


Licence
-------

All code and resources in this repository are released under the MIT License,
unless otherwise specified.
