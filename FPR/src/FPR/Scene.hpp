#pragma once

#include <Swift/Core/Layer.hpp>

#include <Swift/Renderer/CommandBuffer.hpp>
#include <Swift/Renderer/RenderPass.hpp>
#include <Swift/Renderer/Image.hpp>

#include <Swift/Renderer/Shader.hpp>
#include <Swift/Renderer/Buffers.hpp>
#include <Swift/Renderer/Pipeline.hpp>
#include <Swift/Renderer/Descriptors.hpp>

#include <entt/entt.hpp>

#include "FPR/Camera.hpp"

using namespace Swift;

class Scene
{
public:
	Scene();
	virtual ~Scene();

	void OnUpdate(float deltaTime);
	void OnRender();
	void OnEvent(Event& e);
	void OnImGuiRender();

	static Ref<Scene> Create();

private:
	void InitHeatMap();
	void RenderHeatMap();

	const glm::uvec2 GetTileCount() const;

	bool OnResize(WindowResizeEvent& e);

private:
	entt::registry m_Registry = {};

	Ref<Camera> m_Camera = nullptr;

	// Heatmap
	Ref<Image2D> m_HeatAttachment = nullptr;

	Ref<Pipeline> m_HeatPipeline = nullptr;
	Ref<DescriptorSets> m_HeatSets = nullptr;

	Ref<CommandBuffer> m_HeatCommand = nullptr;
	Ref<ComputeShader> m_HeatShader = nullptr;
};