#pragma once

#include <string>
#include <typeinfo>
#include <type_traits>

#include <glm/glm.hpp>

#include <Swift/Core/Core.hpp>
#include <Swift/Utils/Utils.hpp>

#include <Swift/Utils/Mesh.hpp>
#include <Swift/Renderer/Image.hpp>

using namespace Swift;

struct TransformComponent
{
public:
	glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
	glm::vec3 Size = { 1.0f, 1.0f, 1.0f };
	glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };

public:
	TransformComponent() = default;
	TransformComponent(const glm::vec3& position, const glm::vec3& size, const glm::vec3& rotation);
	TransformComponent(const TransformComponent& other) = default;

	glm::mat4 GetMatrix() const;
};

struct MeshComponent
{
public:
	Ref<Mesh> MeshObject = nullptr;
	Ref<Image2D> Albedo = nullptr;

public:
	MeshComponent() = default;
	MeshComponent(Ref<Mesh> mesh, Ref<Image2D> albedo);
	MeshComponent(const MeshComponent& other) = default;
};

struct PointLightComponent
{
public:
	glm::vec3 Colour = { 1.0f, 1.0f, 1.0f };
	float Radius = 5.0f;
	float Intensity = 1.0f;

public:
	PointLightComponent() = default;
	PointLightComponent(const glm::vec3& colour, float radius, float intensity);
	PointLightComponent(const PointLightComponent& other) = default;
};



template<typename TComponent>
static std::string ComponentToString()
{
	if constexpr (std::is_same_v<TComponent, TransformComponent>)
		return "TransformComponent";
	else if constexpr (std::is_same_v<TComponent, MeshComponent>)
		return "MeshComponent";
	else if constexpr (std::is_same_v<TComponent, PointLightComponent>)
		return "PointLightComponent";

	return "Undefined Component";
}