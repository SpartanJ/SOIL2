#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>
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

#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif

typedef GLuint (APIENTRY *SOIL_GL_CREATE_SHADER_PROC)( GLenum type );
typedef void (APIENTRY *SOIL_GL_SHADER_SOURCE_PROC)(
	GLuint shader, GLsizei count, const char *const *string, const GLint *length );
typedef void (APIENTRY *SOIL_GL_COMPILE_SHADER_PROC)( GLuint shader );
typedef void (APIENTRY *SOIL_GL_GET_SHADER_IV_PROC)(
	GLuint shader, GLenum pname, GLint *params );
typedef void (APIENTRY *SOIL_GL_GET_SHADER_INFO_LOG_PROC)(
	GLuint shader, GLsizei maxLength, GLsizei *length, char *infoLog );
typedef GLuint (APIENTRY *SOIL_GL_CREATE_PROGRAM_PROC)( void );
typedef void (APIENTRY *SOIL_GL_ATTACH_SHADER_PROC)( GLuint program, GLuint shader );
typedef void (APIENTRY *SOIL_GL_LINK_PROGRAM_PROC)( GLuint program );
typedef void (APIENTRY *SOIL_GL_GET_PROGRAM_IV_PROC)(
	GLuint program, GLenum pname, GLint *params );
typedef void (APIENTRY *SOIL_GL_GET_PROGRAM_INFO_LOG_PROC)(
	GLuint program, GLsizei maxLength, GLsizei *length, char *infoLog );
typedef void (APIENTRY *SOIL_GL_USE_PROGRAM_PROC)( GLuint program );
typedef GLint (APIENTRY *SOIL_GL_GET_UNIFORM_LOCATION_PROC)(
	GLuint program, const char *name );
typedef void (APIENTRY *SOIL_GL_UNIFORM_1I_PROC)( GLint location, GLint value );
typedef void (APIENTRY *SOIL_GL_UNIFORM_1F_PROC)( GLint location, GLfloat value );
typedef void (APIENTRY *SOIL_GL_DELETE_SHADER_PROC)( GLuint shader );
typedef void (APIENTRY *SOIL_GL_DELETE_PROGRAM_PROC)( GLuint program );

struct HDRShader
{
	GLuint program;
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLint exposure_location;
	SOIL_GL_USE_PROGRAM_PROC use_program;
	SOIL_GL_UNIFORM_1F_PROC uniform_1f;
	SOIL_GL_DELETE_SHADER_PROC delete_shader;
	SOIL_GL_DELETE_PROGRAM_PROC delete_program;
};

static void destroy_hdr_shader( HDRShader *shader );

static void *get_gl_proc( const char *name )
{
	void *proc = SDL_GL_GetProcAddress( name );
	if( proc == NULL )
	{
		std::cerr << "Missing OpenGL function: " << name << std::endl;
	}
	return proc;
}

static bool check_shader_compile(
	GLuint shader,
	SOIL_GL_GET_SHADER_IV_PROC get_shader_iv,
	SOIL_GL_GET_SHADER_INFO_LOG_PROC get_shader_info_log )
{
	GLint compiled = GL_FALSE;
	get_shader_iv( shader, GL_COMPILE_STATUS, &compiled );
	if( compiled == GL_TRUE )
	{
		return true;
	}

	GLint log_length = 0;
	get_shader_iv( shader, GL_INFO_LOG_LENGTH, &log_length );
	std::vector<char> log( log_length > 1 ? (size_t)log_length : 1u );
	get_shader_info_log( shader, (GLsizei)log.size(), NULL, log.data() );
	std::cerr << "HDR shader compilation failed: " << log.data() << std::endl;
	return false;
}

static bool create_hdr_shader( HDRShader *shader )
{
	static const char *vertex_source =
		"#version 120\n"
		"varying vec2 texture_coordinate;\n"
		"void main()\n"
		"{\n"
		"    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
		"    texture_coordinate = gl_MultiTexCoord0.xy;\n"
		"}\n";
	static const char *fragment_source =
		"#version 120\n"
		"uniform sampler2D texture_image;\n"
		"uniform float exposure;\n"
		"varying vec2 texture_coordinate;\n"
		"void main()\n"
		"{\n"
		"    vec3 hdr = texture2D(texture_image, texture_coordinate).rgb;\n"
		"    vec3 mapped = vec3(1.0) - exp(-hdr * exposure);\n"
		"    mapped = pow(mapped, vec3(1.0 / 2.2));\n"
		"    gl_FragColor = vec4(mapped, 1.0);\n"
		"}\n";

	SOIL_GL_CREATE_SHADER_PROC create_shader =
		(SOIL_GL_CREATE_SHADER_PROC)get_gl_proc( "glCreateShader" );
	SOIL_GL_SHADER_SOURCE_PROC shader_source =
		(SOIL_GL_SHADER_SOURCE_PROC)get_gl_proc( "glShaderSource" );
	SOIL_GL_COMPILE_SHADER_PROC compile_shader =
		(SOIL_GL_COMPILE_SHADER_PROC)get_gl_proc( "glCompileShader" );
	SOIL_GL_GET_SHADER_IV_PROC get_shader_iv =
		(SOIL_GL_GET_SHADER_IV_PROC)get_gl_proc( "glGetShaderiv" );
	SOIL_GL_GET_SHADER_INFO_LOG_PROC get_shader_info_log =
		(SOIL_GL_GET_SHADER_INFO_LOG_PROC)get_gl_proc( "glGetShaderInfoLog" );
	SOIL_GL_CREATE_PROGRAM_PROC create_program =
		(SOIL_GL_CREATE_PROGRAM_PROC)get_gl_proc( "glCreateProgram" );
	SOIL_GL_ATTACH_SHADER_PROC attach_shader =
		(SOIL_GL_ATTACH_SHADER_PROC)get_gl_proc( "glAttachShader" );
	SOIL_GL_LINK_PROGRAM_PROC link_program =
		(SOIL_GL_LINK_PROGRAM_PROC)get_gl_proc( "glLinkProgram" );
	SOIL_GL_GET_PROGRAM_IV_PROC get_program_iv =
		(SOIL_GL_GET_PROGRAM_IV_PROC)get_gl_proc( "glGetProgramiv" );
	SOIL_GL_GET_PROGRAM_INFO_LOG_PROC get_program_info_log =
		(SOIL_GL_GET_PROGRAM_INFO_LOG_PROC)get_gl_proc( "glGetProgramInfoLog" );
	SOIL_GL_GET_UNIFORM_LOCATION_PROC get_uniform_location =
		(SOIL_GL_GET_UNIFORM_LOCATION_PROC)get_gl_proc( "glGetUniformLocation" );
	SOIL_GL_UNIFORM_1I_PROC uniform_1i =
		(SOIL_GL_UNIFORM_1I_PROC)get_gl_proc( "glUniform1i" );

	shader->use_program =
		(SOIL_GL_USE_PROGRAM_PROC)get_gl_proc( "glUseProgram" );
	shader->uniform_1f =
		(SOIL_GL_UNIFORM_1F_PROC)get_gl_proc( "glUniform1f" );
	shader->delete_shader =
		(SOIL_GL_DELETE_SHADER_PROC)get_gl_proc( "glDeleteShader" );
	shader->delete_program =
		(SOIL_GL_DELETE_PROGRAM_PROC)get_gl_proc( "glDeleteProgram" );

	if( create_shader == NULL || shader_source == NULL || compile_shader == NULL ||
		get_shader_iv == NULL || get_shader_info_log == NULL ||
		create_program == NULL || attach_shader == NULL || link_program == NULL ||
		get_program_iv == NULL || get_program_info_log == NULL ||
		get_uniform_location == NULL || uniform_1i == NULL ||
		shader->use_program == NULL || shader->uniform_1f == NULL ||
		shader->delete_shader == NULL || shader->delete_program == NULL )
	{
		destroy_hdr_shader( shader );
		return false;
	}

	shader->vertex_shader = create_shader( GL_VERTEX_SHADER );
	shader_source( shader->vertex_shader, 1, &vertex_source, NULL );
	compile_shader( shader->vertex_shader );
	if( !check_shader_compile(
		shader->vertex_shader, get_shader_iv, get_shader_info_log ) )
	{
		destroy_hdr_shader( shader );
		return false;
	}

	shader->fragment_shader = create_shader( GL_FRAGMENT_SHADER );
	shader_source( shader->fragment_shader, 1, &fragment_source, NULL );
	compile_shader( shader->fragment_shader );
	if( !check_shader_compile(
		shader->fragment_shader, get_shader_iv, get_shader_info_log ) )
	{
		destroy_hdr_shader( shader );
		return false;
	}

	shader->program = create_program();
	attach_shader( shader->program, shader->vertex_shader );
	attach_shader( shader->program, shader->fragment_shader );
	link_program( shader->program );

	GLint linked = GL_FALSE;
	get_program_iv( shader->program, GL_LINK_STATUS, &linked );
	if( linked != GL_TRUE )
	{
		GLint log_length = 0;
		get_program_iv( shader->program, GL_INFO_LOG_LENGTH, &log_length );
		std::vector<char> log( log_length > 1 ? (size_t)log_length : 1u );
		get_program_info_log(
			shader->program, (GLsizei)log.size(), NULL, log.data() );
		std::cerr << "HDR shader linking failed: " << log.data() << std::endl;
		destroy_hdr_shader( shader );
		return false;
	}

	shader->use_program( shader->program );
	const GLint texture_location =
		get_uniform_location( shader->program, "texture_image" );
	shader->exposure_location =
		get_uniform_location( shader->program, "exposure" );
	uniform_1i( texture_location, 0 );
	shader->use_program( 0 );
	if( shader->exposure_location < 0 )
	{
		std::cerr << "HDR shader exposure uniform was not found" << std::endl;
		destroy_hdr_shader( shader );
		return false;
	}
	return true;
}

static void destroy_hdr_shader( HDRShader *shader )
{
	if( shader->use_program != NULL )
	{
		shader->use_program( 0 );
	}
	if( shader->delete_program != NULL && shader->program != 0 )
	{
		shader->delete_program( shader->program );
	}
	if( shader->delete_shader != NULL && shader->vertex_shader != 0 )
	{
		shader->delete_shader( shader->vertex_shader );
	}
	if( shader->delete_shader != NULL && shader->fragment_shader != 0 )
	{
		shader->delete_shader( shader->fragment_shader );
	}
}

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

	std::cout << "Starting demo with SOIL version: " << SOIL_version() << std::endl;

	SDL_GL_SetSwapInterval( 1 );

	SDL_GL_MakeCurrent( window, context );

	//	log what the use is asking us to load
	std::string load_me;
	bool isCubemap = argc >= 7;
	std::vector<std::string> sides;

	if ( argc >= 2 )
	{
		load_me = std::string( argv[1] );

		if ( isCubemap )
		{
			for ( int i = 1; i < 7; i++ )
			{
				sides.push_back( std::string( argv[i] ) );
			}
		}
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
	bool native_hdr = false;
	HDRShader hdr_shader = {};
	float exposure = 1.0f;

	std::cout << "Attempting to load as a cubemap" << std::endl;
	time_me = SDL_GetPerformanceCounter();

	if ( isCubemap )
	{
		tex_ID = SOIL_load_OGL_cubemap(
				sides[0].c_str(),
				sides[1].c_str(),
				sides[2].c_str(),
				sides[3].c_str(),
				sides[4].c_str(),
				sides[5].c_str(),
				SOIL_LOAD_AUTO,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_POWER_OF_TWO
				| SOIL_FLAG_MIPMAPS
				| SOIL_FLAG_DDS_LOAD_DIRECT
				| SOIL_FLAG_PVR_LOAD_DIRECT
				| SOIL_FLAG_ETC1_LOAD_DIRECT
				);
	}
	else
	{
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
	}

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
		std::cout << "Attempting to load as a native HDR texture" << std::endl;
		time_me = SDL_GetPerformanceCounter();

		tex_ID = SOIL_load_OGL_HDR_texture_f32(
				load_me.c_str(),
				SOIL_HDR_TEXTURE_RGB16F,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_GL_MIPMAPS );
		native_hdr = tex_ID != 0;

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
			if( native_hdr && !create_hdr_shader( &hdr_shader ) )
			{
				std::cerr << "Could not create the HDR tone-mapping shader" << std::endl;
				glDeleteTextures( 1, &tex_ID );
				tex_ID = 0;
				native_hdr = false;
			}

			if( tex_ID > 0 )
			{
				//	enable texturing
				glEnable( GL_TEXTURE_2D );

				//  bind an OpenGL texture ID
				glBindTexture( GL_TEXTURE_2D, tex_ID );

				//	report
				std::cout << "the loaded texture ID was " << tex_ID << std::endl;
				if( native_hdr )
				{
					std::cout << "Native HDR preview enabled. Use +/- or Up/Down to change exposure."
						<< std::endl;
				}
			}
		}
		if( tex_ID == 0 )
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
				case SDL_KEYDOWN:
				{
					if ( SDLK_ESCAPE == evt.key.keysym.sym )
					{
						running = false;
					}
					else if( native_hdr &&
						(evt.key.keysym.sym == SDLK_PLUS ||
						 evt.key.keysym.sym == SDLK_EQUALS ||
						 evt.key.keysym.sym == SDLK_UP) )
					{
						exposure *= 1.25f;
						std::cout << "HDR exposure: " << exposure << std::endl;
					}
					else if( native_hdr &&
						(evt.key.keysym.sym == SDLK_MINUS ||
						 evt.key.keysym.sym == SDLK_DOWN) )
					{
						exposure /= 1.25f;
						std::cout << "HDR exposure: " << exposure << std::endl;
					}
					break;
				}
			}
		}

		theta += dt * 40;

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		if( native_hdr )
		{
			hdr_shader.use_program( hdr_shader.program );
			hdr_shader.uniform_1f( hdr_shader.exposure_location, exposure );
		}
		
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

	if( native_hdr )
	{
		destroy_hdr_shader( &hdr_shader );
	}
	if( tex_ID != 0 )
	{
		glDeleteTextures( 1, &tex_ID );
	}

	// Close OpenGL window and terminate SDL2
	SDL_GL_DeleteContext( context );

	SDL_DestroyWindow( window );

	exit( EXIT_SUCCESS );
}

