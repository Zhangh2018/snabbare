// Lab 1-1.
// This is the same as the first simple example in the course book,
// but with a few error checks.
// Remember to copy your file to a new on appropriate places during the lab so you keep old results.
// Note that the files "lab2-1.frag", "lab2-1.vert" are required.

// Should work as is on Linux and Mac. MS Windows needs GLEW or glee.
// See separate Visual Studio version of my demos.
#ifdef __APPLE__
	#include <OpenGL/gl3.h>
	// Linking hint for Lightweight IDE
	// uses framework Cocoa
#endif
#include "MicroGlut.h"
#include "GL_utilities.h"
#include <math.h>
#include "loadobj.h"
#include "LoadTGA.h"
#include "VectorUtils3.h"

// Globals
#define PI 3.1415
#define WORLD_SIZE 100.0
#define SKYBOX_SIZE 100
#define NIGHT_MODE false
#define TERRAIN_REPEAT 100.0

#define NEAR 1.0
#define FAR 5000.0
#define RIGHT 0.5
#define LEFT -0.5
#define TOP 0.5
#define BOTTOM -0.5

typedef struct {
	Model *model;
	unsigned int arrayNormalArrow;
	unsigned int bufferNormalArrow;
} Terrain;

typedef struct {
	Model *model;
	GLuint texture;
} Skybox;

typedef struct
{
	vec3 center;
	double radius;
} Sphere;

typedef struct
{
	vec3 forward;
	vec3 up;
	vec3 pos;
} Camera;

GLuint program;
Skybox *skybox;
Terrain *terrain;

Model *windmillWallsModel;
Model *windmillBalconyModel;
Model *windmillRoofModel;
Model *bladeModel;
Model *bunnyModel;

Sphere *bunnyCollisionSphere, *windmillCollisionSphere;
Model *collisionSphereModel;

// camera setup init
vec3 forward = {10,-5,10};
vec3 up = {0,1,0};
vec3 cam = {20, 20, 0};
int oldMouseX = 0, oldMouseY = 0;

bool showNormals;

vec3 bunnyPos = {20, 0, 0};
vec3 bunnyUp = {0, 1, 0};

GLfloat projectionMatrix[] =
{
	2.0f*NEAR/(RIGHT-LEFT), 0.0f, (RIGHT+LEFT)/(RIGHT-LEFT), 0.0f,
	0.0f, 2.0f*NEAR/(TOP-BOTTOM), (TOP+BOTTOM)/(TOP-BOTTOM), 0.0f,
	0.0f, 0.0f, -(FAR + NEAR)/(FAR - NEAR), -2*FAR*NEAR/(FAR - NEAR),
	0.0f, 0.0f, -1.0f, 0.0f
};

GLuint grassTexture;

Terrain* GenerateTerrain(TextureData *tex)
{
	int vertexCount = tex->width * tex->height;
	int triangleCount = (tex->width-1) * (tex->height-1) * 2;
	int x, z;

	GLfloat *vertexArray = malloc(sizeof(GLfloat) * 3 * vertexCount);
	GLfloat *normalArray = malloc(sizeof(GLfloat) * 3 * vertexCount);
	GLfloat *texCoordArray = malloc(sizeof(GLfloat) * 2 * vertexCount);
	GLfloat *normalArrowVertices = malloc(sizeof(GLfloat) * 3 * 2 * vertexCount);
	GLuint *indexArray = malloc(sizeof(GLuint) * triangleCount*3);

	printf("bpp %d\n", tex->bpp);
	printf("width ; height => %d %d\n", tex->width, tex->height);

	for (x = 0; x < tex->width; x++)
	{
		for (z = 0; z < tex->height; z++)
		{
			// Vertex array. You need to scale this properly
			vertexArray[(x + z * tex->width)*3 + 0] = x / 1.0;
			vertexArray[(x + z * tex->width)*3 + 1] = tex->imageData[(x + z * tex->width) * (tex->bpp/8)] / (5.0*2.0);
			vertexArray[(x + z * tex->width)*3 + 2] = z / 1.0;
		}
	}

	for (x = 0; x < tex->width-1; x++)
	{
		for (z = 0; z < tex->height-1; z++)
		{
			// Triangle 1
			indexArray[(x + z * (tex->width-1))*6 + 0] = x + z * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 1] = x + (z+1) * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 2] = x+1 + z * tex->width;

			// Triangle 2
			indexArray[(x + z * (tex->width-1))*6 + 3] = x+1 + z * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 4] = x + (z+1) * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 5] = x+1 + (z+1) * tex->width;
		}
	}

	for (x = 0; x < tex->width; x++)
	{
		for (z = 0; z < tex->height; z++)
		{
			vec3 temp;

			// Normal vectors. You need to calculate these.
			if (x > 1 && z > 1 && x + 1 < tex->width && z + 1 < tex->height)
			{
				temp.x = -(vertexArray[(x + 1 + z * tex->width )*3 + 1] - vertexArray[(x - 1 + z * tex->width)*3 + 1]);
				temp.y = 2.0;
				temp.z = -(vertexArray[(x + (z + 1) * tex->width )*3 + 1] - vertexArray[(x + (z-1) * tex->width)*3 + 1]);
		  }
			else
			{
				temp.x = 0.0;
				temp.y = 1.0;
				temp.z = 0.0;
			}

			temp = Normalize(temp);
			normalArray[(x + z * tex->width)*3 + 0] = temp.x;
			normalArray[(x + z * tex->width)*3 + 1] = temp.y;
			normalArray[(x + z * tex->width)*3 + 2] = temp.z;

			// 2 points per normal-arrow
			normalArrowVertices[(x + z * tex->width)*6 + 0] = vertexArray[(x + z * tex->width)*3 + 0];
			normalArrowVertices[(x + z * tex->width)*6 + 1] = vertexArray[(x + z * tex->width)*3 + 1];
			normalArrowVertices[(x + z * tex->width)*6 + 2] = vertexArray[(x + z * tex->width)*3 + 2];
			normalArrowVertices[(x + z * tex->width)*6 + 3] = vertexArray[(x + z * tex->width)*3 + 0] + 2 * temp.x;
			normalArrowVertices[(x + z * tex->width)*6 + 4] = vertexArray[(x + z * tex->width)*3 + 1] + 2 * temp.y;
			normalArrowVertices[(x + z * tex->width)*6 + 5] = vertexArray[(x + z * tex->width)*3 + 2] + 2 * temp.z;

			// Texture coordinates. You may want to scale them.
			texCoordArray[(x + z * tex->width)*2 + 0] = x; // (float)x / tex->width;
			texCoordArray[(x + z * tex->width)*2 + 1] = z; // (float)z / tex->height;
		}
	}

	Terrain* terrain = malloc(sizeof(Terrain));

	// Create model
	terrain->model = LoadDataToModel(
			vertexArray,
			normalArray,
			texCoordArray,
			NULL,
			indexArray,
			vertexCount,
			triangleCount*3
	);

	// Save normals
	glGenVertexArrays(1, &terrain->arrayNormalArrow);
	glBindVertexArray(terrain->arrayNormalArrow);
	glGenBuffers(1, &terrain->bufferNormalArrow);

	glBindBuffer(GL_ARRAY_BUFFER, terrain->bufferNormalArrow);
	glBufferData(GL_ARRAY_BUFFER, vertexCount*2*3*sizeof(GLfloat), normalArrowVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(program, "in_Position"), 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(glGetAttribLocation(program, "in_Position"));


	return terrain;
}

void DrawNormals(Terrain* terrain)
{
	glBindVertexArray(terrain->arrayNormalArrow);
	glDrawArrays(GL_LINES, 0, 2*terrain->model->numVertices);
}

void DrawTerrain(Terrain* terrain, mat4 modelToWorld, mat4 worldToView)
{
	glUniformMatrix4fv(glGetUniformLocation(program, "modelToWorld"), 1, GL_TRUE, modelToWorld.m);
	glUniformMatrix4fv(glGetUniformLocation(program, "worldToView"), 1, GL_TRUE, worldToView.m);
	glUniform1ui(glGetUniformLocation(program, "enableLight"), true);
	DrawModel(terrain->model, program, "in_Position", "in_Normal", "in_TexCoord");
}

Sphere* CollisionSphere(Model* model)
{
	Sphere* collisionSphere = malloc(sizeof(Sphere));

	vec3 sum = {0,0,0};
	float farthest;
	for (int i = 0; i < model->numVertices; i++)
	{
		sum.x += model->vertexArray[i * 3 + 0];
		sum.y += model->vertexArray[i * 3 + 1];
		sum.z += model->vertexArray[i * 3 + 2];
	}
	collisionSphere->center = ScalarMult(sum, 1.0 / (float) model->numVertices);

	for (int i = 0; i < model->numVertices; i++)
	{
		vec3 current = {
			model->vertexArray[i * 3 + 0],
			model->vertexArray[i * 3 + 1],
			model->vertexArray[i * 3 + 2],
		};
		float distance = Norm(VectorSub(collisionSphere->center, current));

		if (distance > farthest) farthest = distance;
	}
	collisionSphere->radius = farthest;

	// printf("%f %f %f\n", collisionSphere->center.x, collisionSphere->center.y, collisionSphere->center.z);
	// printf("%f\n", collisionSphere->radius);

	return collisionSphere;
}

mat4 RotateTowards(vec3 src, vec3 dest)
{
	vec3 v = CrossProduct(src, dest);
	float cos = DotProduct(Normalize(src), Normalize(dest));
	return ArbRotate(v, acos(cos));
}

Skybox* CreateSkybox(const char* modelFileName, char *textureFileName)
{
	Skybox* skybox = malloc(sizeof(Skybox));

	skybox->model = LoadModelPlus(modelFileName);
	LoadTGATextureSimple(textureFileName, &skybox->texture);

	return skybox;
}

void LoadMatrixToUniform(const char* variableName, mat4 matrix)
{
	glUniformMatrix4fv(glGetUniformLocation(program, variableName), 1, GL_TRUE, matrix.m);
}

void DrawSkybox(Skybox* skybox, mat4 worldToView)
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, skybox->texture);
	glUniform1ui(glGetUniformLocation(program, "enableLight"), false);

	mat4 modelToWorldSkyBox = Mult(T(0,-10,0), S(SKYBOX_SIZE, SKYBOX_SIZE, SKYBOX_SIZE));
	mat4 worldToViewSkyBox = worldToView;

	worldToViewSkyBox.m[3] = 0.0;
	worldToViewSkyBox.m[7] = 0.0;
	worldToViewSkyBox.m[11] = 0.0;
	LoadMatrixToUniform("modelToWorld", modelToWorldSkyBox);
	LoadMatrixToUniform("worldToView", worldToViewSkyBox);

	DrawModel(skybox->model, program, "in_Position", "in_Normal", "in_TexCoord");
	glUniform1ui(glGetUniformLocation(program, "enableLight"), true);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

void init(void)
{
	dumpInfo();
	forward = Normalize(forward);
	showNormals = false;

	// GL inits
	glClearColor(0.15,0.42,0.70,0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glutHideCursor();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable( GL_BLEND );

	// Lightning
	vec3 lightSourcesDirectionsPositions[] =
	{
		{0.0f, 5.0f, 15.0f}, 		// positional
		{0.0f, 5.0f, 10.0f}, 		// positional
		{-5.0f, 0.0f, 0.0f},	 	// along X (towards +X)
		{0.0f, 0.0f, -30.0f}, 	// along Z (towards +Z)
	};

  vec3 lightSourcesColorsArr[] =
	{
		{1.0f, 1.0f, 1.0f}, 		// White light
		{1.0f, 1.0f, 1.0f}, 		// White light
		{1.0f, 1.0f, 1.0f}, 		// White light
		{1.0f, 1.0f, 1.0f}, 		// White light
	};

  GLint isDirectional[] = { 0,0,1,1 };
	GLfloat specularExponent[] = {100.0, 200.0, 60.0, 50.0, 300.0, 150.0}; // should be 1 per object


	// Load and compile shader
	program = loadShaders("lab4-6.vert", "lab4-6.frag");

	// ------------------- textures loading
	LoadTGATextureSimple("maskros512.tga", &grassTexture);
	// LoadTGATextureSimple("SkyBox512.tga", &skyboxTexture);
	glUniform1i(glGetUniformLocation(program, "texUnit"), 0); 		// Texture unit 0
	glActiveTexture(GL_TEXTURE0);

	// ------------------- Load skybox
	skybox = CreateSkybox("skybox.obj", "SkyBox512.tga");
	// skyboxModel = LoadModelPlus("skybox.obj");

	// ------------------- Load models
	windmillWallsModel = LoadModelPlus("windmill-walls.obj");
	windmillBalconyModel = LoadModelPlus("windmill-balcony.obj");
	windmillRoofModel = LoadModelPlus("windmill-roof.obj");
	bladeModel = LoadModelPlus("blade.obj");
	bunnyModel = LoadModelPlus("bunnyplus.obj");
	collisionSphereModel = LoadModelPlus("groundsphere2.obj");

	bunnyCollisionSphere = CollisionSphere(bunnyModel);

	bunnyCollisionSphere->center = bunnyPos;
	windmillCollisionSphere = CollisionSphere(windmillWallsModel);

	// ------------------- Global variable
	// Projection
	glUniformMatrix4fv(glGetUniformLocation(program, "projectionMatrix"), 1, GL_TRUE, projectionMatrix);
	// Lighting
	glUniform3fv(glGetUniformLocation(program, "lightSourcesDirPosArr"), 4, &lightSourcesDirectionsPositions[0].x);
	glUniform3fv(glGetUniformLocation(program, "lightSourcesColorArr"), 4, &lightSourcesColorsArr[0].x);
	glUniform1f(glGetUniformLocation(program, "specularExponent"), specularExponent[0]);
	glUniform1iv(glGetUniformLocation(program, "isDirectional"), 4, isDirectional);

	// ------------------- Load terrain data
	TextureData terrainTextureMap;	 // terrain
	LoadTGATextureData("fft-terrain.tga", &terrainTextureMap);
	terrain = GenerateTerrain(&terrainTextureMap);

}

void OnTimer(int value)
{
    glutPostRedisplay();
    glutTimerFunc(20, &OnTimer, value);
}

void mouseHandler(int x, int y)
{
	int dx = oldMouseX - x;
	int dy = y - oldMouseY;

	vec3 left = Normalize(CrossProduct(up, forward));
	float step = 0.002;

	// calculate camera rotation
  mat4 camRotationUp = Ry(dx * step);
	mat4 camRotationLeft = ArbRotate(left, dy * step);
	mat4 camRotation = Mult(camRotationUp, camRotationLeft);

	// update forward and up vectors
	forward = MultVec3(camRotation, forward);
	up = MultVec3(camRotation, up);

	// update (x,y) buffer
	oldMouseX = glutGet(GLUT_WINDOW_WIDTH) / 2;
	oldMouseY = glutGet(GLUT_WINDOW_HEIGHT) / 2;
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
}

bool areColliding(Sphere* a, Sphere* b)
{
	return Norm(VectorSub(a->center, b->center)) < a->radius + b->radius;
}

void keyHandler(unsigned char key, int x, int y)
{
	float step = 0.4;
	vec3 left = Normalize(CrossProduct(up ,forward)); // basis (up, forward, left)
	float objSpeed = 0.3;

	vec3 bunnyMovement = {0,0,0};

	// printf("%d\n", key);
	switch(key) {
		case 'w':
			cam = VectorAdd(cam, ScalarMult(forward, step));
			break;
		case 's':
			cam = VectorSub(cam, ScalarMult(forward, step));
			break;
		case 'a':
			cam = VectorAdd(cam, ScalarMult(left, step));
			break;
		case 'd':
			cam = VectorSub(cam, ScalarMult(left, step));
			break;

		case 'z':
			cam.y -= step;
			break;
		case 32: // SPACEBAR
			cam.y += step;
			break;

		// lapinous control
		case 'i':
			bunnyMovement.x = objSpeed;
			break;
		case 'k':
			bunnyMovement.x = -objSpeed;
			break;
		case 'j':
			bunnyMovement.z = objSpeed;
			break;
		case 'l':
			bunnyMovement.z = -objSpeed;
			break;

		// terrain nromals
		case 'n':
			showNormals = !showNormals;
			break;
	}

	Sphere newBunnySphere;
	newBunnySphere.radius = bunnyCollisionSphere->radius;
	newBunnySphere.center = VectorAdd(bunnyCollisionSphere->center, bunnyMovement);
	if (!areColliding(&newBunnySphere, windmillCollisionSphere)) {
		bunnyPos = VectorAdd(bunnyPos, bunnyMovement);
		*(bunnyCollisionSphere) = newBunnySphere;
	}
}

void display(void)
{
	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniform1ui(glGetUniformLocation(program, "enableTransparency"), false);

	// night node activated
	glUniform1ui(glGetUniformLocation(program, "NIGHT_MODE"), NIGHT_MODE);

	// update modelToView matrix
	GLfloat t = (GLfloat)glutGet(GLUT_ELAPSED_TIME);
	float a = t / 1000;
	glUniform1f(glGetUniformLocation(program, "time"), a);

	mat4 modelToWorld = Mult(T(0,0,0), Ry(0));
	mat4 worldToView = lookAtv(cam, VectorAdd(cam, forward), up);

	// -------- Draw skybox
	DrawSkybox(skybox, worldToView);

	// -------- Draw terrain
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, grassTexture);

	DrawTerrain(terrain, modelToWorld, worldToView);

	if(showNormals)
		DrawNormals(terrain);

	// -------- Draw windmill
	glUniformMatrix4fv(glGetUniformLocation(program, "modelToWorld"), 1, GL_TRUE, modelToWorld.m);
	glUniformMatrix4fv(glGetUniformLocation(program, "worldToView"), 1, GL_TRUE, worldToView.m);

	DrawModel(windmillWallsModel, program, "in_Position", "in_Normal", "in_TexCoord");
	DrawModel(windmillBalconyModel, program, "in_Position", "in_Normal", "in_TexCoord");
	DrawModel(windmillRoofModel, program, "in_Position", "in_Normal", "in_TexCoord");

	// draw the 4 blades
	mat4 bladePos = T(4.5,8.9,0.05);
	modelToWorld = Mult(bladePos, Rx(a));
	glUniformMatrix4fv(glGetUniformLocation(program, "modelToWorld"), 1, GL_TRUE, modelToWorld.m);
	DrawModel(bladeModel, program, "in_Position", "in_Normal", "in_TexCoord");

	modelToWorld = Mult(bladePos, Rx(PI/2+a));
	glUniformMatrix4fv(glGetUniformLocation(program, "modelToWorld"), 1, GL_TRUE, modelToWorld.m);
	DrawModel(bladeModel, program, "in_Position", "in_Normal", "in_TexCoord");

	modelToWorld = Mult(bladePos, Rx(PI+a));
	glUniformMatrix4fv(glGetUniformLocation(program, "modelToWorld"), 1, GL_TRUE, modelToWorld.m);
	DrawModel(bladeModel, program, "in_Position", "in_Normal", "in_TexCoord");

	modelToWorld = Mult(bladePos, Rx(3*PI/2+a));
	glUniformMatrix4fv(glGetUniformLocation(program, "modelToWorld"), 1, GL_TRUE, modelToWorld.m);
	DrawModel(bladeModel, program, "in_Position", "in_Normal", "in_TexCoord");

	// collision sphere
	modelToWorld = Mult(
		T(windmillCollisionSphere->center.x, windmillCollisionSphere->center.y, windmillCollisionSphere->center.z),
		S(windmillCollisionSphere->radius, windmillCollisionSphere->radius, windmillCollisionSphere->radius)
	);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelToWorld"), 1, GL_TRUE, modelToWorld.m);
	glUniform1ui(glGetUniformLocation(program, "enableTransparency"), true);
	glCullFace(GL_FRONT);
	DrawModel(collisionSphereModel, program, "in_Position", "in_Normal", "in_TexCoord");
	glCullFace(GL_BACK);
	DrawModel(collisionSphereModel, program, "in_Position", "in_Normal", "in_TexCoord");
	glUniform1ui(glGetUniformLocation(program, "enableTransparency"), false);

	// -------- Draw bunny
	modelToWorld = T(bunnyPos.x, bunnyPos.y, bunnyPos.z);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelToWorld"), 1, GL_TRUE, modelToWorld.m);
	DrawModel(bunnyModel, program, "in_Position", "in_Normal", "in_TexCoord");

	// collision sphere
	glUniform1ui(glGetUniformLocation(program, "enableTransparency"), true);
	glCullFace(GL_FRONT);
	DrawModel(collisionSphereModel, program, "in_Position", "in_Normal", "in_TexCoord");
	glCullFace(GL_BACK);
	DrawModel(collisionSphereModel, program, "in_Position", "in_Normal", "in_TexCoord");
	glUniform1ui(glGetUniformLocation(program, "enableTransparency"), false);

	glutSwapBuffers();
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitContextVersion(3, 2);
	glutInitWindowSize(1200, 1200);
	glutCreateWindow("World collisions");
	glutDisplayFunc(display);
	init();

	glutTimerFunc(20, &OnTimer, 0);
	glutPassiveMotionFunc(mouseHandler);
	glutKeyboardFunc(keyHandler);
	glutMainLoop();
	exit(0);
}