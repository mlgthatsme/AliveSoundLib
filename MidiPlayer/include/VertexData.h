#pragma once

#include "gl_core_4_4.h"
#include "glfw3.h"
#include "glm.hpp"
#include "ext.hpp"

using namespace glm;

class mgVertexData
{
public:

	mgVertexData();
	~mgVertexData();
	void SetVertexData(vec3 * vertices, int count);
	void Render(int mode);

	static mgVertexData * CreateCircle(int edges);
	static mgVertexData * CreateCircleFilled(int edges);

	unsigned int vao = 0;
	unsigned int vbo = 0;
	int vertexCount = 0;
};