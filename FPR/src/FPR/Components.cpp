#include "Components.hpp"

#include <glm/gtc/matrix_transform.hpp>

TransformComponent::TransformComponent(const glm::vec3& position, const glm::vec3& size, const glm::vec3& rotation)
	: Position(position), Size(size), Rotation(rotation)
{
}

glm::mat4 TransformComponent::GetMatrix() const
{
	glm::mat4 matrix = glm::mat4(1.0f);
	matrix = glm::translate(matrix, Position);
	matrix = glm::scale(matrix, Size);
	
	// Note(Jorben): There is probably a better way to do rotation
	matrix = glm::rotate(matrix, glm::radians(Rotation.x), { 1.0f, 0.0f, 0.0f });
	matrix = glm::rotate(matrix, glm::radians(Rotation.y), { 0.0f, 1.0f, 0.0f });
	matrix = glm::rotate(matrix, glm::radians(Rotation.z), { 0.0f, 0.0f, 1.0f });

	return matrix;
}

MeshComponent::MeshComponent(Ref<Mesh> mesh, Ref<Image2D> albedo)
	: MeshObject(mesh), Albedo(albedo)
{
}

PointLightComponent::PointLightComponent(const glm::vec3& colour, float radius, float intensity)
	: Colour(colour), Radius(radius), Intensity(intensity)
{
}
