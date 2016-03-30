#ifdef _WINDOWS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#include <vector>
#include <string>
#include <SDL_mixer.h>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>


#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define TILE_SIZE 0.5f 
#define SPRITE_COUNT_X 30
#define SPRITE_COUNT_Y 30
#define LEVEL_HEIGHT 32
#define LEVEL_WIDTH 128

SDL_Window* displayWindow;
unsigned char ** levelData;
int mapWidth;
int mapHeight;
Matrix modelMatrixForAlien;
Matrix projectionMatrixForAlien;
Matrix viewMatrixAlien;
ShaderProgram *program;
std::vector<float> vertexData;
std::vector<float> texCoordData;
const Uint8 *keys = SDL_GetKeyboardState(NULL);
GLuint sheet;

using namespace std;

void placeEntity(string type, int placeX, int placeY)
{






}


GLuint LoadTexture(const char *image_path) {
	SDL_Surface *surface = IMG_Load(image_path);
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, surface->w, surface->h, 0, GL_RGB,
		GL_UNSIGNED_BYTE, surface->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	SDL_FreeSurface(surface);
	return textureID;
}


bool readHeader(std::ifstream &stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height"){
			mapHeight = atoi(value.c_str());
		}
	}
	if (mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else { // allocate our map data
		levelData = new unsigned char*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new unsigned char[mapWidth];
		}
		return true;
	}
}


void render() {
	program->setModelMatrix(modelMatrixForAlien);
	for (int y = 0; y < LEVEL_HEIGHT; y++) {
		for (int x = 0; x < LEVEL_WIDTH; x++) {
			float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
			float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
			float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
			float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
			vertexData.insert(vertexData.end(), {
				TILE_SIZE * x, -TILE_SIZE * y,
				TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
				(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
				TILE_SIZE * x, -TILE_SIZE * y,
				(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
				(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
			});
			texCoordData.insert(texCoordData.end(), {
				u, v,
				u, v + (spriteHeight),
				u + spriteWidth, v + (spriteHeight),
				u, v,
				u + spriteWidth, v + (spriteHeight),
				u + spriteWidth, v
			});
		}
	}

	glUseProgram(program->programID);
	glBindTexture(GL_TEXTURE_2D, sheet);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

bool readLayerData(std::ifstream &stream) 
{
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					unsigned char val = (unsigned char)atoi(tile.c_str());
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						levelData[y][x] = val - 1;
					}
					else {
						levelData[y][x] = 0;
					}
				}
			}
		}
	}
	return true;
}
bool readEntityData(std::ifstream &stream) {
	string line;
	string type;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "type") {
			type = value;
		}
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');
			float placeX = atoi(xPosition.c_str()) / 16 * TILE_SIZE;
			float placeY = atoi(yPosition.c_str()) / 16 * -TILE_SIZE;
			cout << placeX << endl;
			cout << placeY << endl;
			placeEntity(type, placeX, placeY);
		}
	}
	return true;
}
void read() {
	ifstream infile("world.txt");
	string line;
	if (!infile)
	{
		cout << "OH NOOOOOOOOOOOO!" << endl;
	}
	else
	{
		cout << "SUCCESS!!!" << endl;
		while (getline(infile, line))
		{
			if (line == "[header]")
			{
				if (!readHeader(infile))
				{
					break;
				}
			}
			else if (line == "[layer]")
			{
				readLayerData(infile);
			}
			else if (line == "[Player]")
			{
				readEntityData(infile);
			}
		}
	}

}



int main(int argc, char *argv[])
{

	//allows me to print to the console. only affects windows users.
#ifdef _WIN32
	AllocConsole();
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stderr);
#endif

	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Jet's World", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 320, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 360);
	program->setModelMatrix(modelMatrixForAlien);
	program->setProjectionMatrix(projectionMatrixForAlien);
	program->setViewMatrix(viewMatrixAlien);
	projectionMatrixForAlien.setOrthoProjection(-5.33f, 5.33f, -3.0f, 3.0f, -1.0f, 1.0f);
	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	glUseProgram(program->programID);

	SDL_Event event;
	bool done = false;

	read();
	
	sheet = LoadTexture("spritesheet.png");
	render();

	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}

		}

		
		glClear(GL_COLOR_BUFFER_BIT);

		SDL_GL_SwapWindow(displayWindow);

	}

	delete program;
#ifdef _WIN32
	std::cin.get();
#endif


	SDL_Quit();
	return 0;
}

//#ifdef _WIN32
//	std::cin.get();
//#endif
//

//allows me to print to the console. Only affects windows users.
//#ifdef _WINDOWS
//	AllocConsole();
//	freopen("CONIN$", "r", stdin);
//	freopen("CONOUT$", "w", stdout);
//	freopen("CONOUT$", "w", stderr);
//	//===============================================================================================================================================
//#endif