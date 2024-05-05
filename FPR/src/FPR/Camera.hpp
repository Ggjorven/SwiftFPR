#pragma once

#include <Swift/Core/Core.hpp>
#include <Swift/Core/Events.hpp>
#include <Swift/Utils/Utils.hpp>

#include "FPR/Resources.hpp"

using namespace Swift;

class Camera
{
public:
	enum class State
	{
		None = 0, ArcBall, FlyCam
	};
public:
	Camera() = default;
	virtual ~Camera() = default;

	void OnUpdate(float deltaTime);
	void OnEvent(Event& e);

	void SwitchState();

	inline float& GetFOV() { return m_FOV; }
	inline float& GetFlyCamSpeed() { return m_MovementSpeed; }
	inline float& GetArcBallSpeed() { return m_Speed; }
	inline glm::vec3& GetPosition() { return m_Position; }

	inline State GetState() const { return m_State; }
	inline ShaderCamera& GetCamera() { return m_Camera; }

	static Ref<Camera> Create();

private:
	void UpdateArcBall(float deltaTime);
	void UpdateFlyCam(float deltaTime);

	bool OnMouseScroll(MouseScrolledEvent& e);

private:
	ShaderCamera m_Camera = {};

	// Main
	State m_State = State::ArcBall;
	float m_FOV = 45.0f;

	float m_Near = 0.1f;
	float m_Far = 1000.0f;

	glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };

	// Flycam
	glm::vec3 m_Front = { 0.0f, 0.0f, -1.0f };
	glm::vec3 m_Up = { 0.0f, 1.0f, 0.0f };
	glm::vec3 m_Right = { 1.0f, 0.0f, 0.0f };

	float m_Yaw = 0.0f;
	float m_Pitch = 0.0f;

	float m_MovementSpeed = 5.0f;
	float m_MouseSensitivity = 0.1f;

	bool m_FirstUpdate = true;

	// ArcBall
	float m_Radius = 4.0f;
	float m_Change = 0.5f;

	float m_Speed = 0.005f;
};