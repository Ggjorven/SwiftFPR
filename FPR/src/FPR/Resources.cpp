#include "Resources.hpp"

#include <Swift/Core/Application.hpp>
#include <Swift/Utils/Mesh.hpp>

#include <Swift/Renderer/Renderer.hpp>

// Depth
Ref<Pipeline>				Resources::Depth::Pipeline = nullptr;
Ref<RenderPass>				Resources::Depth::RenderPass = nullptr;
Ref<DescriptorSets>			Resources::Depth::DescriptorSets = nullptr;

// LightCulling
Ref<Pipeline>				Resources::LightCulling::Pipeline = nullptr;
Ref<DescriptorSets>			Resources::LightCulling::DescriptorSets = nullptr;

Ref<ComputeShader>			Resources::LightCulling::ComputeShader = nullptr;
Ref<CommandBuffer>			Resources::LightCulling::CommandBuffer = nullptr;

Ref<StorageBuffer>			Resources::LightCulling::LightsBuffer = nullptr;
Ref<StorageBuffer>			Resources::LightCulling::LightVisibilityBuffer = nullptr;

// Shading
Ref<Pipeline>				Resources::Shading::Pipeline = nullptr;
Ref<RenderPass>				Resources::Shading::RenderPass = nullptr;
Ref<DescriptorSets>			Resources::Shading::DescriptorSets = nullptr;

// Resources
Ref<UniformBuffer>			Resources::SceneBuffer = nullptr;
Ref<DynamicUniformBuffer>	Resources::ModelBuffer = nullptr;
Ref<UniformBuffer>			Resources::CameraBuffer = nullptr;

void Resources::Init()
{
	Ref<ShaderCompiler> compiler = ShaderCompiler::Create();
	Ref<ShaderCacher> cacher = ShaderCacher::Create();

	InitDepth(compiler, cacher);
	InitLightCulling(compiler, cacher);
	InitShading(compiler, cacher);
	InitResources();
}

void Resources::Destroy()
{
	// Depth
	Resources::Depth::Pipeline.reset();
	Resources::Depth::RenderPass.reset();
	Resources::Depth::DescriptorSets.reset();

	// LightCulling
	Resources::LightCulling::Pipeline.reset();
	Resources::LightCulling::DescriptorSets.reset();

	Resources::LightCulling::ComputeShader.reset();
	Resources::LightCulling::CommandBuffer.reset();

	Resources::LightCulling::LightsBuffer.reset();
	Resources::LightCulling::LightVisibilityBuffer.reset();

	// Shading
	Resources::Shading::Pipeline.reset();
	Resources::Shading::RenderPass.reset();
	Resources::Shading::DescriptorSets.reset();

	// Resources
	Resources::SceneBuffer.reset();
	Resources::ModelBuffer.reset();
	Resources::CameraBuffer.reset();
}

void Resources::Resize(uint32_t width, uint32_t height)
{
	// Depth
	{
		Resources::Depth::RenderPass->Resize(width, height);
	}

	// LightCulling
	{
		uint32_t tiles = ((Application::Get().GetWindow().GetWidth() + TILE_SIZE - 1) / TILE_SIZE) * ((Application::Get().GetWindow().GetHeight() + TILE_SIZE - 1) / TILE_SIZE);
		size_t size = ((sizeof(uint32_t) + (sizeof(char) * 12) + ((sizeof(uint32_t) + (sizeof(char) * 12)) * MAX_POINTLIGHTS_PER_TILE))) * tiles;
		Resources::LightCulling::LightVisibilityBuffer = StorageBuffer::Create(size);
	}

	// Shading
	{
		Resources::Shading::RenderPass->Resize(width, height);
	}
}

void Resources::InitDepth(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher)
{
	Resources::Depth::DescriptorSets = DescriptorSets::Create(
	{
		// Set 0
		{ PreAllocatedModels, { 0, {
			{ DescriptorType::DynamicUniformBuffer, 0, "u_Model", ShaderStage::Vertex }
		}}},

		// Set 1
		{ 1, { 1, {
			{ DescriptorType::UniformBuffer, 0, "u_Camera", ShaderStage::Vertex }
		}}}
	});

	CommandBufferSpecification cmdBufSpecs = {};
	cmdBufSpecs.Usage = CommandBufferUsage::Sequence;

	auto cmdBuf = CommandBuffer::Create(cmdBufSpecs);

	RenderPassSpecification renderPassSpecs = {};
	renderPassSpecs.DepthLoadOp = LoadOperation::Clear;
	renderPassSpecs.DepthAttachment = Renderer::GetDepthImage();
	renderPassSpecs.PreviousDepthImageLayout = ImageLayout::Depth;
	renderPassSpecs.FinalDepthImageLayout = ImageLayout::Depth;

	Resources::Depth::RenderPass = RenderPass::Create(renderPassSpecs, cmdBuf);

	ShaderSpecification shaderSpecs = {};
	shaderSpecs.Vertex = cacher->GetLatest(compiler, "assets/shaders/caches/Depth.vert.cache", "assets/shaders/Depth.vert.glsl", ShaderStage::Vertex);
	shaderSpecs.Fragment = cacher->GetLatest(compiler, "assets/shaders/caches/Depth.frag.cache", "assets/shaders/Depth.frag.glsl", ShaderStage::Fragment);

	auto shader = Shader::Create(shaderSpecs);

	PipelineSpecification pipelineSpecs = {};
	pipelineSpecs.Bufferlayout = MeshVertex::GetLayout();
	pipelineSpecs.Polygonmode = PolygonMode::Fill;
	pipelineSpecs.Cullingmode = CullingMode::None;
	pipelineSpecs.LineWidth = 1.0f;
	pipelineSpecs.Blending = false;

	Resources::Depth::Pipeline = Pipeline::Create(pipelineSpecs, Resources::Depth::DescriptorSets, shader, Resources::Depth::RenderPass);
}

void Resources::InitLightCulling(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher)
{
	Resources::LightCulling::DescriptorSets = DescriptorSets::Create(
	{
		// Set 0
		{ 1, { 0, {
			{ DescriptorType::Image, 0, "u_DepthBuffer", ShaderStage::Compute },
			{ DescriptorType::StorageBuffer, 1, "u_Lights", ShaderStage::Compute },
			{ DescriptorType::StorageBuffer, 2, "u_Visibility", ShaderStage::Compute }
		}}},

		// Set 1
		{ 1, { 1, {
			{ DescriptorType::UniformBuffer, 0, "u_Camera", ShaderStage::Compute },
			{ DescriptorType::UniformBuffer, 1, "u_Scene", ShaderStage::Compute }
		}}},
	});

	{
		size_t size = sizeof(uint32_t) + (sizeof(char) * 12) + (sizeof(ShaderPointLight) * MAX_POINTLIGHTS);
		Resources::LightCulling::LightsBuffer = StorageBuffer::Create(size);

		auto& window = Application::Get().GetWindow();
		uint32_t tiles = ((Application::Get().GetWindow().GetWidth() + TILE_SIZE - 1) / TILE_SIZE) * ((Application::Get().GetWindow().GetHeight() + TILE_SIZE - 1) / TILE_SIZE);
		size = ((sizeof(uint32_t) + (sizeof(char) * 12) + ((sizeof(uint32_t) + (sizeof(char) * 12)) * MAX_POINTLIGHTS_PER_TILE))) * tiles;
		Resources::LightCulling::LightVisibilityBuffer = StorageBuffer::Create(size);
	}

	CommandBufferSpecification cmdBufSpecs = {};
	cmdBufSpecs.Usage = CommandBufferUsage::Sequence;

	Resources::LightCulling::CommandBuffer = CommandBuffer::Create(cmdBufSpecs);

	ShaderSpecification shaderSpecs = {};
	shaderSpecs.Compute = cacher->GetLatest(compiler, "assets/shaders/caches/LightCulling.comp.cache", "assets/shaders/LightCulling.comp.glsl", ShaderStage::Compute);

	Resources::LightCulling::ComputeShader = ComputeShader::Create(shaderSpecs);
	Resources::LightCulling::Pipeline = Pipeline::Create({ }, Resources::LightCulling::DescriptorSets, Resources::LightCulling::ComputeShader);
}

void Resources::InitShading(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher)
{
	Resources::Shading::DescriptorSets = DescriptorSets::Create(
	{
		// Set 0
		{ PreAllocatedModels, { 0, {
			{ DescriptorType::DynamicUniformBuffer, 0, "u_Model", ShaderStage::Vertex },
			{ DescriptorType::Image, 1, "u_Albedo", ShaderStage::Fragment },
			{ DescriptorType::StorageBuffer, 2, "u_Lights", ShaderStage::Fragment },
			{ DescriptorType::StorageBuffer, 3, "u_Visibility", ShaderStage::Fragment }
		}}},

		// Set 1
		{ 1, { 1, {
			{ DescriptorType::UniformBuffer, 0, "u_Camera", ShaderStage::Vertex },
			{ DescriptorType::UniformBuffer, 1, "u_Scene", ShaderStage::Fragment }
		}}}
	});

	CommandBufferSpecification cmdBufSpecs = {};
	cmdBufSpecs.Usage = CommandBufferUsage::Sequence;

	auto cmdBuf = CommandBuffer::Create(cmdBufSpecs);

	RenderPassSpecification renderPassSpecs = {};
	renderPassSpecs.ColourAttachment = Renderer::GetSwapChainImages();
	renderPassSpecs.ColourLoadOp = LoadOperation::Clear;
	renderPassSpecs.ColourClearColour = { 0.0f, 0.0f, 0.0f, 1.0f };
	renderPassSpecs.PreviousColourImageLayout = ImageLayout::Undefined;
	renderPassSpecs.FinalColourImageLayout = ImageLayout::Presentation;

	renderPassSpecs.DepthLoadOp = LoadOperation::Clear;
	renderPassSpecs.DepthAttachment = Renderer::GetDepthImage();
	renderPassSpecs.PreviousDepthImageLayout = ImageLayout::DepthRead;
	renderPassSpecs.FinalDepthImageLayout = ImageLayout::Depth;

	Resources::Shading::RenderPass = RenderPass::Create(renderPassSpecs, cmdBuf);

	ShaderSpecification shaderSpecs = {};
	shaderSpecs.Vertex = cacher->GetLatest(compiler, "assets/shaders/caches/Shading.vert.cache", "assets/shaders/Shading.vert.glsl", ShaderStage::Vertex);
	shaderSpecs.Fragment = cacher->GetLatest(compiler, "assets/shaders/caches/Shading.frag.cache", "assets/shaders/Shading.frag.glsl", ShaderStage::Fragment);

	auto shader = Shader::Create(shaderSpecs);

	PipelineSpecification pipelineSpecs = {};
	pipelineSpecs.Bufferlayout = MeshVertex::GetLayout();
	pipelineSpecs.Polygonmode = PolygonMode::Fill;
	pipelineSpecs.Cullingmode = CullingMode::None;
	pipelineSpecs.LineWidth = 1.0f;
	pipelineSpecs.Blending = false;

	Resources::Shading::Pipeline = Pipeline::Create(pipelineSpecs, Resources::Shading::DescriptorSets, shader, Resources::Shading::RenderPass);
}

void Resources::InitResources()
{
	SceneBuffer = UniformBuffer::Create(sizeof(ShaderScene));

	ModelBuffer = DynamicUniformBuffer::Create(PreAllocatedModels, sizeof(ShaderModel));
	CameraBuffer = UniformBuffer::Create(sizeof(ShaderCamera));
}