/************************************************************************
     File:        TrainView.H

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:     
						The TrainView is the window that actually shows the 
						train. Its a
						GL display canvas (Fl_Gl_Window).  It is held within 
						a TrainWindow
						that is the outer window with all the widgets. 
						The TrainView needs 
						to be aware of the window - since it might need to 
						check the widgets to see how to draw

	  Note:        we need to have pointers to this, but maybe not know 
						about it (beware circular references)

     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#pragma once

#include <sstream>
#include <iomanip>
#include "RenderUtilities/BufferObject.h"
#include "RenderUtilities/Shader.h"
#include "RenderUtilities/Texture.h"
#include "Mesh.h"
#include "Skybox.h"
#include "Framebuffer.h"

// Preclarify for preventing the compiler error
class TrainWindow;
class CTrack;


//#######################################################################
// TODO
// You might change the TrainView in order to add different objects to
// be drawn, or adjust the lighting, or ...
//#######################################################################


// The TrainView also needs to handle its user interface events.
// since this is an FLTk Window, we need to include FlTk Headers
// the pragmas turn off the warnings from FlTk - our code should have no warnings,
// but their code doesn't meet that standard
#pragma warning(push)
#pragma warning(disable:4312)
#pragma warning(disable:4311)
#include <Fl/Fl_Gl_Window.h>
#pragma warning(pop)

#include <AL/alut.h>
//#include <AL/al.h>
//#include <AL/alc.h>

// this uses the old ArcBall Code
#include "Utilities/ArcBallCam.H"

class TrainView : public Fl_Gl_Window
{
	public:
		// note that we keep the "standard widget" constructor arguments
		TrainView(int x, int y, int w, int h, const char* l = 0);

		// overrides of important window things
		virtual int handle(int);
		virtual void draw();

		// all of the actual drawing happens in this routine
		// it has to be encapsulated, since we draw differently if
		// we're drawing shadows (no colors, for example)
		void drawStuff(bool doingShadows=false);

		// setup the projection - assuming that the projection stack has been
		// cleared for you
		void setProjection();

		// Reset the Arc ball control
		void resetArcball();

		// pick a point (for when the mouse goes down)
		void doPick();

		//set ubo
		void setUBO();

		void setUBOForPosAndDir(glm::vec3 eye, glm::vec3 target, glm::vec3 up);
	public:
		ArcBallCam		arcball;			// keep an ArcBall for the UI
		int				selectedCube = -1;  // simple - just remember which cube is selected

		TrainWindow*	tw;				// The parent of this display window
		CTrack*			m_pTrack;		// The track of the entire scene

		Shader* shader		= nullptr;
		Texture2D* texture_heightmap = nullptr;
		Texture2D* texture_surface = nullptr;
		VAO* plane			= nullptr;
		UBO* commom_matrices= nullptr;

		//OpenAL
		glm::vec3 source_pos;
		glm::vec3 listener_pos;
		ALCdevice* device	= nullptr;
		ALCcontext* context = nullptr;
		ALuint source;
		ALuint buffer;

		float oldWidth = 0, oldHeight = 0;

		Mesh* waveMesh = nullptr;
		Mesh* floorMesh = nullptr;
		Skybox* skybox = nullptr;
		Shader* floorShader = nullptr;
		Shader* skyboxShader = nullptr;
		Shader* screenShader = nullptr;
		Framebuffer* framebuffer = nullptr;
		unsigned int cubemapTexture;
		Framebuffer* framebuffersForR;
		unsigned int quadVAO;
		unsigned int quadVBO;
		float floorWidth = 20.0f;
		float waterWidth = 10.0f;
		int waveMode = 0;
		float waterHeight = 5.0f;
		float floorHeight = 0.0f;
		//float amplitude = 5.0f;
		float amplitude = 0.1f;
		float waveLength = 1.0f;
		float speed = 0.05f;
		glm::vec2 direction = glm::vec2(1.0f, 0.0f);
		int onetime = 1;

		unsigned int fbo, cubeMap, rbo;

		vector<Texture2D*> waveTextures;
		float currentTime = 0.0f;
		int currentWaveNum = 0;
		int maxWaveNum = 199;

};