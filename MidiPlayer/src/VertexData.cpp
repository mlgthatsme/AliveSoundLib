#include "VertexData.h"

mgVertexData::mgVertexData()
{

}

mgVertexData::~mgVertexData()
{
	if (vbo != 0)
	{
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
	}
}

void mgVertexData::SetVertexData(vec3 * vertices, int count)
{
	if (vbo != 0)
	{
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
	}

	vertexCount = count / sizeof(vec3);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, count, vertices, GL_STATIC_DRAW);

	// Get the location of the attributes that enters in the vertex shader
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
}

void mgVertexData::Render(int mode)
{
	glBindVertexArray(vao);
	glDrawArrays(mode, 0, vertexCount);
}

mgVertexData * mgVertexData::CreateCircle(int edges)
{
	edges++;

	mgVertexData * mesh = new mgVertexData();

	vec3 * verts = new vec3[edges];

	float radion90Degrees = glm::pi<float>() / 4.0f;

	float piDevide = (glm::pi<float>() * 2) / ((float)edges - 1);
	for (int i = 0; i < edges; i++)
	{
		verts[i] = vec3(sinf((i * piDevide) + radion90Degrees), cosf((i * piDevide) + radion90Degrees), 0) * 0.5f;
	}

	mesh->SetVertexData(verts, sizeof(vec3) * edges);

	delete[] verts;

	return mesh;
}

mgVertexData * mgVertexData::CreateCircleFilled(int edges)
{
	mgVertexData * mesh = new mgVertexData();

	vec3 * verts = new vec3[edges * 2];

	float radion90Degrees = glm::pi<float>() / 4.0f;
	float piDevide = (glm::pi<float>() * 2) / ((float)edges - 1) / 2.0f;
	for (int i = 0; i < edges * 2; i+=2)
	{
		verts[i] = vec3(sinf((i * piDevide) + radion90Degrees), cosf((i * piDevide) + radion90Degrees), 0) * 0.5f;
		verts[i + 1] = vec3();
	}

	mesh->SetVertexData(verts, sizeof(vec3) * edges * 2);

	delete[] verts;

	return mesh;
}