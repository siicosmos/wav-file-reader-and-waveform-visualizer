//
//  Created by Liam Ling on 2018-05-27.
//  Copyright © 2018 Liam Ling. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <linux/limits.h>
#include <GLFW/glfw3.h>

using namespace std;

typedef struct waveFileHeader_h
{
	unsigned char ChunkID[4];     // 4 bytes
	uint32_t ChunkSize;           // 4 bytes
	unsigned char Format[4];      // 4 bytes
	unsigned char SubChunk1ID[4]; // 4 bytes
	uint32_t SubChunk1Size;       // 4 bytes
	uint16_t AudioFormat;         // 2 bytes
	uint16_t NumChannels;         // 2 bytes
	uint32_t SampleRate;          // 4 bytes
	uint32_t ByteRate;            // 4 bytes
	uint16_t BlockAlign;          // 2 bytes
	uint16_t BitsPersample;       // 2 bytes
	unsigned char SubChunk2ID[4]; // 4 bytes
	uint32_t SubChunk2Size;       // 4 bytes -- indicates total bytes of data
								  //  == NumSamples * NumChannels * BitsPersample/8
} wfh;

typedef struct waveForm_f
{
	int numberOfPoints;
	GLdouble* waveFormVerticsBuffer;
} wf;

FILE* openFileDialog();
void removeReturnChar(char* filePath);
const void printHeaderInfo(wfh* waveFileHeader);
const int findMaxValue(short int* buffer, int size);
void glfwError_Callback(int error, const char* errorContent);
void glfwWindowSize_Callback(GLFWwindow* window, int width, int height);
void glfwKey_Callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main(int argc, char** argv)
{
	wfh* waveFileHeader = new waveFileHeader_h;
	short int* dataBuffer;
	wf* waveForm = new waveForm_f;

	FILE* filePath_f = openFileDialog();
	char filePath[PATH_MAX];
	while(filePath_f == NULL)
	{
		perror("Error opening file");
		filePath_f = openFileDialog();
	}

	while(fgets(filePath, PATH_MAX, filePath_f) != NULL) {}
	cout << "Selected File Path: " << filePath;
	removeReturnChar(filePath); // remove the extra return charator at the end
	pclose(filePath_f);
	
	FILE* waveFile = fopen(filePath, "r");
	if(waveFile == NULL)
	{
		delete waveFileHeader;
		delete waveForm;;
		perror("Error loading file");
		return 1;
	}
	else
	{
		size_t waveFileHeaderSize = sizeof(*waveFileHeader);
		size_t headrOffset = fread(waveFileHeader, 1, waveFileHeaderSize, waveFile);
		cout << "Header Size: " << headrOffset << " bytes.\n";
		if(headrOffset > 0)
		{
			printHeaderInfo(waveFileHeader);

			int numOfSamples = waveFileHeader->SubChunk2Size/waveFileHeader->NumChannels/waveFileHeader->BitsPersample*8;
			int sampleSize = waveFileHeader->BitsPersample/8;
			dataBuffer = new short int[numOfSamples];
			memset(dataBuffer, 0, sizeof(short int)*numOfSamples);
			size_t dataBufferSize = sizeof(*dataBuffer);
			size_t dataSize = 0;
			size_t temp = 0;
			while((temp = fread(&dataBuffer[0], sampleSize, numOfSamples, waveFile)) > 0) {dataSize += temp;}
			int maxValue = findMaxValue(dataBuffer, numOfSamples);

			if(dataSize*16/8 == waveFileHeader->SubChunk2Size)
			{
				cout << "*************************************************\n";
				cout << "Number of Samples: " << numOfSamples << endl;
				cout << "Maximum value: " << maxValue << "(in signed short int(16 bits))" << endl;
				cout << "*************************************************\n";

				//--------------------------------------------- GLFW
				int windowWidth = 2000;
				int windowHeight = 600;
				GLdouble Xpos = 0.0;
				GLdouble Xdistance = windowWidth/static_cast<GLdouble>(numOfSamples);
				GLdouble Ypos = windowHeight/2.0;
				cout << "Xpos: " << Xpos << ", dist: " << Xdistance << endl;

				GLFWwindow* window1 = NULL;
				wf* waveForm = new waveForm_f;
				waveForm->numberOfPoints = numOfSamples;
				waveForm->waveFormVerticsBuffer = new GLdouble[waveForm->numberOfPoints*2];

				//--------------------------------------------- constrcut vertices
				for(int i = 0; i < numOfSamples; i++)
				{
					waveForm->waveFormVerticsBuffer[2*i]   = Xpos;
					waveForm->waveFormVerticsBuffer[2*i+1] = Ypos+(dataBuffer[i])/90;
					Xpos += Xdistance;
					// cout << waveForm->waveFormVerticsBuffer[2*i] << ", " << waveForm->waveFormVerticsBuffer[2*i+1] << endl;
				}
				//---------------------------------------------

				glfwSetErrorCallback(glfwError_Callback);

				if(!glfwInit())
				{
					cerr << "Error initialize GLFW" << endl;
	    			delete waveFileHeader;
					delete [] dataBuffer;
					delete [] waveForm->waveFormVerticsBuffer;
					delete waveForm;
					fclose(waveFile);
					exit(-1);
				}
				else
				{
					cout << "Opening a WaveForm Window..." << endl;
					window1 = glfwCreateWindow(windowWidth, windowHeight, "WaveForm", NULL, NULL);
				}
				if(!window1)
				{
					cerr << "Error initialize a window" << endl;
	    			delete waveFileHeader;
					delete [] dataBuffer;
					delete [] waveForm->waveFormVerticsBuffer;
					delete waveForm;
					fclose(waveFile);
					glfwTerminate();
					exit(-1);
				}
				else
				{
					glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
					glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
					glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
					glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
					glfwWindowHint(GLFW_DECORATED, false);
					glfwMakeContextCurrent(window1);
					glfwSetWindowSizeCallback(window1, glfwWindowSize_Callback);
					glfwSetKeyCallback(window1, glfwKey_Callback);
					glfwSwapInterval(1);

					//--------------------------------------------- OPENGL
					cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
					glViewport(0.0f, 0.0f, windowWidth, windowHeight);
					glMatrixMode(GL_PROJECTION);
					glLoadIdentity();
					glOrtho(0, windowWidth, 0, windowHeight, 0, 1);
					glMatrixMode(GL_MODELVIEW);
					glLoadIdentity();

					glClearColor(0, 0, 0, 1);
					
					while(!glfwWindowShouldClose(window1))
					{
						glClear(GL_COLOR_BUFFER_BIT);
						// Draw the wave form
						glLineWidth(1);
						glEnableClientState(GL_VERTEX_ARRAY);
						glVertexPointer(2, GL_DOUBLE, 0, waveForm->waveFormVerticsBuffer);
						glDrawArrays(GL_LINE_STRIP, 0, waveForm->numberOfPoints);
						glDisableClientState(GL_VERTEX_ARRAY);
						//---------------------------------------------
						glfwSwapBuffers(window1);
						glfwPollEvents();
					}
					cout << "Window closed..." << endl;
	    			delete waveFileHeader;
					delete [] dataBuffer;
					delete [] waveForm->waveFormVerticsBuffer;
					delete waveForm;
					fclose(waveFile);
					glfwDestroyWindow(window1);
					glfwTerminate();
					return 0;
				}
    			//---------------------------------------------
			}
			else
			{
				delete waveFileHeader;
				delete [] dataBuffer;
				delete waveForm;
				fclose(waveFile);
				perror("Error loading sound data");
				return 1;
			}
		}
		else
		{
			perror("Error reading wave file header");
			delete waveFileHeader;
			delete waveForm;
			fclose(waveFile);
			return 1;
		}
	}
	return 0;
}

FILE* openFileDialog()
{
	FILE* filePath_f = popen("zenity --file-selection --title=Choose' 'a' 'Wave' 'File --file-filter=*.wav 2> /dev/null", "r");
	return filePath_f;
}

void removeReturnChar(char* filePath)
{
	int i = 0;
	while(filePath[i] != '\0')
	{
		if(filePath[i] == '\n')
		{
			filePath[i] = '\0';
		}
		i++;
	}
}

const void printHeaderInfo(wfh* waveFileHeader)
{
	printf("ChunkID: %.4s\n", waveFileHeader->ChunkID);
	printf("ChunkSize: %.4u\n", waveFileHeader->ChunkSize);
	printf("Format: %.4s\n", waveFileHeader->Format);
	printf("SubChunk1ID: %.4s\n", waveFileHeader->SubChunk1ID);
	printf("SubChunk1Size: %.4u\n", waveFileHeader->SubChunk1Size);
	printf("AudioFormat: %.2u\n", waveFileHeader->AudioFormat);
	printf("NumChannels: %.2u\n", waveFileHeader->NumChannels);
	printf("SampleRate: %.4u\n", waveFileHeader->SampleRate);
	printf("ByteRate: %.4u\n", waveFileHeader->ByteRate);
	printf("BlockAlign: %.2u\n", waveFileHeader->BlockAlign);
	printf("BitsPersample: %.2u\n", waveFileHeader->BitsPersample);
	printf("SubChunk2ID: %.4s\n", waveFileHeader->SubChunk2ID);
	printf("SubChunk2Size: %.4u\n", waveFileHeader->SubChunk2Size);
}

const int findMaxValue(short int* buffer, int size)
{
	short int temp = buffer[0];
	for(int i = 1; i < size; i++)
		if(buffer[i] > temp)
			temp = buffer[i];
	return temp;
}

void glfwError_Callback(int error, const char* errorContent)
{
	cerr << "Glfw3 error: " << errorContent << endl;
}

void glfwWindowSize_Callback(GLFWwindow* window, int width, int height)
{
	cout << "Glfw3 window size changed to: width: " << width << " height: " << height << endl;
}

void glfwKey_Callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		cout << "Window closed..." << endl;
		glfwTerminate();
		exit(0);
	}
}