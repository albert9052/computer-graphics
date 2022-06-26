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
#include "GL/glu.h"

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"


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
void TrainView::draw()
{

	//*********************************************************************
	//
	// * Set up basic opengl informaiton
	//
	//**********************************************************************
	//initialized glad
	if (gladLoadGL())
	{
		//initiailize VAO, VBO, Shader...
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");

	// Set up the view port
	glViewport(0,0,w(),h());

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0,0,.3f,0);		// background should be blue

	// we need to clear out the stencil buffer since we'll use
	// it for shadows
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH);

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
	//glEnable(GL_LIGHT0);

	// top view only needs one light
	if (tw->topCam->value()) {
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
	} else {
		//glEnable(GL_LIGHT1);
		//glEnable(GL_LIGHT2);
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

	// Light. 
	if (tw->lightBrowser->selected(1)) {

		glDisable(GL_LIGHT4);
		glEnable(GL_LIGHT3);
		glDisable(GL_LIGHT5);
		float noAmbient[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		float whiteDiffuse[] = { 2.0f, 2.0f, 2.0f, 2.0f };

		float position[] = { 1.0f, 1.0f, 0.0f, 0.0f };

		glLightfv(GL_LIGHT3, GL_AMBIENT, noAmbient);
		glLightfv(GL_LIGHT3, GL_DIFFUSE, whiteDiffuse);
		glLightfv(GL_LIGHT3, GL_POSITION, position);
	}
	else if (tw->lightBrowser->selected(2)) {

		glDisable(GL_LIGHT3);
		glEnable(GL_LIGHT4);
		glDisable(GL_LIGHT5);
		double spotCutOff = 90.0;
		float noAmbient[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		float diffuse[] = { 2.0f, 2.0f, 2.0f, 2.0f };
		Pnt3f dir = tw->m_Track.trainDir[0];
		dir.normalize();
		dir = dir * 10;
		Pnt3f orient = tw->m_Track.trainOrient[0];
		orient.normalize();
		orient = orient * 5;
		float position[] = { tw->m_Track.trainPosition[0].x + dir.x + orient.x, tw->m_Track.trainPosition[0].y + dir.y + orient.y, tw->m_Track.trainPosition[0].z + dir.z + orient.z, 2.0f};
		Pnt3f direction = tw->m_Track.trainDir[0];
		direction.normalize();

		//glBegin(GL_LINES);
		//glLineWidth(10.0f);
		//glVertex3f(position[0], position[1], position[2]);
		//glVertex3f(0, 0, 0);
		//glEnd();

		// Properties of the light. 
		glLightfv(GL_LIGHT4, GL_AMBIENT, noAmbient);
		glLightfv(GL_LIGHT4, GL_DIFFUSE, diffuse);
		glLightfv(GL_LIGHT4, GL_POSITION, position);

		float directionArray[] = { direction.x, direction.y, direction.z };

		// Spot direction. 
		glLightfv(GL_LIGHT4, GL_SPOT_DIRECTION, directionArray);
		// Angle of the cone light emitted by the spot: value between 0 to 180. 
		glLightf(GL_LIGHT4, GL_SPOT_CUTOFF, spotCutOff);

		// Exponent properite defines the concentration of the light. 
		glLightf(GL_LIGHT4, GL_SPOT_EXPONENT, 15.0f);

		// Light attenuation (default values used here: no attenuation with the distance). 
		glLightf(GL_LIGHT4, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf(GL_LIGHT4, GL_LINEAR_ATTENUATION, 0.0f);
		glLightf(GL_LIGHT4, GL_QUADRATIC_ATTENUATION, 0.0f);
	}
	else {
	
		glEnable(GL_LIGHT5);
		glDisable(GL_LIGHT3);
		glDisable(GL_LIGHT4);
		float yellowAmbientDiffuse[] = { 1.0f, 1.0f, 0.0f, 1.0f };
		
		float position[] = { -2.0f, 2.0f, -5.0f, 1.0f };

		glLightfv(GL_LIGHT5, GL_AMBIENT, yellowAmbientDiffuse);
		glLightfv(GL_LIGHT5, GL_DIFFUSE, yellowAmbientDiffuse);
		glLightfv(GL_LIGHT5, GL_POSITION, position);
	}

	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	glDisable(GL_LIGHTING);
	drawFloor(200,10);


	//*********************************************************************
	// now draw the object and we need to do it twice
	// once for real, and then once for shadows
	//*********************************************************************
	glEnable(GL_LIGHTING);
	setupObjects();

	drawStuff();

	// this time drawing is for shadows (except for top view)
	if (!tw->topCam->value()) {
		setupShadows();
		drawStuff(true);
		unsetupShadows();
	}
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

		//glViewport(0, 0, 1000, 1000);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(
			100.0,
			1000.0 / 1000.0,
			0.001,
			10000.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		Pnt3f currentPosition = m_pTrack->trainPosition[0];
		Pnt3f currentDir = m_pTrack->trainDir[0];
		currentDir.normalize();
		currentPosition = currentPosition + currentDir * 10;
		gluLookAt(	currentPosition.x, currentPosition.y, currentPosition.z,
					(currentPosition + currentDir).x, (currentPosition + currentDir).y, (currentPosition + currentDir).z, 
					0.0, 1.0, 0.0);
		glPushMatrix();
#ifdef EXAMPLE_SOLUTION
		trainCamView(this,aspect);
#endif
	}
}

std::vector<std::vector<double>>* multiply(std::vector<std::vector<double>>& first, std::vector<std::vector<double>>& second) {

	if (first[0].size() != second.size()) {

		std::cerr << "Dimension not mached in multiply function. (" << first[0].size() << ", " << second.size() << ")" << std::endl;
		exit(1);
	}
	std::vector<std::vector<double>>* result = new std::vector<std::vector<double>>;
	result->resize(first.size());
	for (int i = 0; i < result->size(); i++) {

		result->at(i).resize(second[0].size());
		std::fill(result->at(i).begin(), result->at(i).end(), 0);
	}
	for (int i = 0; i < first.size(); i++) {

		for (int j = 0; j < second[0].size(); j++) {

			for (int k = 0; k < first[0].size(); k++) {

				result->at(i).at(j) += first.at(i).at(k) * second.at(k).at(j);
			}
		}
	}
	return result;
}

// The dir and orient and cross must be vertical to each others. 
//void TrainView::buildAHexahedron(Pnt3f mainCorner, Pnt3f dir, Pnt3f orient, Pnt3f cross) {
//	
//	// Top
//	glBegin(GL_QUADS);
//	glNormal3f(-orient.x, -orient.y, -orient.z);
//	glVertex3f(mainCorner.x, mainCorner.y, mainCorner.z);
//	glVertex3f(mainCorner.x + dir.x, mainCorner.y + dir.y, mainCorner.z + dir.z);
//	glVertex3f(mainCorner.x + dir.x + cross.x, mainCorner.y + dir.y + cross.y, mainCorner.z + dir.z + cross.z);
//	glVertex3f(mainCorner.x + cross.x, mainCorner.y + cross.y, mainCorner.y + cross.y);
//	glEnd();
//
//	// Button
//	glBegin(GL_QUADS);
//	glNormal3f(orient.x, orient.y, orient.z);
//	glVertex3f(mainCorner.x + orient.x, mainCorner.y + orient.y, mainCorner.z + orient.z);
//	glVertex3f(mainCorner.x + orient.x + dir.x, mainCorner.y + orient.y + dir.y, mainCorner.z + orient.z + dir.z);
//	glVertex3f(mainCorner.x + orient.x + dir.x + cross.x, mainCorner.y + orient.y + dir.y + cross.y, mainCorner.z + orient.z + dir.z + cross.z);
//	glVertex3f(mainCorner.x + orient.x + cross.x, mainCorner.y + orient.y + cross.y, mainCorner.y + orient.y + cross.y);
//	glEnd();
//
//	// Left
//	glBegin(GL_QUADS);
//	glNormal3f(-cross.x, -cross.y, -cross.z);
//	glVertex3f(mainCorner.x, mainCorner.y, mainCorner.z);
//	glVertex3f(mainCorner.x + dir.x, mainCorner.y + dir.y, mainCorner.z + dir.z);
//	glVertex3f(mainCorner.x + dir.x + orient.x, mainCorner.y + dir.y + orient.y, mainCorner.z + dir.z + orient.z);
//	glVertex3f(mainCorner.x + orient.x, mainCorner.y + orient.y, mainCorner.z + orient.z);
//	glEnd();
//
//	// Right
//	glBegin(GL_QUADS);
//	glNormal3f(cross.x, cross.y, cross.z);
//	glVertex3f(mainCorner.x + cross.x, mainCorner.y + cross.y, mainCorner.z + cross.z);
//	glVertex3f(mainCorner.x + cross.x + dir.x, mainCorner.y + cross.y + dir.y, mainCorner.z + cross.z + dir.z);
//	glVertex3f(mainCorner.x + cross.x + dir.x + orient.x, mainCorner.y + cross.y + dir.y + orient.y, mainCorner.z + cross.z + dir.z + orient.z);
//	glVertex3f(mainCorner.x + cross.x + orient.x, mainCorner.y + cross.y + orient.y, mainCorner.z + cross.z + orient.z);
//	glEnd();
//
//	// back
//	glBegin(GL_QUADS);
//	glNormal3f(-dir.x, -dir.y, -dir.z);
//	glVertex3f(mainCorner.x, mainCorner.y, mainCorner.z);
//	glVertex3f(mainCorner.x + cross.x, mainCorner.y + cross.y, mainCorner.z + cross.z);
//	glVertex3f(mainCorner.x + cross.x + orient.x, mainCorner.y + cross.y + orient.y, mainCorner.z + cross.z + orient.z);
//	glVertex3f(mainCorner.x + orient.x, mainCorner.y + orient.y, mainCorner.z + orient.z);
//	glEnd();
//
//	// Front
//	glBegin(GL_QUADS);
//	glNormal3f(dir.x, dir.y, dir.z);
//	glVertex3f(mainCorner.x + dir.x, mainCorner.y + dir.y, mainCorner.z + dir.z);
//	glVertex3f(mainCorner.x + dir.x + cross.x, mainCorner.y + dir.y + cross.y, mainCorner.z + dir.z + cross.z);
//	glVertex3f(mainCorner.x + dir.x + cross.x + orient.x, mainCorner.y + dir.y + cross.y + orient.y, mainCorner.z + dir.z + cross.z + orient.z);
//	glVertex3f(mainCorner.x + dir.x + orient.x, mainCorner.y + dir.y + orient.y, mainCorner.z + dir.z + orient.z);
//	glEnd();
//}

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
	// draw the track
	//####################################################################
	// TODO: 
	// call your own track drawing code
	//####################################################################

	m_pTrack->allThePoints.clear();

	const int TRACK_DIVIDED_NUMBER = m_pTrack->DIVIDED_LINE;

	double totalLength = 0.0;

	int railNum = 0;

	float* arcLength = new float[TRACK_DIVIDED_NUMBER * m_pTrack->points.size()];
	if (m_pTrack->arcLength != NULL) {

		free(m_pTrack->arcLength);
	}

	for (int i = 0; i < m_pTrack->points.size(); ++i) {

		// pos
		Pnt3f cp_pos_p1 = m_pTrack->points[i].pos;
		Pnt3f cp_pos_p2 = m_pTrack->points[(i + 1) % m_pTrack->points.size()].pos;

		// orient
		Pnt3f cp_orient_p1 = m_pTrack->points[i].orient;
		Pnt3f cp_orient_p2 = m_pTrack->points[(i + 1) % m_pTrack->points.size()].orient;

		float percent = 1.0f / TRACK_DIVIDED_NUMBER;
		float t = 0;
		Pnt3f qt0, qt1;

		std::vector<std::vector<double>>* GAndMatrix;
		bool made = false;

		if (tw->splineBrowser->selected(1) == true) {

			qt0 = (1 - t) * cp_pos_p1 + t * cp_pos_p2;
			qt1 = qt0;

			for (size_t j = 0; j < (float)TRACK_DIVIDED_NUMBER; j++) {

				qt0 = qt1;
				t += percent;
				qt1 = (1 - t) * cp_pos_p1 + t * cp_pos_p2;
				m_pTrack->allThePoints.push_back(qt1);

				// cross
				Pnt3f orient_t = (1 - t) * cp_orient_p1 + t * cp_orient_p2;
				orient_t = orient_t * (qt1 - qt0) * (qt1 - qt0);

				orient_t.normalize();
				orient_t = orient_t * (-1);
				Pnt3f cross_t = (qt1 - qt0) * orient_t;
				cross_t.normalize();
				cross_t = cross_t * 4.1;

				// Store arcLength. 

				arcLength[i * (int)TRACK_DIVIDED_NUMBER + j] = (qt1 - qt0).length();

				if (!doingShadows) {

					glColor3ub(32, 32, 64);
				}


				glLineWidth(5);
				glBegin(GL_LINES);
				glNormal3f(orient_t.x, orient_t.y, orient_t.z);
				glVertex3f(qt0.x - cross_t.x, qt0.y - cross_t.y, qt0.z - cross_t.z);
				glVertex3f(qt1.x - cross_t.x, qt1.y - cross_t.y, qt1.z - cross_t.z);

				glBegin(GL_LINES);
				glNormal3f(orient_t.x, orient_t.y, orient_t.z);
				glVertex3f(qt0.x + cross_t.x, qt0.y + cross_t.y, qt0.z + cross_t.z);
				glVertex3f(qt1.x + cross_t.x, qt1.y + cross_t.y, qt1.z + cross_t.z);
				glEnd();

				glLineWidth(1);

				totalLength += (qt0 - qt1).length();

				if (totalLength / 7 >= railNum) {

					++railNum;
					Pnt3f middlePoint = qt1;
					Pnt3f currentLength = qt1 - qt0;
					currentLength.normalize();
					currentLength = currentLength * 1.5f;
					cross_t = cross_t / 2.5f * 6;
					if (!doingShadows) {

						glColor3ub(255, 255, 0);
					}
					glBegin(GL_POLYGON);
					glNormal3f(orient_t.x, orient_t.y, orient_t.z);
					glVertex3f((middlePoint + currentLength + cross_t).x, (middlePoint + currentLength + cross_t).y, (middlePoint + currentLength + cross_t).z);
					glVertex3f((middlePoint + currentLength - cross_t).x, (middlePoint + currentLength - cross_t).y, (middlePoint + currentLength - cross_t).z);
					glVertex3f((middlePoint - currentLength - cross_t).x, (middlePoint - currentLength - cross_t).y, (middlePoint - currentLength - cross_t).z);
					glVertex3f((middlePoint - currentLength + cross_t).x, (middlePoint - currentLength + cross_t).y, (middlePoint - currentLength + cross_t).z);
					glEnd();
				}
			}
		}
		else if (tw->splineBrowser->selected(2)) {

			std::vector<std::vector<double>> cardinal_cubic_matrix;
			cardinal_cubic_matrix = { { -1.0 / 2.0, 2.0 / 2.0, -1.0 / 2.0, 0.0 / 2.0 },
										{ 3.0 / 2.0, -5.0 / 2.0, 0.0 / 2.0, 2.0 / 2.0 },
										{ -3.0 / 2.0, 4.0 / 2.0, 1.0 / 2.0, 0.0 / 2.0 },
										{ 1.0 / 2.0, -1.0 / 2.0, 0.0 / 2.0, 0 / 2.0 }, };
			auto points = m_pTrack->points;

			std::vector<std::vector<double>> G;
			G.resize(3);
			for (int k = 0; k < 3; k++) {

				G[k].resize(4);
			}
			for (int k = 0; k < 4; k++) {

				G[0][k] = points[(i + points.size() - 1 + k) % points.size()].pos.x;
				G[1][k] = points[(i + points.size() - 1 + k) % points.size()].pos.y;
				G[2][k] = points[(i + points.size() - 1 + k) % points.size()].pos.z;
			}
			GAndMatrix = multiply(G, cardinal_cubic_matrix);

			std::vector<std::vector<double>> T;
			T = { {pow(t, 3)}, {pow(t, 2)}, {t}, {1} };
			std::vector<std::vector<double>>* result = multiply(*GAndMatrix, T);

			qt0 = Pnt3f(result->at(0)[0], result->at(1)[0], result->at(2)[0]);
			qt1 = qt0;

			delete(result);


			for (size_t j = 0; j < (float)TRACK_DIVIDED_NUMBER; j++) {

				qt0 = qt1;
				t += percent;
				T = { {pow(t, 3)}, {pow(t, 2)}, {t}, {1} };
				result = multiply(*GAndMatrix, T);
				qt1 = Pnt3f(result->at(0)[0], result->at(1)[0], result->at(2)[0]);
				m_pTrack->allThePoints.push_back(qt1);
				delete(result);

				// Store arcLength. 

				arcLength[i * (int)TRACK_DIVIDED_NUMBER + j] = (qt1 - qt0).length();

				// cross
				Pnt3f orient_t = (1 - t) * cp_orient_p1 + t * cp_orient_p2;
				orient_t = orient_t * (qt1 - qt0) * (qt1 - qt0);
				orient_t.normalize();
				orient_t = orient_t * (-1);
				Pnt3f cross_t = (qt1 - qt0) * orient_t;
				cross_t.normalize();
				cross_t = cross_t * 4.1;

				if (!doingShadows) {

					glColor3ub(32, 32, 64);
				}

				glLineWidth(5);
				glBegin(GL_LINES);
				glNormal3f(orient_t.x, orient_t.y, orient_t.z);
				glVertex3f(qt0.x - cross_t.x, qt0.y - cross_t.y, qt0.z - cross_t.z);
				glVertex3f(qt1.x - cross_t.x, qt1.y - cross_t.y, qt1.z - cross_t.z);

				glBegin(GL_LINES);
				glNormal3f(orient_t.x, orient_t.y, orient_t.z);
				glVertex3f(qt0.x + cross_t.x, qt0.y + cross_t.y, qt0.z + cross_t.z);
				glVertex3f(qt1.x + cross_t.x, qt1.y + cross_t.y, qt1.z + cross_t.z);
				glEnd();

				glLineWidth(1);

				totalLength += (qt0 - qt1).length();

				if (totalLength / 7 >= railNum) {

					++railNum;
					Pnt3f middlePoint = qt1;
					Pnt3f currentLength = qt1 - qt0;
					currentLength.normalize();
					currentLength = currentLength * 1.5f;
					cross_t = cross_t / 2.5f * 6;
					if (!doingShadows) {

						glColor3ub(255, 255, 0);
					}
					glBegin(GL_POLYGON);
					glNormal3f(orient_t.x, orient_t.y, orient_t.z);
					glVertex3f((middlePoint + currentLength + cross_t).x, (middlePoint + currentLength + cross_t).y, (middlePoint + currentLength + cross_t).z);
					glVertex3f((middlePoint + currentLength - cross_t).x, (middlePoint + currentLength - cross_t).y, (middlePoint + currentLength - cross_t).z);
					glVertex3f((middlePoint - currentLength - cross_t).x, (middlePoint - currentLength - cross_t).y, (middlePoint - currentLength - cross_t).z);
					glVertex3f((middlePoint - currentLength + cross_t).x, (middlePoint - currentLength + cross_t).y, (middlePoint - currentLength + cross_t).z);
					glEnd();
				}
			}
		}
		else if (tw->splineBrowser->selected(3)) {

			std::vector<std::vector<double>> cardinal_cubic_matrix;
			std::vector<std::vector<double>> cubic_bSpline_matrix;
			cubic_bSpline_matrix = { { -1.0 / 6.0, 3.0 / 6.0, -3.0 / 6.0, 1.0 / 6.0 },
									{ 3.0 / 6.0, -6.0 / 6.0, 0.0 / 6.0, 4.0 / 6.0 },
									{ -3.0 / 6.0, 3.0 / 6.0, 3.0 / 6.0, 1.0 / 6.0 },
									{ 1.0 / 6.0, 0.0 / 6.0, 0.0 / 6.0, 0.0 / 6.0 }, };
			auto points = m_pTrack->points;

			std::vector<std::vector<double>> G;
			G.resize(3);
			for (int k = 0; k < 3; k++) {

				G[k].resize(4);
			}
			for (int k = 0; k < 4; k++) {

				G[0][k] = points[(i + points.size() - 1 + k) % points.size()].pos.x;
				G[1][k] = points[(i + points.size() - 1 + k) % points.size()].pos.y;
				G[2][k] = points[(i + points.size() - 1 + k) % points.size()].pos.z;
			}
			GAndMatrix = multiply(G, cubic_bSpline_matrix);

			std::vector<std::vector<double>> T;
			T = { {pow(t, 3)}, {pow(t, 2)}, {t}, {1} };
			std::vector<std::vector<double>>* result = multiply(*GAndMatrix, T);

			qt0 = Pnt3f(result->at(0)[0], result->at(1)[0], result->at(2)[0]);
			qt1 = qt0;

			delete(result);

			for (size_t j = 0; j < (float)TRACK_DIVIDED_NUMBER; j++) {

				qt0 = qt1;
				t += percent;
				T = { {pow(t, 3)}, {pow(t, 2)}, {t}, {1} };
				result = multiply(*GAndMatrix, T);
				qt1 = Pnt3f(result->at(0)[0], result->at(1)[0], result->at(2)[0]);
				m_pTrack->allThePoints.push_back(qt1);
				delete(result);

				// Store arcLength. 

				arcLength[i * (int)TRACK_DIVIDED_NUMBER + j] = (qt1 - qt0).length();

				// cross
				Pnt3f orient_t = (1 - t) * cp_orient_p1 + t * cp_orient_p2;
				orient_t = orient_t * (qt1 - qt0) * (qt1 - qt0);
				orient_t.normalize();
				orient_t = orient_t * (-1);
				Pnt3f cross_t = (qt1 - qt0) * orient_t;
				cross_t.normalize();
				cross_t = cross_t * 4.1f;

				if (!doingShadows) {

					glColor3ub(32, 32, 64);
				}

				glLineWidth(5);
				glBegin(GL_LINES);
				glNormal3f(orient_t.x, orient_t.y, orient_t.z);
				glVertex3f(qt0.x - cross_t.x, qt0.y - cross_t.y, qt0.z - cross_t.z);
				glVertex3f(qt1.x - cross_t.x, qt1.y - cross_t.y, qt1.z - cross_t.z);

				glBegin(GL_LINES);
				glNormal3f(orient_t.x, orient_t.y, orient_t.z);
				glVertex3f(qt0.x + cross_t.x, qt0.y + cross_t.y, qt0.z + cross_t.z);
				glVertex3f(qt1.x + cross_t.x, qt1.y + cross_t.y, qt1.z + cross_t.z);
				glEnd();

				glLineWidth(1);

				totalLength += (qt0 - qt1).length();

				if (totalLength / 7 >= railNum) {

					++railNum;
					Pnt3f middlePoint = qt1;
					Pnt3f currentLength = qt1 - qt0;
					currentLength.normalize();
					currentLength = currentLength * 1.5f;
					cross_t = cross_t / 2.5f * 6;
					if (!doingShadows) {

						glColor3ub(255, 255, 0);
					}
					glBegin(GL_POLYGON);
					glNormal3f(orient_t.x, orient_t.y, orient_t.z);
					glVertex3f((middlePoint + currentLength + cross_t).x, (middlePoint + currentLength + cross_t).y, (middlePoint + currentLength + cross_t).z);
					glVertex3f((middlePoint + currentLength - cross_t).x, (middlePoint + currentLength - cross_t).y, (middlePoint + currentLength - cross_t).z);
					glVertex3f((middlePoint - currentLength - cross_t).x, (middlePoint - currentLength - cross_t).y, (middlePoint - currentLength - cross_t).z);
					glVertex3f((middlePoint - currentLength + cross_t).x, (middlePoint - currentLength + cross_t).y, (middlePoint - currentLength + cross_t).z);
					glEnd();
				}
			}
		}
	}

	m_pTrack->arcLength = arcLength;


#ifdef EXAMPLE_SOLUTION
	drawTrack(this, doingShadows);
#endif

	// Draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	float supportStructureWidth = 2;
	float expandingStructureWidth = 10;
	float avoidingRadius = 10;

	for (size_t i = 0; i < m_pTrack->points.size(); ++i) {

		if (!tw->trainCam->value()) {

			if (!doingShadows) {

				if (((int)i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			m_pTrack->points[i].draw();
		}
		if (!doingShadows) {

			glColor3ub(255, 0, 239);
		}
		Pnt3f currentPoint = m_pTrack->points[i].pos;
		
		// Check whether there's any track through the supporting structure. 

		Pnt3f* interruptingPoint = NULL;
		Pnt3f expandingStructureDir;
		while (true) {
			
			interruptingPoint = NULL;

			//printf("(%f, %f, %f)\n", currentPoint.x, currentPoint.y, currentPoint.z);

			if (currentPoint.y == 0) {

				break;
			}

			for (int i = 0; i < m_pTrack->allThePoints.size() - 1; i++) {

				if (m_pTrack->allThePoints[i].y < currentPoint.y) {

					if (sqrt(pow(currentPoint.x - m_pTrack->allThePoints[i].x, 2) + pow(currentPoint.z - m_pTrack->allThePoints[i].z, 2)) <= avoidingRadius) {

						if (interruptingPoint != NULL) {

							if (m_pTrack->allThePoints[i].y > interruptingPoint->y) {

								interruptingPoint = &(m_pTrack->allThePoints[i]);
								expandingStructureDir = m_pTrack->allThePoints[i + 1] - m_pTrack->allThePoints[i];
							}
						}
						else {

							interruptingPoint = &(m_pTrack->allThePoints[i]);
							expandingStructureDir = m_pTrack->allThePoints[i + 1] - m_pTrack->allThePoints[i];
						}
					}
				}
			}

			Pnt3f endingPoint;
			if (interruptingPoint == NULL) {

				endingPoint = currentPoint;
				endingPoint.y = 0.0;
			}
			else {

				Pnt3f cross = expandingStructureDir * Pnt3f(0, 1, 0);
				cross.normalize();
				endingPoint = *interruptingPoint + (cross * (avoidingRadius + 1));
			}

			// Top
			glBegin(GL_QUADS);
			glNormal3f(0.0, 1.0, 0.0);
			glVertex3f(currentPoint.x + supportStructureWidth / 2, currentPoint.y, currentPoint.z + supportStructureWidth / 2);
			glVertex3f(currentPoint.x + supportStructureWidth / 2, currentPoint.y, currentPoint.z - supportStructureWidth / 2);
			glVertex3f(currentPoint.x - supportStructureWidth / 2, currentPoint.y, currentPoint.z - supportStructureWidth / 2);
			glVertex3f(currentPoint.x - supportStructureWidth / 2, currentPoint.y, currentPoint.z + supportStructureWidth / 2);
			glEnd();

			// Button
			glBegin(GL_QUADS);
			glNormal3f(0.0, -1.0, 0.0);
			glVertex3f(endingPoint.x + supportStructureWidth / 2, endingPoint.y, endingPoint.z + supportStructureWidth / 2);
			glVertex3f(endingPoint.x + supportStructureWidth / 2, endingPoint.y, endingPoint.z - supportStructureWidth / 2);
			glVertex3f(endingPoint.x - supportStructureWidth / 2, endingPoint.y, endingPoint.z - supportStructureWidth / 2);
			glVertex3f(endingPoint.x - supportStructureWidth / 2, endingPoint.y, endingPoint.z + supportStructureWidth / 2);
			glEnd();

			// Right
			glBegin(GL_QUADS);
			glNormal3f(1.0, 0.0, 0.0);
			glVertex3f(currentPoint.x + supportStructureWidth / 2, currentPoint.y, currentPoint.z + supportStructureWidth / 2);
			glVertex3f(currentPoint.x + supportStructureWidth / 2, currentPoint.y, currentPoint.z - supportStructureWidth / 2);
			glVertex3f(endingPoint.x + supportStructureWidth / 2, endingPoint.y, endingPoint.z - supportStructureWidth / 2);
			glVertex3f(endingPoint.x + supportStructureWidth / 2, endingPoint.y, endingPoint.z + supportStructureWidth / 2);
			glEnd();

			// Left
			glBegin(GL_QUADS);
			glNormal3f(-1.0, 0.0, 0.0);
			glVertex3f(currentPoint.x - supportStructureWidth / 2, currentPoint.y, currentPoint.z + supportStructureWidth / 2);
			glVertex3f(currentPoint.x - supportStructureWidth / 2, currentPoint.y, currentPoint.z - supportStructureWidth / 2);
			glVertex3f(endingPoint.x - supportStructureWidth / 2, endingPoint.y, endingPoint.z - supportStructureWidth / 2);
			glVertex3f(endingPoint.x - supportStructureWidth / 2, endingPoint.y, endingPoint.z + supportStructureWidth / 2);
			glEnd();

			// Front
			glBegin(GL_QUADS);
			glNormal3f(0.0, 0.0, 1.0);
			glVertex3f(currentPoint.x + supportStructureWidth / 2, currentPoint.y, currentPoint.z + supportStructureWidth / 2);
			glVertex3f(currentPoint.x - supportStructureWidth / 2, currentPoint.y, currentPoint.z + supportStructureWidth / 2);
			glVertex3f(endingPoint.x - supportStructureWidth / 2, endingPoint.y, endingPoint.z + supportStructureWidth / 2);
			glVertex3f(endingPoint.x + supportStructureWidth / 2, endingPoint.y, endingPoint.z + supportStructureWidth / 2);
			glEnd();

			// Back
			glBegin(GL_QUADS);
			glNormal3f(0.0, 0.0, -1.0);
			glVertex3f(currentPoint.x + supportStructureWidth / 2, currentPoint.y, currentPoint.z - supportStructureWidth / 2);
			glVertex3f(currentPoint.x - supportStructureWidth / 2, currentPoint.y, currentPoint.z - supportStructureWidth / 2);
			glVertex3f(endingPoint.x - supportStructureWidth / 2, endingPoint.y, endingPoint.z - supportStructureWidth / 2);
			glVertex3f(endingPoint.x + supportStructureWidth / 2, endingPoint.y, endingPoint.z - supportStructureWidth / 2);
			glEnd();

			currentPoint = endingPoint;
		}
	}


	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################

	float distance = 15.0;
	int carNumber = 3;

	for (int x = 0; x < m_pTrack->trainNum; x++) {

		if (!doingShadows) {

			glColor3ub(40, 80, 20);
		}

		int i = floor(m_pTrack->trainU[x]);
		float t = m_pTrack->trainU[x] - i;
		for (int car = 0; car < carNumber; car++) {

			double height = 10;
			double width = 8;
			double length = 10;
			double floatDistance = 7;

			double WHEEL_DIVIDED_NUMBER = 360.0;
			double wheelRadius = 2;
			double wheelWidth = width + 0.2;
			double wheelAxleSpace = 5.5;

			// pos
			Pnt3f cp_pos_p1 = m_pTrack->points[i].pos;
			Pnt3f cp_pos_p2 = m_pTrack->points[(i + 1) % m_pTrack->points.size()].pos;

			// orient
			Pnt3f cp_orient_p1 = m_pTrack->points[i].orient;
			Pnt3f cp_orient_p2 = m_pTrack->points[(i + 1) % m_pTrack->points.size()].orient;

			float percent = 1.0f / (float)TRACK_DIVIDED_NUMBER;
			Pnt3f qt0, qt1;
			Pnt3f dir_t, orient_t, cross_t, floatDis_t;

			if (tw->splineBrowser->selected(1)) {

				qt0 = ((1 - (t - percent)) * cp_pos_p1 + (t - percent) * cp_pos_p2);
				qt1 = (1 - t) * cp_pos_p1 + t * cp_pos_p2;

				// direction
				dir_t = qt1 - qt0;
				dir_t.normalize();
				dir_t = dir_t * length / 2.0;

				// orient
				orient_t = (1 - t) * cp_orient_p1 + t * cp_orient_p2;
				orient_t = orient_t * dir_t * dir_t * (-1);
				orient_t.normalize();
				orient_t = orient_t * height / 2.0;

				// cross
				cross_t = (qt1 - qt0) * orient_t;
				cross_t.normalize();
				cross_t = cross_t * width / 2.0;

				// float distance
				floatDis_t = orient_t;
				floatDis_t.normalize();
				floatDis_t = floatDis_t * floatDistance;

				qt1 = qt1 + floatDis_t;
			}
			else if (tw->splineBrowser->selected(2) == true) {

				std::vector<std::vector<double>> cardinal_cubic_matrix;
				cardinal_cubic_matrix = { { -1.0 / 2.0, 2.0 / 2.0, -1.0 / 2.0, 0.0 / 2.0 },
											{ 3.0 / 2.0, -5.0 / 2.0, 0.0 / 2.0, 2.0 / 2.0 },
											{ -3.0 / 2.0, 4.0 / 2.0, 1.0 / 2.0, 0.0 / 2.0 },
											{ 1.0 / 2.0, -1.0 / 2.0, 0.0 / 2.0, 0 / 2.0 }, };
				auto points = m_pTrack->points;

				std::vector<std::vector<double>> G;
				G.resize(3);
				for (int k = 0; k < 3; k++) {

					G[k].resize(4);
				}
				for (int k = 0; k < 4; k++) {

					G[0][k] = points[(i + points.size() - 1 + k) % points.size()].pos.x;
					G[1][k] = points[(i + points.size() - 1 + k) % points.size()].pos.y;
					G[2][k] = points[(i + points.size() - 1 + k) % points.size()].pos.z;
				}
				std::vector<std::vector<double>> T;
				T = { {pow(t, 3)}, {pow(t, 2)}, {t}, {1} };
				std::vector<std::vector<double>>* matrixAndT = multiply(cardinal_cubic_matrix, T);
				std::vector<std::vector<double>>* result = multiply(G, *matrixAndT);

				qt1 = Pnt3f(result->at(0)[0], result->at(1)[0], result->at(2)[0]);
				delete(matrixAndT);
				delete(result);

				T = { {pow(t - percent, 3)}, {pow(t - percent, 2)}, {t - percent}, {1} };
				matrixAndT = multiply(cardinal_cubic_matrix, T);
				result = multiply(G, *matrixAndT);

				qt0 = Pnt3f(result->at(0)[0], result->at(1)[0], result->at(2)[0]);
				delete(matrixAndT);
				delete(result);

				// direction
				dir_t = qt1 - qt0;
				dir_t.normalize();
				dir_t = dir_t * length / 2.0;

				// orient
				orient_t = (1 - t) * cp_orient_p1 + t * cp_orient_p2;
				orient_t = orient_t * dir_t * dir_t * (-1);
				orient_t.normalize();
				orient_t = orient_t * height / 2.0;

				// cross
				cross_t = (qt1 - qt0) * orient_t;
				cross_t.normalize();
				cross_t = cross_t * width / 2.0;

				// float distance
				floatDis_t = orient_t;
				floatDis_t.normalize();
				floatDis_t = floatDis_t * floatDistance;

				qt1 = qt1 + floatDis_t;
			}
			else if (tw->splineBrowser->selected(3) == true) {

				std::vector<std::vector<double>> cubic_bSpline_matrix;
				cubic_bSpline_matrix = { { -1.0 / 6.0, 3.0 / 6.0, -3.0 / 6.0, 1.0 / 6.0 },
										{ 3.0 / 6.0, -6.0 / 6.0, 0.0 / 6.0, 4.0 / 6.0 },
										{ -3.0 / 6.0, 3.0 / 6.0, 3.0 / 6.0, 1.0 / 6.0 },
										{ 1.0 / 6.0, 0.0 / 6.0, 0.0 / 6.0, 0.0 / 6.0 }, };

				auto points = m_pTrack->points;

				std::vector<std::vector<double>> G;
				G.resize(3);
				for (int k = 0; k < 3; k++) {

					G[k].resize(4);
				}
				for (int k = 0; k < 4; k++) {

					G[0][k] = points[(i + points.size() - 1 + k) % points.size()].pos.x;
					G[1][k] = points[(i + points.size() - 1 + k) % points.size()].pos.y;
					G[2][k] = points[(i + points.size() - 1 + k) % points.size()].pos.z;
				}
				std::vector<std::vector<double>> T;
				T = { {pow(t, 3)}, {pow(t, 2)}, {t}, {1} };
				std::vector<std::vector<double>>* matrixAndT = multiply(cubic_bSpline_matrix, T);
				std::vector<std::vector<double>>* result = multiply(G, *matrixAndT);

				qt1 = Pnt3f(result->at(0)[0], result->at(1)[0], result->at(2)[0]);
				delete(matrixAndT);
				delete(result);

				T = { {pow(t - percent, 3)}, {pow(t - percent, 2)}, {t - percent}, {1} };
				matrixAndT = multiply(cubic_bSpline_matrix, T);
				result = multiply(G, *matrixAndT);

				qt0 = Pnt3f(result->at(0)[0], result->at(1)[0], result->at(2)[0]);
				delete(matrixAndT);
				delete(result);

				// direction
				dir_t = qt1 - qt0;
				dir_t.normalize();
				dir_t = dir_t * length / 2.0;

				// orient
				orient_t = (1 - t) * cp_orient_p1 + t * cp_orient_p2;
				orient_t = orient_t * dir_t * dir_t * (-1);
				orient_t.normalize();
				orient_t = orient_t * height / 2.0;

				// cross
				cross_t = (qt1 - qt0) * orient_t;
				cross_t.normalize();
				cross_t = cross_t * width / 2.0;

				// float distance
				floatDis_t = orient_t;
				floatDis_t.normalize();
				floatDis_t = floatDis_t * floatDistance;

				qt1 = qt1 + floatDis_t;
			}

			if (car == 0) {

				m_pTrack->trainPosition[x] = qt1;
				m_pTrack->trainDir[x] = dir_t;
				m_pTrack->trainOrient[x] = orient_t;
			}

			glBegin(GL_POLYGON);
			glNormal3f(-orient_t.x, -orient_t.y, -orient_t.z);
			glVertex3f(qt1.x + dir_t.x + cross_t.x - orient_t.x, qt1.y + dir_t.y + cross_t.y - orient_t.y, qt1.z + dir_t.z + cross_t.z - orient_t.z);
			glVertex3f(qt1.x + dir_t.x - cross_t.x - orient_t.x, qt1.y + dir_t.y - cross_t.y - orient_t.y, qt1.z + dir_t.z - cross_t.z - orient_t.z);
			glVertex3f(qt1.x - dir_t.x - cross_t.x - orient_t.x, qt1.y - dir_t.y - cross_t.y - orient_t.y, qt1.z - dir_t.z - cross_t.z - orient_t.z);
			glVertex3f(qt1.x - dir_t.x + cross_t.x - orient_t.x, qt1.y - dir_t.y + cross_t.y - orient_t.y, qt1.z - dir_t.z + cross_t.z - orient_t.z);
			glEnd();

			glBegin(GL_POLYGON);
			glNormal3f(orient_t.x, orient_t.y, orient_t.z);
			glVertex3f(qt1.x + dir_t.x + cross_t.x + orient_t.x, qt1.y + dir_t.y + cross_t.y + orient_t.y, qt1.z + dir_t.z + cross_t.z + orient_t.z);
			glVertex3f(qt1.x + dir_t.x - cross_t.x + orient_t.x, qt1.y + dir_t.y - cross_t.y + orient_t.y, qt1.z + dir_t.z - cross_t.z + orient_t.z);
			glVertex3f(qt1.x - dir_t.x - cross_t.x + orient_t.x, qt1.y - dir_t.y - cross_t.y + orient_t.y, qt1.z - dir_t.z - cross_t.z + orient_t.z);
			glVertex3f(qt1.x - dir_t.x + cross_t.x + orient_t.x, qt1.y - dir_t.y + cross_t.y + orient_t.y, qt1.z - dir_t.z + cross_t.z + orient_t.z);
			glEnd();

			glBegin(GL_POLYGON);
			glNormal3f(-cross_t.x, -cross_t.y, -cross_t.z);
			glVertex3f(qt1.x - cross_t.x + orient_t.x + dir_t.x, qt1.y - cross_t.y + orient_t.y + dir_t.y, qt1.z - cross_t.z + orient_t.z + dir_t.z);
			glVertex3f(qt1.x - cross_t.x - orient_t.x + dir_t.x, qt1.y - cross_t.y - orient_t.y + dir_t.y, qt1.z - cross_t.z - orient_t.z + dir_t.z);
			glVertex3f(qt1.x - cross_t.x - orient_t.x - dir_t.x, qt1.y - cross_t.y - orient_t.y - dir_t.y, qt1.z - cross_t.z - orient_t.z - dir_t.z);
			glVertex3f(qt1.x - cross_t.x + orient_t.x - dir_t.x, qt1.y - cross_t.y + orient_t.y - dir_t.y, qt1.z - cross_t.z + orient_t.z - dir_t.z);
			glEnd();

			glBegin(GL_POLYGON);
			glNormal3f(cross_t.x, cross_t.y, cross_t.z);
			glVertex3f(qt1.x + cross_t.x + orient_t.x + dir_t.x, qt1.y + cross_t.y + orient_t.y + dir_t.y, qt1.z + cross_t.z + orient_t.z + dir_t.z);
			glVertex3f(qt1.x + cross_t.x - orient_t.x + dir_t.x, qt1.y + cross_t.y - orient_t.y + dir_t.y, qt1.z + cross_t.z - orient_t.z + dir_t.z);
			glVertex3f(qt1.x + cross_t.x - orient_t.x - dir_t.x, qt1.y + cross_t.y - orient_t.y - dir_t.y, qt1.z + cross_t.z - orient_t.z - dir_t.z);
			glVertex3f(qt1.x + cross_t.x + orient_t.x - dir_t.x, qt1.y + cross_t.y + orient_t.y - dir_t.y, qt1.z + cross_t.z + orient_t.z - dir_t.z);
			glEnd();

			glColor3ub(0, 0, 240);
			glBegin(GL_POLYGON);
			glNormal3f(dir_t.x, dir_t.y, dir_t.z);
			glVertex3f(qt1.x + dir_t.x + orient_t.x + cross_t.x, qt1.y + dir_t.y + orient_t.y + cross_t.y, qt1.z + dir_t.z + orient_t.z + cross_t.z);
			glVertex3f(qt1.x + dir_t.x - orient_t.x + cross_t.x, qt1.y + dir_t.y - orient_t.y + cross_t.y, qt1.z + dir_t.z - orient_t.z + cross_t.z);
			glVertex3f(qt1.x + dir_t.x - orient_t.x - cross_t.x, qt1.y + dir_t.y - orient_t.y - cross_t.y, qt1.z + dir_t.z - orient_t.z - cross_t.z);
			glVertex3f(qt1.x + dir_t.x + orient_t.x - cross_t.x, qt1.y + dir_t.y + orient_t.y - cross_t.y, qt1.z + dir_t.z + orient_t.z - cross_t.z);
			glEnd();

			glColor3ub(0, 0, 0);
			glBegin(GL_POLYGON);
			glNormal3f(-dir_t.x, -dir_t.y, -dir_t.z);
			glVertex3f(qt1.x - dir_t.x + orient_t.x + cross_t.x, qt1.y - dir_t.y + orient_t.y + cross_t.y, qt1.z - dir_t.z + orient_t.z + cross_t.z);
			glVertex3f(qt1.x - dir_t.x - orient_t.x + cross_t.x, qt1.y - dir_t.y - orient_t.y + cross_t.y, qt1.z - dir_t.z - orient_t.z + cross_t.z);
			glVertex3f(qt1.x - dir_t.x - orient_t.x - cross_t.x, qt1.y - dir_t.y - orient_t.y - cross_t.y, qt1.z - dir_t.z - orient_t.z - cross_t.z);
			glVertex3f(qt1.x - dir_t.x + orient_t.x - cross_t.x, qt1.y - dir_t.y + orient_t.y - cross_t.y, qt1.z - dir_t.z + orient_t.z - cross_t.z);
			glEnd();

			// Draw wheels. 
			dir_t.normalize();
			orient_t.normalize();
			cross_t.normalize();
			Pnt3f centersOfCircles[4];
			centersOfCircles[0] = qt1 + cross_t * wheelWidth / 2.0 + dir_t * wheelAxleSpace / 2.0 - orient_t * height / 2.0;
			centersOfCircles[1] = qt1 - cross_t * wheelWidth / 2.0 + dir_t * wheelAxleSpace / 2.0 - orient_t * height / 2.0;
			centersOfCircles[2] = qt1 - cross_t * wheelWidth / 2.0 - dir_t * wheelAxleSpace / 2.0 - orient_t * height / 2.0;
			centersOfCircles[3] = qt1 + cross_t * wheelWidth / 2.0 - dir_t * wheelAxleSpace / 2.0 - orient_t * height / 2.0;
			Pnt3f mainSpoke[4];
			mainSpoke[0] = orient_t;
			mainSpoke[1] = orient_t * cross_t;
			mainSpoke[2] = orient_t * (-1);
			mainSpoke[3] = orient_t * cross_t * (-1);
			for (int i = 0; i < 4; i++) {

				mainSpoke[i].normalize();
			}
			for (int wheel = 0; wheel < 4; wheel++) {

				if (doingShadows == false) {

					glColor3ub(20, 40, 50);
				}
				glBegin(GL_TRIANGLE_FAN);
				glVertex3f(centersOfCircles[wheel].x, centersOfCircles[wheel].y, centersOfCircles[wheel].z);
				for (int i = 0; i < 4; i++) {

					double t = 0.0;
					double percent = 1.0 / (WHEEL_DIVIDED_NUMBER / 4.0);
					t += percent;
					Pnt3f currentSpoke = mainSpoke[i % 4] + centersOfCircles[wheel];
					glVertex3f(currentSpoke.x, currentSpoke.y, currentSpoke.z);
					while (t <= 1.1) {

						currentSpoke = (mainSpoke[i % 4] * (1.0 - t) + mainSpoke[(i + 1) % 4] * t);
						currentSpoke.normalize();
						currentSpoke = currentSpoke * wheelRadius + centersOfCircles[wheel];
						glVertex3f(currentSpoke.x, currentSpoke.y, currentSpoke.z);
						t += percent;
						if (wheel <= 1)
							glNormal3f(cross_t.x, cross_t.y, cross_t.z);
						else
							glNormal3f(-cross_t.x, -cross_t.y, -cross_t.z);
					}
				}
				glEnd();
			}
			float currentDistance = distance;
			int currentTrack = (int)floor((i + t) * m_pTrack->DIVIDED_LINE);
			while (m_pTrack->arcLength[currentTrack % (m_pTrack->DIVIDED_LINE * m_pTrack->points.size())] < currentDistance) {

				if (currentTrack < 0) {

					currentTrack += m_pTrack->points.size() * m_pTrack->DIVIDED_LINE;
				}
				currentDistance -= m_pTrack->arcLength[currentTrack % (m_pTrack->DIVIDED_LINE * m_pTrack->points.size())];
				currentTrack--;
			}
			currentTrack = currentTrack % (m_pTrack->DIVIDED_LINE * m_pTrack->points.size());
			i = currentTrack / m_pTrack->DIVIDED_LINE;
			t = (float)(currentTrack % m_pTrack->DIVIDED_LINE) / m_pTrack->DIVIDED_LINE;
		}
	}

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