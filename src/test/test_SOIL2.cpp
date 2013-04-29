#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <string>
#include <iostream>
#include "../SOIL2/SOIL2.h"
#include "../SOIL2/stb_image.h"

#include <GL/glfw.h>
#ifndef GL_REFLECTION_MAP
#define GL_REFLECTION_MAP 0x8512
#endif

#ifndef GL_TEXTURE_CUBE_MAP
#define GL_TEXTURE_CUBE_MAP 0x8513
#endif

int main(  int argc, char **argv  )
{
	GLboolean running;

	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		exit( EXIT_FAILURE );
	}

	// Open OpenGL window
	if( !glfwOpenWindow( 512, 512, 0, 0, 0, 0, 0, 0, GLFW_WINDOW ) )
	{
		fprintf( stderr, "Failed to open GLFW window\n" );
		glfwTerminate();
		exit( EXIT_FAILURE );
	}

	glfwSetWindowTitle( "SOIL2 Test" );

	// Enable sticky keys
	glfwEnable( GLFW_STICKY_KEYS );

	// Enable vertical sync (on cards that support it)
	glfwSwapInterval( 1 );

	//	log what the use is asking us to load
	std::string load_me;

	if ( argc >= 2 )
	{
		load_me = std::string( argv[1] );
	}
	else
	{
		load_me = "img_test.png";
	}

	std::cout << "'" << load_me << "'" << std::endl;

	int x, y, c;
	stbi_info( load_me.c_str(), &x, &y, &c );

	//	1st try to load it as a single-image-cubemap
	//	(note, need DDS ordered faces: "EWUDNS")
	GLuint tex_ID;
	double time_me;

	std::cout << "Attempting to load as a cubemap" << std::endl;
	time_me = glfwGetTime();

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

	time_me = glfwGetTime() - time_me;

	std::cout << "the load time was " << time_me << " seconds" << std::endl;

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
		time_me = glfwGetTime();
		
		tex_ID = SOIL_load_OGL_HDR_texture(
				load_me.c_str(),
				SOIL_HDR_RGBdivA2,
				0,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_POWER_OF_TWO
				| SOIL_FLAG_MIPMAPS
				);
		
		time_me = glfwGetTime() - time_me;
		
		std::cout << "the load time was " << time_me << " seconds" << std::endl;
		
		//	did I fail?
		if( tex_ID < 1 )
		{
			//	loading of the single-image-cubemap failed, try it as a simple texture
			std::cout << "Attempting to load as a simple 2D texture" << std::endl;
			
			//	load the texture, if specified
			time_me = glfwGetTime();
			
			tex_ID = SOIL_load_OGL_texture(
					load_me.c_str(),
					SOIL_LOAD_AUTO,
					SOIL_CREATE_NEW_ID,
					SOIL_FLAG_POWER_OF_TWO
					| SOIL_FLAG_MIPMAPS
					| SOIL_FLAG_DDS_LOAD_DIRECT
					| SOIL_FLAG_PVR_LOAD_DIRECT
					| SOIL_FLAG_ETC1_LOAD_DIRECT
					);
			
			time_me = glfwGetTime() - time_me;
			
			std::cout << "the load time was " << time_me << " seconds" << std::endl;
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

	while( running )
	{
		theta = glfwGetTime() * 8;

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
		glfwSwapBuffers();

		// Check if the ESC key was pressed or the window was closed
		running = !glfwGetKey( GLFW_KEY_ESC ) && glfwGetWindowParam( GLFW_OPENED );
	}

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	exit( EXIT_SUCCESS );
}

