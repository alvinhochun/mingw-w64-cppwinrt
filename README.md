C++/WinRT on MinGW-w64
======================

[C++/WinRT][cppwinrt] is a C++ binding (officially "language projection") for the WinRT[^1] APIs. Originally developed by Kenny Kerr, it has become a part of the official Windows SDK and the recommended way to consume WinRT APIs from C++.

C++/WinRT was made with MSVC in mind, but a series of changes to make it compatible with GCC and Clang (also libstdc++ and libc++ respectively) has been merged upstream, making it usable with various MinGW-w64 toolchains (with some limitations). This is a collection of documentation and example code intended to help you understand how to use this binding with a MinGW-w64 toolchain.

> ⚠️ **Warning: The information here is work in progress and experimental.**

[^1]: Windows Runtime, not to be confused with the operating system that was called "Windows RT".

[cppwinrt]: https://github.com/microsoft/cppwinrt


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

### Getting the Headers

We are still figuring out how the headers and the host tool should be distributed by toolchains. If you want to try this out now, you may build cppwinrt and generate the headers yourself.

1. Get the latest cppwinrt source code from https://github.com/microsoft/cppwinrt:
    ```console
    > git clone https://github.com/microsoft/cppwinrt.git
    ```
2. Compile cppwinrt:
    > Note: When building cppwinrt for production use, you should always specify a version string with the `CPPWINRT_BUILD_VERSION` CMake variable. Normally this should follow the version number of the official release tag. In case you are building from a commit not yet released, I suggest following the format `2.0.YYMMDD.0` using today's date.
    ```console
    > cmake -S cppwinrt -B cppwinrt-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCPPWINRT_BUILD_VERSION="2.0.221213.0"
    > cmake --build cppwinrt-build
    ```
    The tool should now be available at `cppwinrt-build/cppwinrt` or `cppwinrt-build/cppwinrt.exe`.
3. Get the WinMD (Windows Metadata) files:
    * If you are on Windows, the system already comes with the WinMD files for the version of Windows you are using. You only need to get the files manually if you want to use APIs introduced in newer versions of Windows.
    * They also come with the Windows SDK.
    * A copy of `Windows.winmd` can be download from the [`windows-rs`][windows-rs] repository (under `crates/libs/metadata/default/Windows.winmd`).
4. Generate the headers:
    * To use the WinMD files included in Windows:
        ```console
        > cppwinrt  -in local -out cppwinrt-headers
        ```
    * To use a specific WinMD file:
        ```console
        > cppwinrt -in path/to/Windows.winmd -out cppwinrt-headers
        ```
    The headers should now be available at `cppwinrt-header/`.

[windows-rs]: https://github.com/microsoft/windows-rs

### Using in Build

At this time we are still figuring out the best way to support using C++/WinRT with different build systems. To use C++/WinRT now you will need to set the build up manually.

C++/WinRT is a header-only library, which means all you need to do is to add the path of `cppwinrt-header/` to the include paths. How this is done depends on the build system. For example with CMake, you can add `include_directories("path/to/cppwinrt-header")` before creating the targets.

The C++/WinRT headers are named using the WinRT API namespaces with exact capitalization (PascalCase). To support cross-compiling from Linux, any C++/WinRT includes shall use the exact matching case.

Some functions may require 16-byte compare-and-exchange on x86_64. If you get `undefined reference to '__sync_bool_compare_and_swap_16'` when linking, then you need to add `-mcx16` to the compile flags.


<!--
Examples
--------

Examples are provided in the [`examples/`](examples/) directory.
-->


Licence
-------

All code and resources in this repository are released under the MIT License,
unless otherwise specified.
