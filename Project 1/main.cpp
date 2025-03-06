#include <iostream>			
#include <windows.h>        
#include <stdlib.h>         
#include <stdio.h>
#include <GL/glew.h>        
#include <GL/freeglut.h>    
#include "loadShaders.h"	
#include "glm/glm.hpp"		
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <cmath>
#include <algorithm>
#include <vector>
#include "SOIL.h"
#include "time.h"

using namespace std;


GLuint
VaoId,
VboId,
EboId,
ColorBufferId,
ProgramId,
myMatrixLocation,
matrScaleLocation,
matrTranslLocation,
matrRotlLocation,
codColLocation;

GLfloat
winWidth = 500, winHeight = 600;

glm::mat4
myMatrix, resizeMatrix, matrTransl, matrScale1, matrScale2, matrRot, matrDepl, matrTranslInit1Hand1, matrTranslInit2Hand1, matrTranslInit1Hand2, matrTranslInit2Hand2, matrTranslBall;
#define M_PI 3.14159265

int codCol;

float xMin = 0.0f, xMax = 500.0f, yMin = 0.0f, yMax = 600.0f;

float i = 0, j = 0, alpha = 0.0, step = 0.3, beta = 0.002, angle = 0, angleInit1 = -M_PI / 6, angleInit2 = M_PI / 6, angleHand1 = -M_PI / 6, angleHand2 = M_PI / 6;
float xvel = 0, yvel = -1.5f, xpos = 200.0, ypos = 500.0, xposInit;
float surfaceModifier1, surfaceModifier2;







void DisplayMatrix()
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
			cout << myMatrix[i][j] << "  ";
		cout << endl;
	};
	cout << "\n";
};




double calculateAngle(double x, double y) {
	return atan2(y, x); 
}

bool isPointOnSegment(double ix, double iy, double p1x, double p1y, double p2x, double p2y) {
	return std::min(p1x, p2x) <= ix && ix <= std::max(p1x, p2x) &&
		std::min(p1y, p2y) <= iy && iy <= std::max(p1y, p2y);
}

std::pair<bool, double> findFirstIntersectionAngle(
	double p1x, double p1y, double p2x, double p2y, double cx, double cy, double r) {

	p1x -= cx; p1y -= cy;
	p2x -= cx; p2y -= cy;

	double dx = p2x - p1x;
	double dy = p2y - p1y;
	double dr2 = dx * dx + dy * dy;
	double D = p1x * p2y - p2x * p1y;
	double discriminant = r * r * dr2 - D * D;

	if (discriminant >= 0) {
		double sqrtDisc = sqrt(discriminant);
		double signDy = (dy < 0) ? -1 : 1;

		double ix1 = (D * dy + signDy * dx * sqrtDisc) / dr2;
		double iy1 = (-D * dx + fabs(dy) * sqrtDisc) / dr2;

		if (isPointOnSegment(ix1 + cx, iy1 + cy, p1x + cx, p1y + cy, p2x + cx, p2y + cy)) {
			return { true, calculateAngle(ix1, iy1) };
		}
		if (discriminant > 0) {
			double ix2 = (D * dy - signDy * dx * sqrtDisc) / dr2;
			double iy2 = (-D * dx - fabs(dy) * sqrtDisc) / dr2;
			if (isPointOnSegment(ix2 + cx, iy2 + cy, p1x + cx, p1y + cy, p2x + cx, p2y + cy)) {
				return { true, calculateAngle(ix2, iy2) };
			}
		}
	}
	return { false, -1 };
}

double getFirstIntersectionAngle(double Sx, double Sy, double r,
	double Ax, double Ay,
	double Bx, double By,
	double Cx, double Cy) {

	auto intersectionAB = findFirstIntersectionAngle(Ax, Ay, Bx, By, Sx, Sy, r);
	if (intersectionAB.first) {
		return intersectionAB.second;
	}

	auto intersectionBC = findFirstIntersectionAngle(Bx, By, Cx, Cy, Sx, Sy, r);
	if (intersectionBC.first) {
		return intersectionBC.second;
	}

	auto intersectionCA = findFirstIntersectionAngle(Cx, Cy, Ax, Ay, Sx, Sy, r);
	if (intersectionCA.first) {
		return intersectionCA.second;
	}

	return -10;
}
struct Triangle {
	double Ax, Ay, Bx, By, Cx, Cy;
};
/*
		0.0f, 550.0f, 0.0f, 1.0f,
		500.0f, 550.0f, 0.0f, 1.0f,
		0.0f, 600.0f, 0.0f, 1.0f,
		500.0f, 600.0f, 0.0f, 1.0f,

*/
Triangle triangles[12] = {
	{224.952, 29.8205, 217.452, 16.8301, 162.5, 71.6506},//hand 1
	{282.548, 16.8301, 337.5, 71.6506, 275.048, 29.8205},//hand 2
	{0.0,0.0, 80.0,0.0,80.0,600.0},//left wall
	{420.0,0.0,500.0,0.0,420.0,600.0},//wall right
	{125.0,200.0,125.0,250.0,170.0,160.0},//bumper left
	{375.0,200.0,375.0,250.0,330.0,160.0},//bumper right
	{0.0,135.0,150.0,50.0,162.5,71.6506},//pipe left
	{500.0,135.0,350.0,50.0,337.5,71.6506},//pipe right
	{0.0,550.0,500.0,550.0,0.0,600.0},//top wall1
	{500.0,550.0,0.0,600.0,500.0,600.0}, //top wall 2
	{250.0,400.0,200.0,375.0,300.0,375.0}, //middle bumper top
	{200.0,375.0,300.0,375.0,250.0,350.0}, //middle bumper bot


};
double clamp(double x, double min, double max) {
	if (x < min)
		return min;
	if (x > max)
		return max;
	return x;
}


double getNewAngle() {
	surfaceModifier1 = 1;
	//std::cout << triangles[0].Ax << "\n";
	float newAngle = -10;
	for (int i = 0; i < 12; i++) {
		newAngle = getFirstIntersectionAngle(xpos, ypos, 10.0, triangles[i].Ax, triangles[i].Ay, triangles[i].Bx, triangles[i].By, triangles[i].Cx, triangles[i].Cy);
		if (newAngle != -10) {
			switch (i) {
			case 0://LHand
				surfaceModifier1 = 0.8;
				surfaceModifier2 = 0;
				break;
			case 1://RHand
				surfaceModifier1 = 0.8;
				surfaceModifier2 = 0;
				break;
			case 2://LWall
				surfaceModifier1 = 0.9;
				surfaceModifier2 = 0.3;
				break;
			case 3://RWall
				surfaceModifier1 = 0.9;
				surfaceModifier2 = 0.3;
				break;
			case 4://LBump
				surfaceModifier1 = 4;
				surfaceModifier2 = 1.5;
				break;
			case 5://RBump
				surfaceModifier1 = 4;
				surfaceModifier2 = 1.5;
				break;
			case 6://LPipe
				surfaceModifier1 = 0.9;
				surfaceModifier2 = 0;
				break;
			case 7://RPipe
				surfaceModifier1 = 0.9;
				surfaceModifier2 = 0;
				break;
			case 8://Top wall
				surfaceModifier1 = 0.9;
				surfaceModifier2 = 0;
				break;
			case 9://Top wall
				surfaceModifier1 = 0.9;
				surfaceModifier2 = 0;
				break;
			case 10://Mid bumper top
				surfaceModifier1 = 1.5;
				surfaceModifier2 = 0;
				break;
			case 11://Mid bumper bot
				surfaceModifier1 = 1.5;
				surfaceModifier2 = 0;
				break;
			}
			return newAngle;
		}
	}
	return -10;
}

void moveBall(int value) {

	yvel = clamp(yvel - 0.02, -5.0, 5.0);
	i += xvel;
	j += yvel;
	//i = clamp(i + xvel+200,100,400);
	//j = clamp(j+yvel+500,-30,480);
	xpos = i + 200;
	ypos = j + 500;
	//xpos = clamp(i + 200, 100, 400);
	//ypos = clamp(j + 500, -30, 480);
	//std::cout << xpos << " " << ypos << "\n";
	double newAngle = getNewAngle();
	if (newAngle != -10) {
		/*std::cout << newAngle << std::endl;
		double nx = cos(newAngle);
		double ny = sin(newAngle);
		double dotProduct = xvel * nx + yvel * ny;
		//i += nx* 10;
		//j += ny* 10;
		xvel = xvel - 2 * dotProduct * nx;
		yvel = yvel - 2 * dotProduct * ny;*/


		xvel = (cos(2 * M_PI + newAngle + M_PI) * surfaceModifier1) + surfaceModifier2;
		yvel = (sin(2 * M_PI + newAngle + M_PI) * surfaceModifier1) + surfaceModifier2;
		//std::cout <<  sin(M_PI+ newAngle) << std::endl;
	}

	glutPostRedisplay();
	glutTimerFunc(5, moveBall, 0);
}


void UseMouse(int button, int state, int x, int y)
{
	//std::cout << "test";
	switch (button) {
	case GLUT_LEFT_BUTTON:			
		if (state == GLUT_DOWN) {
			angleHand1 = M_PI / 6;
			double newAngle = getFirstIntersectionAngle(xpos, ypos, 10.0, 224.952, 29.8205, 204.952, 104.821, 137.5, 71.6506);
			if (newAngle != -10) {
				//std::cout << "testH1\n";
				xvel = 3;
				yvel = 4;

			}
		}
		//CHECK IF BALL IS NEAR HAND AND DO PHYSICS
		else
			angleHand1 = -M_PI / 6;
		glutPostRedisplay();
		break;
	case GLUT_RIGHT_BUTTON:			
		if (state == GLUT_DOWN) {
			angleHand2 = -M_PI / 6;
			double newAngle = getFirstIntersectionAngle(xpos, ypos, 10.0, 275.048, 29.8205, 362.5, 71.6506, 295.048, 104.821);
			if (newAngle != -10) {
				//std::cout << "testH2\n";
				xvel = -3;
				yvel = 4;
			}
		}
		else
			angleHand2 = M_PI / 6;
		glutPostRedisplay();
		break;
	default:
		break;
	}
}

void ProcessNormalKeys(unsigned char key, int x, int y)
{
	switch (key) {
	case 32:
		i = (rand() % 300) - 100;
		//std::cout << i;
		xvel = 0;
		yvel = -1.5f;
		xpos = i;
		ypos = 500.0;
		j = 0;
		//Initialize();


	}

	if (key == 27)
		exit(0);
}


void rotatePoint(double Px, double Py, double Ox, double Oy, double angle) {
	// Translate point to the origin
	double translatedX = Px - Ox;
	double translatedY = Py - Oy;

	// Apply rotation
	double rotatedX = translatedX * cos(angle) - translatedY * sin(angle);
	double rotatedY = translatedX * sin(angle) + translatedY * cos(angle);

	// Translate back to the original origin
	double finalX = rotatedX + Ox;
	double finalY = rotatedY + Oy;
	//std::cout << finalX << " " << finalY << "\n";
	//return { finalX, finalY };
}



/*








150-225 H1   275-350 H2






225-275 gap
75 wide hands
25 tall hands




*/

void CreateShaders(void)
{
	ProgramId = LoadShaders("03_02_Shader.vert", "03_02_Shader.frag");
	glUseProgram(ProgramId);
}





void CreateVBO(void)
{
	//  Coordonatele varfurilor;
	GLfloat Vertices[] = {
		200.0f, 500.0f, 0.0f, 1.0f,


		225.0f, 55.0f, 0.0f, 1.0f,
		150.0f, 50.0f, 0.0f, 1.0f,
		225.0f, 70.0f, 0.0f, 1.0f,
		150.0f, 75.0f, 0.0f, 1.0f,

		350.0f, 50.0f, 0.0f, 1.0f,
		275.0f, 55.0f, 0.0f, 1.0f,
		350.0f, 75.0f, 0.0f, 1.0f,
		275.0f, 70.0f, 0.0f, 1.0f,


		//TESTING POINTS
		224.952, 29.8205, 0.0f, 1.0f,
		217.452, 16.8301, 0.0f, 1.0f,
		162.5, 71.6506, 0.0f, 1.0f,
		204.952, 104.821, 0.0f, 1.0f,
		212.452, 91.8301, 0.0f, 1.0f,
		137.5, 71.6506, 0.0f, 1.0f,

		282.548, 16.8301, 0.0f, 1.0f,
		337.5, 71.6506, 0.0f, 1.0f,
		275.048, 29.8205, 0.0f, 1.0f,
		287.548, 91.8301, 0.0f, 1.0f,
		362.5, 71.6506, 0.0f, 1.0f,
		295.048, 104.821, 0.0f, 1.0f,//20

		//END TESTING POINTS
		//Wall left
		0.0f, 0.0f, 0.0f, 1.0f,
		80.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 600.0f, 0.0f, 1.0f,
		80.0f, 600.0f, 0.0f, 1.0f,
		//Wall right
		420.0f, 0.0f, 0.0f, 1.0f,
		500.0f, 0.0f, 0.0f, 1.0f,
		420.0f, 600.0f, 0.0f, 1.0f,
		500.0f, 600.0f, 0.0f, 1.0f,

		//
		0.0f, 135.0f, 0.0f, 1.0f,//29
		500.0f, 135.0f, 0.0f, 1.0f,

		//Bumper triangles
		125.0f, 200.0f, 0.0f, 1.0f,
		125.0f, 250.0f, 0.0f, 1.0f,
		170.0f, 160.0f, 0.0f, 1.0f,

		375.0f, 200.0f, 0.0f, 1.0f,
		375.0f, 250.0f, 0.0f, 1.0f,
		330.0f, 160.0f, 0.0f, 1.0f,

		//Top wall

		0.0f, 550.0f, 0.0f, 1.0f,
		500.0f, 550.0f, 0.0f, 1.0f,
		0.0f, 600.0f, 0.0f, 1.0f,
		500.0f, 600.0f, 0.0f, 1.0f,

		//Mid bumper
		250.0f, 400.0f, 0.0f, 1.0f,
		200.0f, 375.0f, 0.0f, 1.0f,
		300.0f, 375.0f, 0.0f, 1.0f,
		250.0f, 350.0f, 0.0f, 1.0f,






	};
	rotatePoint(225.0, 70.0, 150.0, 50.0, -M_PI / 6);
	rotatePoint(225.0, 55.0, 150.0, 50.0, -M_PI / 6);
	rotatePoint(150.0, 75.0, 150.0, 50.0, -M_PI / 6);

	rotatePoint(225.0, 70.0, 150.0, 50.0, M_PI / 6);
	rotatePoint(225.0, 55.0, 150.0, 50.0, M_PI / 6);
	rotatePoint(150.0, 75.0, 150.0, 50.0, M_PI / 6);


	rotatePoint(275.0, 55.0, 350, 50.0, M_PI / 6);
	rotatePoint(350.0, 75.0, 350, 50.0, M_PI / 6);
	rotatePoint(275.0, 70.0, 350, 50.0, M_PI / 6);

	rotatePoint(275.0, 55.0, 350, 50.0, -M_PI / 6);
	rotatePoint(350.0, 75.0, 350, 50.0, -M_PI / 6);
	rotatePoint(275.0, 70.0, 350, 50.0, -M_PI / 6);




	//  Culorile axelor;
	GLfloat Colors[] = {
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
	};

	static const GLuint Indices[] =
	{
		0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,
		//Pipe leading to hands
		29,2,11,
		30,5,16,
		//Bumbers
		31,32,33,
		34,35,36,
		//Top
		37,38,39,40, //41 start
		//Top bumper
		41,42,43,44

	};




	glGenVertexArrays(1, &VaoId);
	glBindVertexArray(VaoId);
	glGenBuffers(1, &VboId);
	glBindBuffer(GL_ARRAY_BUFFER, VboId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glGenBuffers(1, &ColorBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, ColorBufferId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Colors), Colors, GL_STATIC_DRAW); \
		glGenBuffers(1, &EboId);														//  Generarea bufferului si indexarea acestuia catre variabila EboId;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);












}

void DestroyShaders(void)
{
	glDeleteProgram(ProgramId);
}

void DestroyVBO(void)
{
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &ColorBufferId);
	glDeleteBuffers(1, &VboId);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &VaoId);
}

void Cleanup(void)
{
	DestroyShaders();
	DestroyVBO();
}

void Initialize(void)
{
	srand(time(NULL));
	xposInit = rand() % 300 + 100;
	//std::cout << xposInit;
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	CreateVBO();
	CreateShaders();
	codColLocation = glGetUniformLocation(ProgramId, "codCol");
	myMatrixLocation = glGetUniformLocation(ProgramId, "myMatrix");
}








void RenderFunction(void)
{
	glClear(GL_COLOR_BUFFER_BIT);

	resizeMatrix = glm::ortho(xMin, xMax, yMin, yMax);
	matrTransl = glm::translate(glm::mat4(1.0f), glm::vec3(i, j, 0.0));
	//matrDepl = glm::translate(glm::mat4(1.0f), glm::vec3(0, 80.0, 0.0));		
	matrScale1 = glm::scale(glm::mat4(1.0f), glm::vec3(1.0, 1.0, 0.0));
	matrScale2 = glm::scale(glm::mat4(1.0f), glm::vec3(0.25, 0.25, 0.0));

	matrTranslInit2Hand1 = glm::translate(glm::mat4(1.0f), glm::vec3(-150.0, -50.0, 0.0));
	matrTranslInit1Hand1 = glm::translate(glm::mat4(1.0f), glm::vec3(150.0, 50.0, 0.0));
	matrTranslInit2Hand2 = glm::translate(glm::mat4(1.0f), glm::vec3(-350.0, -50.0, 0.0));
	matrTranslInit1Hand2 = glm::translate(glm::mat4(1.0f), glm::vec3(350.0, 50.0, 0.0));
	matrTranslBall = glm::translate(glm::mat4(1.0f), glm::vec3(i, j, 0.0));

	//	Matricea de redimensionare (pentru elementele "fixe" - axele);
	myMatrix = resizeMatrix;
	glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, &myMatrix[0][0]);

	//Caring about 4 and 24

	codCol = 5;
	glUniform1i(codColLocation, codCol);
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(116));
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(128));


	codCol = 4;
	glUniform1i(codColLocation, codCol);
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(84));
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(88));

	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(100));
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(104));
	//glPointSize(50);
	//glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, (void*)(0));


	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(41 * 4));
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(42 * 4));




	codCol = 5;
	glUniform1i(codColLocation, codCol);

	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(140));
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(152));
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(45 * 4));
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(46 * 4));


	myMatrix = resizeMatrix;

	codCol = 1;
	glUniform1i(codColLocation, codCol);
	glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, &myMatrix[0][0]);
	//DRAW HANDS
	matrRot = glm::rotate(glm::mat4(1.0f), angleHand1, glm::vec3(0.0, 0.0, 1.0));	//	TODO STEF use 2 rot mat
	myMatrix = resizeMatrix * matrTranslInit1Hand1 * matrScale1 * matrRot * matrTranslInit2Hand1;
	glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, &myMatrix[0][0]);
	glUniform1i(codColLocation, codCol);
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(4));
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(8));


	matrRot = glm::rotate(glm::mat4(1.0f), angleHand2, glm::vec3(0.0, 0.0, 1.0));	//	TODO STEF use 2 rot mat
	myMatrix = resizeMatrix * matrTranslInit1Hand2 * matrScale1 * matrRot * matrTranslInit2Hand2;
	glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, &myMatrix[0][0]);
	glUniform1i(codColLocation, codCol);
	//glDrawArrays(GL_POLYGON, 8, 4);//RIGHT HAND
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(20));
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(24));


	myMatrix = resizeMatrix;
	glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, &myMatrix[0][0]);
	glUniform1i(codColLocation, codCol);


	///DEBUG
	glPointSize(5);
	codCol = 2;
	glUniform1i(codColLocation, codCol);
	//glDrawElements(GL_POINTS, 3, GL_UNSIGNED_INT, (void*)(36));

	codCol = 3;
	glUniform1i(codColLocation, codCol);
	//glDrawElements(GL_POINTS, 3, GL_UNSIGNED_INT, (void*)(48));

	codCol = 2;
	glUniform1i(codColLocation, codCol);
	//glDrawElements(GL_POINTS, 3, GL_UNSIGNED_INT, (void*)(60));

	codCol = 3;
	glUniform1i(codColLocation, codCol);
	//glDrawElements(GL_POINTS, 3, GL_UNSIGNED_INT, (void*)(72));


	///END DEBUG





	myMatrix = resizeMatrix * matrTranslBall;
	glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, &myMatrix[0][0]);
	glUniform1i(codColLocation, codCol);
	glPointSize(20);
	glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, (void*)(0));




	glutSwapBuffers();
	glFlush();
}

int main(int argc, char* argv[])
{

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowPosition(300, 100);
	glutInitWindowSize(winWidth, winHeight);
	glutCreateWindow("Pinball");
	glewInit();

	Initialize();
	glutDisplayFunc(RenderFunction);
	glutMouseFunc(UseMouse);
	glutKeyboardFunc(ProcessNormalKeys);
	//glutKeyboardUpFunc(ProcessNormalUpKeys);
	glutTimerFunc(5, moveBall, 0);
	glutCloseFunc(Cleanup);

	glutMainLoop();

	return 0;
}