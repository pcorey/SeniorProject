/*
 * GPU Fractal!
 *
 * Written by Pete Corey
 * Senior project for Cal Poly Computer Science
 * December 2010
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include <GL/glew.h>
#include <GL/glut.h>

#include <GL/glui.h>

#define ILUT_USE_OPENGL

#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>

typedef struct {
	float x,y;
}vec2;

float GW,GH,zoomDelta=0.05;
float xDelta=0.01,yDelta=0.01;

GLuint zoomFactorLocation=0;
GLuint zoomTargetLocation=0;
GLuint colorTexLocation=0;
GLuint colorTexHeightLocation=0;
GLuint cInitLocation=0;

float zoomFactor;
vec2 zoomTarget;
vec2 cInit;
bool initComplete=false;

GLuint program;
GLuint mandelbrot;
GLuint julia;
GLuint mandelbrotAvgConst;
GLuint juliaAvgConst;
GLuint mandelbrotConst;
GLuint juliaConst;
GLuint vert;

bool useIterColoring=true;

float zoomSpeed=50.0;

ILuint colorTex;
GLuint glColorTex;

int oldScreenX=-1;
int oldScreenY=-1;

bool useAvgAlgo=false;

GLUI_Listbox *colorTexBox;
GLUI_Listbox *colorAlgo;
GLUI_String zTargetXText="";
GLUI_String zTargetYText="";
GLUI_String cRText="";
GLUI_String cIText="";
GLUI *glui;
GLUI_EditText *zTarX;
GLUI_EditText *zTarY;
GLUI_EditText *cR;
GLUI_EditText *cI;

bool first=true;

void loadColorTex(const char *fileName) {
	colorTex=0;
	if (first) {
		ilGenImages(1,&colorTex);
		first=false;
	}
	ilBindImage(colorTex);
	ilLoadImage(fileName);
	glColorTex=ilutGLBindTexImage();
	glBindTexture(GL_TEXTURE_2D,glColorTex);
}

vec2 setZoomTarget(float x,float y) {
	zoomTarget.x=x;
	zoomTarget.y=y;
	glUniform2f(zoomTargetLocation,x,y);
	char *xTmp=(char*)malloc(100);
	sprintf(xTmp,"%f",zoomTarget.x);
	zTargetXText.assign(xTmp);
	char *yTmp=(char*)malloc(100);
	sprintf(yTmp,"%f",zoomTarget.y);
	zTargetYText.assign(yTmp);
	if (initComplete)
		glui->sync_live();
	return zoomTarget;
}

vec2 setCInit(float x,float y) {
	cInit.x=x;
	cInit.y=y;
	glUniform2f(cInitLocation,x,y);

	char *xTmp=(char*)malloc(100);
	sprintf(xTmp,"%f",cInit.x);
	cRText.assign(xTmp);
	char *yTmp=(char*)malloc(100);
	sprintf(yTmp,"%f",cInit.y);
	cIText.assign(yTmp);
	if (initComplete)
	glui->sync_live();
	return zoomTarget;
}

float setZoomFactor(float val) {
	zoomFactor=val;
	zoomDelta=(zoomFactor)/zoomSpeed;
	if (zoomDelta<=0.0)
		zoomDelta=1.0/zoomSpeed;
	glUniform1f(zoomFactorLocation,zoomFactor);
	return zoomFactor;
}

void useColor(const char *name) {
	char *path=(char*)malloc(1000);
	sprintf(path,"colors/%s\0",name);
	loadColorTex(path);
	glUniform1i(colorTexLocation,glColorTex);
}

int init() {
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		printf("Error %s\n", glewGetErrorString(err));
		exit(1);
	}
	printf("Using GLEW version %s.\n", glewGetString(GLEW_VERSION));

	if (glewIsSupported("GL_VERSION_2_0"))
		printf("OpenGL 2.0 supported.\n");
	else {
		printf("OpenGL 2.0 not supported.\n");
		return 0;
	}

	setZoomTarget(0,0);
	setZoomFactor(1.0);

	ilInit();
	iluInit();
	ilutInit();

	ilutRenderer(ILUT_OPENGL);

	useColor("greenBlack.png");
	return 1;
}

static char* readShaderSource(const char* shaderFile) {
	struct stat statBuf;
	char* buf;

	FILE* fp=fopen(shaderFile,"r");

	stat(shaderFile,&statBuf);
	buf=(char*)malloc((statBuf.st_size+1)*sizeof(char));
	fread(buf,1,statBuf.st_size,fp);
	buf[statBuf.st_size]='\0';
	fclose(fp);
	return buf;
}

GLuint initShader(char *path,GLuint type) {
	const GLchar *source=(const GLchar*)readShaderSource(path);

	GLuint shader=glCreateShader(type);
	glShaderSource(shader,1,&source,NULL);
	glCompileShader(shader);
	int status=0;
	glGetShaderiv(shader,GL_COMPILE_STATUS,&status);
	if (status!=GL_TRUE) {
		printf("Unable to compile shader %s.\n",path);
		char log[2048];
		glGetShaderInfoLog(shader,2048,&status,log);
		printf("--- Shader Info Log ---\n%s\n-----------------------\n\n",log);
	}
	return shader;
}

void findShaderVariableLocations() {
	zoomFactorLocation=glGetUniformLocation(program,"zoomFactor");
	zoomTargetLocation=glGetUniformLocation(program,"zoomTarget");
	colorTexLocation=glGetUniformLocation(program,"colorTex");
	cInitLocation=glGetUniformLocation(program,"cInit");
}

int initShaders() {
	mandelbrot=initShader((char*)"shaders/mandelbrot.frag",GL_FRAGMENT_SHADER);
	julia=initShader((char*)"shaders/julia.frag",GL_FRAGMENT_SHADER);

	mandelbrotAvgConst=initShader((char*)"shaders/mandelbrotAvg.frag",GL_FRAGMENT_SHADER);
	juliaAvgConst=initShader((char*)"shaders/juliaAvg.frag",GL_FRAGMENT_SHADER);
	mandelbrotConst=initShader((char*)"shaders/mandelbrot.frag",GL_FRAGMENT_SHADER);
	juliaConst=initShader((char*)"shaders/julia.frag",GL_FRAGMENT_SHADER);

	vert=initShader((char*)"shaders/default.vert",GL_VERTEX_SHADER);
	program=glCreateProgram();
	glAttachShader(program,vert);
	glAttachShader(program,mandelbrot);
	glLinkProgram(program);
	glUseProgram(program);

	findShaderVariableLocations();
	return 1;
}

void renderScene(void) {
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBegin(GL_QUADS);
	glVertex3f(-4,2,-1);
	glVertex3f(2,2,-1);
	glVertex3f(2,-2,-1);
	glVertex3f(-4,-2,-1);
	glEnd();

	glUseProgram(0);
	glColor3f(1,0,0);
	glBegin(GL_POINTS);
	glVertex3f(0,0,0);
	glEnd();

	glUseProgram(program);

	glFlush();
}

void keyboard(unsigned char key, int x, int y )
{
	switch( key ) {
	case 'q': case 'Q' : {
		glDetachShader(program,mandelbrot);
		glDeleteShader(mandelbrot);
		glDetachShader(program,vert);
		glDeleteShader(vert);
		glDeleteProgram(program);
		exit( EXIT_SUCCESS );
	} break;
	case 'e': {
		setZoomFactor(zoomFactor+zoomDelta);
	} break;
	case 'f': {
		setZoomFactor(zoomFactor-zoomDelta);
	} break;
	case 'p': printf("%f\n",zoomFactor); break;
	case 'w': setZoomTarget(zoomTarget.x,zoomTarget.y+yDelta*zoomFactor); break;
	case 's': setZoomTarget(zoomTarget.x,zoomTarget.y-yDelta*zoomFactor); break;
	case 'd': setZoomTarget(zoomTarget.x+xDelta*zoomFactor,zoomTarget.y); break;
	case 'a': setZoomTarget(zoomTarget.x-xDelta*zoomFactor,zoomTarget.y); break;
	case 9: {
		int setCInitFlag=false;
		useIterColoring=!useIterColoring;
		glUseProgram(0);
		if (useIterColoring) {
			glDetachShader(program,julia);
			glAttachShader(program,mandelbrot);

		} else {
			glDetachShader(program,mandelbrot);
			glAttachShader(program,julia);
			setCInitFlag=true;
		}
		glLinkProgram(program);
		glUseProgram(program);

		findShaderVariableLocations();

		setZoomTarget(zoomTarget.x,zoomTarget.y);
		setZoomFactor(zoomFactor);
		//glUniform1i(colorTexLocation,glColorTex);

		if (setCInitFlag) {
			setCInit(zoomTarget.x,zoomTarget.y);
		}
	}
	}
	glutPostRedisplay();
}

void idle()
{
	//glutPostRedisplay();
}

void reshape(int w, int h) {
	GW=w; GH=h;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0*(float)w/h, 1.0*(float)w/h, -1, 1, -100.0, 100.0);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, w, h);
	glutPostRedisplay();
}

void typeControlCallback(int ID) {
	printf("typeControlCallback\n");
}

void colorAlgoCallback(int ID) {
	printf("colorAlgoCallback\n");
	int id=colorAlgo->get_int_val();

	if (useIterColoring)
		glDetachShader(program,mandelbrot);
	else
		glDetachShader(program,julia);

	if (id==0) {
		printf("using normal");
		mandelbrot=mandelbrotConst;
		julia=juliaConst;
	} else if (id==1) {
		printf("using julia");
		mandelbrot=mandelbrotAvgConst;
		julia=juliaAvgConst;
	} else {
		//printf("wat %d",id);
	}
	glUseProgram(0);
	if (useIterColoring) {
		printf("attaching mendelbrot");
		glAttachShader(program,mandelbrot);
	} else {
		printf("attaching julia");
		glAttachShader(program,julia);
	}
	glLinkProgram(program);
	glUseProgram(program);
	findShaderVariableLocations();
	setZoomTarget(zoomTarget.x,zoomTarget.y);
	setZoomFactor(zoomFactor);
	setCInit(cInit.x,cInit.y);
	glutPostRedisplay();
}

void colorTexCallback(int ID) {
	int id=colorTexBox->get_int_val();
	fflush(stdout);
	if (strcmp(colorTexBox->get_item_ptr(id)->text.c_str(),"--")!=0) {
		useColor(colorTexBox->get_item_ptr(id)->text.c_str());
		//printf(colorTexBox->get_item_ptr(id)->text.c_str());
		//fflush(stdout);
	}
	glUseProgram(0);
	glLinkProgram(program);
	glUseProgram(program);

	setZoomTarget(zoomTarget.x,zoomTarget.y);
	setZoomFactor(zoomFactor);
	//glUniform1i(colorTexLocation,glColorTex);
	glutPostRedisplay();
}

void updateTargetXCallback(int id) {
	float val=atof(zTargetXText.c_str());
	setZoomTarget(val,zoomTarget.y);
}

void updateTargetYCallback(int id) {
	float val=atof(zTargetYText.c_str());
	setZoomTarget(zoomTarget.x,val);
}

void updateCRCallback(int id) {
	float val=atof(cRText.c_str());
	setCInit(val,cInit.y);
}

void updateCICallback(int id) {
	float val=atof(cIText.c_str());
	setCInit(cInit.x,val);
}

void processMouse(int button, int state, int x, int y)
{
	// Used for wheels, has to be up
	if (state==0) {
		if (button==3) {
			setZoomFactor(zoomFactor-zoomDelta);
		} else if (button==4) {
			setZoomFactor(zoomFactor+zoomDelta);
		}

		if (button==0) {
			oldScreenX=x-GW/2;
			oldScreenY=y-GH/2;
		}
	}
	fflush(stdout);
	glutPostRedisplay();
}

void processMove(int x,int y) {
	//printf("move %d %d\n",x,y);
	x=x-GW/2;
	y=y-GH/2;
	float movX=(float)(x-oldScreenX)/(float)GW;
	float movY=(float)(y-oldScreenY)/(float)GH;
	oldScreenX=x;
	oldScreenY=y;
	setZoomTarget(zoomTarget.x+(xDelta*movX)*zoomFactor*100,
			zoomTarget.y-(yDelta*movY)*zoomFactor*100);
}

void populateColorTex(GLUI_Listbox *colorTex) {
	DIR *dir;
	struct dirent *dirEnt;
	if ((dir=opendir("./colors/"))==NULL) {
		colorTex->add_item(0,"--");
		return;
	}
	int i=0;
	while ((dirEnt=readdir(dir))!=NULL) {
		if ((strcmp(dirEnt->d_name,".")!=0)&&(strcmp(dirEnt->d_name,"..")!=0))
			colorTex->add_item(i++,dirEnt->d_name);
	}
}

int main(int argc, char **argv) {
	printf("GPU Fractals!\nWritten by Pete Corey (pcorey@calpoly.edu)\nCal Poly State University\nDecember 2010\n\n");
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(1200,800);
	GW=1200;
	GH=800;
	int gfxWindow=glutCreateWindow("GPU Fractals!");

	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(processMouse);
	glutMotionFunc(processMove);
	//glutIdleFunc(idle);


	glEnable(GL_DEPTH_TEST);

	if (!init()) {
		printf("Problem initializing... Quitting.");
	}

	if (!initShaders()) {
		printf("Problem initializing shaders... Quitting.");
	}

	glDisable(GL_TEXTURE_2D);

	glClearColor(0,0,0,1);
	glutDisplayFunc(renderScene);

	glui=GLUI_Master.create_glui("GPU Fractals!",0);

	GLUI_Panel *mainPanel=glui->add_panel("");
	GLUI_Panel *colorPanel=glui->add_panel_to_panel(mainPanel,"Coloring Options:");

	int colorAlgoTmp;
	colorAlgo=glui->add_listbox_to_panel(colorPanel, "Algorithm",NULL,1, colorAlgoCallback);
	colorAlgo->add_item(0,"Iteration Count");
	colorAlgo->add_item(1,"Triangle Inequality (Smooth)");

	int colorTexTmp;
	colorTexBox=glui->add_listbox_to_panel(colorPanel, "Texture   ",&colorTexTmp,4, colorTexCallback);
	populateColorTex(colorTexBox);
	if (strcmp(colorTexBox->get_item_ptr(0)->text.c_str(),"--")!=0) {
		colorTexCallback(0);
	}
	glui->add_column_to_panel(mainPanel,false);

	GLUI_Panel *targetPanel=glui->add_panel_to_panel(mainPanel,"Zoom Target:");
	zTarX=glui->add_edittext_to_panel(targetPanel,"y",zTargetXText,5,updateTargetXCallback);
	zTarY=glui->add_edittext_to_panel(targetPanel,"x",zTargetYText,6,updateTargetYCallback);

	glui->add_column_to_panel(mainPanel,false);

	GLUI_Panel *cPanel=glui->add_panel_to_panel(mainPanel,"\'c\' Parameter:");
	cR=glui->add_edittext_to_panel(cPanel,"r",cRText,7,updateCRCallback);
	cI=glui->add_edittext_to_panel(cPanel,"i",cIText,8,updateCICallback);

	glui->add_statictext("Press [tab] to alternate between a Mandelbrot fractal, and a Julia Set with c=zoomTarget.");
	glui->add_statictext("Additional keyboard controls:");
	glui->add_statictext("[W/S/A/D] - Move zoomTarget up/down/left/right respectively");
	glui->add_statictext("[E/F] - Zoom out/in respectively");


	glui->set_main_gfx_window(gfxWindow);
	GLUI_Master.set_glutIdleFunc(idle);

	initComplete=true;

	glutMainLoop();
	return 0;
}

