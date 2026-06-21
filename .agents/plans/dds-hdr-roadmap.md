# SOIL2 Texture Loading Roadmap

This document tracks remaining work only.

## Open issue scope

Issue #20 remains partially open because volume DDS textures are not yet
supported. Its high-precision 2D texture and cubemap requirements are complete.

## Prioritized roadmap

### 1. DDS 2D texture arrays

SOIL2 already has `GL_TEXTURE_2D_ARRAY`, `glTexImage3D`, and
`glTexSubImage3D` infrastructure.

Remaining work:

- Parse DX10 `arraySize`.
- Load `glCompressedTexSubImage3D`.
- Allocate storage for every mip level.
- Traverse DDS data in array-layer and mip-level order.
- Support compressed and uncompressed layers.
- Add a dedicated public DDS texture-array API.
- Add procedural array fixtures and upload/readback tests.

### 2. Dependency-free KTX2 direct-upload loader

Add a native KTX2 parser after the direct ASTC and ETC2/EAC mappings are
stable. The first implementation should only upload payloads already encoded
in a GPU-native format.

Initial scope:

- File and memory APIs.
- 2D textures and cubemaps.
- Complete and partial mip chains.
- Uncompressed formats already understood by SOIL2.
- BC1 through BC7.
- ETC1, ETC2, and EAC.
- ASTC 2D LDR formats.
- Correct linear and sRGB Vulkan-format mappings.
- KTX2 level index parsing, alignment, bounds checking, and endianness-safe
  field reads.
- Key/value metadata parsing only where required for correct orientation or
  upload behavior.

Explicitly defer:

- Basis Universal ETC1S and UASTC transcoding.
- Zstandard and zlib supercompression.
- BasisLZ supercompression.
- Texture arrays, cubemap arrays, and volume textures until the corresponding
  SOIL2 upload infrastructure is complete.
- KTX1 compatibility.

Files that require transcoding or decompression must fail with a precise error
instead of silently falling back to an incorrect upload.

Testing:

- Add project-owned KTX2 fixtures for each supported compression family.
- Test file and memory loading, mip chains, cubemaps, linear/sRGB mappings,
  malformed level indexes, invalid offsets, truncation, and unsupported
  supercompression schemes.
- Cross-check fixtures and parser behavior against the Khronos KTX2
  specification and `libktx`.

### 3. Volume DDS textures

Complete the remaining portion of issue #20:

- Parse `DDSD_DEPTH` and `DDSCAPS2_VOLUME`.
- Support DX10 `DDS_DIMENSION_TEXTURE3D`.
- Upload with `glTexImage3D` or `glCompressedTexImage3D`.
- Reduce width, height, and depth correctly for each mip level.
- Support compressed and uncompressed volume payloads.
- Add volume fixtures and automated validation.

### 4. Cubemap arrays

- Add `GL_TEXTURE_CUBE_MAP_ARRAY` support.
- Interpret the effective layer count as `arraySize * 6`.
- Validate face and array payload ordering.
- Define a dedicated public API.
- Add compressed and uncompressed tests.

### 5. DDS parser and upload robustness

- Define an explicit policy for typeless DXGI formats.
- Validate all six legacy cubemap face flags.
- Use overflow-safe payload and mip-size calculations.
- Extend row-pitch handling to unusual legacy DDS mip layouts where the
  top-level pitch is insufficient to infer lower-level padding.
- Check each OpenGL allocation and upload for errors.
- Delete only texture IDs created by the current call on failure.
- Never delete a caller-owned reused texture ID during validation failure.
- Improve errors for unsupported dimensions, arrays, and formats.
- Consider endian handling for 16-bit and 32-bit source data.

### 6. Automated DDS test coverage

- Generate project-owned procedural fixtures for every supported format.
- Test legacy and DX10 headers.
- Test complete and partial mip chains.
- Test 2D textures, cubemaps, arrays, cubemap arrays, and volume textures.
- Test compressed and uncompressed upload paths.
- Test malformed, truncated, and overflow-inducing headers.
- Validate internal formats and representative values with GPU readback.
- Replace graphical-only checks with automated pass/fail tests where possible.

## Recommended implementation order

1. DDS 2D texture arrays.
2. Dependency-free KTX2 direct upload.
3. Volume textures for the remainder of issue #20.
4. Cubemap arrays.
5. Parser hardening and expanded automated tests throughout each step.
