#include "AppAbeSound.h"
#include <algorithm>
#include "SequencePlayer.h"
#include <thread>

TwBar * m_WindowMassProperties;

TwBar * m_GUISound;

SequencePlayer player;

void TW_CALL PlaySound(void *clientData)
{
	AliveAudio::DebugPlayFirstToneSample(((int *)clientData)[0], ((int *)clientData)[1]);
}

void TW_CALL PlaySong(void *clientData)
{
	player.LoadSequence((char*)clientData);
	player.PlaySequence();
}

int AppAbeSound::Start()
{
	mgApplication::Start();
	SetWindowTitle("Alive Sound Engine - by Michael Grima (mlgthatsme@hotmail.com.au)");

	m_GUISound = TwNewBar("Sound");

	//TwAddVarCB(m_GUISound, "interp", TW_TYPE_BOOLCPP, )
	TwAddVarRW(m_GUISound, "interp", TW_TYPE_BOOLCPP, &AliveAudio::Interpolation, "group='Settings' label='Interpolation'");
	TwAddButton(m_GUISound, nullptr, PlaySong, "TEST.SEQ", "group='Settings' label='Play Song 1'");
	TwAddButton(m_GUISound, nullptr, PlaySong, "SECRET.SEQ", "group='Settings' label='Play Song 2'");
	TwAddButton(m_GUISound, nullptr, PlaySong, "TEST.SEQ", "group='Settings' label='Play Song 3'");
	for (int e = 15; e < 25; e++)
	{
		for (int i = 0; i < AliveAudio::m_CurrentSoundbank->m_Programs[e]->m_Tones.size(); i++)
		{
			char labelTest[100];
			sprintf(labelTest, "group='Program %i' label='Play Tone %i'",e, i);
			TwAddButton(m_GUISound, nullptr, PlaySound, (void*)new int[2]{ e,i }, labelTest);
		}
	}

	return true;
}

int AppAbeSound::Shutdown()
{
	mgApplication::Shutdown();
	return 0;
}

int AppAbeSound::Update()
{
	static int prevKeyPress = GLFW_RELEASE;

	int currentState = glfwGetKey(p_Window, GLFW_KEY_F);

	if (currentState == GLFW_PRESS && prevKeyPress == GLFW_RELEASE)
	{
		AliveAudio::NoteOn(27, 36, 127);
	}
	else if (currentState == GLFW_RELEASE && prevKeyPress == GLFW_PRESS)
	{
		AliveAudio::NoteOff(27, 36);
	}

	prevKeyPress = currentState;

	return true;
}

int AppAbeSound::Draw()
{
	return mgApplication::Draw();
}