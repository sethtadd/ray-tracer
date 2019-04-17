#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#include "CompShader.h"

// Constructor
CompShader::CompShader(const GLchar* compPath)
{
	// Make sure ifstream objects can throw exceptions
	compShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		// Open shader source code file
		compShaderFile.open(compPath);

		// Read file's buffer contents into string stream
		compShaderStream << compShaderFile.rdbuf();

		// Close file handler
		compShaderFile.close();

		// Send string stream data to string buffer in string format
		compSourceCodeBuffer = compShaderStream.str();
	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR::COMPUTE_SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
	}

	// Send string buffer's content to char*'s as c_strings
	compSourceCode = compSourceCodeBuffer.c_str();

	// ---------- Compute Shader ---------- //
	compShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(compShader, 1, &compSourceCode, NULL);
	glCompileShader(compShader);

	// Error checking
	glGetShaderiv(compShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(compShader, 512, NULL, infoLog);
		std::cout << "ERROR::COMPUTE_SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// ---------- End ---------- //

	// ---------- Shader Program ---------- //
	ID = glCreateProgram();
	glAttachShader(ID, compShader);
	glLinkProgram(ID);

	// Error checking
	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		std::cout << "ERROR::COMPUTE_SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	// ---------- End ---------- //

	glDeleteShader(compShader);
}

// Set shader to current
void CompShader::use()
{
	glUseProgram(ID);
}

// Set shader uniforms
// -------------------
void CompShader::setBool(const std::string &name, bool value) const
{
	glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void CompShader::setInt(const std::string &name, int value) const
{
	glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void CompShader::setFloat(const std::string &name, float value) const
{
	glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void CompShader::setFloat3(const std::string &name, float value1, float value2, float value3) const
{
	glUniform3f(glGetUniformLocation(ID, name.c_str()), value1, value2, value3);
}
void CompShader::setFloat3(const std::string &name, glm::vec3 val) const
{
	glUniform3f(glGetUniformLocation(ID, name.c_str()), val.x, val.y, val.z);
}

void CompShader::setFloat4(const std::string &name, float value1, float value2, float value3, float value4) const
{
	glUniform4f(glGetUniformLocation(ID, name.c_str()), value1, value2, value3, value4);
}

void CompShader::setMatrix3f(const std::string &name, glm::mat3 matrix) const
{
	glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
}

void CompShader::setMatrix4f(const std::string &name, glm::mat4 matrix) const
{
	glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
}

void CompShader::setDouble(const std::string &name, double value) const
{
	glUniform1d(glGetUniformLocation(ID, name.c_str()), value);
}