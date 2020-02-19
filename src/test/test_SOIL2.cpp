#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <string>
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

#ifndef GL_REFLECTION_MAP
#define GL_REFLECTION_MAP 0x8512
#endif

#ifndef GL_TEXTURE_CUBE_MAP
#define GL_TEXTURE_CUBE_MAP 0x8513
#endif

double get_total_ms( Uint64 time_me )
{
	return ( (double)(SDL_GetPerformanceCounter() - time_me) / (double)SDL_GetPerformanceFrequency() ) * 1000;
}

int main( int argc, char* argv[] )
{
	GLboolean running;

	// Initialise SDL2
	if( 0 != SDL_Init( SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS ) )
	{
		fprintf( stderr, "Failed to initialize SDL2: %s\n", SDL_GetError() );
		exit( EXIT_FAILURE );
	}

	// Open OpenGL window
	SDL_Window * window = SDL_CreateWindow( "SOIL2 Test",SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 512, 512, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );

	if( NULL == window )
	{
		fprintf( stderr, "Failed to open SDL2 window: %s\n", SDL_GetError() );
		exit( EXIT_FAILURE );
	}

	SDL_GLContext context = SDL_GL_CreateContext( window );

	if ( NULL == context )
	{
		fprintf( stderr, "Failed to create SDL2 OpenGL Context: %s\n", SDL_GetError() );

		SDL_DestroyWindow( window );

		exit( EXIT_FAILURE );
	}

	SDL_GL_SetSwapInterval( 1 );

	SDL_GL_MakeCurrent( window, context );

	//	log what the use is asking us to load
	std::string load_me;

	if ( argc >= 2 )
	{
		load_me = std::string( argv[1] );
	}
	else
	{
		load_me = ResourcePath( "img_test.png" );
	}

	std::cout << "'" << load_me << "'" << std::endl;

	//	1st try to load it as a single-image-cubemap
	//	(note, need DDS ordered faces: "EWUDNS")
	GLuint tex_ID;
	Uint64 time_me;

	std::cout << "Attempting to load as a cubemap" << std::endl;
	time_me = SDL_GetPerformanceCounter();

	tex_ID = SOIL_load_OGL_single_cubemap(
			load_me.c_str(),
			SOIL_DDS_CUBEMAP_FACE_ORDER,
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_POWER_OF_TWO
			| SOIL_FLAG_MIPMAPS
			| SOIL_FLAG_DDS_LOAD_DIRECT
			| SOIL_FLAG_PVR_LOAD_DIRECT
			| SOIL_FLAG_ETC1_LOAD_DIRECT
			);

	std::cout << "the load time was " << get_total_ms(time_me) << " milliseconds" << std::endl;

	if( tex_ID > 0 )
	{
		glEnable( GL_TEXTURE_CUBE_MAP );
		glEnable( GL_TEXTURE_GEN_S );
		glEnable( GL_TEXTURE_GEN_T );
		glEnable( GL_TEXTURE_GEN_R );
		glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
		glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
		glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
		glBindTexture( GL_TEXTURE_CUBE_MAP, tex_ID );
		
		std::cout << "the loaded single cube map ID was " << tex_ID << std::endl;
	}
	else
	{
		std::cout << "Attempting to load as a HDR texture" << std::endl;
		time_me = SDL_GetPerformanceCounter();
		
		tex_ID = SOIL_load_OGL_HDR_texture(
				load_me.c_str(),
				SOIL_HDR_RGBdivA2,
				0,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_POWER_OF_TWO
				| SOIL_FLAG_MIPMAPS
				| SOIL_FLAG_GL_MIPMAPS
				);

		std::cout << "the load time was " << get_total_ms(time_me) << " milliseconds" << std::endl;
		
		//	did I fail?
		if( tex_ID < 1 )
		{
			//	loading of the single-image-cubemap failed, try it as a simple texture
			std::cout << "Attempting to load as a simple 2D texture" << std::endl;
			
			//	load the texture, if specified
			time_me = SDL_GetPerformanceCounter();
			
			tex_ID = SOIL_load_OGL_texture(
					load_me.c_str(),
					SOIL_LOAD_AUTO,
					SOIL_CREATE_NEW_ID,
					  SOIL_FLAG_POWER_OF_TWO
					| SOIL_FLAG_MIPMAPS
					| SOIL_FLAG_GL_MIPMAPS
					| SOIL_FLAG_DDS_LOAD_DIRECT
					| SOIL_FLAG_PVR_LOAD_DIRECT
					| SOIL_FLAG_ETC1_LOAD_DIRECT
					| SOIL_FLAG_COMPRESS_TO_DXT
					);

			std::cout << "the load time was " << get_total_ms(time_me) << " milliseconds" << std::endl;
		}

		if( tex_ID > 0 )
		{
			//	enable texturing
			glEnable( GL_TEXTURE_2D );
			
			//  bind an OpenGL texture ID
			glBindTexture( GL_TEXTURE_2D, tex_ID );
			
			//	report
			std::cout << "the loaded texture ID was " << tex_ID << std::endl;
		}
		else
		{
			//	loading of the texture failed...why?
			glDisable( GL_TEXTURE_2D );
			
			std::cout << "Texture loading failed: '" << SOIL_last_result() << "'" << std::endl;
		}
	}

	running = GL_TRUE;

	const float ref_mag = 0.1f;
	float theta = 0.0f;
	float tex_u_max = 1.0f;
	float tex_v_max = 1.0f;
	Uint64 counterOld = SDL_GetPerformanceCounter();

	while( running )
	{
		float dt = (float)((double)(SDL_GetPerformanceCounter() - counterOld) / (double)SDL_GetPerformanceFrequency());
		counterOld = SDL_GetPerformanceCounter();

		SDL_Event evt;

		while (SDL_PollEvent(&evt))
		{
			switch (evt.type)
			{
				case SDL_QUIT:
				{
					running = false;
					break;
				}
				case SDL_KEYUP:
				{
					if ( SDLK_ESCAPE == evt.key.keysym.sym )
					{
						running = false;
					}
					break;
				}
			}
		}

		theta += dt * 40;

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		// Draw our textured geometry (just a rectangle in this instance)
		glPushMatrix();
		glScalef( 0.8f, 0.8f, 0.8f );
		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		glNormal3f( 0.0f, 0.0f, 1.0f );
		glBegin(GL_QUADS);
			glNormal3f( -ref_mag, -ref_mag, 1.0f );
			glTexCoord2f( 0.0f, tex_v_max );
			glVertex3f( -1.0f, -1.0f, -0.1f );

			glNormal3f( ref_mag, -ref_mag, 1.0f );
			glTexCoord2f( tex_u_max, tex_v_max );
			glVertex3f( 1.0f, -1.0f, -0.1f );

			glNormal3f( ref_mag, ref_mag, 1.0f );
			glTexCoord2f( tex_u_max, 0.0f );
			glVertex3f( 1.0f, 1.0f, -0.1f );

			glNormal3f( -ref_mag, ref_mag, 1.0f );
			glTexCoord2f( 0.0f, 0.0f );
			glVertex3f( -1.0f, 1.0f, -0.1f );
		glEnd();
		glPopMatrix();

		glPushMatrix();
		glScalef( 0.8f, 0.8f, 0.8f );
		glRotatef(theta, 0.0f, 0.0f, 1.0f);
		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		glNormal3f( 0.0f, 0.0f, 1.0f );
		glBegin(GL_QUADS);
			glTexCoord2f( 0.0f, tex_v_max );		glVertex3f( 0.0f, 0.0f, 0.1f );
			glTexCoord2f( tex_u_max, tex_v_max );	glVertex3f( 1.0f, 0.0f, 0.1f );
			glTexCoord2f( tex_u_max, 0.0f );		glVertex3f( 1.0f, 1.0f, 0.1f );
			glTexCoord2f( 0.0f, 0.0f );				glVertex3f( 0.0f, 1.0f, 0.1f );
		glEnd();
		glPopMatrix();

		// Swap buffers
		SDL_GL_SwapWindow( window );
	}

	// Close OpenGL window and terminate SDL2
	SDL_GL_DeleteContext( context );

	SDL_DestroyWindow( window );

	exit( EXIT_SUCCESS );
}

