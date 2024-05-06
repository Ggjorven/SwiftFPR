#include "Scene.hpp"

#include <Swift/Core/Application.hpp>

#include "FPR/Resources.hpp"
#include "FPR/Components.hpp"

#include <imgui.h>

#include <glm/gtc/type_ptr.hpp>

Scene::Scene()
{
	Resources::Init();

	m_Camera = Camera::Create();

	// Manually add some entities
	{
		TransformComponent transform = {};
		transform.Position = { 0.0f, 0.0f, 0.0f };
		transform.Size = { 1.0f, 1.0f, 1.0f };
		transform.Rotation = { -90.0f, 0.0, 270.0f };

		MeshComponent mesh = {};
		mesh.MeshObject = Mesh::Create("assets/objects/viking_room.obj");
		mesh.Albedo = Image2D::Create({"assets/objects/viking_room.png"});

		PointLightComponent light = {};
		light.Colour = { 0.0f, 1.0f, 1.0f };
		light.Intensity = 0.5f;
		light.Radius = 5.00f;

		// Viking mesh
		entt::entity viking = m_Registry.create();
		m_Registry.emplace<TransformComponent>(viking, transform);
		m_Registry.emplace<MeshComponent>(viking, mesh);

		// Light
		entt::entity pointLight = m_Registry.create();
		
		transform.Position = { 0.5f, 0.5f, 0.5f };

		m_Registry.emplace<TransformComponent>(pointLight, transform);
		m_Registry.emplace<PointLightComponent>(pointLight, light);
	}

	InitHeatMap();
}

Scene::~Scene()
{
	Resources::Destroy();
}

void Scene::OnUpdate(float deltaTime)
{
	// Camera
	{
		m_Camera->OnUpdate(deltaTime);

		Resources::CameraBuffer->SetData((void*)&m_Camera->GetCamera(), sizeof(ShaderCamera));

		Resources::CameraBuffer->Upload(Resources::Depth::DescriptorSets->GetSets(1)[0], Resources::Depth::DescriptorSets->GetLayout(1).GetDescriptorByName("u_Camera"));
		Resources::CameraBuffer->Upload(Resources::LightCulling::DescriptorSets->GetSets(1)[0], Resources::LightCulling::DescriptorSets->GetLayout(1).GetDescriptorByName("u_Camera"));
		Resources::CameraBuffer->Upload(Resources::Shading::DescriptorSets->GetSets(1)[0], Resources::Shading::DescriptorSets->GetLayout(1).GetDescriptorByName("u_Camera"));
	}

	// Model matrices
	{
		// TODO: Check if we need to resize 

		auto view = m_Registry.view<MeshComponent>();
		std::vector<ShaderModel> matrices(view.size());

		size_t i = 0;
		for (auto& entity : view)
		{
			auto transforms = m_Registry.view<TransformComponent>();
			APP_ASSERT(transforms.contains(entity), "Entity with MeshComponent doesn't have TransformComponent.");

			matrices[i] = { transforms.get<TransformComponent>(entity).GetMatrix() };
			Resources::ModelBuffer->SetDataIndexed((uint32_t)i, &matrices[i], sizeof(ShaderModel));

			i++;
		}
		Resources::ModelBuffer->UploadIndexedData();
	}

	// Point Lights
	{
		auto view = m_Registry.view<PointLightComponent>();
		std::vector<ShaderPointLight> lights(view.size());

		size_t i = 0;
		for (auto& entity : view)
		{
			PointLightComponent pointLight = view.get<PointLightComponent>(entity);

			auto transforms = m_Registry.view<TransformComponent>();
			APP_ASSERT(transforms.contains(entity), "Entity with PointLightComponent doesn't have TransformComponent.");

			ShaderPointLight light = {};
			light.Position = transforms.get<TransformComponent>(entity).Position;
			light.Colour = pointLight.Colour;
			light.Radius = pointLight.Radius;
			light.Intensity = pointLight.Intensity;

			lights[i] = light;

			i++;
		}

		uint32_t size = (uint32_t)lights.size();
		Resources::LightCulling::LightsBuffer->SetData((void*)&size, sizeof(uint32_t));
		Resources::LightCulling::LightsBuffer->SetData((void*)lights.data(), sizeof(ShaderPointLight) * lights.size(), sizeof(uint32_t) + (sizeof(char) * 12)); 
		Resources::LightCulling::LightsBuffer->Upload(Resources::LightCulling::DescriptorSets->GetSets(0)[0], Resources::LightCulling::DescriptorSets->GetLayout(0).GetDescriptorByName("u_Lights"));

		Resources::LightCulling::LightVisibilityBuffer->Upload(Resources::LightCulling::DescriptorSets->GetSets(0)[0], Resources::LightCulling::DescriptorSets->GetLayout(0).GetDescriptorByName("u_Visibility"));
	}

	// Scene Data
	{
		ShaderScene uniform = { { Application::Get().GetWindow().GetWidth(), Application::Get().GetWindow().GetHeight() } };
		Resources::SceneBuffer->SetData((void*)&uniform, sizeof(ShaderScene));
		Resources::SceneBuffer->Upload(Resources::LightCulling::DescriptorSets->GetSets(1)[0], Resources::LightCulling::DescriptorSets->GetLayout(1).GetDescriptorByName("u_Scene"));
		Resources::SceneBuffer->Upload(Resources::Shading::DescriptorSets->GetSets(1)[0], Resources::Shading::DescriptorSets->GetLayout(1).GetDescriptorByName("u_Scene"));
	}
}

void Scene::OnRender()
{
	// Depth pre pass
	Renderer::Submit([this]()
	{
		auto modelSets = Resources::Depth::DescriptorSets->GetSets(0);
		auto cameraSet = Resources::Depth::DescriptorSets->GetSets(1)[0];

		Resources::Depth::RenderPass->Begin();

		Resources::Depth::Pipeline->Use(Resources::Depth::RenderPass->GetCommandBuffer());
		cameraSet->Bind(Resources::Depth::Pipeline, Resources::Depth::RenderPass->GetCommandBuffer());

		auto view = m_Registry.view<MeshComponent>();
		std::vector<ShaderModel> matrices(view.size());

		size_t i = 0;
		for (auto& entity : view)
		{
			MeshComponent mesh = view.get<MeshComponent>(entity);

			Resources::ModelBuffer->Upload(modelSets[i], Resources::Depth::DescriptorSets->GetLayout(0).GetDescriptorByName("u_Model"), sizeof(ShaderModel) * i);
			modelSets[i]->Bind(Resources::Depth::Pipeline, Resources::Depth::RenderPass->GetCommandBuffer(), PipelineBindPoint::Graphics, { 0 });

			mesh.MeshObject->GetVertexBuffer()->Bind(Resources::Depth::RenderPass->GetCommandBuffer());
			mesh.MeshObject->GetIndexBuffer()->Bind(Resources::Depth::RenderPass->GetCommandBuffer());

			Renderer::DrawIndexed(Resources::Depth::RenderPass->GetCommandBuffer(), mesh.MeshObject->GetIndexBuffer());

			i++;
		}

		Resources::Depth::RenderPass->End();
		Resources::Depth::RenderPass->Submit();
	});

	// Light culling
	Renderer::Submit([this]()
	{
		const glm::uvec2 tiles = GetTileCount();
		auto& set0 = Resources::LightCulling::DescriptorSets->GetSets(0)[0];
		auto& set1 = Resources::LightCulling::DescriptorSets->GetSets(1)[0];

		Resources::LightCulling::CommandBuffer->Begin();

		Renderer::GetDepthImage()->Transition(ImageLayout::Depth, ImageLayout::DepthRead);
		Renderer::GetDepthImage()->Upload(set0, Resources::LightCulling::DescriptorSets->GetLayout(0).GetDescriptorByName("u_DepthBuffer"));

		Resources::LightCulling::Pipeline->Use(Resources::LightCulling::CommandBuffer, PipelineBindPoint::Compute);

		set0->Bind(Resources::LightCulling::Pipeline, Resources::LightCulling::CommandBuffer, PipelineBindPoint::Compute);
		set1->Bind(Resources::LightCulling::Pipeline, Resources::LightCulling::CommandBuffer, PipelineBindPoint::Compute);

		Resources::LightCulling::ComputeShader->Dispatch(Resources::LightCulling::CommandBuffer, tiles.x, tiles.y, 1);

		Resources::LightCulling::CommandBuffer->End();
		Resources::LightCulling::CommandBuffer->Submit(Queue::Compute);
	});

	// Heatmap
	RenderHeatMap();

	// Final shading
	Renderer::Submit([this]()
	{
		auto& set0 = Resources::Shading::DescriptorSets->GetSets(0);
		auto& set1 = Resources::Shading::DescriptorSets->GetSets(1)[0];

		Resources::Shading::RenderPass->Begin();

		Resources::Shading::Pipeline->Use(Resources::Shading::RenderPass->GetCommandBuffer());
		set1->Bind(Resources::Shading::Pipeline, Resources::Shading::RenderPass->GetCommandBuffer());

		auto view = m_Registry.view<MeshComponent>();

		size_t i = 0;
		for (auto& entity : view)
		{
			MeshComponent mesh = view.get<MeshComponent>(entity);

			Resources::ModelBuffer->Upload(set0[i], Resources::Shading::DescriptorSets->GetLayout(0).GetDescriptorByName("u_Model"), sizeof(ShaderModel) * i);
			mesh.Albedo->Upload(set0[i], Resources::Shading::DescriptorSets->GetLayout(0).GetDescriptorByName("u_Albedo"));
			Resources::LightCulling::LightsBuffer->Upload(set0[i], Resources::Shading::DescriptorSets->GetLayout(0).GetDescriptorByName("u_Lights"));
			Resources::LightCulling::LightVisibilityBuffer->Upload(set0[i], Resources::Shading::DescriptorSets->GetLayout(0).GetDescriptorByName("u_Visibility"));
			
			set0[i]->Bind(Resources::Shading::Pipeline, Resources::Shading::RenderPass->GetCommandBuffer(), PipelineBindPoint::Graphics, { 0 });

			mesh.MeshObject->GetVertexBuffer()->Bind(Resources::Shading::RenderPass->GetCommandBuffer());
			mesh.MeshObject->GetIndexBuffer()->Bind(Resources::Shading::RenderPass->GetCommandBuffer());

			Renderer::DrawIndexed(Resources::Shading::RenderPass->GetCommandBuffer(), mesh.MeshObject->GetIndexBuffer());

			i++;
		}

		Resources::Shading::RenderPass->End();
		Resources::Shading::RenderPass->Submit();
	});
}

void Scene::OnEvent(Event& e)
{
	EventHandler handler(e);

	handler.Handle<WindowResizeEvent>(APP_BIND_EVENT_FN(Scene::OnResize));

	m_Camera->OnEvent(e);
}

void Scene::OnImGuiRender()
{
}

Ref<Scene> Scene::Create()
{
	return RefHelper::Create<Scene>();
}

void Scene::InitHeatMap()
{
	auto& window = Application::Get().GetWindow();
	ImageSpecification imageSpecs = ImageSpecification(window.GetWidth(), window.GetHeight(), ImageUsageFlags::Colour | ImageUsageFlags::NoMipMaps | ImageUsageFlags::Storage);
	imageSpecs.Layout = ImageLayout::General;

	m_HeatAttachment = Image2D::Create(imageSpecs);

	Ref<ShaderCompiler> compiler = ShaderCompiler::Create();
	Ref<ShaderCacher> cacher = ShaderCacher::Create();

	m_HeatSets = DescriptorSets::Create(
	{
		// Set 0 
		{ 1, { 0, {
			{ DescriptorType::StorageImage, 0, "u_Image", ShaderStage::Compute },
			{ DescriptorType::StorageBuffer, 1, "u_Visibility", ShaderStage::Compute }
		}}},

		// Set 1 
		{ 1, { 1, {
			{ DescriptorType::UniformBuffer, 0, "u_Scene", ShaderStage::Compute }
		}}},
	});

	CommandBufferSpecification cmdSpecs = {};
	cmdSpecs.Usage = CommandBufferUsage::Sequence;

	m_HeatCommand = CommandBuffer::Create(cmdSpecs);

	ShaderSpecification shaderSpecs = {};
	shaderSpecs.Compute = cacher->GetLatest(compiler, "assets/shaders/caches/Heatmap.comp.cache", "assets/shaders/Heatmap.comp.glsl", ShaderStage::Compute);

	m_HeatShader = ComputeShader::Create(shaderSpecs);
	m_HeatPipeline = Pipeline::Create({ }, m_HeatSets, m_HeatShader);
}

void Scene::RenderHeatMap()
{
	Renderer::Submit([this]() 
	{
		const glm::uvec2 tiles = GetTileCount();
		auto& set0 = m_HeatSets->GetSets(0)[0];
		auto& set1 = m_HeatSets->GetSets(1)[0];

		m_HeatCommand->Begin();

		m_HeatAttachment->Upload(set0, m_HeatSets->GetLayout(0).GetDescriptorByName("u_Image"));
		Resources::LightCulling::LightVisibilityBuffer->Upload(set0, m_HeatSets->GetLayout(0).GetDescriptorByName("u_Visibility"));
		Resources::SceneBuffer->Upload(set1, m_HeatSets->GetLayout(1).GetDescriptorByName("u_Scene"));

		m_HeatPipeline->Use(m_HeatCommand, PipelineBindPoint::Compute);

		set0->Bind(m_HeatPipeline, m_HeatCommand, PipelineBindPoint::Compute);
		set1->Bind(m_HeatPipeline, m_HeatCommand, PipelineBindPoint::Compute);

		m_HeatShader->Dispatch(m_HeatCommand, tiles.x, tiles.y, 1);

		m_HeatCommand->End();
		m_HeatCommand->Submit(Queue::Compute);
	});
}

const glm::uvec2 Scene::GetTileCount() const
{
	glm::uvec2 tiles = {};
	tiles.x = (Application::Get().GetWindow().GetWidth() + TILE_SIZE - 1) / TILE_SIZE;
	tiles.y = (Application::Get().GetWindow().GetHeight() + TILE_SIZE - 1) / TILE_SIZE;
	return tiles;
}

bool Scene::OnResize(WindowResizeEvent& e)
{
	Renderer::GetDepthImage()->Transition(ImageLayout::Undefined, ImageLayout::Depth);

	m_HeatAttachment->Resize(e.GetWidth(), e.GetHeight());
	
	Resources::Resize(e.GetWidth(), e.GetHeight());

	return false;
}
