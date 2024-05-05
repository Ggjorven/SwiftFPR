#pragma once

#include <Swift/Core/Layer.hpp>

#include "FPR/Scene.hpp"

using namespace Swift;

class FPRCore : public Layer
{
public:
	void OnAttach() override;
	void OnDetach() override;

	void OnUpdate(float deltaTime) override;
	void OnRender() override;
	void OnEvent(Event& e) override;
	void OnImGuiRender() override;

private:
	Ref<Scene> m_Scene = nullptr;
};