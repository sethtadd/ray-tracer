#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glad\glad.h>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#include "Shader.h"

// Constructor
Shader::Shader(const GLchar* vertexPath, const GLchar* fragmentPath)
{
	// Make sure ifstream objects can throw exceptions
	vertexShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fragmentShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		// Open shader source code files
		vertexShaderFile.open(vertexPath);
		fragmentShaderFile.open(fragmentPath);

		// Read files' buffer contents into string streams
		vertexShaderStream << vertexShaderFile.rdbuf();
		fragmentShaderStream << fragmentShaderFile.rdbuf();

		// Close file handlers
		vertexShaderFile.close();
		fragmentShaderFile.close();

		// Send string stream data to string buffers in string format
		vertexSourceCodeBuffer = vertexShaderStream.str();
		fragmentSourceCodeBuffer = fragmentShaderStream.str();
	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
	}

	// Send string buffers' content to char*'s as c_strings
	vertexSourceCode = vertexSourceCodeBuffer.c_str();
	fragmentSourceCode = fragmentSourceCodeBuffer.c_str();

	// ---------- Vertex Shader ---------- //
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSourceCode, NULL);
	glCompileShader(vertexShader);

	// Error checking
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// ---------- End ---------- //

	// ---------- Fragment Shader ---------- //
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSourceCode, NULL);
	glCompileShader(fragmentShader);

	// Error checking
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// ---------- End ---------- //

	// ---------- Shader Program ---------- //
	ID = glCreateProgram();
	glAttachShader(ID, vertexShader);
	glAttachShader(ID, fragmentShader);
	glLinkProgram(ID);

	// Error checking
	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	// ---------- End ---------- //

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

// Set shader to current
void Shader::use()
{
	glUseProgram(ID);
}

// Set shader uniforms
// -------------------
void Shader::setBool(const std::string &name, bool value) const
{
	glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string &name, int value) const
{
	glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) const
{
	glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat3(const std::string &name, float value1, float value2, float value3) const
{
	glUniform3f(glGetUniformLocation(ID, name.c_str()), value1, value2, value3);
}
void Shader::setFloat3(const std::string &name, glm::vec3 val) const
{
	glUniform3f(glGetUniformLocation(ID, name.c_str()), val.x, val.y, val.z);
}

void Shader::setFloat4(const std::string &name, float value1, float value2, float value3, float value4) const
{
	glUniform4f(glGetUniformLocation(ID, name.c_str()), value1, value2, value3, value4);
}

void Shader::setMatrix3f(const std::string &name, glm::mat3 matrix) const
{
	glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::setMatrix4f(const std::string &name, glm::mat4 matrix) const
{
	glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::setDouble(const std::string &name, double value) const
{
	glUniform1d(glGetUniformLocation(ID, name.c_str()), value);
}