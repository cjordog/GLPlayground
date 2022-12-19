#pragma once

#include "Camera.h"
#include "Input.h"
#include "ShaderProgram.h"

#include <deque>

class World
{
public:
	World();
	~World();

	static bool InitShared();
	bool Init();
	void Render();
	void Update(float updateTime, Input::InputData* inputData);

private:
	unsigned CalcFrameRate(float frameTime);
	void UpdateCamera(float updateTime, Input::InputData* inputData);
#ifdef IMGUI_ENABLED
	void ImGuiBeginRender();
	void ImGuiRenderStart();
	void ImGuiRenderEnd();
#endif

	static ShaderProgram shaderProgram1;
	static ShaderProgram screenShader;
	unsigned m_VAO = 0;
	unsigned m_VBO = 0;
	unsigned m_offsetVBO = 0;
	unsigned m_EBO = 0;

	unsigned m_screenVAO = 0;
	unsigned m_screenVBO = 0;

	unsigned m_fbo = 0;
	unsigned m_depthMapFBO = 0;

	unsigned m_screenTexture = 0;
	unsigned m_shadowTex = 0;

	glm::mat4 m_modelMat = glm::mat4(1.0f);
	glm::mat4 m_lightViewMat = glm::mat4(1.0f);
	glm::mat4 m_lightProjMat = glm::mat4(1.0f);

	Camera m_camera;

	std::deque<float> m_frameTimes;
	float m_frameTimeTotal = 0;
	unsigned m_frameRate = 0;
};