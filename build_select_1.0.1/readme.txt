Build Select by Alex Free V1.0.1 12/31/2020

Build Select makes compiling for various Mac OS X versions, SDKs, and archs easy. Instead of patching Makefiles, Xcode projects, and fighting with autotools, it just works. As a bonus, build select also allows you to optionally provide additional build flags to be used in every compile.

License: 3-BSD (read license.txt).

Why Use Build Select?
- Anything compiled on Leopard using Xcode only builds for your Mac's arch. If you have a new G4, anything you build doesn't work on old G4s or G3s by default. You have to patch/edit or pass flags to specify -arch ppc. Tiger and everything before compiles for -arch ppc by default.
- Apple removed gcc_select in Xcode 3.1.4, the latest available for PowerPC. That at least symlinked gcc versions to be the default automatically.
- I wanted to be able to set a default Mac OS X SDK to compile for other versions of Mac OS X automatically.
- I wanted to add other custom flags to whatever I build without patching anything.

With Build Select You Can:
- Change compiler versions without gcc_select in the latest Xcode for PowerPC.
- Change the SDK.
- Change the MACOSX_DEPLOYMENT_TARGET.
- Compile for all PowerPC Macs on Leopard by default (604, G3, G4, G5) like all previous Mac OS X versions.
- Add custom flags without patching anything. Useful when compiling into a .app prefix.

Requirements:
- Build Select works on Tiger and Leopard with Xcode installed. 
- If you want GCC 3.3 and the 10.3.9 SDK you have to select if installing Xcode version 3.1.4 as it is an optional install. Note that on Tiger, GCC 4.2 is unavailable in Xcode and can not be used in Build Select.

Install It:
Drag the 'install' script into a Terminal window and execute it with sudo.

Uninstall It:
Drag the 'uninstall' script into a Terminal window and execute it with sudo.

Use It:

Note: If you need a newer SDK to build software that can not be built using the target OS SDK, you can try selecting the SDK/relevent compiler but setting an older MACOSX_DEPLOYMENT_TARGET in the forth argument to build select. This is not gaurenteed to work, but sometimes will.

Example 1: Compile with the GCC 4.2 compiler using the MAC OS X 10.4u SDK for the generic ppc arch at the Mac OS X 10.3 API value. Include and link the directory /usr/local before any others when looking for libs. Targets Mac OS X 10.3.9 ppc.

$ sudo bselect 4.2 10.4u ppc 10.3 "-I/usr/local -L/usr/local/"

Example 2: Compile with the GCC 4.2 compiler using the Mac OS X 10.5 SDK for the generic ppc arch at the Mac OS X 10.5 API value.

$ sudo bselect 4.2 10.5 ppc 10.5

Example 3: Compile with the GCC 3.3 compiler using the Mac OS X 10.3.9 SDK for the generic ppc arch at the Mac OS X 10.3 API value.

$ sudo bselect 3.3 10.3.9 ppc 10.3

Example 3: Compile with the GCC 4.0 compiler using the MAC OS X 10.4u SDK for the generic ppc arch at the Mac OS X 10.3 API value.

$ sudo bselect 4.0 10.4u ppc 10.3

Example 4: Compile with the GCC 4.2 compiler using the MAC OS X 10.3.9 SDK for the generic ppc arch at the Mac OS X 10.3 API value.

$ sudo bselect 4.2 10.4u ppc 10.3

Understand It:
bselect --help	Display the help text.
bselect --reset Reset to the default xcode compiler setup.
bselect <compiler_version> <sdk_version> <arch> <additional_flags>

The first argument is the compiler version. Valid options are 3.3, 4.0, and 4.2.

The second argument is the SDK version. Valid options are 10.3.9, 10.4u, and 10.5.

The third argument is the arch to compile for. Valid options are ppc, ppc750, ppc7400, ppc7450, and ppc970. If you compile for a newer arch then what your Mac is itself, you will not be able to execute what you compile on that Mac.

The fourth argument is the MACOSX_DEPLOYMENT_TARGET value. This may be the same as your SDK or lower. Valid options are 10.3, 10.4, and 10.5.

The fifth argument of additional flags is the only argument that is optional. If you specify additional flags that contain spaces, you must double quote them.

Changelog:
V1.0.1 - 12/31/2020
Build Select now accepts up to five arguments. The forth argument is the MACOSX_DEPLOYMENT_TARGET value, and the fifth is now the custom flags optional argument.

v1.0 - 12/23/2020