// tag::C++11check[]
#define STRING2(x) #x
#define STRING(x) STRING2(x)

#if __cplusplus < 201103L
#pragma message("WARNING: the compiler may not be C++11 compliant. __cplusplus version is : " STRING(__cplusplus))
#endif
// end::C++11check[]

// tag::includes[]
#include <iostream>
#include <istream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <set>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
// end::includes[]


// tag::using[]
//using namespace std;
using std::string;
//using std::char;
using std::cout;
using std::endl;
using std::ifstream;
using std::vector;
using std::max;
// end::using[]

class HeatMapMaker
{
private:
	int* Histogram;

public:
		
	float BinRange[44] = { -5000, -4650, -4300, -3950 , -3600, -3250, -2900 , -2550 ,-2200, -1850, -1500, -1150,-800,-450 ,-100, 250, 600,950,1300 , 1650,  2000, 2350,  2700 ,3050 ,  3400, 3750, 4100, 4450, 4800, 5150, 5500, 5850,6200, 6550,  6900, 7250,7600, 7950,8300, 8650, 9000};
	// ranges to test though
	float cellsize;
	int C;
		//count value 
	HeatMapMaker(vector<GLfloat> data, float size, int cellcount)
	{
		Histogram = new int[cellcount * cellcount];

		for (int i = 0; i < cellcount * cellcount; i++)
		{
			Histogram[i] = 0;
		}

		C = cellcount;

		for (int i = 0; i < data.size() - 1; i += 2)
			CalcBin(data[i], data[i + 1]);		//for each set of co-ordinents..(go to CalcBin)
		
	}

	int getter(int column, int row)
	{
		return Histogram[(row * C) + column];	//return the value of a given position 
	}

private:
	void CalcBin(float x, float y)
	{
		int tempRow = 0;
		int tempCol = 0;
		for (int row = 0; row < C; row++)
		{
			if (x >= BinRange[row] && x < BinRange[row + 1])	//test the given coordients against the bin ranges in x
			{
				tempRow = row;		//keep the x row to use later 
				for (int col = 0; col < C; col++)	
				{
					if (y >= BinRange[col] && y < BinRange[col + 1]) //test the given coordients against the bin range in y 
					{
						tempCol = col;
						increase(tempRow, tempCol);	//pass the collected possition of the block  the player positions is over 
						return;
					}
				}
			}
		}
	}
	void increase(int X, int Y)
	{
		Histogram[(Y * C) + X] += 1; //add's 1 the the currect possition of the grid this is then used for a color value on the heatmap later
	}

};





HeatMapMaker* Heat;


// tag::globalVariables[]
std::string exeName;
SDL_Window *win; //pointer to the SDL_Window
SDL_GLContext context; //the SDL_GLContext
int frameCount = 0;
std::string frameLine = "";
// end::globalVariables[]

// tag::vertexShader[]
//string holding the **source** of our vertex shader, to save loading from a file
const std::string strVertexShader = R"(
	#version 330
	in vec2 position;
  in vec3 vertColor;
	uniform vec2 offset;
  out vec3 fragColor;
	void main()
	{
  		fragColor = vertColor;
		vec2 tmpPosition = position + offset;
		gl_Position = vec4(tmpPosition, 0.0, 5400.0);
	}
)";
// end::vertexShader[]

GLfloat offset[] = { -5400.0, 0.0 }; //using different values from CPU and static GLSL examples, to make it clear this is working
GLfloat offsetVelocity[] = { 0.2, 0.2 }; //rate of change of offset in units per second


										 // tag::fragmentShader[]
										 //string holding the **source** of our fragment shader, to save loading from a file
const std::string strFragmentShader = R"(
	#version 330
  in vec3 fragColor;
	out vec4 outputColor;
	uniform vec4 color;
	void main()
	{
		 outputColor = vec4(color) + vec4(fragColor, 1.0f);
	}
)";
// end::fragmentShader[]
								//fragcolor allows for changing of spesificic blocks of data without having an effect on others 
// tag::ourVariables[]
//our variables
bool done = false;

//the color we'll pass to the GLSL
GLfloat color[] = { 0.0f, 1.0f, 1.0f, 1.0f }; //using different values from CPU and static GLSL examples, to make it clear this is working
											  //our GL and GLSL variables

GLuint theProgram; //GLuint that we'll fill in to refer to the GLSL program (only have 1 at this point)
GLint posLoc; //GLuint that we'll fill in with the location of the `position` attribute in the GLSL
GLint colorLocation;
GLint vertexCol;
GLint offsetLocation; //GLuint that we'll fill in with the location of the `offset` variable in the GLSL

GLuint vertexDataBufferObject;
GLuint TrajectoryVertexObject;
GLuint BatArrObject;
GLuint GridArrayObject;
// end::ourVariables[]

//ifstream movementFile ("../../GamesArchitecture/Content/MovementLog.txt");
//vector<string> Lines;



string movementFile = "../../logs/MovementLog.txt";	//paths
vector<GLfloat> TV;

string  batteryFile = "../../logs/BatteryLog.txt";		//paths 
vector<GLfloat> BV;
string currentLine;

vector<GLfloat> GridVertex;

int PixelSize = 3;


// end Global Variables
/////////////////////////

// tag::initialise[]
void initialise()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		exit(1);
	}

	cout << "SDL initialised OK!\n";
}
// end::initialise[]

// tag::createWindow[]
void createWindow()
{
	//get executable name, and use as window title
	int beginIdxWindows = exeName.rfind("\\"); //find last occurrence of a backslash
	int beginIdxLinux = exeName.rfind("/"); //find last occurrence of a forward slash
	int beginIdx = max(beginIdxWindows, beginIdxLinux);
	std::string exeNameEnd = exeName.substr(beginIdx + 1);
	const char *exeNameCStr = exeNameEnd.c_str();

	//create window
	win = SDL_CreateWindow(exeNameCStr, 100, 100, 800, 800, SDL_WINDOW_OPENGL); //same height and width makes the window square ...

																				//error handling
	if (win == nullptr)
	{
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}
	cout << "SDL CreatedWindow OK!\n";
}
// end::createWindow[]

// tag::setGLAttributes[]
void setGLAttributes()
{
	int major = 3;
	int minor = 3;
	cout << "Built for OpenGL Version " << major << "." << minor << endl; //ahttps://en.wikipedia.org/wiki/OpenGL_Shading_Language#Versions
																		  // set the opengl context version
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //core profile
	cout << "Set OpenGL context to versicreate remote branchon " << major << "." << minor << " OK!\n";
}
// tag::setGLAttributes[]

// tag::createContext[]
void createContext()
{
	setGLAttributes();

	context = SDL_GL_CreateContext(win);
	if (context == nullptr) {
		SDL_DestroyWindow(win);
		std::cout << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}
	cout << "Created OpenGL context OK!\n";
}
// end::createContext[]

// tag::initGlew[]
void initGlew()
{
	GLenum rev;
	glewExperimental = GL_TRUE; //GLEW isn't perfect - see https://www.opengl.org/wiki/OpenGL_Loading_Library#GLEW
	rev = glewInit();
	if (GLEW_OK != rev) {
		std::cout << "GLEW Error: " << glewGetErrorString(rev) << std::endl;
		SDL_Quit();
		exit(1);
	}
	else {
		cout << "GLEW Init OK!\n";
	}
}
// end::initGlew[]

// tag::createShader[]
GLuint createShader(GLenum eShaderType, const std::string &strShaderFile)
{
	GLuint shader = glCreateShader(eShaderType);
	//error check
	const char *strFileData = strShaderFile.c_str();
	glShaderSource(shader, 1, &strFileData, NULL);

	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

		const char *strShaderType = NULL;
		switch (eShaderType)
		{
		case GL_VERTEX_SHADER: strShaderType = "vertex"; break;
		case GL_GEOMETRY_SHADER: strShaderType = "geometry"; break;
		case GL_FRAGMENT_SHADER: strShaderType = "fragment"; break;
		}

		fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
		delete[] strInfoLog;
	}

	return shader;
}
// end::createShader[]

// tag::createProgram[]
GLuint createProgram(const std::vector<GLuint> &shaderList)
{
	GLuint program = glCreateProgram();

	for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
		glAttachShader(program, shaderList[iLoop]);

	glLinkProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
		fprintf(stderr, "Linker failure: %s\n", strInfoLog);
		delete[] strInfoLog;
	}

	for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
		glDetachShader(program, shaderList[iLoop]);

	return program;
}
// end::createProgram[]

// tag::initializeProgram[]
void initialize()
{
	std::vector<GLuint> shaderList;

	shaderList.push_back(createShader(GL_VERTEX_SHADER, strVertexShader));
	shaderList.push_back(createShader(GL_FRAGMENT_SHADER, strFragmentShader));

	theProgram = createProgram(shaderList);
	if (theProgram == 0)
	{
		cout << "GLSL program creation error." << std::endl;
		SDL_Quit();
		exit(1);
	}
	else {
		cout << "GLSL program creation OK! GLUint is: " << theProgram << std::endl;
	}

	posLoc = glGetAttribLocation(theProgram, "position");
	colorLocation = glGetUniformLocation(theProgram, "color");
	vertexCol = glGetAttribLocation(theProgram, "vertColor");
	offsetLocation = glGetUniformLocation(theProgram, "offset");
	//clean up shaders (we don't need them anymore as they are no in theProgram
	for_each(shaderList.begin(), shaderList.end(), glDeleteShader);
}
// end::initializeProgram[]
void GridVertexArrayObject()
{
	glGenVertexArrays(1, &GridArrayObject); //create a Vertex Array Object
	cout << "Vertex Array Object created OK! GLUint is: " << GridArrayObject << std::endl;

	glBindVertexArray(GridArrayObject); //make the just created vertexArrayObject the active one

	glBindBuffer(GL_ARRAY_BUFFER, GridArrayObject); //bind vertexDataBufferObject

	glEnableVertexAttribArray(posLoc); //enable attribute at index positionLocation

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, (5 * sizeof(GLfloat)), 0); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation

	glEnableVertexAttribArray(vertexCol);
	glVertexAttribPointer(vertexCol, 3, GL_FLOAT, GL_FALSE, (5 * sizeof(GLfloat)), (GLvoid*)(2 * sizeof(GLfloat)));

	glBindVertexArray(0); //unbind the vertexArrayObject so we can't change it

						  //cleanup
	glDisableVertexAttribArray(posLoc); //disable vertex attribute at index positionLocation
	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind array buffer

}
// tag::initializeVertexArrayObject[]
//setup a GL object (a VertexArrayObject) that stores how to access data and from where
void initializeVertexArrayObject()
{
	glGenVertexArrays(1, &TrajectoryVertexObject); //create a Vertex Array Object
	cout << "Vertex Array Object created OK! GLUint is: " << TrajectoryVertexObject << std::endl;

	glBindVertexArray(TrajectoryVertexObject); //make the just created vertexArrayObject the active one

	glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObject); //bind vertexDataBufferObject

	glEnableVertexAttribArray(posLoc); //enable attribute at index positionLocation

	glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, (2 * sizeof(GLfloat)), 0); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation

	glBindVertexArray(0); //unbind the vertexArrayObject so we can't change it

						  //cleanup
	glDisableVertexAttribArray(posLoc); //disable vertex attribute at index positionLocation
	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind array buffer

}
// end::initializeVertexArrayObject[]

void initializeBatteryArrayObject()
{
	glGenVertexArrays(1, &BatArrObject); //create a Vertex Array Object
	cout << "Vertex Array Object created OK! GLUint is: " << BatArrObject << std::endl;

	glBindVertexArray(BatArrObject); //make the just created vertexArrayObject the active one

	glBindBuffer(GL_ARRAY_BUFFER, BatArrObject); //bind vertexDataBufferObject

	glEnableVertexAttribArray(posLoc); //enable attribute at index positionLocation

	glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, (2 * sizeof(GLfloat)), 0); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation

	glBindVertexArray(0); //unbind the vertexArrayObject so we can't change it

						  //cleanup
	glDisableVertexAttribArray(posLoc); //disable vertex attribute at index positionLocation
	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind array buffer

}


void initalizGridVertexBuffer()
{
	if (GridVertex.size() != 0)
	{
		glGenBuffers(1, &GridArrayObject);

		glBindBuffer(GL_ARRAY_BUFFER, GridArrayObject);
		glBufferData(GL_ARRAY_BUFFER, GridVertex.size() * sizeof(GLfloat), &GridVertex[0], GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		cout << "vertexDataBufferObject created OK! GLUint is: " << GridArrayObject << std::endl;

		GridVertexArrayObject();
	}
}
// tag::initializeVertexBuffer[]
void initVertexBuffer()
{
	if (TV.size() != 0)
	{
		glGenBuffers(1, &vertexDataBufferObject);

		glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObject);
		glBufferData(GL_ARRAY_BUFFER, TV.size() * sizeof(GLfloat), &TV[0], GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		cout << "vertexDataBufferObject created OK! GLUint is: " << vertexDataBufferObject << std::endl;

		initializeVertexArrayObject();
	}
}
// end::initializeVertexBuffer[]



void initBatteryBuffer()
{
	if (BV.size() != 0)
	{
		glGenBuffers(1, &vertexDataBufferObject);

		glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObject);
		glBufferData(GL_ARRAY_BUFFER, BV.size() * sizeof(GLfloat), &BV[0], GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		cout << "vertexDataBufferObject created OK! GLUint is: " << vertexDataBufferObject << std::endl;

		initializeBatteryArrayObject();
	}
}
/* file converter*/
vector<GLfloat> ConvertMyNumbers(vector<string> InputLines)
{
	vector<GLfloat> data;
	 //convert from string to float and push in to a list of glfloats
	for (int i = 0; i <= InputLines.size() - 5; i++)
	{
		currentLine = InputLines[i];
		data.push_back(stof(currentLine, 0));
	}

	return data;


	//		while (getline(inputfile, currentLine))
	//		{
	//			GLfloat Y = stof(currentLine, 0);
	//			vertexdata.push_back(Y);
	//		}
}
/*file opener */
vector<string> Loader(string Directory)
{
	ifstream myfile("");
	myfile = ifstream(Directory);
	vector<string> lines;

	if (myfile.is_open())
	{
		string line;
		while (!myfile.eof())
		{
			getline(myfile, line);
			lines.push_back(line);
		}

		myfile.close();
	}

	return lines;
}



/* pushes a set of 6 values in to the grid*/
void VertexPush(float x, float y, float c1, float c2, float c3)
{
	GridVertex.push_back(x - 4950.0f);
	GridVertex.push_back(y - 5000.0f);
	//GridVertex.push_back(z);
	GridVertex.push_back(c1);
	GridVertex.push_back(c2);
	GridVertex.push_back(c3);

}



void DrawColor()
{
	float CSize = 350.0f;

	for (int i = 0; i < Heat->C; i++)
	{
		for (int j = 0; j < Heat->C; j++)
		{
			float left = CSize * j;
			float top = CSize * i;
			float right = left + CSize;
			float bottom = top + CSize;

			//DrawBox(left, top, right, bottom);
			float color = Heat->getter(j, i) == 0 ? 0.0f : (float)Heat->getter(j, i) / 75.0f;

			VertexPush(left, top, color, 0, 0);
			VertexPush(right, top, color, 0, 0);
			VertexPush(left, bottom, color, 0, 0);
			VertexPush(right, top, color, 0, 0);
			VertexPush(right, bottom,  color, 0, 0);
			VertexPush(left, bottom, color, 0, 0);

			//add vertaxies for each point with there given color
			//mapping each block in the range's color
		}
	}
}

void loadAssets()
{

	TV = ConvertMyNumbers(Loader(movementFile));
	

	BV = ConvertMyNumbers(Loader(batteryFile));

	Heat = new HeatMapMaker(TV, 350.0f, 44);

	DrawColor();

	initialize(); //create GLSL Shaders, link into a GLSL program, and get IDs of attributes and variables

	initVertexBuffer(); //load data into a vertex buffer
	initBatteryBuffer();
	initalizGridVertexBuffer();

	cout << "Loaded Assets OK!\n";
}
// end::loadAssets[]

// tag::handleInput[]
void handleInput()
{
	//Event-based input handling
	//The underlying OS is event-based, so **each** key-up or key-down (for example)
	//generates an event.
	//  - https://wiki.libsdl.org/SDL_PollEvent
	//In some scenarios we want to catch **ALL** the events, not just to present state
	//  - for instance, if taking keyboard input the user might key-down two keys during a frame
	//    - we want to catch based, and know the order
	//  - or the user might key-down and key-up the same within a frame, and we still want something to happen (e.g. jump)
	//  - the alternative is to Poll the current state with SDL_GetKeyboardState

	SDL_Event event; //somewhere to store an event

					 //NOTE: there may be multiple events per frame
	while (SDL_PollEvent(&event)) //loop until SDL_PollEvent returns 0 (meaning no more events)
	{
		switch (event.type)
		{

		case SDL_QUIT:
			done = true; //set donecreate remote branch flag if SDL wants to quit (i.e. if the OS has triggered a close event,
						 //  - such as window close, or SIGINT
			break;

			//keydown handling - we should to the opposite on key-up for direction controls (generally)
		case SDL_KEYDOWN:

			//Keydown can fire repeatable if key-repeat is on.
			//  - the repeat flag is set on the keyboard event, if this is a repeat event
			//  - in our case, we're going to ignore repeat events
			//  - https://wiki.libsdl.org/SDL_KeyboardEvent
			if (!event.key.repeat)
				switch (event.key.keysym.sym)
				{
					//hit escape to exit
				case SDLK_ESCAPE:
					done = true;
					break;
				case SDLK_UP:
					PixelSize++;			//allowing fo the pixel density of the battery and tragectory to increase.. 
					break;
				case SDLK_DOWN:
					PixelSize--;			//..or decrease 
					break;
				}
			break;

		}
	}
}
// end::handleInput[]

// tag::updateSimulation[]

// end::updateSimulation[]

// tag::preRender[]
void preRender()
{
	glViewport(0, 0, 800, 800); //set viewpoint
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //set clear colour
	glClear(GL_COLOR_BUFFER_BIT); //clear the window (technical the scissor box bounds)
}
// end::preRender[]

// tag::render[]
void render()
{
	glUseProgram(theProgram); //installs the program object specified by program as part of current rendering state

	glUniform2f(offsetLocation, offset[0], offset[1]);
	glUniform2fv(offsetLocation, 1, offset);
	//load data to GLSL that **may** have changed
	//alternatively, use glUnivform2fv
	//glUniform2fv(colorLocation, 1, color); //Note: the count is 1, because we are setting a single uniform vec2 - https://www.opengl.org/wiki/GLSL_:_common_mistakes#How_to_use_glUniform

	//glUniform4f(colorLocation, color[2], color[0], color[0], 1.0f);

	glUniform4f(colorLocation, color[0], color[0], color[0], 1.0f);


	glBindVertexArray(GridArrayObject);

	glDrawArrays(GL_TRIANGLES, 0, GridVertex.size() / 2);



	glUniform4f(colorLocation, color[0], color[1], color[1], 1.0f);

	glBindVertexArray(TrajectoryVertexObject);


	glLineWidth(PixelSize);


	glDrawArrays(GL_LINE_STRIP, 0, TV.size() / 2);


	glUniform4f(colorLocation, color[2], color[2], color[2], 1.0f);


	glBindVertexArray(BatArrObject);

	glBufferData(GL_ARRAY_BUFFER, BV.size() * sizeof(GLfloat), &BV[0], GL_DYNAMIC_DRAW);

	glPointSize(PixelSize);

	glDrawArrays(GL_POINTS, 0, BV.size() / 2); //Draw something, using Triangles, and 3 vertices - i.e. one lonely triangle


	glBindVertexArray(0);

	glUseProgram(0); //clean up

}
// end::render[]

// tag::postRender[]
void postRender()
{
	SDL_GL_SwapWindow(win);; //present the frame buffer to the display (swapBuffers)
	frameLine += "Frame: " + std::to_string(frameCount++);
	cout << "\r" << frameLine << std::flush;
	frameLine = "";
}
// end::postRender[]

// tag::cleanUp[]
void cleanUp()
{
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);
	cout << "Cleaning up OK!\n";
}
// end::cleanUp[]

// tag::main[]
int main(int argc, char* args[])
{
	exeName = args[0];
	//setup
	//- do just once
	initialise();
	createWindow();

	createContext();

	initGlew();

	glViewport(0, 0, 800, 800); //should check what the actual window res is?

	SDL_GL_SwapWindow(win); //force a swap, to make the trace clearer

							//do stuff that only needs to happen once
							//- create shaders
							//- load vertex data
	loadAssets();

	while (!done) //loop until done flag is set)
	{
		handleInput(); // this should ONLY SET VARIABLES

		preRender();

		render(); // this should render the world state according to VARIABLES -

		postRender();

	}

	//cleanup and exit
	cleanUp();
	SDL_Quit();

	return 0;
}
// end::main[]
