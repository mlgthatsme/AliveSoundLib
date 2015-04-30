#include "ShaderProgram.h"

mgShaderProgram::mgShaderProgram()
{
}

mgShaderProgram::~mgShaderProgram()
{
	glDeleteShader(m_ProgramID);
}

void mgShaderProgram::UseProgram()
{
	glUseProgram(m_ProgramID);
	
	float t = glfwGetTime();

	SetUniform3f("_Time", glm::vec3(t, t * 2, t * 3));
}

void mgShaderProgram::InitFromCharBuffer(const char * vsSource, const char * fsSource)
{
	// Make sure that if we have loaded a shader program already
	// to delete the previous one
	if (isLoaded)
	{
		glDeleteShader(m_ProgramID);
	}

	int b_Success = GL_FALSE;

	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexShader, 1, (const char**)&vsSource, 0);
	glCompileShader(vertexShader);
	glShaderSource(fragmentShader, 1, (const char**)&fsSource, 0);
	glCompileShader(fragmentShader);

	m_ProgramID = glCreateProgram();
	glAttachShader(m_ProgramID, vertexShader);
	glAttachShader(m_ProgramID, fragmentShader);
	glLinkProgram(m_ProgramID);

	glGetProgramiv(m_ProgramID, GL_LINK_STATUS, &b_Success);
	if (b_Success == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		char * errorMessage = new char[maxLength];
		glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, &errorMessage[0]);

		printf("Error: Failed to link shader program\n");
		printf("Line: %s\n", errorMessage);

		delete[] errorMessage;
	}
	else
	{
		// Hey we've loaded a shader!
		isLoaded = true;
	}

	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
}

void mgShaderProgram::InitFromFile(const char * vertexShaderPath, const char * fragmentShaderPath)
{
	char * vertexSrc = mgFileManager::ReadFileToString(vertexShaderPath);
	char * fragmentSrc = mgFileManager::ReadFileToString(fragmentShaderPath);

	InitFromCharBuffer(vertexSrc, fragmentSrc);

	delete[] vertexSrc;
	delete[] fragmentSrc;
}

void mgShaderProgram::SetUniformMatrix4fv(char * param, const float * matrix)
{
	unsigned int paramID = glGetUniformLocation(m_ProgramID, param);
	glUniformMatrix4fv(paramID, 1, false, matrix);
}

void mgShaderProgram::SetUniform1f(char * param, float value)
{
	unsigned int paramID = glGetUniformLocation(m_ProgramID, param);
	glUniform1f(paramID, value);
}

void mgShaderProgram::SetUniform3f(char * param, glm::vec3 value)
{
	unsigned int paramID = glGetUniformLocation(m_ProgramID, param);
	glUniform3f(paramID, value.x, value.y, value.z);
}

void mgShaderProgram::SetUniform4f(char * param, glm::vec4 value)
{
	unsigned int paramID = glGetUniformLocation(m_ProgramID, param);
	glUniform4f(paramID, value.x, value.y, value.z, value.w);
}

void mgShaderProgram::SetUniform2f(char * param, glm::vec2 value)
{
	unsigned int paramID = glGetUniformLocation(m_ProgramID, param);
	glUniform2f(paramID, value.x, value.y);
}

void mgShaderProgram::SetUniform1i(char * param, int value)
{
	unsigned int paramID = glGetUniformLocation(m_ProgramID, param);
	glUniform1i(paramID, value);
}

void mgShaderProgram::SetUniform2i(char * param, int v1, int v2)
{
	unsigned int paramID = glGetUniformLocation(m_ProgramID, param);
	glUniform2i(paramID, v1, v2);
}

void mgShaderProgram::SetUniform3i(char * param, int v1, int v2, int v3)
{
	unsigned int paramID = glGetUniformLocation(m_ProgramID, param);
	glUniform3i(paramID, v1, v2, v3);
}

void mgShaderProgram::SetUniformMatrix4fv(char * param, int arrayLength, float * arrayPointer)
{
	int paramID = glGetUniformLocation(m_ProgramID, param);
	glUniformMatrix4fv(paramID, arrayLength, GL_FALSE,
		arrayPointer);
}

void mgShaderProgram::SetUniformMatrix4fv(char * param, int arrayLength, glm::mat4 * arrayPointer)
{
	int paramID = glGetUniformLocation(m_ProgramID, param);
	glUniformMatrix4fv(paramID, arrayLength, GL_FALSE,
		(float*)arrayPointer);
}
