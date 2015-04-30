#pragma once
#include "gl_core_4_4.h"
#include "fwd.hpp""
#include "glm.hpp"
#include "ext.hpp"
#include "FileManager.h"
#include "glfw3.h"

using namespace glm;

class mgShaderProgram
{
public:
	mgShaderProgram();
	~mgShaderProgram();

	unsigned int m_ProgramID;

	void UseProgram();
	void InitFromFile(const char * vertexShaderPath, const char * fragmentShaderPath);
	void InitFromCharBuffer(const char * vertexShader, const char * fragmentShader);

	void SetUniformMatrix4fv(char * param, const float *);

	// int
	void SetUniform1i(char * param, int value);
	void SetUniform2i(char * param, int v1, int v2);
	void SetUniform3i(char * param, int v1, int v2, int v3);

	// float
	void SetUniform1f(char * param, float value);
	void SetUniform2f(char * param, vec2 value);
	void SetUniform3f(char * param, vec3 value);
	void SetUniform4f(char * param, vec4 value);

	void SetUniformMatrix4fv(char * param, int arrayLength, float * arrayPointer);
	void SetUniformMatrix4fv(char * param, int arrayLength, mat4 * arrayPointer);
private:
	bool isLoaded = false;
};