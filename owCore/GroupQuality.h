#pragma once

class 
	__declspec(uuid("CDC1D2DA-0258-43B7-9561-A15C31F3D7A2"))
	CGroupQuality : public ISettingGroup, public IInputListener
{
public:
	CGroupQuality();
	~CGroupQuality();

	void UpdateByFog(float _fogDist);

	// ISettingGroup
	void InitDefault() override;
	
	// IInputListener
	void OnMouseMoved(cvec2 _mousePos) override {}
	bool OnMouseButtonPressed(int _button, int _mods, cvec2 _mousePos) override { return false; }
	bool OnMouseButtonReleased(int _button, int _mods, cvec2 _mousePos) override { return false; }
	bool OnMouseWheel(int _yoffset) override { return false; }
	bool OnKeyboardPressed(int _key, int _scancode, int _mods) override;
	bool OnKeyboardReleased(int _key, int _scancode, int _mods) override { return false; }
	bool OnCharInput(uint32 _char) override { return false; }

public:
	float ADT_MCNK_Distance;
	float ADT_MDX_Distance;
	float ADT_WMO_Distance;
	float WMO_MODD_Distance;
	float ADT_MCNK_HighRes_Distance;

	float fogdistance;
	float culldistance;

	uint16 Texture_Sampler;

	// Drawing
	bool draw_mcnk;
	bool draw_mcnk_low;
	bool draw_map_wmo;
	bool draw_wmo_doodads;
	bool draw_map_m2;
	bool draw_water;
	bool draw_wmo_water;

	bool WMO_MOCV;
	bool WMO_AmbColor;

	bool drawfog;
	bool timeEnable;

private:
	const float C_ADT_MCNK_Distance = 998.0f;
	const float C_ADT_MCNK_HighRes_Distance = 384.0f * 0.65f;
	const float C_ADT_MDX_Distance = 384.0f * 3.0f;
	const float C_ADT_WMO_Distance = C_ADT_MDX_Distance * 1.5f;
	const float C_WMO_MODD_Distance = 64.0f * 7.0f;
};