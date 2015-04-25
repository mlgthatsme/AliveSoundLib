#include "Application.h"

GLFWwindow* mgApplication::p_Window = nullptr;
int			mgApplication::i_WindowWidth = 1920;
int			mgApplication::i_WindowHeight = 1080;
bool		mgApplication::b_WindowFocused = true;

mgApplication::mgApplication()
{
}


mgApplication::~mgApplication()
{
}

// Callbacks

void OnMouseButton(GLFWwindow*, int b, int a, int m) {
	TwEventMouseButtonGLFW(b, a);
}
void OnMousePosition(GLFWwindow*, double x, double y) {
	TwEventMousePosGLFW((int)x, (int)y);
}
void OnMouseScroll(GLFWwindow*, double x, double y) {
	TwEventMouseWheelGLFW((int)y);
}
void OnKey(GLFWwindow*, int k, int s, int a, int m) {
	TwEventKeyGLFW(k, a);
}
void OnChar(GLFWwindow*, unsigned int c) {
	TwEventCharGLFW(c, GLFW_PRESS);
}

void WindowResizeCallback(GLFWwindow* a_pWindow, int a_Width, int a_Height)
{
	TwWindowSize(a_Width, a_Height);
	glViewport(0, 0, a_Width, a_Height);
	mgApplication::i_WindowWidth = a_Width;
	mgApplication::i_WindowHeight = a_Height;
}

///////////////

void window_focus_callback(GLFWwindow* window, int focused)
{
	mgApplication::b_WindowFocused = focused;
}


void mgApplication::SetWindowTitle(const char * sTitle)
{
	glfwSetWindowTitle(p_Window, sTitle);
}

int mgApplication::Start()
{
	// Init glfw
	if (glfwInit() == false)
		return -1;

	// Create our Window

	//glfwWindowHint(GLFW_SAMPLES, 8);

	p_Window = glfwCreateWindow(i_WindowWidth, i_WindowHeight, "Name Me", nullptr, nullptr);

	// Set resize callback, so we can maximize the window
	glfwSetWindowSizeCallback(p_Window, WindowResizeCallback);

	// Set focus callback
	glfwSetWindowFocusCallback(p_Window, window_focus_callback);

	// Set other callbacks
	glfwSetMouseButtonCallback(p_Window, OnMouseButton);
	glfwSetCursorPosCallback(p_Window, OnMousePosition);
	glfwSetScrollCallback(p_Window, OnMouseScroll);
	glfwSetKeyCallback(p_Window, OnKey);
	glfwSetCharCallback(p_Window, OnChar);

	// Check if our window failed to create
	if (p_Window == nullptr)
	{
		glfwTerminate();
		return -2;
	}

	// Set the OpenGL context
	glfwMakeContextCurrent(p_Window);

	// Load OpenGL Functions
	if (ogl_LoadFunctions() == ogl_LOAD_FAILED)
	{
		glfwDestroyWindow(p_Window);
		glfwTerminate();
	}

	// print our OpenGL version
	printf("GL: %i.%i\n", ogl_GetMajorVersion(), ogl_GetMinorVersion());

	// Enable depth buffer testing
	//glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	//glEnable(GL_CULL_FACE);

	TwInit(TW_OPENGL_CORE, nullptr);
	TwWindowSize(i_WindowWidth, i_WindowHeight);

	return 1;
}

int mgApplication::Shutdown()
{
	// Destroy and clean up everything
	TwDeleteAllBars();
	TwTerminate();
	glfwDestroyWindow(p_Window);
	glfwTerminate();
	return 0;
}

void mgApplication::Run()
{
	if (Start() == true)
	{
		while (BeginUpdate() == true)
		{
			Update();
			BeginDraw();
			Draw();
			EndDraw();
			EndUpdate();
		}
		Shutdown();
	}
}

int mgApplication::Update()
{
	static int ShaderUpdateTimer = 0;
	ShaderUpdateTimer++;

	// Increase app tick
	m_Tick += 0.01f;
	

	float currentTime = (float)glfwGetTime();
	float deltaTime = currentTime - m_PreviousTime; // prev of last frame
	m_PreviousTime = currentTime;

	return 0;
}

int mgApplication::BeginUpdate()
{
	// GLFW poll events
	glfwPollEvents();

	return glfwWindowShouldClose(p_Window) == false && glfwGetKey(p_Window, GLFW_KEY_ESCAPE) != GLFW_PRESS;
}

void mgApplication::EndUpdate()
{
}

int mgApplication::Draw()
{
	return 0;
}

void mgApplication::BeginDraw()
{
	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void mgApplication::EndDraw()
{
	TwDraw(); // Draw GUI

	glfwSwapBuffers(p_Window);
}