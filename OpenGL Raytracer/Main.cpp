/* Ray Tracing! Compute Shader!
*
* Created by Seth Taddiken!
* MM/DD/YYYY: 3/24/2019
*
* ----- Resources Used -----------------------------------------------
* | - GLAD (OpenGL function-pointer-loading library)                 |
* | - GLFW (OpenGL Framework: Window-creating library)               |
* | - GLM (OpenGL Mathematics: Vector and matrix operations library) |
* -------------------------------------------------------------------- */

#include <string>
#include <cmath>
#include <iostream>
#include <tuple>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm\gtc\matrix_transform.hpp>

#include "lodepng.h"

#include "Shader.h"
#include "CompShader.h"
#include "OBJLoader.h"
#include <glm/ext/quaternion_geometric.inl>
#include <stdlib.h>
#include <algorithm>
#include <chrono>

const float GOLDEN_RATIO = 1.61803398875f;

const bool CPU = false;

float aspectRatio = GOLDEN_RATIO;

int width = 1024;
int height = width;// / aspectRatio;

float fov = 40; // in degrees
glm::vec3 bgColor = glm::vec3(1.0f, 0.5f, 0.25f);
glm::vec3 bgColor2 = glm::vec3(0.0f, 0.3f, 0.28f);

unsigned int frameCount;

// mouse coordinates, WARNING: mouseX and mouseY are only initialized when mouse first moves
bool mouseInit = false; // true when mouse has been moved at least once
float mouseX;
float mouseY;

// reflections toggling
bool reflections = false;
bool reflectionsPrimed = false;

float texCoords[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f
};

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

// Timing Logic
// ------------
float deltaTime = 0.0f;
float lastFrameTime = 0.0f;

bool initGLFW(GLFWwindow **window);
void processInput(GLFWwindow *window);
void rotateCamera(glm::vec3 about, float amount);

int nextPowerOfTwo(int x);

// callback functions
void mouse_callback(GLFWwindow *window, double xPos, double yPos);  // rotating camera / looking around
void scroll_callback(GLFWwindow *window, double xOffset, double yOffset); // scaling
void framebuffer_size_callback(GLFWwindow* window, int newWidth, int newHeight); // resizing window


std::vector<glm::vec4> computeMeshData(std::vector<glm::vec3>, std::vector<glm::vec3>);
glm::vec4 trace(glm::vec3, glm::vec3, glm::vec3, glm::vec3, std::vector<glm::vec4> *, int, int);

int main(void) {
	//load our obj file
	std::cout << "Loading obj file" << std::endl;

	OBJLoader loader;

	/*
	* AVAILABLE OBJ FILES
	* 
	*  cube	- basic cube	-	~ 12 tris  
	*  monkey	- monkey head	-	~ 500 tris
	*  skull	- human skull	-	~ 22k tris
	*  brain	- human brain	-	~ 380k tris
	*  car		- old car		-	~ 2m tris
	*/
	std::string file = "skull_new.obj";

	std::tuple<std::vector<glm::vec3>, std::vector<glm::vec2>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<glm::vec3>> objData = loader.loadObj(file);
	std::vector<glm::vec3> vertexData = std::get<0>(objData);
	std::vector<glm::vec3> indexData = std::get<3>(objData);

	std::cout << "Done loading obj file" << std::endl;

	//generate mesh from obj data
	std::cout << "Generating Mesh" << std::endl;
	std::vector<glm::vec4> meshData = computeMeshData(vertexData, indexData);
	std::cout << "Done generating mesh" << std::endl;

	//array of pixels we will output our render to
	std::vector<unsigned char> output_pixels;
	output_pixels.resize(width * height * 4);
	const char *outfile_name = "render.png";

	//setup our camera in the scene
	//cam.moveSpeed = 1.5;
	//cam.rotSpeed = 1;
	//cam.scaleSpeed = 0.1;
	//cam.zoomSpeed = 0.01;
	//cam.scale = 1;
	//cam.zoom = 1;


	//postion light and camera coordinates, rotate, then translate
	cam.lightPos = glm::vec3(0, 0, -2 - 1.402232f);
	cam.frontDir = glm::vec3(0, 1, 0);
	cam.rightDir = glm::vec3(-1, 0, 0);
	cam.upDir = glm::vec3(0, 0, -1);
	cam.pos = glm::vec3(0, 0, 0);
	//tateCamera(glm::vec3(0, 1, 0), 45);
	//rotateCamera(cam.rightDir, 90);
	cam.pos = glm::vec3(0, -2, 0);
	cam.lightPos = cam.pos - 0.5f/(float)asin(glm::radians(fov/2)) * cam.frontDir; // move the light-ray source behind the camera position such that the rays at either side of the width of the


	//int startTime = std::chrono::high_resolution_clock::now();

	if(CPU) {
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
		
		GLFWwindow *window;

		// initialize GLFW
		if (!initGLFW(&window))
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			return -1;
		}
		// set callback functions
		glfwSetCursorPosCallback(window, mouse_callback);
		glfwSetScrollCallback(window, scroll_callback);
		glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

		// initialize GLAD
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "Failed to initialize GLAD" << std::endl;
			return -1;
		}
		

		glViewport(0,0,width,height);

		// some info
		int work_grp_cnt[3];
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
		printf("max global (total) work group size x:%i y:%i z:%i\n", work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);
		int work_grp_size[3];
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
		printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n", work_grp_size[0], work_grp_size[1], work_grp_size[2]);
		int work_grp_inv;
		glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
		printf("max local work group invocations %i\n", work_grp_inv);

		

		// Initialize shaders
		CompShader compShader("comp.glsl");
		Shader shader("vert.glsl", "frag.glsl");

		// Create rectangle, this will be the screen and represents the camera lens surface

		// texture coordinates
		float vertices[] = {
			// positions    // texture coords
			1.0f,  1.0f,   1.0f, 1.0f,   // top right
			1.0f, -1.0f,   1.0f, 0.0f,   // bottom right
			-1.0f, -1.0f,   0.0f, 0.0f,   // bottom left
			-1.0f,  1.0f,   0.0f, 1.0f    // top left 
		};

		unsigned int indices[6] = {
			0, 1, 3,
			1, 2, 3
		};

		unsigned int VAO, VBO, EBO;

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(0));
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0); // unbind VAO

							  // Create texture
		unsigned int tex_output;
		glGenTextures(1, &tex_output);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_output);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		// OpenGL-Machine settings
		glClearColor(bgColor.r, bgColor.g, bgColor.b, 1.0f);		 // set clear color
		glEnable(GL_BLEND);											 // blend colors with alpha
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);			 // alpha settings
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // disable cursor





		/*
		* THIS CODE SHOULD LOAD TEXTURE
		*/
		/*
		std::string img_file = "uvgrid.png";

		std::cout << "Loading texture" << std::endl;
		std::vector<unsigned char> image;
		unsigned tex_width, tex_height;
		unsigned error = lodepng::decode(image, tex_width, tex_height, img_file);
		if(error != 0) {
			std::cout << "error " << error << ": " << lodepng_error_text(error) << std::endl;
			return 1;
		}

		//this code loads texture to GPU
		GLuint textureID;
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		std::cout << "Done loading texture" << std::endl;
		*/




		//setup SSBO for our scene geometry
		GLuint ssbo;
		glGenBuffers(1, &ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * meshData.size(), meshData.data(),  GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 1);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		// render loop
		//for (frameCount = 0; !glfwWindowShouldClose(window); frameCount++)
		//{
			// Timing Logic
			//float currentFrameTime = (float)glfwGetTime();
			//deltaTime = currentFrameTime - lastFrameTime; 
			//lastFrameTime = currentFrameTime;
			//if (frameCount % 100 == 0)
			//{
			//	printf("FPS: %f\n", 1.0f / deltaTime);
			//}
			//printf("%f, %f, %f\n", cam.frontDir.x, cam.frontDir.y, cam.frontDir.z);

			// Compute Shader
			/**
			* THIS CODE SHOULD UPDATE TEXTURE PASSED TO SHADER
			*/
			//GLuint texUnit = 2;
			//glActiveTexture(GL_TEXTURE0 + texUnit);
			//glBindTexture(GL_TEXTURE_2D, textureID);
			//glBindImageTexture(texUnit, textureID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
			compShader.use();
			//glUniform1i(glGetUniformLocation(compShader.ID, "tex"), texUnit);


			// set compute shader uniforms
			compShader.setFloat3("bgColor", bgColor2);
			compShader.setInt("numShapeVerts", meshData.size());
			// compShader.setFloat("aspectRatio", aspectRatio);
			// compShader.setBool("reflections", reflections);
			// compShader.setFloat("zoom", cam.zoom);
			// compShader.setFloat("scale", cam.scale);
			compShader.setFloat3("test", glm::vec3(1,0,1));
			compShader.setFloat3("camFront", cam.frontDir);
			compShader.setFloat3("camRight", cam.rightDir);
			compShader.setFloat3("camUp", cam.upDir);
			compShader.setFloat3("camPos", cam.pos);
			compShader.setFloat3("camLight", cam.lightPos);
			glDispatchCompute(width, height, 1);
			glFinish();
			// make sure writing to image has finished before read
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			glClear(GL_COLOR_BUFFER_BIT); // clear back buffer

			shader.use();
			glBindVertexArray(VAO); // bind vao
			//reset the texture to the output texture
			glActiveTexture(GL_TEXTURE0);
			// shader.setInt("tex",0);
			glBindTexture(GL_TEXTURE_2D, tex_output);
			glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // draw to back buffer
			glBindVertexArray(0); // unbind all VAO's

								  // user actions
			//glfwPollEvents();
			//processInput(window);
			
			// Swap front/back buffers
			glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &output_pixels[0]);
			glfwSwapBuffers(window);
		//}

		glDeleteBuffers(1, &EBO);
		glDeleteBuffers(1, &VBO);
		glDeleteVertexArrays(1, &VAO);

		glfwTerminate();
	}

	unsigned error = lodepng::encode(outfile_name, output_pixels, width, height);
	if (error) std::cout << "Encoding error" << error << ": " << lodepng_error_text(error) << std::endl;

	/*
	std::ofstream perf_file;
	perf_file.open("perf.txt");

	perf_file << file << "\n";
	perf_file << width << "x" << height << "\n";
	perf_file << meshData.size() / 3 << " tris\n";
	*/

	
	
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
	else backFacing = false;

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
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);				   // set maximum OpenGL version requirement to OpenGL 3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);				   // set minimum OpenGL version requirement to OpenGL 3
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // tell GLWF that we want to use the core profile of OpenGL
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);					   // window is resizeable

	*window = glfwCreateWindow(width, height, "Ray Tracing", 0, NULL);

	// Test for successful window creation
	if (window == NULL) return false;

	glfwMakeContextCurrent(*window);

	return true;
}

// This function is called every frame, updates values based on key states
void processInput(GLFWwindow *window)
{
	// exit program
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	// move forward/backward
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		cam.pos += cam.frontDir * cam.moveSpeed * cam.scale * deltaTime;
	}
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		cam.pos -= cam.frontDir * cam.moveSpeed * cam.scale * deltaTime;
		if (!(frameCount % 100))
			printf("cam dist: %f\n", glm::length(cam.pos));
	}
	// move speed
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
	{
		cam.moveSpeed *= 1.05;
	}
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS && cam.moveSpeed > 0)
	{
		cam.moveSpeed /= 1.05;
	}
	// pan vertical
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		cam.pos += cam.upDir * cam.moveSpeed * cam.scale * deltaTime;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		cam.pos -= cam.upDir * cam.moveSpeed * cam.scale * deltaTime;
	}
	// pan horizontal
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		cam.pos += cam.rightDir * cam.moveSpeed * cam.scale * deltaTime;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		cam.pos -= cam.rightDir * cam.moveSpeed * cam.scale * deltaTime;
	}
	// camera roll
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		rotateCamera(cam.frontDir, cam.rotSpeed * 1.5f);
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		rotateCamera(-cam.frontDir, cam.rotSpeed * 1.5f);
	}
	// camera zoom
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
	{
		cam.zoom *= 1 + cam.zoomSpeed;
		printf("cam zoom: %f\n", cam.zoom);
	}
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
	{
		cam.zoom /= 1 + cam.zoomSpeed;
		printf("cam zoom: %f\n", cam.zoom);
	}
	// reflections toggling
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
	{
		reflectionsPrimed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_R) != GLFW_PRESS && reflectionsPrimed)
	{
		reflections = !reflections;
		reflectionsPrimed = false;
	}
}

void rotateCamera(glm::vec3 about, float amount)
{
	glm::mat4 rot = glm::mat4(1.0f);
	rot = glm::rotate(rot, amount * deltaTime, glm::vec3(about));

	// WARNING this is implicitly casting vec4's to vec3's
	cam.frontDir = glm::vec4(cam.frontDir, 1.0f) * rot;
	cam.rightDir = glm::vec4(cam.rightDir, 1.0f) * rot;
	cam.upDir = glm::vec4(cam.upDir, 1.0f) * rot;
}

void mouse_callback(GLFWwindow *window, double xPos, double yPos)
{
	// prevent abrupt camera movement on first mouse movement by considering mouse's initial position
	if (!mouseInit)
	{
		mouseX = xPos;
		mouseY = yPos;
		mouseInit = true;
	}

	float xOffset = xPos - mouseX;
	float yOffset = yPos - mouseY; // mouse coordinates are reversed on y-axis ... ?

	mouseX = xPos;
	mouseY = yPos;

	// rotateCamera(cam.upDir, xOffset);
	rotateCamera(glm::vec3(0,1,0), xOffset);
	rotateCamera(cam.rightDir, yOffset);
}

void scroll_callback(GLFWwindow *window, double xOffset, double yOffset)
{
	if (yOffset < 0)
	{
		cam.scale *= 1 + cam.scaleSpeed;
	}

	if (yOffset > 0)
	{
		cam.scale /= 1 + cam.scaleSpeed;
	}

	// THIS DOESN'T WORK IDK WHY
	// cam.pos /= cam.scale;
}

void framebuffer_size_callback(GLFWwindow* window, int newWidth, int newHeight)
{
	height = newHeight;
	width = newWidth;
	aspectRatio = (float)width/height;
	glViewport(0, 0, width, height);
} 

int nextPowerOfTwo(int x) {
	x--;
	x |= x >> 1; // handle 2 bit numbers
	x |= x >> 2; // handle 4 bit numbers
	x |= x >> 4; // handle 8 bit numbers
	x |= x >> 8; // handle 16 bit numbers
	x |= x >> 16; // handle 32 bit numbers
	x++;
	return x;
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
