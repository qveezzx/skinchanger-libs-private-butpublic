# Building CS2 Skin Changer

This project is a C++20 application that uses several external libraries. We have configured the project to use **NuGet** to manage these dependencies automatically, making the build process much easier.

## Prerequisites

1.  **Visual Studio 2022** (Community, Professional, or Enterprise).
2.  **Desktop development with C++** workload installed.
3.  **MSVC v143 - VS 2022 C++ x64/x86 build tools**.

## Dependencies

The project uses the following libraries, which are managed via `packages.config` and will be automatically restored by Visual Studio:
-   **nlohmann.json**: JSON parsing.
-   **openssl**: Crypto functions.
-   **curl**: HTTP requests.
-   **Microsoft.DXSDK.D3DX**: DirectX SDK.

## How to Build

1.  (Optional but recommended) Run `setup_project.bat` to automatically download NuGet and restore all packages.
2.  Open `ext-cs2-skin-changer.sln` in Visual Studio 2022.
3.  Select the **Release** configuration and **x64** platform from the toolbar.
4.  Build the solution (**Ctrl+Shift+B** or **Build** -> **Build Solution**).

The compiled executable `ext-cs2-skin-changer.exe` will be located in the `x64/Release` directory.

## Offset Updates

This project includes an automatic offset updater that fetches the latest data from `sezzyaep/CS2-OFFSETS` on GitHub. If the game updates and the cheat stops working:
-   The updater will attempt to download `client_dll.json` and `offsets.json` automatically.
-   You can also manually update the files in `ext/a2x/` using the latest dumps from the repository.
-   Make sure you are testing on **Bots** or in **Practice mode** as per Valve's recommendations for educational purposes.

## Troubleshooting

-   **NuGet Restore Failed**: Ensure you have internet access and that the NuGet Package Manager is installed and enabled in Visual Studio.
-   **Linker Errors or 'openssl/evp.h' missing**: 
    -   Go to **Project** -> **Manage NuGet Packages**.
    -   Search for `openssl-native` or `openssl-v143-static-x86_64`.
    -   Install it manually if the restore doesn't pick it up correctly.
    -   Ensure **Project Properties** -> **C/C++** -> **General** -> **Additional Include Directories** contains the path to the installed package (e.g., `$(SolutionDir)packages\openssl-native.1.0.1\build\native\include`). NuGet usually handles this automatically, but if it fails, adding it manually fixes it.

## For Analysts
This project uses a standard `.vcxproj` structure with NuGet references. No external build scripts or makefiles are required.
