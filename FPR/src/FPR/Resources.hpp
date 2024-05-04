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

	static Ref<UniformBuffer> SceneBuffer;

private:
	static void InitDepth(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher);
	static void InitLightCulling(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher);
	static void InitShading(Ref<ShaderCompiler> compiler, Ref<ShaderCacher> cacher);
};