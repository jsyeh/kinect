//Speckle3D v0.1 Show Speckle in 3D by OpenGL
//2013-01-11 15:52-18:00
#include <GL/glut.h>
#include <stdio.h>
#include <math.h>
#include <opencv/highgui.h>

int TestBMP=0;
bool bOrtho=false;
int winW=800, winH=600, winTitleH=30;
float bigger=1.3;
float rot=0;
float rotX=0, rotY=0;
float dX=0,dY=-0.5;
int oldx=0,oldy=0;
int oldxT=0, oldyT=0;

unsigned int myPointN=0;
unsigned int myColorN=0;
unsigned int myIndexN=0;
float * myPointArray=NULL;
unsigned char * myColorArray=NULL;
unsigned int * myIndexArray=NULL;

void myBuildTriangle(unsigned int& index, unsigned int v1, unsigned int v2, unsigned int v3)
{
	myIndexArray[index++]=v1;
	myIndexArray[index++]=v2;
	myIndexArray[index++]=v3;
}
void initCV(char *filename)
{
	IplImage * img=cvLoadImage(filename,0);
	cvNamedWindow("opencv");
	cvShowImage("opencv", img);
	cvWaitKey(1);
	//construct 3D
	myPointN=img->width*img->height;
	myPointArray = new float[myPointN*3];
	myColorN=img->width*img->height;
	myColorArray = new unsigned char[myColorN*3];
	myIndexArray = new unsigned int[img->width*img->height*12];
	
	cvReleaseImage(&img);	
}
void updateCV(char *filename)
{
	IplImage * img=cvLoadImage(filename,0);
	cvNamedWindow("opencv");
	cvShowImage("opencv", img);
	cvWaitKey(1);
	for(int i=0;i<img->height;i++){
		unsigned char * ptr=(unsigned char*)img->imageData + i*img->widthStep;
		for(int j=0;j<img->width;j++){
			myPointArray[ (i*img->width+j)*3+0]=0.5-j/(float)img->width;
			myPointArray[ (i*img->width+j)*3+1]=0.5-i/(float)img->height;
			myPointArray[ (i*img->width+j)*3+2]=(ptr[j]/255.0-0.5);
			myColorArray[ (i*img->width+j)*3+0]=ptr[j];
			myColorArray[ (i*img->width+j)*3+1]=ptr[j];
			myColorArray[ (i*img->width+j)*3+2]=ptr[j];
			if(ptr[j]==255){//put read for over-saturation
//				myColorArray[ (i*img->width+j)*3+1]=0;
//				myColorArray[ (i*img->width+j)*3+2]=0;
			}
		}
	}
	myIndexN=0;
	for(int i=0,h=img->height;i<img->height-1;i++){
		for(int j=0,w=img->width;j<img->width-1;j++){
			myBuildTriangle(myIndexN, i*w+j, (i+1)*w+j+1, i*w+j+1);
			myBuildTriangle(myIndexN, i*w+j, (i+1)*w+j, (i+1)*w+j+1);
		}
	}
	cvReleaseImage(&img);	
}
void drawSpeckle3D()
{
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 3, GL_FLOAT, 0, myPointArray );
	glEnableClientState( GL_COLOR_ARRAY );
	glColorPointer( 3, GL_UNSIGNED_BYTE, 0, myColorArray );	
	glDrawElements( GL_TRIANGLES, myIndexN, GL_UNSIGNED_INT, myIndexArray );

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );
}
void display()
{
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
		glScalef(bigger, bigger, bigger);
		glRotatef(rotY, 1,0,0);
		glRotatef(rotX, 0,1,0);
		drawSpeckle3D();
	glPopMatrix();
	glutSwapBuffers();
}
void resize(int w, int h)
{
	winW=w;winH=h;
	glViewport(0,0,w,h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if(bOrtho){
		glOrtho(-1.0*w/h,1.0*w/h,-1.0,1.0,-10,10);
	}else{
		gluPerspective(57, w/(float)h, 0.1f, 3000.0f);
	}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0,0,1.7,0,0,0,0,1,0);
}
void timer(int t)
{
	glutTimerFunc(33,timer, t+1);
	glutPostRedisplay();
	//rotX+=dX;
	rotY+=dY;
	oldxT=oldx;
	oldyT=oldy;
	//if(rotY>90){rotY=90;dY=0;}
	//if(rotY<-80){rotY=-80;dY=0;}

}
void keyboard(unsigned char key, int x, int y)
{
	if(key==27){
		exit(0);
	}else if(key=='+' || key=='='){
		bigger*=1.1;
	}else if(key=='-'){
		bigger*=0.9;
	}else if(key=='o' || key=='O'){
		bOrtho= !bOrtho; //true
		resize(winW, winH);
	}else if(key=='p' || key=='P'){
		bOrtho= !bOrtho; //false;
		resize(winW, winH);
	}
	glutPostRedisplay();
}
void special(int key, int x, int y)
{
	if(key==GLUT_KEY_LEFT){
		TestBMP--;
		if(TestBMP<0)TestBMP=0;
		//updateCV(TestBMP);
	}else if(key==GLUT_KEY_RIGHT){
		TestBMP++;
		if(TestBMP>=300)TestBMP=300;
		///updateCV(TestBMP);
	}else if(key==GLUT_KEY_UP){
		rotY-=2;
	///	if(rotY<-90)rotY=-90;
	}else if(key==GLUT_KEY_DOWN){
		rotY+=2;
	//	if(rotY>90)rotY=90;
	}
}
void mouse(int button, int state, int x, int y)
{
	if(button==GLUT_LEFT_BUTTON && state==GLUT_DOWN){
		oldx=x;oldy=y;
		dX=0;dY=0;
	}else if(button==GLUT_LEFT_BUTTON && state==GLUT_UP){
		//dX=(x-oldxT)/2;
		dY=(y-oldyT)/2;
	}
}
void motion(int x, int y)
{
	rotX+=(x-oldx)/2;
	rotY+=(y-oldy)/2;
	//if(rotY>90)rotY=90;
	//if(rotY<-90)rotY=-90;
	oldx=x;oldy=y;
	glutPostRedisplay();
}
int main(int argc, char**argv)
{
	char filename[100]="Depth.png";
	printf("Usage: %s [imagefile]\n");
	printf("  You can drag an image on this program to view in 3D\n");
	if(argc==1){
		printf("Now open default image file: %s\n", filename);
	}else if(argc==2){
		sprintf(filename, "%s", argv[1]);
		printf("Now open image file: %s\n", filename);
	}
	printf("Operation Usage\n"
		"(1) Mouse Drag to rotate\n"
		"(2) Key UP/DOWN to rotate\n"
		"(3) Key LEFT/RIGHT to change Test BMP file\n"
		"(4) Key '+'/'-' to resize\n"
		"(5) Key 'o' for Orthogonal Projection\n"
		"(6) Key 'p' for Perspective Projection\n");

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(winW,winH);
	glutCreateWindow("Speckle3D");
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutReshapeFunc(resize);
	glutTimerFunc(0,timer,0);
	
	initCV(filename);
	updateCV(filename);
	glutMainLoop();
	return 0;
}

