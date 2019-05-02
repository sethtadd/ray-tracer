/* APPM Matrix Methods Project - Moller Trumbore Algorithm for Ray Tracing
 *
 * Made by Seth Taddiken and Nick Elsasser!
 * MM/DD/YYYY: 3/24/2019
 *
 * ----- Resources Used -----------------------------------------------
 * | - GLAD (OpenGL function-pointer-loading library)                 |
 * | - GLFW (OpenGL Framework: Window-creating library)               |
 * | - GLM (OpenGL Mathematics: Vector and matrix operations library) |
 * | - LodePNG (PNG image encoder/decoder)                            |
 * --------------------------------------------------------------------
 */

#define _CRT_SECURE_NO_WARNINGS


#include <string>
#include <cmath>
#include <iostream>
#include <tuple>
#include <algorithm>
#include <chrono>

#include <stdlib.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "lodepng.h"
#include "CompShader.h"
#include "OBJLoader.h"


bool CPU = false;

int width = 1024;
int height = width;// / aspectRatio;

float fov = 40; // in degrees
glm::vec3 bgColor = glm::vec3(1.0f, 0.5f, 0.25f);
glm::vec3 bgColor2 = glm::vec3(0.0f, 0.3f, 0.28f);

// rayBounces = 0 will render shapes with inverse-square lighting
// rayBounces = 1 will render shapes with inverse-square lighting AND reflections
// rayBounces = 2 will render shapes with inverse-square lighting AND reflections AND reflections in the first reflections
// rayBounces > 2 will literally do the same thing as rayBounces = 2 (the code won't do more)
int rayBounces = 0;

// Camera
struct Camera
{
	float moveSpeed;
	float rotSpeed;
	float scaleSpeed;
	float scale;
	float zoom;
	float zoomSpeed;

	glm::vec3 lightPos;

	glm::vec3 pos;
	glm::vec3 frontDir;
	glm::vec3 rightDir;
	glm::vec3 upDir;
} cam;

// initializes GLFW
bool initGLFW(GLFWwindow **window);

// rotates the camera object
void rotateCamera(glm::vec3 about, float amount);

// returns 'file' name from user has user set CPU bool (also validates user input)
std::string userInput();

// generates a mesh from vertices and vertex indices
std::vector<glm::vec4> computeMeshData(std::vector<glm::vec3>, std::vector<glm::vec3>);

// indexed normals // broken!
std::vector<glm::vec4> computeNormalData(std::vector<glm::vec3> normalData, std::vector<glm::vec3> normalIndexData);

// CPU ray tracing function
glm::vec4 trace(glm::vec3, glm::vec3, glm::vec3, glm::vec3, std::vector<glm::vec4> *, int, int);

int main(void) {
	/*
	* AVAILABLE OBJ FILES
	* 
	*  cube	- basic cube	-	~ 12 tris  
	*  monkey	- monkey head	-	~ 500 tris
	*  skull	- human skull	-	~ 22k tris
	*  brain	- human brain	-	~ 380k tris
	*  car		- old car		-	~ 2m tris
	*/

	// prompt user for file and processor selection
	std::string file = userInput();

	// load our obj file
	std::cout << "Loading obj file" << std::endl;

	OBJLoader loader;

	std::tuple<std::vector<glm::vec3>, std::vector<glm::vec2>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>> objData = loader.loadObj(file);
	std::vector<glm::vec3> vertexData = std::get<0>(objData);
	std::vector<glm::vec3> indexData = std::get<3>(objData);
	std::vector<glm::vec3> normalData = std::get<2>(objData);
	std::vector<glm::vec3> normalIndexData = std::get<5>(objData);

	std::cout << "Done loading obj file" << std::endl;

	// generate mesh from obj data
	std::cout << "Generating Mesh" << std::endl;
	std::vector<glm::vec4> meshData = computeMeshData(vertexData, indexData);
	std::cout << "Done generating mesh" << std::endl;

	std::cout << "Generating Normal Data" << std::endl;
	std::vector<glm::vec4> normals = computeNormalData(normalData, normalIndexData);
	std::cout << "Done generating Normal Data" << std::endl;

	// Load file and decode image.
	std::vector<unsigned char> texture;
	unsigned width, height;
	unsigned tex_error = lodepng::decode(texture, width, height, "color.png");
	if(tex_error != 0) {
		std::cout << "error " << tex_error << ": " << lodepng_error_text(tex_error) << std::endl;
		return 1;
	}

	// array of pixels we will output our render to
	std::vector<unsigned char> output_pixels;
	output_pixels.resize(width * height * 4);
	const char *outfile_name = "render.png";

	// postion light and camera coordinates, rotate, then translate

	// these need to be set to determine viewing specifics

	// // for scene.obj
	// cam.pos = glm::vec3(-12, 7, -12);
	// cam.frontDir = glm::normalize(glm::vec3(1, -0.4f, 1));
	// cam.rightDir = glm::normalize(glm::cross(cam.frontDir, glm::vec3(0,-1,0)));

	// for reflect.obj
	cam.pos = glm::vec3(40, 10, -8);
	cam.frontDir = glm::normalize(glm::vec3(-1, -0.15, 0.2));
	cam.rightDir = glm::normalize(glm::cross(cam.frontDir, glm::vec3(0,-1,0)));


	// cam.rightDir = glm::vec3(-1, 0, 0);

	// this can be derived
	cam.upDir = glm::cross(cam.rightDir, cam.frontDir);
	// move the light-ray source behind the camera position such that we get parallax perspective
	cam.lightPos = cam.pos - 0.5f/(float)asin(glm::radians(fov/2)) * cam.frontDir;

	//int startTime = std::chrono::high_resolution_clock::now();

	if(CPU) {
		std::cout << "Rendering image with CPU, this will be a while...\n";

		#pragma omp parallel for
		for(int x = 0; x < width; x++) {
			for(int y = 0; y < height; y++) {
				glm::vec4 outcolor = trace(cam.pos, cam.upDir, cam.rightDir, cam.lightPos, &meshData, x, y);
				int index = x * height * 4 + y * 4 + 0;
				int size = output_pixels.size();
				output_pixels[x*height * 4 + y*4 + 0] = (unsigned char)(outcolor.r * 255);
				output_pixels[x*height * 4 + y*4 + 1] = (unsigned char)(outcolor.g * 255);
				output_pixels[x*height * 4 + y*4 + 2] = (unsigned char)(outcolor.b * 255);
				output_pixels[x*height * 4 + y*4 + 3] = (unsigned char)(outcolor.a * 255);
			}
		}
	} else {
		std::cout << "Rendering image with GPU, this shouldn't take long...\n";

		// initialize GLFW
		GLFWwindow *window;
		if (!initGLFW(&window))
		{
			std::cout << "Failed to initialize GLFW" << std::endl;
			return -1;
		}

		// initialize GLAD
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "Failed to initialize GLAD" << std::endl;
			return -1;
		}

		// // some info
		// std::cout << "----------\n";
		// std::cout << "GPU info:\n";
		// int work_grp_cnt[3];
		// glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
		// glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
		// glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
		// printf("max global (total) work group size x:%i y:%i z:%i\n", work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);
		// int work_grp_size[3];
		// glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
		// glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
		// glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
		// printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n", work_grp_size[0], work_grp_size[1], work_grp_size[2]);
		// int work_grp_inv;
		// glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
		// printf("max local work group invocations %i\n", work_grp_inv);
		// std::cout << "----------\n";


		// Initialize compute shader
		CompShader compShader("comp.glsl");

		// Create texture for compute shader to write to
		GLuint tex_output;
		glGenTextures(1, &tex_output);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_output);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		// create SSBO to send geometry/vertex data to compute shader
		GLuint ssbo;
		glGenBuffers(1, &ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * meshData.size(), meshData.data(),  GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo); // bind
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		// // create SSBO to send vertex-normal data to compute shader
		GLuint ssbo2;
		glGenBuffers(1, &ssbo2);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo2);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * normals.size(), normals.data(),  GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo2);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo2); // bind
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		glEnable(GL_TEXTURE_2D);
		GLuint texID;
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 1024, 1024, 0, GL_RGBA, GL_FLOAT, &texture[0]);
		glBindImageTexture(1, texID, 1, GL_FALSE, 1, GL_READ_ONLY, GL_RGBA32F);
		glActiveTexture(GL_TEXTURE0 + 1);
		compShader.use();

		// set compute shader uniforms
		compShader.setFloat3("bgColor", bgColor2);
		compShader.setInt("numShapeVerts", meshData.size());
		compShader.setInt("rayBounces", rayBounces);
		compShader.setFloat3("test", glm::vec3(1,0,1));
		compShader.setFloat3("camFront", cam.frontDir);
		compShader.setFloat3("camRight", cam.rightDir);
		compShader.setFloat3("camUp", cam.upDir);
		compShader.setFloat3("camPos", cam.pos);
		compShader.setFloat3("camLight", cam.lightPos);
		glDispatchCompute(width, height, 1);
		glFinish();
		glActiveTexture(GL_TEXTURE0);

		// make sure writing to image has finished before read
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// read data from compute shader
		GLuint pbo;
		glGenBuffers(1, &pbo);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
		glBufferData(GL_PIXEL_PACK_BUFFER, width*height*sizeof(glm::vec4), NULL, GL_DYNAMIC_READ);

		glBindTexture(GL_TEXTURE_2D, tex_output);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)0);

		void* ptr = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, width*height*4*sizeof(unsigned char), GL_MAP_READ_BIT);
		memcpy(&output_pixels[0], ptr, width*height*4*sizeof(unsigned char));

		// clean
		glDeleteBuffers(1, &pbo);
		glDeleteBuffers(1, &ssbo);

		glfwTerminate();
	}

	std::cout << "Render complete! Writing image file...\n";

	// write render data to png
	unsigned error = lodepng::encode(outfile_name, output_pixels, width, height);
	if (error) 
	{
		std::cout << "Encoding error" << error << ": " << lodepng_error_text(error) << std::endl;
		return -1;
	}
	
	std::cout << "Image file written to 'render.png'! Opening file...\n";
	
	// open file
	system("render.png");
}

bool rayTriDist(glm::vec3 orig, glm::vec3 dir, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, float *t) {
	float epsilon = 0.000001;
	glm::vec3 v0v1 = v1 - v0;
	glm::vec3 v0v2 = v2 - v0;
	glm::vec3 pvec = cross(dir,v0v2);
	float det = glm::dot(v0v1,pvec);

	// assume ray and triangle parallel if triple-scalar-product (det) small enough FRONT CULLING
	if (abs(det) < epsilon) return false;

	glm::vec3 tvec = orig - v0;
	float u = dot(tvec,pvec) / det;
	if (u < 0 || u > 1) return false;

	glm::vec3 qvec = cross(tvec,v0v1);
	float v = dot(dir,qvec) / det;
	if (v < 0 || u + v > 1) return false;

	*t = dot(v0v2,qvec) / det;
	if (*t < 0) return false;

	return true;
}

bool rayTriInter(glm::vec3 orig, glm::vec3 dir, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, float* t, float* u, float* v, bool* backFacing) {
	float epsilon = 0.000001;
	glm::vec3 v0v1 = v1 - v0;
	glm::vec3 v0v2 = v2 - v0;
	glm::vec3 pvec = cross(dir,v0v2);
	float det = dot(v0v1,pvec);

	// assume ray and triangle parallel if triple-scalar-product (det) small enough FRONT CULLING
	if (abs(det) < epsilon) return false;

	// determine if triangle is back-facing
	if (det < epsilon) *backFacing = true;
	else *backFacing = false;

	glm::vec3 tvec = orig - v0;
	*u = dot(tvec,pvec) / det;
	if (*u < 0 || *u > 1) return false;

	glm::vec3 qvec = cross(tvec,v0v1);
	*v = dot(dir,qvec) / det;
	if (*v < 0 || *u + *v > 1) return false;

	*t = dot(v0v2,qvec) / det;
	if (*t < 0) return false;

	return true;
}

float maxf(float f1, float f2) {
	return ((f1 > f2) ? f1 : f2);
}

glm::vec4 trace(glm::vec3 origin, glm::vec3 upDir, glm::vec3 rightDir, glm::vec3 lightPos, std::vector<glm::vec4> *meshData, int x, int y) {
	float normalX = ((float)x / ((float)width - 1)) - 0.5;
	float normalY = ((float)y / ((float)height - 1)) - 0.5;

	glm::vec3 orig = normalX * rightDir + normalY * upDir + origin;
	glm::vec3 dir = glm::normalize(orig - lightPos);

	bool inter;
	int i; 
	int i_closest = 0;
	bool backFacing;
	float t = 1000000, t_temp;
	float u;
	float v;
	// ASSUME TRIANGLES ARE GROUPS OF 3 VERTICES
	for (i = 0; i < meshData->size() - 2; i += 3)
	{
		inter = rayTriDist(orig, dir, glm::vec3((*meshData)[i]), glm::vec3((*meshData)[i+1]), glm::vec3((*meshData)[i+2]), &t_temp);
		if (inter && t_temp < t)
		{
			t = t_temp;
			i_closest = i;
		}
	}


	// again at index of closest intersection to orig
	inter = rayTriInter(orig, dir, glm::vec3((*meshData)[i_closest]), glm::vec3((*meshData)[i_closest+1]), glm::vec3((*meshData)[i_closest+2]), &t, &u, &v, &backFacing);
	// inter stores if intersection occured
	// intersection is stored at t,u,v
	// index of first of 3 vertices of intersection with shape is i
	if (inter)
	{
		glm::vec4 col = glm::vec4(glm::vec3(1, 1, 1) / maxf(t*t, 1.0f), 1.0f);
		if (backFacing) {
			return col;
		}
		else {return glm::vec4(0, 0, 0, 1);}
	}
	else {return glm::vec4(bgColor, 1.0f);}
}

// More a convenience, this function encapsulates all of the initialization proceedures for creating a GLFW window
bool initGLFW(GLFWwindow **window)
{
	if ( glfwInit() )
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);				   // set maximum OpenGL version requirement to OpenGL 3
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);				   // set minimum OpenGL version requirement to OpenGL 3
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // tell GLWF that we want to use the core profile of OpenGL
		glfwWindowHint(GLFW_VISIBLE, GL_FALSE);						   // make window invisible

		*window = glfwCreateWindow(width, height, "Ray Tracing", 0, NULL);

		// Test for successful window creation
		if (window == NULL) return false;

		glfwMakeContextCurrent(*window);

		return true;
	}
	else return false;
	
}

void rotateCamera(glm::vec3 about, float amount)
{
	glm::mat4 rot = glm::mat4(1.0f);
	rot = glm::rotate(rot, amount, glm::vec3(about));

	// WARNING this is implicitly casting vec4's to vec3's
	cam.frontDir = glm::vec4(cam.frontDir, 1.0f) * rot;
	cam.rightDir = glm::vec4(cam.rightDir, 1.0f) * rot;
	cam.upDir = glm::vec4(cam.upDir, 1.0f) * rot;
}

std::string userInput()
{
	std::string file;
	std::string gpucpu;
	std::string bounces;
	bool validInput1 = false;
	bool validInput2 = false;
	bool validInput3 = false;

	while (!validInput1)
	{
		// get input
		std::cout << "\nFile to load? (Type 'file_name.obj'): ";
		std::cin >> file;

		// check if file is '.obj'
		if (file.find(".obj", file.length() - 4) == std::string::npos) std::cout << "File must be object format (.obj)";
		// check if file exists
		else if (FILE * test =  fopen(file.c_str(), "r")) { fclose(test); validInput1 = true; }
		else std::cout << "File " << "\"" << file << "\"" << " doesn't exist!\n";
	}
	
	while (!validInput2)
	{
		// get input
		std::cout << "Render with GPU or CPU? (Type 'g' or 'c'): ";
		std::cin >> gpucpu;

		// check if gpu/cpu input is valid
		if ( (gpucpu.length() == 1) && (gpucpu[0] == 'g' || gpucpu[0] == 'G' || gpucpu[0] == 'c' || gpucpu[0] == 'C') ) validInput2 = true;
		else std::cout << "Enter 'g' for GPU or 'c' for CPU\n";
	}

	while (!validInput3)
	{
		// get input
		std::cout << "0 ray bounces == shadows, no relfections\n";
		std::cout << "1 ray bounce == shadows and relfections\n";
		std::cout << "2 ray bounces == shadows and deep relfections\n";
		std::cout << "Number of ray bounces? (Type a number between 0 to 2 inclusive): ";
		std::cin >> bounces;

		if ( (bounces.length() == 1) && isdigit(bounces[0]) && (stoi(bounces) >= 0) && (stoi(bounces) <= 2) ) validInput3 = true;
		else std::cout << "Enter a digit between 0 to 2 inclusive\n";
	}
	
	CPU = (gpucpu[0] == 'c' || gpucpu[0] == 'C');
	rayBounces = stoi(bounces);
	return file;
}

std::vector<glm::vec4> computeMeshData(std::vector<glm::vec3> vertexData, std::vector<glm::vec3> indexData) {
	std::vector<glm::vec4> mesh;

	for(int i = 0; i < indexData.size(); i++) {
		glm::vec3 v1 = vertexData[(int)indexData[i].x];
		glm::vec3 v2 = vertexData[(int)indexData[i].y];
		glm::vec3 v3 = vertexData[(int)indexData[i].z];

		glm::vec4 v41(v1.x, v1.y, v1.z, 0);
		glm::vec4 v42(v2.x, v2.y, v2.z, 0);
		glm::vec4 v43(v3.x, v3.y, v3.z, 0);


		mesh.push_back(v41);
		mesh.push_back(v42);
		mesh.push_back(v43);
	}

	return mesh;
}

// broken!!!
std::vector<glm::vec4> computeNormalData(std::vector<glm::vec3> normalData, std::vector<glm::vec3> normalIndexData) {
	std::vector<glm::vec4> data;

	
	int minIndex = normalIndexData[0].x;
	int maxIndex = normalIndexData[0].x;

	for(int i = 0; i < normalIndexData.size(); i++) {
		for(int j = 0; j < 3; j++) {
			minIndex = ((normalIndexData[i][j] < minIndex) ? normalIndexData[i][j] : minIndex);
			maxIndex = ((normalIndexData[i][j] > maxIndex) ? normalIndexData[i][j] : maxIndex);
		}
	}
	

	for(int i = 0; i < normalIndexData.size(); i++) {
		glm::vec3 v1 = normalData[(int)normalIndexData[i].x];
		glm::vec3 v2 = normalData[(int)normalIndexData[i].y];
		glm::vec3 v3 = normalData[(int)normalIndexData[i].z];

		glm::vec4 v41(v1.x, v1.y, v1.z, 0);
		glm::vec4 v42(v2.x, v2.y, v2.z, 0);
		glm::vec4 v43(v3.x, v3.y, v3.z, 0);


		data.push_back(v41);
		data.push_back(v42);
		data.push_back(v43);
	}

	return data;
}