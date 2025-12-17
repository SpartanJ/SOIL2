#include <cstdlib>
#include <iostream>

#include "../common/common.hpp"
#include "../SOIL2/SOIL2.h"

#define NO_SDL_GLEXT
#if ( ( defined( _MSCVER ) || defined( _MSC_VER ) ) || defined( __APPLE_CC__ ) || defined ( __APPLE__ ) ) && !defined( SOIL2_NO_FRAMEWORKS )
    #include <SDL.h>
    #include <SDL_opengl.h>
#else
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_opengl.h>
#endif

#ifndef GL_TEXTURE_2D_ARRAY
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#endif

struct TestCase
{
    const char* name;
    int loadFlags;
    int soilFlags;
};

int main()
{
    if ( SDL_Init( SDL_INIT_VIDEO ) != 0 )
    {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return EXIT_FAILURE;
    }

    SDL_Window* window = SDL_CreateWindow(
        "hidden",
        0, 0,
        1, 1,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN
    );

    if ( !window )
    {
        std::cerr << "Window creation failed\n";
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_GLContext context = SDL_GL_CreateContext( window );
    if ( !context )
    {
        std::cerr << "GL context creation failed\n";
        SDL_DestroyWindow( window );
        SDL_Quit();
        return EXIT_FAILURE;
    }

    std::string texPath = ResourcePath("./png_sprite_sheet.png");
    const char* texImagePath = texPath.c_str();

    printf("Using texture atlas: %s\n", texImagePath);

    const int cols = 4;
    const int rows = 4;

    TestCase tests[] =
    {
        { "AUTO + INVERT_Y", SOIL_LOAD_AUTO, SOIL_FLAG_INVERT_Y},
        { "AUTO + SRGB",     SOIL_LOAD_AUTO, SOIL_FLAG_SRGB_COLOR_SPACE },
        { "RGBA + INVERT_Y", SOIL_LOAD_RGBA, SOIL_FLAG_INVERT_Y },
        { "RGB + DXT",       SOIL_LOAD_RGB,  SOIL_FLAG_COMPRESS_TO_DXT },
        { "AUTO + NO FLAGS", SOIL_LOAD_AUTO, 0 }
    };

    for ( const auto& t : tests )
    {
        GLuint textureRef = SOIL_load_OGL_texture_array_from_atlas_grid(
            texImagePath,
            cols, rows,
            t.loadFlags,
            SOIL_CREATE_NEW_ID,
            t.soilFlags
        );

        if ( textureRef == 0 )
        {
            std::cerr << "[FAIL] " << t.name << ": "
                      << SOIL_last_result() << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "[OK] " << t.name
                  << " (id=" << textureRef << ")" << std::endl;

        glDeleteTextures( 1, &textureRef );
    }

    SDL_GL_DeleteContext( context );
    SDL_DestroyWindow( window );
    SDL_Quit();

    return EXIT_SUCCESS;
}
