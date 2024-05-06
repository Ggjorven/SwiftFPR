#include "Core.hpp"

#include <Swift/Core/Application.hpp>
#include <Swift/Core/Input/Input.hpp>

#include <Swift/Renderer/Shader.hpp>

void FPRCore::OnAttach()
{
	m_Scene = Scene::Create();
}

void FPRCore::OnDetach()
{
}

void FPRCore::OnUpdate(float deltaTime)
{
	// Note(Jorben): All of this below is just to show some stats in the titlebar
	// I'm not using ImGui since it eats away at frametimes too much.
	static float timer = 0.0f;
	static uint32_t FPS = 0;
	static uint32_t tempFPS = 0;
	timer += deltaTime;
	tempFPS += 1u;

	if (timer >= 1.0f)
	{
		FPS = (uint32_t)((float)tempFPS / timer);
		Application::Get().GetWindow().SetTitle(fmt::format("SwiftFPR | FPS: {0} | Frametime: {1:.3f}ms  -  Width: {2} | Height: {3}", FPS, timer / (float)FPS * 1000.0f, Application::Get().GetWindow().GetWidth(), Application::Get().GetWindow().GetHeight()));
		timer = 0.0f;
		tempFPS = 0u;
	}

	m_Scene->OnUpdate(deltaTime);
}

void FPRCore::OnRender()
{
	m_Scene->OnRender();
}

void FPRCore::OnEvent(Event& e)
{
	m_Scene->OnEvent(e);
}

void FPRCore::OnImGuiRender()
{
	m_Scene->OnImGuiRender();
}