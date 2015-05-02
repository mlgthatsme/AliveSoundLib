#include "AppAbeSound.h"
#include <algorithm>
#include "SequencePlayer.h"
#include <thread>
#include "VertexData.h"
#include "ShaderProgram.h"
#include "alive\EQ.h"

TwBar * m_WindowMassProperties;

TwBar * m_GUIInfo;
TwBar * m_GUITones;
TwBar * m_GUISequences;
TwBar * m_GUIScenario;
TwBar * m_GUITheme;
TwBar * m_GUISoundList;
TwBar * m_GUIVoiceList;

mgVertexData * basicRect;
mgShaderProgram * renderShader;

SequencePlayer * player;

bool firstChange = true;
int targetSong = -1;
bool loopSong = false;

// scenario stuff
bool enemyInArea = false;

void TW_CALL PlaySound(void *clientData)
{
	AliveAudio::DebugPlayFirstToneSample(((int *)clientData)[0], ((int *)clientData)[1]);
}

void TW_CALL PlayNote(void *clientData)
{
	AliveAudio::PlayOneShot(*(std::string * )clientData);
}

void TW_CALL StopMusic(void *clientData)
{
	player->StopSequence();
}

void TW_CALL ResumeMusic(void *clientData)
{
	player->PlaySequence();
}

void TW_CALL GetVoiceCount(void *value, void *clientData)
{
	*(int*)value = AliveAudio::m_Voices.size();
}

void TW_CALL KillVoice(void * voice)
{
	((AliveAudioVoice*)voice)->b_NoteOn = false;
}

void TW_CALL PlaySong(void *clientData)
{
	if (firstChange || player->m_PlayerState == ALIVE_SEQUENCER_FINISHED || player->m_PlayerState == ALIVE_SEQUENCER_STOPPED)
	{
		if (player->LoadSequenceData(AliveAudio::m_LoadedSeqData[(int)clientData]) == 0)
		{
			player->PlaySequence();
			firstChange = false;
		}
	}
	else
	{
		targetSong = (int)clientData;
	}
}

void TW_CALL ChangeTheme(void *clientData)
{
	player->StopSequence();

	jsonxx::Object theme = AliveAudio::m_Config.get<jsonxx::Array>("themes").get<jsonxx::Object>((int)clientData);
	AliveAudio::LoadAllFromLvl(theme.get<jsonxx::String>("lvl", "null") + ".LVL", theme.get<jsonxx::String>("vab", "null"), theme.get<jsonxx::String>("seq", "null"));

	TwRemoveAllVars(m_GUITones);
	for (int e = 0; e < 128; e++)
	{
		for (int i = 0; i < AliveAudio::m_CurrentSoundbank->m_Programs[e]->m_Tones.size(); i++)
		{
			char labelTest[100];
			sprintf(labelTest, "group='Program %i' label='%i - Min:%i Max:%i'", e, i, AliveAudio::m_CurrentSoundbank->m_Programs[e]->m_Tones[i]->Min, AliveAudio::m_CurrentSoundbank->m_Programs[e]->m_Tones[i]->Max);
			TwAddButton(m_GUITones, nullptr, PlaySound, (void*)new int[2]{ e, i }, labelTest);
		}
	}

	TwRemoveAllVars(m_GUISequences);
	for (int i = 0; i < AliveAudio::m_LoadedSeqData.size(); i++)
	{
		char labelTest[100];
		sprintf(labelTest, "group='Seq Files' label='Play Seq %i'", i);

		TwAddButton(m_GUISequences, nullptr, PlaySong, (char*)i, labelTest);
	}
}

void BarLoop()
{
	printf("Bar Loop!");

	if (targetSong != -1)
	{
		if (player->LoadSequenceData(AliveAudio::m_LoadedSeqData[targetSong]) == 0);
			player->PlaySequence();

		targetSong = -1;
	}
}

int CurrentEQCutoff = 0;

void TW_CALL SetEQ(const void *value, void *clientData)
{
	CurrentEQCutoff = *(int*)value;
	AliveAudioSetEQ(*(int*)value);
}

void TW_CALL GetEQ(void *value, void *clientData)
{
	*(int*)value = CurrentEQCutoff;
}

int AppAbeSound::Start()
{
	mgApplication::Start();
	player = new SequencePlayer();
	player->m_QuarterCallback = BarLoop;
	SetWindowTitle("Alive Sound Engine - by Michael Grima (mlgthatsme@hotmail.com.au)");

	m_GUIInfo = TwNewBar("Info");
	m_GUIScenario = TwNewBar("Scenarios");
	m_GUITones = TwNewBar("Tones");
	m_GUISequences = TwNewBar("Sequences");
	m_GUITheme = TwNewBar("Themes");
	m_GUISoundList = TwNewBar("SoundList");
	m_GUIVoiceList = TwNewBar("Voices");

	TwDefine("Scenarios position='16 226' ");

	TwDefine("Tones position='230 16' ");
	TwDefine("Sequences position='446 16' ");
	TwDefine("Themes position='660 16' ");
	TwDefine("SoundList position='876 16' ");
	TwDefine("Voices position='1090 16' ");

	TwDefine("Info size='205 200' ");
	TwDefine("Tones size='205 600' ");
	TwDefine("Sequences size='205 600' ");
	TwDefine("Scenarios size='205 200' ");
	TwDefine("Themes size='205 600' ");
	TwDefine("SoundList size='205 600' ");
	TwDefine("Voices size='305 600' ");

	jsonxx::Array soundList = AliveAudio::m_Config.get<jsonxx::Array>("sounds");

	for (int i = 0; i < soundList.size(); i++)
	{
		jsonxx::Object sndObj = soundList.get<jsonxx::Object>(i);
		char labelTest[100];
		sprintf(labelTest, "group='Sound List' label='%s'", sndObj.get<jsonxx::String>("id").c_str());
		std::string * newString = new std::string(sndObj.get<jsonxx::String>("id"));

		TwAddButton(m_GUISoundList, nullptr, PlayNote, (void*)newString, labelTest);
	}

	TwAddVarRW(m_GUIInfo, "interp", TW_TYPE_BOOLCPP, &AliveAudio::Interpolation, "group='Settings' label='Interpolation'");
	TwAddVarRW(m_GUIInfo, "loop", TW_TYPE_BOOLCPP, &loopSong, "group='Settings' label='Loop'");
	TwAddVarCB(m_GUIInfo, "voices", TW_TYPE_INT32, nullptr, GetVoiceCount, nullptr, "group='Info' label='Voices'");
	TwAddVarCB(m_GUIInfo, "eq", TW_TYPE_INT32, SetEQ, GetEQ, nullptr, "group='Audio Settings' label = 'EQ Cutoff' min='0' step='100'");
	TwAddVarRW(m_GUIInfo, "eqen", TW_TYPE_BOOLCPP, &AliveAudio::EQEnabled, "group='Audio Settings' label='EQ Enabled'");
	jsonxx::Array themeList = AliveAudio::m_Config.get<jsonxx::Array>("themes");

	for (int i = 0; i < themeList.size(); i++)
	{
		char labelTest[100];
		if (themeList.get<jsonxx::Object>(i).has<jsonxx::String>("name"))
			sprintf(labelTest, "group='Themes' label='%s'", themeList.get<jsonxx::Object>(i).get<jsonxx::String>("name", "null").c_str());
		else
			sprintf(labelTest, "group='Themes' label='%s %s'", themeList.get<jsonxx::Object>(i).get<jsonxx::String>("lvl", "null").c_str(), themeList.get<jsonxx::Object>(i).get<jsonxx::String>("vab", "null").c_str());
		TwAddButton(m_GUITheme, nullptr, ChangeTheme, (char*)i, labelTest);
	}

	TwAddButton(m_GUIInfo, nullptr, StopMusic, nullptr, "group='Playback' label='Stop'");
	TwAddButton(m_GUIInfo, nullptr, ResumeMusic, nullptr, "group='Playback' label='Resume'");
	
	renderShader = new mgShaderProgram();
	renderShader->InitFromCharBuffer("#version 420\n layout(location=0) in vec4 Position;\nuniform mat4 Transform;\
		uniform mat4 ProjectionView;void main() {gl_Position = ProjectionView * Transform * Position;}",
		"#version 420\nuniform vec4 Color;void main(){gl_FragColor = Color;}");

	basicRect = mgVertexData::CreateCircleFilled(5);

	ChangeTheme((void*)8);
	PlaySong((void*)23);

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
		AliveAudio::NoteOn(14, 40, 127, 0);
	else if (currentState == GLFW_RELEASE && prevKeyPress == GLFW_PRESS)
		AliveAudio::NoteOff(14, 40);

	if (loopSong && player->m_PlayerState == ALIVE_SEQUENCER_FINISHED)
		player->PlaySequence();

	prevKeyPress = currentState;

	TwRefreshBar(m_GUIInfo);

	TwRemoveAllVars(m_GUIVoiceList);
	AliveAudio::voiceListMutex.lock();
	for (auto v : AliveAudio::m_Voices)
	{
		if (v->f_TrackDelay <= 0)
		{
			char labelTest[100];
			sprintf(labelTest, "group='Voices' label='Prog:%i Note:%i'", v->i_Program, v->i_Note, v->f_NoteOffDelay);

			TwAddButton(m_GUIVoiceList, nullptr, KillVoice, (void*)v, labelTest);
		}
	}
	AliveAudio::voiceListMutex.unlock();

	return true;
}

int AppAbeSound::Draw()
{
	mat4 projection = glm::ortho<float>(0, i_WindowWidth, i_WindowHeight, 0);
	mat4 projectionview = projection;

	renderShader->UseProgram();

	renderShader->SetUniformMatrix4fv("ProjectionView", (float*)&projectionview);

	AliveAudio::voiceListMutex.lock();
	for (auto v : AliveAudio::m_Voices)
	{
		if (v->f_TrackDelay > 0 && v->f_TrackDelay < 5000 * 32)
		{
			vec2 blockPos = vec2();
			vec4 color = vec4(1, 1, 1, 1);

			blockPos.x += v->f_TrackDelay / 80;
			blockPos.y += v->i_Note * 10;

			mat4 world = glm::translate(vec3(16, 570, 0) + vec3(blockPos.x, blockPos.y, 0)) * glm::scale(vec3(8, 8, 0));
			renderShader->SetUniformMatrix4fv("Transform", (float*)&world);
			renderShader->SetUniform4f("Color", color);
			basicRect->Render(GL_TRIANGLE_STRIP);

			if (v->f_NoteOffDelay > 0)
			{
				color = vec4(1, 0, 0, 1);
				world = glm::translate(vec3(16, 570, 0) + vec3(blockPos.x + (v->f_NoteOffDelay / 80) - (v->f_TrackDelay / 80), blockPos.y, 0)) * glm::scale(vec3(4, 4, 2));
				renderShader->SetUniformMatrix4fv("Transform", (float*)&world);
				renderShader->SetUniform4f("Color", color);
				basicRect->Render(GL_TRIANGLE_STRIP);
			}
		}
	}
	AliveAudio::voiceListMutex.unlock();

	return mgApplication::Draw();
}