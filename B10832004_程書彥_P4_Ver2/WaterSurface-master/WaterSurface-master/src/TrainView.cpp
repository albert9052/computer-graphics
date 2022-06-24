/************************************************************************
     File:        TrainView.cpp

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

#include <iostream>
#include <Fl/fl.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
//#include "GL/gl.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <GL/glu.h>

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"



#ifdef EXAMPLE_SOLUTION
#	include "TrainExample/TrainExample.H"
#endif


//************************************************************************
//
// * Constructor to set up the GL window
//========================================================================
TrainView::
TrainView(int x, int y, int w, int h, const char* l) 
	: Fl_Gl_Window(x,y,w,h,l)
//========================================================================
{
	mode( FL_RGB|FL_ALPHA|FL_DOUBLE | FL_STENCIL );

	resetArcball();
}

//************************************************************************
//
// * Reset the camera to look at the world
//========================================================================
void TrainView::
resetArcball()
//========================================================================
{
	// Set up the camera to look at the world
	// these parameters might seem magical, and they kindof are
	// a little trial and error goes a long way
	arcball.setup(this, 40, 250, .2f, .4f, 0);
}

//************************************************************************
//
// * FlTk Event handler for the window
//########################################################################
// TODO: 
//       if you want to make the train respond to other events 
//       (like key presses), you might want to hack this.
//########################################################################
//========================================================================
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, 
	// then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->worldCam->value())
		if (arcball.handle(event)) 
			return 1;

	// remember what button was used
	static int last_push;

	switch(event) {
		// Mouse button being pushed event
		case FL_PUSH:
			last_push = Fl::event_button();
			// if the left button be pushed is left mouse button
			if (last_push == FL_LEFT_MOUSE  ) {
				doPick();
				damage(1);
				return 1;
			};
			break;

	   // Mouse button release event
		case FL_RELEASE: // button release
			damage(1);
			last_push = 0;
			return 1;

		// Mouse button drag event
		case FL_DRAG:

			// Compute the new control point position
			if ((last_push == FL_LEFT_MOUSE) && (selectedCube >= 0)) {
				ControlPoint* cp = &m_pTrack->points[selectedCube];

				double r1x, r1y, r1z, r2x, r2y, r2z;
				getMouseLine(r1x, r1y, r1z, r2x, r2y, r2z);

				double rx, ry, rz;
				mousePoleGo(r1x, r1y, r1z, r2x, r2y, r2z, 
								static_cast<double>(cp->pos.x), 
								static_cast<double>(cp->pos.y),
								static_cast<double>(cp->pos.z),
								rx, ry, rz,
								(Fl::event_state() & FL_CTRL) != 0);

				cp->pos.x = (float) rx;
				cp->pos.y = (float) ry;
				cp->pos.z = (float) rz;
				damage(1);
			}
			break;

		// in order to get keyboard events, we need to accept focus
		case FL_FOCUS:
			return 1;

		// every time the mouse enters this window, aggressively take focus
		case FL_ENTER:	
			focus(this);
			break;

		case FL_KEYBOARD:
		 		int k = Fl::event_key();
				int ks = Fl::event_state();
				if (k == 'p') {
					// Print out the selected control point information
					if (selectedCube >= 0) 
						printf("Selected(%d) (%g %g %g) (%g %g %g)\n",
								 selectedCube,
								 m_pTrack->points[selectedCube].pos.x,
								 m_pTrack->points[selectedCube].pos.y,
								 m_pTrack->points[selectedCube].pos.z,
								 m_pTrack->points[selectedCube].orient.x,
								 m_pTrack->points[selectedCube].orient.y,
								 m_pTrack->points[selectedCube].orient.z);
					else
						printf("Nothing Selected\n");

					return 1;
				};
				break;
	}

	return Fl_Gl_Window::handle(event);
}

//************************************************************************
//
// * this is the code that actually draws the window
//   it puts a lot of the work into other routines to simplify things
//========================================================================

unsigned int loadCubemap(vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
				);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void TrainView::draw()
{

	//*********************************************************************
	//
	// * Set up basic opengl informaiton
	//
	//**********************************************************************
	//initialized glad
	int dividedNumber = 200;

	if (gladLoadGL())
	{
		//initiailize VAO, VBO, Shader...
		if (!framebuffer) {

			framebuffer = new Framebuffer(w(), h());
			framebuffersForR = new Framebuffer(w(), h());
			oldWidth = w();
			oldHeight = h();
		}
		else if (oldWidth != w() || oldHeight != h()) {

			framebuffer->updateSize(w(), h());
			framebuffersForR->updateSize(w(), h());
			oldWidth = w();
			oldHeight = h();
		}

		if (!this->shader)
			this->shader = new
			Shader(
				"../WaterSurface-master/WaterSurface-master/src/shaders/simple.vert",
				nullptr, nullptr, nullptr,
				"../WaterSurface-master/WaterSurface-master/src/shaders/simple.frag");

		if (!this->commom_matrices)
			this->commom_matrices = new UBO();
		this->commom_matrices->size = 2 * sizeof(glm::mat4) + sizeof(glm::vec3);
		glGenBuffers(1, &this->commom_matrices->ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
		glBufferData(GL_UNIFORM_BUFFER, this->commom_matrices->size, NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		
		if (floorMesh == nullptr) {

			vector<glm::vec3> vertices;
			vertices.push_back(glm::vec3(-1.0f, 0.0f, -1.0f));
			vertices.push_back(glm::vec3(-1.0f, 0.0f, 1.0f));
			vertices.push_back(glm::vec3(1.0f, 0.0f, 1.0f));
			vertices.push_back(glm::vec3(1.0f, 0.0f, -1.0f));
			vector<glm::vec2> texCoords;
			texCoords.push_back(glm::vec2(0.0f, 0.0f));
			texCoords.push_back(glm::vec2(0.0f, 1.0f));
			texCoords.push_back(glm::vec2(1.0f, 1.0f));
			texCoords.push_back(glm::vec2(1.0f, 0.0f));
			float halfWidth = floorWidth / 2;
			vector<Vertex> floorVertices;
			floorVertices.reserve(4);
			Vertex tempVertex;
			for (int i = 0; i < 4; i++) {

				tempVertex.Position = glm::vec3(halfWidth * vertices[i].x, 0.0f, halfWidth * vertices[i].z);
				tempVertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
				tempVertex.TexCoords = glm::vec2(floorWidth * texCoords[i].x, floorWidth * texCoords[i].y);
				floorVertices.push_back(tempVertex);
			}
			vector<unsigned int> floorIndicies = { 0, 1, 2, 0, 2, 3 };
			vector<Texture2D> floorTextures;
			floorTextures.push_back(*(new Texture2D("../WaterSurface-master/WaterSurface-master/Images/floor/blueBricks.png")));

			floorMesh = new Mesh(floorVertices, floorIndicies, floorTextures);
			floorShader = new
					Shader(
						"../WaterSurface-master/WaterSurface-master/src/shaders/simple.vert",
						nullptr, nullptr, nullptr,
						"../WaterSurface-master/WaterSurface-master/src/shaders/simple.frag");
		}

		if (waveMesh == nullptr) {


			unsigned int fbo;
			unsigned int cubeMap;
			unsigned int rbo;
			glGenFramebuffers(1, &fbo);
			glGenTextures(1, &cubeMap);
			glGenRenderbuffers(1, &rbo);


			GLfloat quadVertices[12] = {

				-1.0f, 1.0f, 0.0f,
				-1.0f, -1.0f, 0.0f,
				1.0f, 1.0f, 0.0f,
				1.0f, -1.0f, 0.0f,
			};

			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

			vector<Vertex> waveVertices;
			vector<unsigned int> waveIndices;
			vector<Texture2D> waveTextures;
			waveVertices.reserve(pow(dividedNumber + 1, 2));
			for (int i = 0; i < dividedNumber + 1; i++) {

				for (int j = 0; j < dividedNumber + 1; j++) {

					int currentPos = i * (dividedNumber + 1) + j;
					glm::vec3 position = glm::vec3(-waterWidth / 2 + waterWidth * i / dividedNumber, 5.0f, -waterWidth / 2 + waterWidth * j / dividedNumber);
					glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
					glm::vec2 texCoords = glm::vec2(0.0f + 1.0f / dividedNumber * i, 0.0f + 1.0f / dividedNumber * j);
					Vertex tempVertex;
					tempVertex.Position = position;
					tempVertex.Normal = normal;
					tempVertex.TexCoords = texCoords;
					waveVertices.push_back(tempVertex);
				}
			}

			waveIndices.reserve(pow(dividedNumber, 2) * 2 * 3);
			for (int i = 0; i < dividedNumber; i++) {

				for (int j = 0; j < dividedNumber; j++) {

					// First triangle
					waveIndices.push_back(i * (dividedNumber + 1) + j);
					waveIndices.push_back(i * (dividedNumber + 1) + j + 1);
					waveIndices.push_back((i + 1) * (dividedNumber + 1) + j);

					// Second triangle
					waveIndices.push_back(i * (dividedNumber + 1) + j + 1);
					waveIndices.push_back((i + 1) * (dividedNumber + 1) + j);
					waveIndices.push_back((i + 1) * (dividedNumber + 1) + j + 1);
				}
			}
			string numberStr = std::to_string(0);
			while (numberStr.size() < 3) {

				numberStr = "0" + numberStr;
			}
			this->texture_heightmap = new Texture2D(("../WaterSurface-master/WaterSurface-master/Images/waves5/" + numberStr + ".png").c_str());
			this->texture_surface = new Texture2D(("../WaterSurface-master/WaterSurface-master/Images/waves5/" + numberStr + ".png").c_str());

			// Initialize waveTextures

			for (int i = 0; i <= maxWaveNum; i++) {

				string numberStr = std::to_string(i);
				while (numberStr.size() < 3) {

					numberStr = "0" + numberStr;
				}
				this->waveTextures.push_back(new Texture2D(("../WaterSurface-master/WaterSurface-master/Images/waves5/" + numberStr + ".png").c_str()));
			}

			waveTextures.push_back(*(this->texture_heightmap));
			waveTextures.push_back(*(this->texture_surface));

			waveMesh = new Mesh(waveVertices, waveIndices, waveTextures);
		}
		else {

			if (waveMode == 1) {

				this->texture_heightmap = waveTextures[currentWaveNum];
				this->texture_surface = waveTextures[currentWaveNum];

				waveMesh->textures[0] = *this->texture_heightmap;
				waveMesh->textures[1] = *this->texture_surface;
			}
		}

		//waveVertices.clear();
		//waveVertices.resize(4);
		//waveVertices[0].Position = glm::vec3(-5.0f, 10.0f, -5.0f);
		//waveVertices[1].Position = glm::vec3(-5.0f, 10.0f, 5.0f);
		//waveVertices[2].Position = glm::vec3(5.0f, 10.0f, 5.0f);
		//waveVertices[3].Position = glm::vec3(5.0f, 10.0f, -5.0f);

		//for (int i = 0; i < 4; i++) {

		//	waveVertices[i].Normal = glm::vec3(0.0f, 1.0f, 0.0f);
		//}

		//waveVertices[0].TexCoords = glm::vec2(0.0f, 0.0f);
		//waveVertices[1].TexCoords = glm::vec2(1.0f, 0.0f);
		//waveVertices[2].TexCoords = glm::vec2(1.0f, 1.0f);
		//waveVertices[3].TexCoords = glm::vec2(0.0f, 1.0f);

		//waveIndices.clear();
		//waveIndices.resize(6);
		//waveIndices[0] = 0;
		//waveIndices[1] = 1;
		//waveIndices[2] = 2;
		//waveIndices[3] = 0;
		//waveIndices[4] = 2;
		//waveIndices[5] = 3;

		//if (!this->plane) {
		//GLfloat  vertices[] = {
		//	-0.5f ,0.0f , -0.5f,
		//	-0.5f ,0.0f , 0.5f ,
		//	0.5f ,0.0f ,0.5f ,
		//	0.5f ,0.0f ,-0.5f };
		//GLfloat  normal[] = {
		//	0.0f, 1.0f, 0.0f,
		//	0.0f, 1.0f, 0.0f,
		//	0.0f, 1.0f, 0.0f,
		//	0.0f, 1.0f, 0.0f };
		//GLfloat  texture_coordinate[] = {
		//	0.0f, 0.0f,
		//	1.0f, 0.0f,
		//	1.0f, 1.0f,
		//	0.0f, 1.0f };
		//GLuint element[] = {
		//	0, 1, 2,
		//	0, 2, 3, };

		//this->plane = new VAO;
		//this->plane->element_amount = sizeof(element) / sizeof(GLuint);
		//glGenVertexArrays(1, &this->plane->vao);
		//glGenBuffers(3, this->plane->vbo);
		//glGenBuffers(1, &this->plane->ebo);

		//glBindVertexArray(this->plane->vao);

		// Position attribute
		//glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
		//glEnableVertexAttribArray(0);

		// Normal attribute
		//glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal, GL_STATIC_DRAW);
		//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
		//glEnableVertexAttribArray(1);

		// Texture Coordinate attribute
		//glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinate), texture_coordinate, GL_STATIC_DRAW);
		//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
		//glEnableVertexAttribArray(2);

		//Element attribute
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
		//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element, GL_STATIC_DRAW);

		// Unbind VAO
		//glBindVertexArray(0);
		//}

		if (!this->device){
			//Tutorial: https://ffainelli.github.io/openal-example/
			this->device = alcOpenDevice(NULL);
			if (!this->device)
				puts("ERROR::NO_AUDIO_DEVICE");

			ALboolean enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
			if (enumeration == AL_FALSE)
				puts("Enumeration not supported");
			else
				puts("Enumeration supported");

			this->context = alcCreateContext(this->device, NULL);
			if (!alcMakeContextCurrent(context))
				puts("Failed to make context current");

			this->source_pos = glm::vec3(0.0f, 5.0f, 0.0f);

			ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
			alListener3f(AL_POSITION, source_pos.x, source_pos.y, source_pos.z);
			alListener3f(AL_VELOCITY, 0, 0, 0);
			alListenerfv(AL_ORIENTATION, listenerOri);

			alGenSources((ALuint)1, &this->source);
			alSourcef(this->source, AL_PITCH, 1);
			alSourcef(this->source, AL_GAIN, 1.0f);
			alSource3f(this->source, AL_POSITION, source_pos.x, source_pos.y, source_pos.z);
			alSource3f(this->source, AL_VELOCITY, 0, 0, 0);
			alSourcei(this->source, AL_LOOPING, AL_TRUE);

			alGenBuffers((ALuint)1, &this->buffer);

			ALsizei size, freq;
			ALenum format;
			ALvoid* data;
			ALboolean loop = AL_TRUE;

			//Material from: ThinMatrix
			alutLoadWAVFile((ALbyte*)"../WaterSurface-master/WaterSurface-master/Audios/bounce.wav", &format, &data, &size, &freq, &loop);
			alBufferData(this->buffer, format, data, size, freq);
			alSourcei(this->source, AL_BUFFER, this->buffer);

			if (format == AL_FORMAT_STEREO16 || format == AL_FORMAT_STEREO8)
				puts("TYPE::STEREO");
			else if (format == AL_FORMAT_MONO16 || format == AL_FORMAT_MONO8)
				puts("TYPE::MONO");

			alSourcePlay(this->source);

			// cleanup context
			//alDeleteSources(1, &source);
			//alDeleteBuffers(1, &buffer);
			//device = alcGetContextsDevice(context);
			//alcMakeContextCurrent(NULL);
			//alcDestroyContext(context);
			//alcCloseDevice(device);
		}
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");

	// Set up the view port
	glViewport(0,0,w(),h());

	//// clear the window, be sure to clear the Z-Buffer too
	//glClearColor(0,0,0,0);		// background should be blue

	//// we need to clear out the stencil buffer since we'll use
	//// it for shadows
	//glClearStencil(0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	//glEnable(GL_DEPTH);

	// Blayne prefers GL_DIFFUSE
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// prepare for projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	setProjection();		// put the code to set up matrices here

	//######################################################################
	// TODO: 
	// you might want to set the lighting up differently. if you do, 
	// we need to set up the lights AFTER setting up the projection
	//######################################################################
	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// top view only needs one light
	if (tw->topCam->value()) {
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
	} else {
		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);
	}

	//*********************************************************************
	//
	// * set the light parameters
	//
	//**********************************************************************
	GLfloat lightPosition1[]	= {0,1,1,0}; // {50, 200.0, 50, 1.0};
	GLfloat lightPosition2[]	= {1, 0, 0, 0};
	GLfloat lightPosition3[]	= {0, -1, 0, 0};
	GLfloat yellowLight[]		= {0.5f, 0.5f, .1f, 1.0};
	GLfloat whiteLight[]			= {1.0f, 1.0f, 1.0f, 1.0};
	GLfloat blueLight[]			= {.1f,.1f,.3f,1.0};
	GLfloat grayLight[]			= {.3f, .3f, .3f, 1.0};

	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
	glLightfv(GL_LIGHT0, GL_AMBIENT, grayLight);

	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);

	glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);

	// set linstener position 
	if(selectedCube >= 0)
		alListener3f(AL_POSITION, 
			m_pTrack->points[selectedCube].pos.x,
			m_pTrack->points[selectedCube].pos.y,
			m_pTrack->points[selectedCube].pos.z);
	else
		alListener3f(AL_POSITION, 
			this->source_pos.x, 
			this->source_pos.y,
			this->source_pos.z);




	//*********************************************************************
	// now draw the object and we need to do it twice
	// once for real, and then once for shadows
	//*********************************************************************
	glEnable(GL_LIGHTING);
	//setupObjects();

	//drawStuff();

	//// this time drawing is for shadows (except for top view)
	//if (!tw->topCam->value()) {
	//	setupShadows();
	//	drawStuff(true);
	//	unsetupShadows();
	//}

	//setUBO();
	//glBindBufferRange(GL_UNIFORM_BUFFER, /*binding point*/0, this->commom_matrices->ubo, 0, this->commom_matrices->size);
	

	if (!skybox) {

		vector<string> faces = 
		{
			"../WaterSurface-master/WaterSurface-master/Images/skybox/right.jpg",
			"../WaterSurface-master/WaterSurface-master/Images/skybox/left.jpg",
			"../WaterSurface-master/WaterSurface-master/Images/skybox/top.jpg",
			"../WaterSurface-master/WaterSurface-master/Images/skybox/bottom.jpg",
			"../WaterSurface-master/WaterSurface-master/Images/skybox/back.jpg", 
			"../WaterSurface-master/WaterSurface-master/Images/skybox/front.jpg",
		};
		cubemapTexture = loadCubemap(faces);
		skybox = new Skybox(cubemapTexture);
	}
	else {

		skybox->cubemapTexture = cubemapTexture;
	}

	if (!skyboxShader) {

		skyboxShader = new
			Shader(
				"../WaterSurface-master/WaterSurface-master/src/shaders/skybox.vert",
				nullptr, nullptr, nullptr,
				"../WaterSurface-master/WaterSurface-master/src/shaders/skybox.frag");
	}
	
	if (!screenShader) {

		screenShader = new
			Shader(
				"../WaterSurface-master/WaterSurface-master/src/shaders/update.vert",
				nullptr, nullptr, nullptr,
				"../WaterSurface-master/WaterSurface-master/src/shaders/update.frag");
	}

	//bind shader
	this->shader->Use();

	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, this->source_pos);
	model_matrix = glm::scale(model_matrix, glm::vec3(10.0f, 10.0f, 10.0f));
	glUniformMatrix4fv(glGetUniformLocation(this->shader->Program, "u_model"), 1, GL_FALSE, &model_matrix[0][0]);
	glUniform3fv(glGetUniformLocation(this->shader->Program, "u_color"), 1, &glm::vec3(0.0f, 1.0f, 0.0f)[0]);
	glUniform1f(glGetUniformLocation(this->shader->Program, "waterHeight"), waterHeight);
	glUniform1f(glGetUniformLocation(this->shader->Program, "time"), currentTime);
	glUniform1f(glGetUniformLocation(this->shader->Program, "amplitude"), amplitude);
	glUniform1f(glGetUniformLocation(this->shader->Program, "waveLength"), waveLength);
	glUniform1f(glGetUniformLocation(this->shader->Program, "speed"), speed);
	glUniform2fv(glGetUniformLocation(this->shader->Program, "direction"), 1, &direction[0]);
	glUniform1i(glGetUniformLocation(this->shader->Program, "mode"), waveMode);
	glUniform2fv(glGetUniformLocation(this->shader->Program, "vecSize"), 1, &glm::vec2(waterWidth / dividedNumber, 0.0f)[0]);
	glUniform1i(glGetUniformLocation(this->shader->Program, "skybox"), 6);

	this->skyboxShader->Use();
	glUniformMatrix4fv(glGetUniformLocation(this->skyboxShader->Program, "u_model"), 1, GL_FALSE, &model_matrix[0][0]);
	glUniform1i(glGetUniformLocation(this->skyboxShader->Program, "skybox"), 3);

	this->shader->Use();

	glm::vec2 dir(0.000001f, 1.0f);
	vector<glm::vec3> dirs;
	dirs.reserve(6);
	dirs.push_back(glm::vec3(dir.y, dir.x, dir.x));
	dirs.push_back(glm::vec3(-dir.y, dir.x, dir.x));
	dirs.push_back(glm::vec3(dir.x, dir.y, dir.x));
	dirs.push_back(glm::vec3(dir.x, -dir.y, dir.x));
	dirs.push_back(glm::vec3(dir.x, dir.x, dir.y));
	dirs.push_back(glm::vec3(dir.x, dir.x, -dir.y));

	//for (int i = 0; i < 6; i++) {

	//	glDrawBuffer(i, framebuffers);
	//	framebuffersForR->clear();
	//	framebuffersForR->use();

	//	// Not sure if this is necessary
	//	//glEnable(GL_LIGHTING);

	//	glm::vec3 eye = glm::vec3(0.0f, waterHeight + 1.1f, 0.0f);
	//	setUBOForPosAndDir(eye, eye + dirs[i], glm::vec3(0.0f, 1.0f, 0.0f));
	//	glBindBufferRange(GL_UNIFORM_BUFFER, /*binding point*/0, this->commom_matrices->ubo, 0, this->commom_matrices->size);

	//	this->skyboxShader->Use();
	//	skybox->Draw(*(this->skyboxShader));

	//	this->shader->Use();
	//	floorMesh->Draw(*(this->shader));
	//}
	////glEnable(GL_LIGHTING);

	this->shader->Use();	
	glUniform1i(glGetUniformLocation(this->shader->Program, "skybox"), 3);

	//-------------------------------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	for (int i = 0; i < 6; i++) {

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w(), h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	}
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubeMap, 0);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w(), h());
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

	glViewport(0, 0, w(), h());
	
	// attach it to currently bound framebuffer object
	for (int i = 0; i < 6; i++) {

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubeMap, 0);

		glBindBufferRange(GL_UNIFORM_BUFFER, /*binding point*/0, this->commom_matrices->ubo, 0, this->commom_matrices->size);	
		
		glm::vec3 eye = glm::vec3(0.0f, waterHeight + 1.1f, 0.0f);
		setUBOForPosAndDir(eye, eye + dirs[i], glm::vec3(0.0f, 1.0f, 0.0f));

		glClearColor(0.5f, 0.8f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // we're not using the stencil buffer now
		//glEnable(GL_DEPTH_TEST);

		this->skyboxShader->Use();
		skybox->Draw(*(this->skyboxShader));

		this->shader->Use();
		floorMesh->Draw(*(this->shader));
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//GLenum fboAttachment[6] = { GL_COLOR_ATTACHMENT0,
	//					GL_COLOR_ATTACHMENT1,
	//					GL_COLOR_ATTACHMENT2,
	//					GL_COLOR_ATTACHMENT3,
	//					GL_COLOR_ATTACHMENT4,
	//					GL_COLOR_ATTACHMENT5 };
	//GLenum  cube[6] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	//				 GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	//				 GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	//				 GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	//				 GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	//				 GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };
	//glActiveTexture(GL_TEXTURE0 + 4);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	//for (int i = 0; i < 6; i++) {

	//	//glBindTexture(GL_TEXTURE_2D, framebuffersForR[i]->texColorBuffer);
	//	glFramebufferTexture2D(GL_FRAMEBUFFER, fboAttachment[i], cube[i], cubeMap, 0);
	//}
	//glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// first pass
	framebuffer->clear();
	framebuffer->use();

	glViewport(0, 0, w(), h());
	//glViewport(0, 0, 128, 128);
	setUBO();
	//glm::vec3 eye = glm::vec3(0.0f, waterHeight + 1.1f, 0.0f);
	//setUBOForPosAndDir(eye, eye + dirs[0], glm::vec3(0.0f, 1.0f, 0.0f));

	//setUBOForPosAndDir(glm::vec3(0.0f, 11.1f, 0.0f), glm::vec3(0.0f, 12.1f, 0.00001f), glm::vec3(0.0f, 1.0f, 0.0f));
	glBindBufferRange(GL_UNIFORM_BUFFER, /*binding point*/0, this->commom_matrices->ubo, 0, this->commom_matrices->size);

	this->skyboxShader->Use();
	//skybox->cubemapTexture = cubeMap;
	skybox->Draw(*(this->skyboxShader));

	//glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	//glActiveTexture(GL_TEXTURE0);
	
	//bind shader
	this->shader->Use();
	glUniform1i(glGetUniformLocation(this->shader->Program, "hasHeightMap"), 1);
	glm::mat4 view_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glm::mat4 viewModel = inverse(view_matrix);
	glm::vec3 cameraPosUniformVersion(viewModel[3]);
	glUniform3fv(glGetUniformLocation(this->shader->Program, "cameraPosUniformVersion"), 1, &cameraPosUniformVersion[0]);
	//glActiveTexture(GL_TEXTURE0 + 3);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	glUniform1i(glGetUniformLocation(this->shader->Program, "skybox"), 3);
	waveMesh->Draw(*(this->shader));

	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	//glUseProgram(0);

	glUniform1i(glGetUniformLocation(this->shader->Program, "mode"), 1);
	glUniform1i(glGetUniformLocation(this->shader->Program, "hasHeightMap"), 0);
	floorMesh->Draw(*(this->shader));

	//setupFloor();
	//glDisable(GL_LIGHTING);
	//drawFloor(200,10);

	glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	screenShader->Use();
	glUniform1i(glGetUniformLocation(screenShader->Program, "screenTexture"), 0);
	glBindVertexArray(quadVAO);
	glDisable(GL_DEPTH_TEST);
	//glBindTexture(GL_TEXTURE_2D, framebuffersForR[0]->texColorBuffer);
	glBindTexture(GL_TEXTURE_2D, framebuffer->texColorBuffer);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindTexture(GL_TEXTURE_2D, 0);

	//bind VAO
	//glBindVertexArray(this->plane->vao);

	//glDrawElements(GL_TRIANGLES, this->plane->element_amount, GL_UNSIGNED_INT, 0);

	//unbind VAO
	//glBindVertexArray(0);

	//unbind shader(switch to fixed pipeline)
	//glUseProgram(0);

	//vector<std::string> faces;
	//{
	//	"right.jpg",
	//	"left.jpg",
	//	"top.jpg",
	//	"bottom.jpg",
	//	"front.jpg",
	//	"back.jpg"
	//};
	//unsigned int cubemapTexture = loadCubemap(faces);
}

//************************************************************************
//
// * This sets up both the Projection and the ModelView matrices
//   HOWEVER: it doesn't clear the projection first (the caller handles
//   that) - its important for picking
//========================================================================
void TrainView::
setProjection()
//========================================================================
{
	// Compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());

	// Check whether we use the world camp
	if (tw->worldCam->value())
		arcball.setProjection(false);
	// Or we use the top cam
	else if (tw->topCam->value()) {
		float wi, he;
		if (aspect >= 1) {
			wi = 110;
			he = wi / aspect;
		} 
		else {
			he = 110;
			wi = he * aspect;
		}

		// Set up the top camera drop mode to be orthogonal and set
		// up proper projection matrix
		glMatrixMode(GL_PROJECTION);
		glOrtho(-wi, wi, -he, he, 200, -200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90,1,0,0);
	} 
	// Or do the train view or other view here
	//####################################################################
	// TODO: 
	// put code for train view projection here!	
	//####################################################################
	else {
#ifdef EXAMPLE_SOLUTION
		trainCamView(this,aspect);
#endif
	}
}

//************************************************************************
//
// * this draws all of the stuff in the world
//
//	NOTE: if you're drawing shadows, DO NOT set colors (otherwise, you get 
//       colored shadows). this gets called twice per draw 
//       -- once for the objects, once for the shadows
//########################################################################
// TODO: 
// if you have other objects in the world, make sure to draw them
//########################################################################
//========================================================================
void TrainView::drawStuff(bool doingShadows)
{
	// Draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (!tw->trainCam->value()) {
		for(size_t i=0; i<m_pTrack->points.size(); ++i) {
			if (!doingShadows) {
				if ( ((int) i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			m_pTrack->points[i].draw();
		}
	}
	// draw the track
	//####################################################################
	// TODO: 
	// call your own track drawing code
	//####################################################################

#ifdef EXAMPLE_SOLUTION
	drawTrack(this, doingShadows);
#endif

	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################
#ifdef EXAMPLE_SOLUTION
	// don't draw the train if you're looking out the front window
	if (!tw->trainCam->value())
		drawTrain(this, doingShadows);
#endif
}

// 
//************************************************************************
//
// * this tries to see which control point is under the mouse
//	  (for when the mouse is clicked)
//		it uses OpenGL picking - which is always a trick
//########################################################################
// TODO: 
//		if you want to pick things other than control points, or you
//		changed how control points are drawn, you might need to change this
//########################################################################
//========================================================================
void TrainView::
doPick()
//========================================================================
{
	// since we'll need to do some GL stuff so we make this window as 
	// active window
	make_current();		

	// where is the mouse?
	int mx = Fl::event_x(); 
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	gluPickMatrix((double)mx, (double)(viewport[3]-my), 
						5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100,buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for(size_t i=0; i<m_pTrack->points.size(); ++i) {
		glLoadName((GLuint) (i+1));
		m_pTrack->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3]-1;
	} else // nothing hit, nothing selected
		selectedCube = -1;

	printf("Selected Cube %d\n",selectedCube);
}

glm::mat4 LookAtRH(glm::vec3 eye, glm::vec3 target, glm::vec3 up)
{
	glm::vec3 zaxis = glm::normalize(eye - target);    // The "forward" vector.
	glm::vec3 xaxis = glm::normalize(glm::cross(up, zaxis));// The "right" vector.
	glm::vec3 yaxis = glm::cross(zaxis, xaxis);     // The "up" vector.

	// Create a 4x4 view matrix from the right, up, forward and eye position vectors
	glm::mat4 viewMatrix = {
		glm::vec4(xaxis.x,            yaxis.x,            zaxis.x,       0),
		glm::vec4(xaxis.y,            yaxis.y,            zaxis.y,       0),
		glm::vec4(xaxis.z,            yaxis.z,            zaxis.z,       0),
		glm::vec4(-glm::dot(xaxis, eye), -glm::dot(yaxis, eye), -glm::dot(zaxis, eye),  1)
	};

	return viewMatrix;
}

void TrainView::setUBOForPosAndDir(glm::vec3 eye, glm::vec3 target, glm::vec3 up) {

	glm::mat4 view_matrix;
	view_matrix = LookAtRH(eye, target, up);
	//HMatrix view_matrix; 
	//this->arcball.getMatrix(view_matrix);

	float FoV = 90.0f;
	glm::mat4 projection_matrix = glm::perspective(
		glm::radians(FoV), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90&deg; (extra wide) and 30&deg; (quite zoomed in)
		1.0f,       // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
		0.1f,              // Near clipping plane. Keep as big as possible, or you'll get precision issues.
		10000.0f             // Far clipping plane. Keep as little as possible.
		);
	//projection_matrix = glm::perspective(glm::radians(this->arcball.getFoV()), (GLfloat)wdt / (GLfloat)hgt, 0.01f, 1000.0f);

	glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &projection_matrix[0][0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &view_matrix[0][0]);
	glm::vec3 cameraPos = eye;
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, sizeof(glm::vec3), &cameraPos);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void TrainView::setUBO()
{
	float wdt = this->pixel_w();
	float hgt = this->pixel_h();

	glm::mat4 view_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	glm::mat4 viewModel = inverse(view_matrix);
	glm::vec3 cameraPos(viewModel[3]);
	printf("(%f, %f, %f)\n", cameraPos.x, cameraPos.y, cameraPos.z);
	//std::cout << cameraPos.x << " " << cameraPos.y << " " << cameraPos.z << std::endl;
	//cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
	//view_matrix = LookAtRH(cameraPos, cameraPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	
	//HMatrix view_matrix; 
	//this->arcball.getMatrix(view_matrix);

	glm::mat4 projection_matrix;
	glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);
	//projection_matrix = glm::perspective(glm::radians(this->arcball.getFoV()), (GLfloat)wdt / (GLfloat)hgt, 0.01f, 1000.0f);

	glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &projection_matrix[0][0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &view_matrix[0][0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, sizeof(glm::vec3), &cameraPos);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}