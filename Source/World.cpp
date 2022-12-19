#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <glad/glad.h>
#include "World.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GX.h"
#include "Mesh.h"

#ifdef IMGUI_ENABLED
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#endif

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

ShaderProgram World::shaderProgram1;
ShaderProgram World::screenShader;

World::World()
	: m_camera(glm::vec3(0, 0, -10), 0, 90.0f)
{

}

World::~World()
{
	//glDeleteFramebuffers()
}

float quadVertices[] = {
	// positions   // texCoords
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f
};

bool World::Init()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	glGenTextures(1, &m_screenTexture);
	glBindTexture(GL_TEXTURE_2D, m_screenTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1600, 900, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_screenTexture, 0);

	//unsigned int texture2;
	//glGenTextures(1, &texture2);
	//glBindTexture(GL_TEXTURE_2D, texture2);
	//glTexImage2D(
	//	GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 800, 600, 0,
	//	GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL
	//);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture2, 0);

	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1600, 900);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_offsetVBO);
	glGenBuffers(1, &m_EBO);
	glGenVertexArrays(1, &m_VAO);

	glBindVertexArray(m_VAO);

	glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexAttribBinding(0, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexAttribBinding(1, 1);
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, 72 * sizeof(float), (float*)Mesh::GetCubeVertices(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(uint8_t), Mesh::GetCubeIndices(), GL_STATIC_DRAW);

	std::vector<glm::vec3> offsets;
	for (int i = -50; i < 50; i++)
	{
		for (int j = 0; j < 100; j++)
		{
			offsets.emplace_back(i * 2, 0, j * 2);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_offsetVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * offsets.size(), (float*)offsets.data(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &m_screenVAO);
	glBindVertexArray(m_screenVAO);

	glGenBuffers(1, &m_screenVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_screenVBO);
	glBufferData(GL_ARRAY_BUFFER, 4 * 6 * sizeof(float), quadVertices, GL_STATIC_DRAW);

	glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexAttribBinding(0, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
	glVertexAttribBinding(1, 0);
	glEnableVertexAttribArray(1);

	// shadows
	glGenFramebuffers(1, &m_depthMapFBO);
	
	constexpr unsigned SHADOW_MAP_SIZE = 1024;
	glGenTextures(1, &m_shadowTex);
	glBindTexture(GL_TEXTURE_2D, m_shadowTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// just depth buffer. no color
	glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	float near_plane = 1.0f, far_plane = 20.0f;
	m_lightProjMat = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);

	m_lightViewMat = glm::lookAt(glm::vec3(0, 10, 0),
		glm::vec3(10.0f, 0.0f, 20.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));
	//m_lightViewMat = glm::lookAt(glm::vec3(-2.0f, 4.0f, -1.0f),
	//	glm::vec3(0.0f, 0.0f, 0.0f),
	//	glm::vec3(0.0f, 1.0f, 0.0f));

	return true;
}

bool World::InitShared()
{
	shaderProgram1 = ShaderProgram("test1.vs.glsl", "test1.fs.glsl");
	screenShader = ShaderProgram("FullscreenTex.vs.glsl", "FullscreenTex.fs.glsl");
	return true;
}

void World::Render()
{
	ZoneScoped;

#ifdef IMGUI_ENABLED
	ImGuiBeginRender();
#endif

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, 1024, 1024);
	glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	shaderProgram1.Use();
	glBindVertexArray(m_VAO);
	glBindVertexBuffer(0, m_VBO, 0, sizeof(GX::VertexP));
	glBindVertexBuffer(1, m_offsetVBO, 0, sizeof(glm::vec3));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);

	glUniformMatrix4fv(0, 1, GL_FALSE, &glm::mat4(m_lightViewMat * m_modelMat)[0][0]);
	glUniformMatrix4fv(1, 1, GL_FALSE, &m_lightProjMat[0][0]);

	glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, 0, 10000);

	glViewport(0, 0, 1600, 900);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniformMatrix4fv(0, 1, GL_FALSE, &glm::mat4(m_camera.GetViewMatrix() * m_modelMat)[0][0]);
	glUniformMatrix4fv(1, 1, GL_FALSE, &m_camera.GetProjMatrix()[0][0]);

	glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, 0, 10000);

	// second pass
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	screenShader.Use();
	glBindVertexArray(m_screenVAO);
	glBindVertexBuffer(0, m_screenVBO, 0, 4 * sizeof(float));
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, m_shadowTex);
	glDrawArrays(GL_TRIANGLES, 0, 6);

#ifdef IMGUI_ENABLED
	ImGuiRenderStart();
	ImGuiRenderEnd();
#endif
}

void World::Update(float updateTime, Input::InputData* inputData)
{
	ZoneScoped;
	UpdateCamera(updateTime, inputData);
	CalcFrameRate(updateTime);
}

constexpr float FRAMERATE_MS = 1000.0f;
unsigned World::CalcFrameRate(float frameTime)
{
	m_frameTimes.push_back(frameTime);
	m_frameTimeTotal += frameTime;

	while (m_frameTimeTotal > FRAMERATE_MS)
	{
		m_frameTimeTotal -= m_frameTimes.front();
		m_frameTimes.pop_front();
	}

	m_frameRate = unsigned(m_frameTimes.size() / FRAMERATE_MS * 1000.f);
	return m_frameRate;
}

void World::UpdateCamera(float updateTime, Input::InputData* inputData)
{
	m_camera.FrameStart();
	if (!inputData->m_disableMouseLook)
	{
		glm::vec3 newPos = m_camera.GetPosition() + (glm::vec3(m_camera.GetForward() * -inputData->m_moveInput.z + m_camera.GetRight() * inputData->m_moveInput.x + glm::vec3(0, inputData->m_moveInput.y, 0)) * updateTime / 1000.0f * 10.0f);
		m_camera.Transform(newPos, -inputData->m_mouseInput.y * 0.5f, inputData->m_mouseInput.x * 0.5f);
	}
}

#ifdef IMGUI_ENABLED
void World::ImGuiBeginRender()
{
	// feed inputs to dear imgui, start new frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void World::ImGuiRenderStart()
{
	// render your GUI
	ImGui::Begin("Demo window");
	ImGui::Text("Framerate: %d", m_frameRate);
	glm::vec3 cameraPos = m_camera.GetPosition();
	ImGui::Text("Position x:%.3f y:%.3f z:%.3f", cameraPos.x, cameraPos.y, cameraPos.z);
}

void World::ImGuiRenderEnd()
{
	//ImGui::ShowDemoWindow();
	ImGui::End();

	// Render dear imgui into screen
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
#endif