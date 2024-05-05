#pragma once

#include <Swift/Renderer/Shader.hpp>
#include <Swift/Renderer/Buffers.hpp>
#include <Swift/Renderer/Pipeline.hpp>
#include <Swift/Renderer/RenderPass.hpp>
#include <Swift/Renderer/Descriptors.hpp>
#include <Swift/Renderer/CommandBuffer.hpp>

using namespace Swift;

#define TILE_SIZE 16
#define MAX_POINTLIGHTS 1024
#define MAX_POINTLIGHTS_PER_TILE 64

class Resources
{
public:
	static void Init();
	static void Destroy();

	struct Depth
	{
	public:
		static Ref<Pipeline>		Pipeline;
		static Ref<RenderPass>		RenderPass;
		static Ref<DescriptorSets>	DescriptorSets;
	};

	struct LightCulling
	{
	public:
		static Ref<Pipeline>		Pipeline;
		static Ref<DescriptorSets>	DescriptorSets;

		static Ref<ComputeShader>	ComputeShader;
		static Ref<CommandBuffer>	CommandBuffer;
		
		static Ref<StorageBuffer>	LightsBuffer;
		static Ref<StorageBuffer>	LightVisibilityBuffer;
	};

	struct Shading
	{
	public:
		static Ref<Pipeline>		Pipeline;
		static Ref<RenderPass>		RenderPass;
		static Ref<DescriptorSets>	DescriptorSets;
	};

	inline static uint32_t PreAllocatedModels = 10u;
	inline static uint32_t AllocatedModels = PreAllocatedModels;

	static Ref<UniformBuffer>			SceneBuffer;
	static Ref<DynamicUniformBuffer>	ModelBuffer;
	static Ref<UniformBuffer>			CameraBuffer;

public:
	static void Resize(uint32_t width, uint32_t height);

private:
	static void InitDepth(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher);
	static void InitLightCulling(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher);
	static void InitShading(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher);
	static void InitResources();
};





// Shader resources
struct ShaderScene
{
public:
	glm::uvec2 ScreenSize = {};
};

struct ShaderModel
{
public:
	glm::mat4 Model = {};

public:
	ShaderModel() = default;
	ShaderModel(const glm::mat4& matrix)
		: Model(matrix)
	{
	}
};

struct ShaderCamera
{
public:
	glm::mat4 View = {};
	glm::mat4 Projection = {};
	glm::vec2 DepthUnpackConsts = {};
	PUBLIC_PADDING(0, 8);
};

struct ShaderPointLight
{
public:
	glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
	float Radius = 5.0f;

	glm::vec3 Colour = { 1.0f, 1.0f, 1.0f };
	float Intensity = 1.0f;
};