Simple OpenGL Image Library 2 ![SOIL2](https://web.ensoft.dev/soil2/soil2-logo.svg)
=============================

[![build status](https://img.shields.io/github/workflow/status/SpartanJ/SOIL2/build)](https://github.com/SpartanJ/SOIL2/actions?query=workflow%3Abuild)

**Introduction:**
--------------

**SOIL2** is a fork of the Jonathan Dummer's [Simple OpenGL Image Library](http://www.lonesock.net/soil.html).

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
    * DDS - DXT1/2/3/4/5, uncompressed, cubemaps (can't read 3D DDS files yet)
    * PSD - (from stb_image documentation)
    * HDR - converted to LDR, unless loaded with *HDR* functions (RGBE or RGBdivA or RGBdivA2)
    * GIF
    * PIC
    * PKM ( ETC1 )
    * PVR ( PVRTC )

* Writeable Image Formats:
    * TGA - Greyscale or RGB or RGBA, uncompressed
    * BMP - RGB, uncompressed
    * DDS - RGB as DXT1, or RGBA as DXT5
    * PNG
    * JPG


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
    * Can load rectangluar textures for GUI elements or splash screens (requires GL_ARB/EXT/NV_texture_rectangle)
Can decompress images from RAM (e.g. via [PhysicsFS](http://icculus.org/physfs/) or similar) into an OpenGL texture (same features as regular 2D textures, above)


* Can load cube maps directly into an OpenGL texture (same features as regular 2D textures, above)
* Can take six image files directly into an OpenGL cube map texture
Can take a single image file where width = 6*height (or vice versa), split it into an OpenGL cube map texture
* No external dependencies
* Tiny
* Cross platform (Windows, Linux, Mac OS X, FreeBSD, Solaris, Haiku, iOS, Android, and probably any platform with OpenGL support)


**Difference between SOIL2 and SOIL:**
--------------------------------------
* Up to date stb_image version.

* Added support for Android and iOS.

* Optimized memory allocation.

* Added support for GIF and PIC formats ( thanks to additions to stb_image ).

* Save images to PNG ( thanks to additions to stb_image ) and JPG.

* `SOIL_create_OGL_texture` expects width and height parameters as pointers, since the real size of the texture loaded could change. This occurs when GL_ARB_texture_non_power_of_two extension is not present and the user tries to load a non-power of two texture.

* Added support for PVRTC and ETC1 ( PKM format ) direct loading and decoding as fallback method if the GPU doesn't support the texture compression method. With the following new functions exposed:
    * `SOIL_direct_load_PVR`
    * `SOIL_direct_load_PVR_from_memory`
    * `SOIL_direct_load_ETC1`
    * `SOIL_direct_load_ETC1_from_memory`

* Added support for glGenerateMipmap if the GPU support it ( and any of its variations, glGenerateMipmapEXT and glGenerateMipmapOES for GLES1 ). Added the flag SOIL_FLAG_GL_MIPMAPS to request GL mipmaps instead of the internal mipmap creation provided by SOIL2.

* Added `SOIL_GL_ExtensionSupported`. To query if a extension is supported by the GPU.

* Added `SOIL_GL_GetProcAddress`. To get the address of a GL function.

* sRGB color space support.

* Added support for OpenGL Core Profile.

* And some minor fixes.

**Compiling:**
------------
To generate project files you will need to [download and install](http://industriousone.com/premake/download) [Premake](http://industriousone.com/what-premake)

Then you can generate the static library for your platform just going to the project directory where the premake4.lua file is located and then execute:

`premake4 gmake` to generate project Makefiles, then `cd make/*YOURPLATFORM*/`, and finally `make` or `make config=release` ( it will generate the static lib, the shared lib and the test application ).

or

`premake4 vs2010` to generate Visual Studio 2010 project.

or

`premake4 xcode4` to generate Xcode 4 project.

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
