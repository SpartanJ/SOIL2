Simple OpenGL Image Library 2 ![SOIL2](https://web.ensoft.dev/soil2/soil2-logo.svg)
=============================

[![build status](https://img.shields.io/github/actions/workflow/status/SpartanJ/SOIL2/main.yml?branch=master)](https://github.com/SpartanJ/SOIL2/actions?query=workflow%3Abuild)

**Introduction:**
--------------

**SOIL2** is a fork of the Jonathan Dummer's [Simple OpenGL Image Library](https://web.archive.org/web/20200728145723/http://lonesock.net/soil.html).

**SOIL2** is a tiny C library used primarily for uploading textures into OpenGL.
It is based on [stb_image](http://www.nothings.org/stb_image.c), the public domain code from Sean Barrett.

**SOIL2** extended stb_image to DDS files, and to perform common functions needed in loading OpenGL textures.

**SOIL2** can also be used to save and load images in a variety of formats (useful for loading height maps, non-OpenGL applications, etc.)

**License:**
--------------

MIT-0 (see LICENSE file)

**Features:**
-------------

* Readable Image Formats:
    * BMP - non-1bpp, non-RLE (from stb_image documentation)
    * PNG - non-interlaced (from stb_image documentation)
    * JPG - JPEG baseline (from stb_image documentation)
    * TGA - greyscale or RGB or RGBA or indexed, uncompressed or RLE
    * DDS - common uncompressed DXGI formats, BC1/BC2/BC3/BC3n/BC5u/BC6H_UF16,
      and high-precision formats,
      including cubemaps (see `DDS support` below)
    * PSD - (from stb_image documentation)
    * [QOI](https://github.com/phoboslab/qoi)
    * HDR - converted to LDR by generic loaders; dedicated HDR loaders support either 8-bit
      RGBE/RGBdivA encodings or native RGB16F/RGB32F OpenGL textures
    * GIF
    * PIC
    * PKM 1.0/2.0 (ETC1, ETC2, and EAC decoding and direct upload)
    * ASTC (direct 2D LDR upload)
    * PVR ( PVRTC )

* Writeable Image Formats:
    * TGA - Greyscale or RGB or RGBA, uncompressed
    * BMP - RGB, uncompressed
    * DDS - RGB as BC1, or RGBA as BC3 (see `DDS support` below)
    * PNG
    * JPG
    * [QOI](https://github.com/phoboslab/qoi)


* Can load an image file directly into a 2D OpenGL texture, optionally performing the following functions:
    * Can generate a new texture handle, or reuse one specified
    * Can automatically rescale the image to the next largest power-of-two size
    * Can automatically create MIPmaps
    * Can scale (not simply clamp) the RGB values into the "safe range" for NTSC displays (16 to 235)
    * Can multiply alpha on load (for more correct blending / compositing)
    * Can flip the image vertically
    * Can compress and upload any image as DXT1 or DXT5 (if EXT_texture_compression_s3tc is available), using an     * internal (very fast!) compressor
    * Can convert the RGB to YCoCg color space (useful with DXT5 compression: see [this link](http://www.nvidia.com/object/real-time-ycocg-dxt-compression.html) from NVIDIA)
    * Will automatically downsize a texture if it is larger than GL_MAX_TEXTURE_SIZE
    * Can directly upload DDS files (DXT1/3/5/uncompressed/cubemap, with or without MIPmaps). Note: directly uploading the compressed DDS image will disable the other options (no flipping, no pre-multiplying alpha, no rescaling, no creation of MIPmaps, no auto-downsizing)
    * Can load rectangular textures for GUI elements or splash screens (requires GL_ARB/EXT/NV_texture_rectangle)
Can decompress images from RAM (e.g. via [PhysicsFS](http://icculus.org/physfs/) or similar) into an OpenGL texture (same features as regular 2D textures, above)


* Can load cube maps directly into an OpenGL texture (same features as regular 2D textures, above)
* Can take six image files directly into an OpenGL cube map texture
Can take a single image file where width = 6*height (or vice versa), split it into an OpenGL cube map texture
* No external dependencies
* Tiny
* Cross platform (Windows, Linux, Mac OS X, FreeBSD, Solaris, Haiku, iOS, Android, and probably any platform with OpenGL support)


**DDS support**
-------------

* BC1 UNORM and sRGB - Compress, decompress, direct GPU upload (a.k.a. DXT1)
* BC2 UNORM and sRGB - decompress, direct GPU upload (a.k.a. DXT2, DXT3)
* BC3 UNORM and sRGB - Compress, decompress, direct GPU upload (a.k.a. DXT4, DXT5)
* BC3n - direct GPU upload
* BC4 UNORM and SNORM - direct GPU upload (a.k.a. ATI1, BC4U, BC4S, RGTC1)
* BC5 UNORM and SNORM - direct GPU upload (a.k.a. 3Dc, ATI2, BC5S, RGTC2)
* BC6H UF16 and SF16 - direct GPU upload (requires OpenGL BPTC texture compression support). BC6H stores
  linear HDR values, so displaying it directly in a normalized framebuffer requires exposure and
  tone mapping in the application's shader.
* BC7 UNORM and sRGB - direct GPU upload (requires OpenGL BPTC texture compression support)
* RGBA8 and BGRA8 UNORM and sRGB - direct GPU upload
* R8 and RG8 UNORM and SNORM - direct GPU upload
* R16 and RG16 UNORM - direct GPU upload
* R16, RG16, R32, and RG32 FLOAT - direct GPU upload
* R10G10B10A2 UNORM and R11G11B10 FLOAT - direct GPU upload
* RGBA16 UNORM - direct GPU upload (DX10 and legacy D3DFMT_A16B16G16R16 DDS)
* RGBA16 FLOAT - direct GPU upload (DX10 and legacy D3DFMT_A16B16G16R16F DDS)
* RGBA32 FLOAT - direct GPU upload (DX10 and legacy D3DFMT_A32B32G32R32F DDS)

Direct DDS upload supports 2D textures and cubemaps. DDS texture arrays and volume textures are not
currently loaded directly. Uncompressed DX10 uploads respect the top-level DDS row pitch and repack
padded rows before passing them to OpenGL.

The `bin/test_*.dds` fixtures use procedural gradients and checkerboards created by SOIL2's
`soil2_generate_dds_fixtures` test utility; they do not contain third-party image content. Pass the
output directory as its first argument to regenerate them. Creating all compressed fixtures requires
an OpenGL driver with S3TC, RGTC, and BPTC support. Run
`bin/soil2-dds-test-release bin` to validate their direct upload and GPU readback.

**Native HDR texture support**
------------------------------

`SOIL_load_OGL_HDR_texture_f32()` and
`SOIL_load_OGL_HDR_texture_f32_from_memory()` decode Radiance HDR images to linear
32-bit float RGB and upload them with `GL_FLOAT`. The caller selects
`SOIL_HDR_TEXTURE_RGB16F` or `SOIL_HDR_TEXTURE_RGB32F` for GPU storage.

Six-file and six-buffer cubemaps are supported by
`SOIL_load_OGL_HDR_cubemap_f32()` and
`SOIL_load_OGL_HDR_cubemap_f32_from_memory()`. Cubemap faces must be square and
have identical dimensions.

The native HDR loaders support power-of-two resizing, float-preserving mipmaps,
vertical flipping, and repeat/clamp texture parameters. Byte-oriented transforms
such as DXT compression, NTSC clamping, alpha multiplication, YCoCg conversion,
and sRGB storage are rejected. Applications must apply exposure and tone mapping
when rendering these linear HDR textures.

**Mobile compressed texture support**
-------------------------------------

`SOIL_direct_load_PKM()` and `SOIL_direct_load_PKM_from_memory()` directly
upload PKM 1.0 ETC1 and PKM 2.0 ETC2/EAC textures when the active OpenGL
context supports their compression formats. Supported PKM 2.0 formats are
ETC2 RGB8, RGB8A1, and RGBA8, plus signed and unsigned EAC R11 and RG11.
The existing `SOIL_direct_load_ETC1()` APIs remain compatibility wrappers.

Generic SOIL image loading also decodes these PKM formats on the CPU. EAC R11
is returned as one grayscale channel. EAC RG11 is returned as RGB `(R, G, 0)`
because stb_image defines two-channel images as luminance-alpha. Signed EAC
values are mapped from `[-1, 1]` to unsigned bytes in `[0, 255]`.

`SOIL_direct_load_ASTC()` and `SOIL_direct_load_ASTC_from_memory()` directly
upload standalone 2D LDR ASTC files for every standard block footprint from
4x4 through 12x12. Use `SOIL_FLAG_SRGB_COLOR_SPACE` to request sRGB storage;
the standalone ASTC header does not contain color-space metadata.

The ASTC direct loader does not decode or transcode. PKM and standalone ASTC
files contain one 2D image without mipmaps, cubemap faces, texture-array
layers, or orientation metadata. Unsupported direct GPU formats fail with a
descriptive error; PKM files can then use the CPU decoding path.
The project-owned fixtures in `bin/mobile` are generated by
`soil2-generate-mobile-compressed-fixtures`.

`bin/test_native_hdr.hdr` is a project-owned procedural fixture generated by
`soil2_generate_dds_fixtures`.

The base graphical test can display the fixture with tone mapping:

```sh
bin/soil2-test-release bin/test_native_hdr.hdr
```

Use `+`/`-` or the Up/Down arrow keys to adjust exposure.

**Difference between SOIL2 and SOIL:**
--------------------------------------
* Up to date stb_image version.

* Added support for Android and iOS.

* Optimized memory allocation.

* Added support for GIF and PIC formats ( thanks to additions to stb_image ).

* Save images to PNG ( thanks to additions to stb_image ) and JPG.

* `SOIL_create_OGL_texture` expects width and height parameters as pointers, since the real size of the texture loaded could change. This occurs when GL_ARB_texture_non_power_of_two extension is not present and the user tries to load a non-power of two texture.

* Added direct loading for PVRTC, PKM 1.0/2.0 ETC1/ETC2/EAC, and standalone
  2D LDR ASTC textures. The exposed direct-loading functions include:
    * `SOIL_direct_load_PVR`
    * `SOIL_direct_load_PVR_from_memory`
    * `SOIL_direct_load_PKM`
    * `SOIL_direct_load_PKM_from_memory`
    * `SOIL_direct_load_ETC1`
    * `SOIL_direct_load_ETC1_from_memory`
    * `SOIL_direct_load_ASTC`
    * `SOIL_direct_load_ASTC_from_memory`

* Added support for glGenerateMipmap if the GPU support it ( and any of its variations, glGenerateMipmapEXT and glGenerateMipmapOES for GLES1 ). Added the flag SOIL_FLAG_GL_MIPMAPS to request GL mipmaps instead of the internal mipmap creation provided by SOIL2.

* Added `SOIL_GL_ExtensionSupported`. To query if a extension is supported by the GPU.

* Added `SOIL_GL_GetProcAddress`. To get the address of a GL function.

* sRGB color space support.

* Added support for OpenGL Core Profile.

* And some minor fixes.

**Compiling:**
------------
To generate project files you will need to [download and install](https://premake.github.io/download) [Premake](https://premake.github.io/docs/What-Is-Premake)

Then you can generate the static library for your platform just going to the project directory where the premake4.lua file is located and then execute:

`premake5 gmake2` to generate project Makefiles, then `cd make/*YOURPLATFORM*/`, and finally `make` or `make config=release_x86_64` or `make config=release_arm64` depending on your architecture ( it will generate the static lib, the shared lib and the test application ).

or

`premake5 vs2022` to generate Visual Studio 2022 project.

or

`premake5 xcode4` to generate Xcode 4 project.

The static library will be located in `lib/*YOURPLATFORM*/` folder project subdirectory.
The test will be located in `bin`, you need [SDL2](http://libsdl.org/) installed to be able to build the test.

**Usage:**
----------

**SOIL2** is meant to be used as a static library (as it's tiny and in the public domain).

Simply include SOIL2.h in your C or C++ file, compile the .c files or link to the static library, and then use any of SOIL2's functions. The file SOIL2.h contains simple doxygen style documentation. (If you use the static library, no other header files are needed besides SOIL2.h)

**Below are some simple usage examples:**

```c
/* load an image file directly as a new OpenGL texture */
GLuint tex_2d = SOIL_load_OGL_texture
	(
		"img.png",
		SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
	);

/* check for an error during the load process */
if( 0 == tex_2d )
{
	printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
}

/* preserve a Radiance HDR image as a native half-float GPU texture */
GLuint hdr_tex = SOIL_load_OGL_HDR_texture_f32
	(
		"environment.hdr",
		SOIL_HDR_TEXTURE_RGB16F,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_GL_MIPMAPS | SOIL_FLAG_INVERT_Y
	);

/* load another image, but into the same texture ID, overwriting the last one */
tex_2d = SOIL_load_OGL_texture
	(
		"some_other_img.dds",
		SOIL_LOAD_AUTO,
		tex_2d,
		SOIL_FLAG_DDS_LOAD_DIRECT
	);

/* load 6 images into a new OpenGL cube map, forcing RGB */
GLuint tex_cube = SOIL_load_OGL_cubemap
	(
		"xp.jpg",
		"xn.jpg",
		"yp.jpg",
		"yn.jpg",
		"zp.jpg",
		"zn.jpg",
		SOIL_LOAD_RGB,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS
	);

/* load and split a single image into a new OpenGL cube map, default format */
/* face order = East South West North Up Down => "ESWNUD", case sensitive! */
GLuint single_tex_cube = SOIL_load_OGL_single_cubemap
	(
		"split_cubemap.png",
		"EWUDNS",
		SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS
	);

/* actually, load a DDS cubemap over the last OpenGL cube map, default format */
/* try to load it directly, but give the order of the faces in case that fails */
/* the DDS cubemap face order is pre-defined as SOIL_DDS_CUBEMAP_FACE_ORDER */
single_tex_cube = SOIL_load_OGL_single_cubemap
	(
		"overwrite_cubemap.dds",
		SOIL_DDS_CUBEMAP_FACE_ORDER,
		SOIL_LOAD_AUTO,
		single_tex_cube,
		SOIL_FLAG_MIPMAPS | SOIL_FLAG_DDS_LOAD_DIRECT
	);

/* load an image as a heightmap, forcing greyscale (so channels should be 1) */
int width, height, channels;
unsigned char *ht_map = SOIL_load_image
	(
		"terrain.tga",
		&width, &height, &channels,
		SOIL_LOAD_L
	);

/* save that image as another type */
int save_result = SOIL_save_image
	(
		"new_terrain.dds",
		SOIL_SAVE_TYPE_DDS,
		width, height, channels,
		ht_map
	);

/* save a screenshot of your awesome OpenGL game engine, running at 1024x768 */
save_result = SOIL_save_screenshot
	(
		"awesomenessity.bmp",
		SOIL_SAVE_TYPE_BMP,
		0, 0, 1024, 768
	);

/* loaded a file via PhysicsFS, need to decompress the image from RAM, */
/* where it's in a buffer: unsigned char *image_in_RAM */
GLuint tex_2d_from_RAM = SOIL_load_OGL_texture_from_memory
	(
		image_in_RAM,
		image_in_RAM_bytes,
		SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_COMPRESS_TO_DXT
	);

/* done with the heightmap, free up the RAM */
SOIL_free_image_data( ht_map );
```

**Clarifications**
----------------

The icon used for the project is part of the [Haiku®'s Icons](http://www.haiku-inc.org/haiku-icons.html), [MIT licensed](http://www.opensource.org/licenses/mit-license.html).
