Binaries for ANGLE - a compatibility layer that lets us use most of OpenGL ES 3.1 features even on platforms that only have DX11 drivers:
 - Snapdragon SoCs running Windows on ARM have nothing except DX11
 - Some Intel iGPUs (e.g. Ivy Bridge, Bay Trail) are capped at OpenGL 4.2/4.0 but do support DX11_0
 - Some GPUs have slow OpenGL drivers - e.g. on Intel ARC 140V, I measured a lot better performance using ANGLE-DX11 than running directly on OpenGL 4.3

ANGLE used to have almost full support for OpenGL ES 3.1 on its DX11 backend, but it got dropped on 25.11.2025 by commit 438f42b754202e556f7fef608f1a0fa8d0516dde ("D3D: Remove ES3.1 features").
Thus, we are using an older build from before that date. These specific binaries were extracted from the build of [Electron v39.8.10](https://github.com/electron/electron/releases/v39.8.10)

Exact ES 3.1 feature support can be found here: https://android.googlesource.com/platform/external/angle/%252Bshow/refs/tags/aml_mpr_341313030/doc/ES31StatusOnD3D11.md
The unsupported features are quite obscure and we don't need any of them. Only maybe atomics not being available outside compute shaders might be a little bit problematic in the future.
But there are some mild inconveniences:
 - Reading whole structs from a SSBO causes a shader link error, instead we have to load them field by field

To use, just copy the .dlls next to the game's .exe and force it by running the game with `-gles` and `-force_gpu_api_dll` flags (flags not necessary on Snapdragons where no OpenGL but the .dlls even exists)