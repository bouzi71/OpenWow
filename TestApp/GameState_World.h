#pragma once

class CGameState_World : public CGameState
{
	typedef CGameState base;
public:
	CGameState_World();
	virtual ~CGameState_World();

	// IGameState
	bool Init();
	void Destroy();

	virtual void OnPreRender(Render3DEventArgs& e);
	virtual void OnRender(Render3DEventArgs& e);
	virtual void OnPostRender(Render3DEventArgs& e);
	virtual void OnRenderUI(RenderUIEventArgs& e);


private:
	void LoadGBuffer();
	void Load3D();
	void LoadUI();

	// Lights update
	void UpdateLights();
	void UpdateNumLights();

private:
	Viewport                             m_Viewport;

	RenderTechnique                      m_3DTechnique;
	RenderTechnique                      m_3DDeferredTechnique;
	RenderUITechnique                    m_UITechnique;

	std::shared_ptr<CCameraController>   m_CameraController;

	std::shared_ptr<Query>               m_FrameQuery;
	double                               m_FrameTime;
	std::shared_ptr<Scene3D>             m_3DScene;
	std::shared_ptr<SceneUI>             m_UIScene;
	std::vector<Light>                   m_Lights;
	std::shared_ptr<StructuredBuffer>    m_LightsStructuredBuffer;

	std::shared_ptr<MapController>       m_MapController;

	std::shared_ptr<UIText>              m_CameraPosText;
	std::shared_ptr<UIText>              m_CameraRotText;

private:
	std::shared_ptr<IRenderTarget>       m_GBufferRenderTarget;

private:
	const uint16 c_WindowsWidth = 1280;
	const uint16 c_WindowsHeight = 1024;
	const vec4 g_ClearColor = vec4(0.39f, 0.58f, 0.93f, 1.0f);
};