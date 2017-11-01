#pragma once

#include "World.h"

enum Commands2
{
	CMD_NONE2,
	CMD_SELECT2,
	CMD_LOAD_WORLD2,
	CMD_DO_LOAD_WORLD2,
	CMD_IN_WORLD2
};

class GameState_Menu : public GameState
{
public:
	GameState_Menu() : GameState() {}

	bool Init();
	void Destroy();
	void InputPhase(double t, double dt);
	void UpdatePhase(double t, double dt);
	void Render(double t, double dt);
	void RenderUI() override;

	//

	void OnBtn(DBC_MapRecord* _e)
	{
		Modules::log().Green("Load level %s [%d]", _e->Get_Directory(), _e->Get_ID());

		_Map->Load_WDT(_e);
		cmd = CMD_SELECT2;
		window->Detach();
	}

	bool LoadWorld(cvec3 _pos);

	//

	V_MOUSE_MOVED;
	V_MOUSE_PRESSED;
	V_MOUSE_RELEASE;
	V_MOUSE_WHEEL;
	V_KEYBD_PRESSED;
	V_KEYBD_RELEASE;
	V_CHAR_INPUT{return false; }

private:
	// Camera moving
	bool enableFreeCamera;
	bool cameraSprint;
	bool cameraSlow;
	vec2 lastMousePos;

	UIWindow* window;

	Commands2 cmd;

	bool minimapActive;

	MDX *backgroundModel;
	float mt;

	void randBackground();
};
