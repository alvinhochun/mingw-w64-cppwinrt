<!--
SPDX-FileCopyrightText: 2022-2023 Alvin Wong
SPDX-License-Identifier: MIT
-->

Using Upstream cppwinrt
=======================

If you want to use upstream cppwinrt directly, you will need to compile the tool and generate the headers manually.

Get the source code
-------------------

Get the latest cppwinrt source code from https://github.com/microsoft/cppwinrt:
```console
> git clone https://github.com/microsoft/cppwinrt.git
```
Compile cppwinrt
----------------

> Note: When building cppwinrt for production use, you should always specify a version string with the `CPPWINRT_BUILD_VERSION` CMake variable. Normally this should follow the version number of the official release tag. In case you are building from a commit not yet released, I suggest following the format `2.0.YYMMDD.0` using today's date.
```console
> cmake -S cppwinrt -B cppwinrt-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCPPWINRT_BUILD_VERSION="2.0.221213.0"
> cmake --build cppwinrt-build
```
The tool should now be available at `cppwinrt-build/cppwinrt` or `cppwinrt-build/cppwinrt.exe`.

Get the WinMD (Windows Metadata) files
--------------------------------------

* If you are on Windows, the system already comes with the WinMD files for the version of Windows you are using. You only need to get the files manually if you want to use APIs introduced in newer versions of Windows.
* They also come with the Windows SDK.
* A copy of `Windows.winmd` can be download from the [`windows-rs`][windows-rs] repository (under `crates/libs/metadata/default/Windows.winmd`).

[windows-rs]: https://github.com/microsoft/windows-rs

Generate the headers
--------------------

* To use the WinMD files included in Windows:
    ```console
    > cppwinrt  -in local -out cppwinrt-headers
    ```
* To use a specific WinMD file:
    ```console
    > cppwinrt -in path/to/Windows.winmd -out cppwinrt-headers
    ```
The headers should now be available at `cppwinrt-header/`.

Using in build
--------------

C++/WinRT is a header-only library, which means all you need to do is to have `cppwinrt-header/` in the include paths, such that `#include <winrt/Windows.Foundation.h>` can find the correct file. How this is done depends on the build system. For example with CMake, you can add `include_directories("path/to/cppwinrt-header")` before creating the targets.

The C++/WinRT headers are named using the WinRT API namespaces with exact capitalization (PascalCase). To support cross-compiling from Linux, any C++/WinRT includes shall use the exact matching case.

> **Hint:** This is different from the tradition of using normal Windows API headers from mingw-w64, in which include header names shall be in lowercase.

Some functions may require 16-byte compare-and-exchange on x86_64. If you get `undefined reference to '__sync_bool_compare_and_swap_16'` when linking, then you need to add `-mcx16` to the compile flags.
