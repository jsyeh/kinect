/*****************************************************************************
*                                                                            *
*  OpenNI 2.x Alpha                                                          *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of OpenNI.                                              *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/
// Undeprecate CRT functions
#ifndef _CRT_SECURE_NO_DEPRECATE 
	#define _CRT_SECURE_NO_DEPRECATE 1
#endif
#include <GL/glut.h>
#include "Viewer.h"

//jsyeh begin
#include <opencv/highgui.h>
#include <opencv/cv.h>
//jsyeh end

//#define GL_WIN_SIZE_X	1280
//#define GL_WIN_SIZE_Y	1024
#define GL_WIN_SIZE_X 400
#define GL_WIN_SIZE_Y 300
#define TEXTURE_SIZE	512

#define DEFAULT_DISPLAY_MODE	DISPLAY_MODE_DEPTH1

#define MIN_NUM_CHUNKS(data_size, chunk_size)	((((data_size)-1) / (chunk_size) + 1))
#define MIN_CHUNKS_SIZE(data_size, chunk_size)	(MIN_NUM_CHUNKS(data_size, chunk_size) * (chunk_size))

SampleViewer* SampleViewer::ms_self = NULL;

void SampleViewer::glutIdle()
{
	glutPostRedisplay();
}
void SampleViewer::glutDisplay()
{
	SampleViewer::ms_self->display();
}
void SampleViewer::glutKeyboard(unsigned char key, int x, int y)
{
	SampleViewer::ms_self->onKey(key, x, y);
}






SampleViewer::SampleViewer(const char* strSampleName, openni::VideoStream& depth1, openni::VideoStream& depth2, openni::VideoStream& color1, openni::VideoStream &color2) :
	m_pTexMap(NULL), m_eViewState(DEFAULT_DISPLAY_MODE), m_depth1(depth1), m_depth2(depth2), m_streams(NULL), m_color1(color1), m_color2(color2)//jsyeh extended color 

{
	ms_self = this;
	strncpy(m_strSampleName, strSampleName, ONI_MAX_STR);
}
SampleViewer::~SampleViewer()
{
	delete[] m_pTexMap;

	ms_self = NULL;

	if (m_streams != NULL)
	{
		delete []m_streams;
	}
}

openni::Status SampleViewer::init(int argc, char **argv)
{
	openni::VideoMode videoMode1 = m_depth1.getVideoMode();
	openni::VideoMode videoMode2 = m_depth2.getVideoMode();

	if (videoMode1.getResolutionX() != videoMode2.getResolutionX() ||
		videoMode1.getResolutionY() != videoMode2.getResolutionY())
	{
		printf("Streams need to match resolution.\n");
		return openni::STATUS_ERROR;
	}

	m_width = videoMode1.getResolutionX();
	m_height = videoMode1.getResolutionY();

	m_streams = new openni::VideoStream*[2];
	m_streams[0] = &m_depth1;
	m_streams[1] = &m_depth2;

	// Texture map init
	m_nTexMapX = MIN_CHUNKS_SIZE(m_width, TEXTURE_SIZE);
	m_nTexMapY = MIN_CHUNKS_SIZE(m_height, TEXTURE_SIZE);
	m_pTexMap = new openni::RGB888Pixel[m_nTexMapX * m_nTexMapY];

	return initOpenGL(argc, argv);

}
openni::Status SampleViewer::run()	//Does not return
{
	glutMainLoop();

	return openni::STATUS_OK;
}

void SampleViewer::calculateHistogram(const openni::VideoFrameRef& frame)
{
	const openni::DepthPixel* pDepth = (const openni::DepthPixel*)frame.getData();
	// Calculate the accumulative histogram (the yellow display...)
	memset(m_pDepthHist, 0, MAX_DEPTH*sizeof(float));
	int restOfRow = frame.getStrideInBytes() / sizeof(openni::DepthPixel) - frame.getWidth();
	int height = frame.getHeight();
	int width = frame.getWidth();

	unsigned int nNumberOfPoints = 0;
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x, ++pDepth)
		{
			if (*pDepth != 0)
			{
				m_pDepthHist[*pDepth]++;
				nNumberOfPoints++;
			}
		}
		pDepth += restOfRow;
	}
	for (int nIndex=1; nIndex<MAX_DEPTH; nIndex++)
	{
		m_pDepthHist[nIndex] += m_pDepthHist[nIndex-1];
	}
	if (nNumberOfPoints)
	{
		for (int nIndex=1; nIndex<MAX_DEPTH; nIndex++)
		{
			m_pDepthHist[nIndex] = (unsigned int)(256 * (1.0f - (m_pDepthHist[nIndex] / nNumberOfPoints)));
		}
	}

}

void SampleViewer::displayFrame(const openni::VideoFrameRef& frame)
{
	if (!frame.isValid())
		return;

	const openni::DepthPixel* pDepthRow = (const openni::DepthPixel*)frame.getData();
	openni::RGB888Pixel* pTexRow = m_pTexMap + frame.getCropOriginY() * m_nTexMapX;
	int rowSize = frame.getStrideInBytes() / sizeof(openni::DepthPixel);

	for (int y = 0; y < frame.getHeight(); ++y)
	{
		const openni::DepthPixel* pDepth = pDepthRow;
		openni::RGB888Pixel* pTex = pTexRow + frame.getCropOriginX();

		for (int x = 0; x < frame.getWidth(); ++x, ++pDepth, ++pTex)
		{
			if (*pDepth != 0)
			{
				int nHistValue = m_pDepthHist[*pDepth];
				pTex->r = nHistValue;
				pTex->g = nHistValue;
				pTex->b = nHistValue;
			}
		}

		pDepthRow += rowSize;
		pTexRow += m_nTexMapX;
	}

}

void SampleViewer::displayBothFrames()
{
	struct
	{
		const openni::DepthPixel* pDepthRow;
		const openni::DepthPixel* pDepth;
	}  mainFrame, maskFrame;

	mainFrame.pDepthRow = (const openni::DepthPixel*)m_depth1Frame.getData();
	maskFrame.pDepthRow = (const openni::DepthPixel*)m_depth2Frame.getData();
	openni::RGB888Pixel* pTexRow = m_pTexMap + m_depth1Frame.getCropOriginY() * m_nTexMapX;
	int rowSize = m_depth1Frame.getStrideInBytes() / sizeof(openni::DepthPixel);

	//jsyeh begin
	cvNamedWindow("depth1",0);cvResizeWindow("depth1",400,300);
	cvNamedWindow("depth2",0);cvResizeWindow("depth2",400,300);
	cvNamedWindow("color1",0);cvResizeWindow("color1",400,300);
	cvNamedWindow("color2",0);cvResizeWindow("color2",400,300);
	IplImage *color1, *color2, *depth1, *depth2;
	depth1=cvCreateImageHeader(cvSize(m_depth1Frame.getWidth(),m_depth1Frame.getHeight()), 8,1);
	depth2=cvCreateImageHeader(cvSize(m_depth2Frame.getWidth(),m_depth2Frame.getHeight()), 8,1);
	color1=cvCreateImageHeader(cvSize(m_color1Frame.getHeight(),m_depth1Frame.getHeight()), 8,3);
	color2=cvCreateImageHeader(cvSize(m_color2Frame.getHeight(),m_color2Frame.getHeight()), 8,3);
	depth1->imageData = (char*)m_depth1Frame.getData();
	depth2->imageData = (char*)m_depth2Frame.getData();
	depth1->widthStep = m_depth1Frame.getStrideInBytes();
	depth2->widthStep = m_depth2Frame.getStrideInBytes();
	color1->imageData = (char*)m_color1Frame.getData();
	color2->imageData = (char*)m_color2Frame.getData();
	color1->widthStep = m_color1Frame.getStrideInBytes();
	color2->widthStep = m_color2Frame.getStrideInBytes();
	cvShowImage("depth1", depth1);
	cvShowImage("depth2", depth2);

//	IplImage * dst=cvCloneImage(color1);
//	cvCvtColor(color1, dst, CV_BGR2RGB);
	cvShowImage("color1", color1);
//	cvCvtColor(color2, dst, CV_BGR2RGB);
	cvShowImage("color2", color2);
//	cvReleaseImage(&dst);

	cvReleaseImageHeader(&depth1);
	cvReleaseImageHeader(&depth2);
	cvReleaseImageHeader(&color1);
	cvReleaseImageHeader(&color2);
	//jsyeh end

	for (int y = 0; y < m_depth1Frame.getHeight(); ++y)
	{
		mainFrame.pDepth = mainFrame.pDepthRow;
		maskFrame.pDepth = maskFrame.pDepthRow;
		openni::RGB888Pixel* pTex = pTexRow + m_depth1Frame.getCropOriginX();

		for (int x = 0; x < m_depth1Frame.getWidth(); ++x, ++maskFrame.pDepth, ++mainFrame.pDepth, ++pTex)
		{
			if (*mainFrame.pDepth != 0)
			{
				int nHistValue = m_pDepthHist[*mainFrame.pDepth];

				if (*maskFrame.pDepth == 0)
				{
					// No match
					pTex->r = nHistValue;
					pTex->g = nHistValue;
					pTex->b = nHistValue;
				}
				else
				{
					nHistValue = m_pDepthHist[(*mainFrame.pDepth+*maskFrame.pDepth)/2];
					// Match
					pTex->r = nHistValue;
					pTex->g = 0;
					pTex->b = 0;
				}
			}
		}

		mainFrame.pDepthRow += rowSize;
		maskFrame.pDepthRow += rowSize;
		pTexRow += m_nTexMapX;
	}

}


void SampleViewer::display()
{
	int changedIndex;
	openni::Status rc = openni::OpenNI::waitForAnyStream(m_streams, 2, &changedIndex);

	if (rc != openni::STATUS_OK)
	{
		printf("Wait failed\n");
		return;
	}

	switch (changedIndex)
	{
	case 1: case 0:
		//jsyeh begin
		m_color1.readFrame(&m_color1Frame);
		m_color2.readFrame(&m_color2Frame);
		//jsyeh end
		m_depth2.readFrame(&m_depth2Frame); //break;
	//case 0:
		m_depth1.readFrame(&m_depth1Frame); break;
	default:
		printf("Error in wait\n");
	}

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, GL_WIN_SIZE_X, GL_WIN_SIZE_Y, 0, -1.0, 1.0);

	if (m_depth1Frame.isValid() && m_eViewState != DISPLAY_MODE_DEPTH2)
		calculateHistogram(m_depth1Frame);
	else
		calculateHistogram(m_depth2Frame);

	memset(m_pTexMap, 0, m_nTexMapX*m_nTexMapY*sizeof(openni::RGB888Pixel));

	// check if we need to draw image frame to texture

	switch (m_eViewState)
	{
	case DISPLAY_MODE_DEPTH1:
		displayFrame(m_depth1Frame);
		break;
	case DISPLAY_MODE_DEPTH2:
		displayFrame(m_depth2Frame);
		break;
	default:
		displayBothFrames();
	}

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_nTexMapX, m_nTexMapY, 0, GL_RGB, GL_UNSIGNED_BYTE, m_pTexMap);

	// Display the OpenGL texture map
	glColor4f(1,1,1,1);

	glBegin(GL_QUADS);

	int nXRes = m_width;
	int nYRes = m_height;

	// upper left
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	// upper right
	glTexCoord2f((float)nXRes/(float)m_nTexMapX, 0);
	glVertex2f(GL_WIN_SIZE_X, 0);
	// bottom right
	glTexCoord2f((float)nXRes/(float)m_nTexMapX, (float)nYRes/(float)m_nTexMapY);
	glVertex2f(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	// bottom left
	glTexCoord2f(0, (float)nYRes/(float)m_nTexMapY);
	glVertex2f(0, GL_WIN_SIZE_Y);

	glEnd();

	// Swap the OpenGL display buffers
	glutSwapBuffers();

}

void SampleViewer::onKey(unsigned char key, int /*x*/, int /*y*/)
{
	switch (key)
	{
	case 27:
		m_depth1.stop();
		m_depth2.stop();
		m_depth1.destroy();
		m_depth2.destroy();

		openni::OpenNI::shutdown();
		exit (1);
	case '1':
		m_eViewState = DISPLAY_MODE_OVERLAY;
		break;
	case '2':
		m_eViewState = DISPLAY_MODE_DEPTH1;
		break;
	case '3':
		m_eViewState = DISPLAY_MODE_DEPTH2;
		break;
	case 'm':
//		m_rContext.SetGlobalMirror(!m_rContext.GetGlobalMirror());
		break;
	}

}

openni::Status SampleViewer::initOpenGL(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	glutCreateWindow (m_strSampleName);
	// 	glutFullScreen();
	glutSetCursor(GLUT_CURSOR_NONE);

	initOpenGLHooks();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	return openni::STATUS_OK;

}
void SampleViewer::initOpenGLHooks()
{
	glutKeyboardFunc(glutKeyboard);
	glutDisplayFunc(glutDisplay);
	glutIdleFunc(glutIdle);
}
