#pragma once

// Third Party
#include "gl_core_4_4.h"
#include "glfw3.h"
#include "AntTweakBar.h"

// Standard
#include <stdio.h>
#include <vector>
#include <iostream>

using namespace std;

class mgApplication
{
public:
	mgApplication();
	~mgApplication();

	void Run();

	virtual int Start();
	virtual int Shutdown();
	virtual int Update();
	virtual int Draw();
	virtual int BeginUpdate();
	virtual void BeginDraw();
	virtual void EndUpdate();
	virtual void EndDraw();

	void SetWindowTitle(const char * sTitle);

	float m_PreviousTime;
	float m_Tick = 0.0f;

	// Global Variables
	static GLFWwindow * p_Window;
	static int i_WindowWidth;
	static int i_WindowHeight;
	static bool b_WindowFocused;
};

