#include "Camera.hpp"

#include <Swift/Core/Application.hpp>
#include <Swift/Core/Input/Input.hpp>

#include <Swift/Renderer/Renderer.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

void Camera::OnUpdate(float deltaTime)
{
	switch (m_State)
	{
	case State::ArcBall:
		UpdateArcBall(deltaTime);
		break;
	case State::FlyCam:
		UpdateFlyCam(deltaTime);
		break;

	default:
		APP_LOG_ERROR("No proper camera state selected");
		break;
	}
}


void Camera::OnEvent(Event& e)
{
	EventHandler handler(e);

	handler.Handle<MouseScrolledEvent>(APP_BIND_EVENT_FN(Camera::OnMouseScroll));
}

void Camera::SwitchState()
{
	switch (m_State)
	{
	case State::ArcBall:
		m_State = State::FlyCam;

		// Reset camera
		glm::vec3 direction = glm::normalize(glm::vec3(0.0f) - m_Position);
		m_Yaw = glm::degrees(atan2(direction.z, direction.x));
		m_Pitch = glm::degrees(asin(direction.y));
		break;
	case State::FlyCam:
		m_State = State::ArcBall;
		break;

	default:
		APP_LOG_ERROR("No proper camera state selected");
		break;
	}
}

Ref<Camera> Camera::Create()
{
	return RefHelper::Create<Camera>();
}

void Camera::UpdateArcBall(float deltaTime)
{
	if (Application::Get().GetWindow().GetWidth() != 0 && Application::Get().GetWindow().GetHeight() != 0)
	{
		static float theta = 0.0f;
		static float phi = 0.0f;

		static glm::vec2 lastPosition = { 0.0f, 0.0f };
		glm::vec2 position = Input::GetMousePosition();

		if (Input::IsMousePressed(MouseButton::Right))
		{
			glm::vec2 delta = position - lastPosition;

			float dTheta = -delta.x * m_Speed;
			float dPhi = delta.y * m_Speed;

			theta += dTheta;
			phi += dPhi;

			float epsilon = 0.001f; // To prevent the camera from flipping
			phi = glm::clamp(phi, -glm::half_pi<float>() + epsilon, glm::half_pi<float>() - epsilon);
		}

		lastPosition = position;

		m_Position = glm::vec3(
			m_Radius * glm::sin(theta) * glm::cos(phi),
			m_Radius * glm::sin(phi),
			m_Radius * glm::cos(theta) * glm::cos(phi)
		);

		m_Camera.View = glm::lookAt(m_Position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		m_Camera.Projection = glm::perspective(glm::radians(m_FOV), (float)Application::Get().GetWindow().GetWidth() / (float)Application::Get().GetWindow().GetHeight(), m_Near, m_Far);
		if (RendererSpecification::API == RendererSpecification::RenderingAPI::Vulkan)
			m_Camera.Projection[1][1] *= -1;

		float depthLinearizeMul = (-m_Camera.Projection[3][2]);
		float depthLinearizeAdd = (m_Camera.Projection[2][2]);
		// correct the handedness issue.
		if (depthLinearizeMul * depthLinearizeAdd < 0)
			depthLinearizeAdd = -depthLinearizeAdd;
		m_Camera.DepthUnpackConsts = { depthLinearizeMul, depthLinearizeAdd };
	}
}

void Camera::UpdateFlyCam(float deltaTime)
{
	if (Application::Get().GetWindow().GetWidth() != 0 && Application::Get().GetWindow().GetHeight() != 0)
	{
		if (Input::IsMousePressed(MouseButton::Right))
		{
			float velocity = m_MovementSpeed * deltaTime;
			glm::vec3 moveDirection = { 0.0f, 0.0f, 0.0f };

			// Calculate forward/backward and left/right movement.
			if (Input::IsKeyPressed(Key::W))
				moveDirection += m_Front;
			if (Input::IsKeyPressed(Key::S))
				moveDirection -= m_Front;
			if (Input::IsKeyPressed(Key::A))
				moveDirection += m_Right;
			if (Input::IsKeyPressed(Key::D))
				moveDirection -= m_Right;

			// Calculate up/down movement.
			if (Input::IsKeyPressed(Key::Space))
				moveDirection += m_Up;
			if (Input::IsKeyPressed(Key::LeftShift))
				moveDirection -= m_Up;

			if (glm::length(moveDirection) > 0.0f)
				moveDirection = glm::normalize(moveDirection);

			// Update the camera position.
			static glm::vec2 lastMousePosition = { 0.0f, 0.0f };
			m_Position += moveDirection * velocity;
			if (m_FirstUpdate)
			{
				lastMousePosition = Input::GetMousePosition();
				m_FirstUpdate = false;
			}

			// Mouse movement
			glm::vec2 mousePosition = Input::GetMousePosition();
			float xOffset = static_cast<float>(mousePosition.x - lastMousePosition.x);
			float yOffset = static_cast<float>(lastMousePosition.y - mousePosition.y);

			//Reset cursor
			Input::SetCursorPosition({ Application::Get().GetWindow().GetWidth() / 2.0f, Application::Get().GetWindow().GetHeight() / 2.0f });
			Input::SetCursorMode(CursorMode::Disabled);

			lastMousePosition.x = static_cast<float>(Application::Get().GetWindow().GetWidth() / 2.f);
			lastMousePosition.y = static_cast<float>(Application::Get().GetWindow().GetHeight() / 2.f);

			xOffset *= m_MouseSensitivity;
			yOffset *= m_MouseSensitivity;

			//Set new settings
			m_Yaw -= xOffset;
			m_Pitch += yOffset;

			// Cap movement
			if (m_Pitch > 89.0f)
				m_Pitch = 89.0f;
			if (m_Pitch < -89.0f)
				m_Pitch = -89.0f;
		}
		else
		{
			Input::SetCursorMode(CursorMode::Shown);
			m_FirstUpdate = true;
		}

		glm::vec3 newFront(1.0f);
		newFront.x = glm::cos(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch));
		newFront.y = glm::sin(glm::radians(m_Pitch));
		newFront.z = glm::sin(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch));

		m_Front = glm::normalize(newFront);
		m_Right = glm::normalize(glm::cross(m_Front, m_Up));

		// Update everything
		m_Camera.View = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
		m_Camera.Projection = glm::perspective(glm::radians(m_FOV), (float)Application::Get().GetWindow().GetWidth() / (float)Application::Get().GetWindow().GetHeight(), m_Near, m_Far);
		if (RendererSpecification::API == RendererSpecification::RenderingAPI::Vulkan)
			m_Camera.Projection[1][1] *= -1;

		float depthLinearizeMul = (-m_Camera.Projection[3][2]);
		float depthLinearizeAdd = (m_Camera.Projection[2][2]);
		// correct the handedness issue.
		if (depthLinearizeMul * depthLinearizeAdd < 0)
			depthLinearizeAdd = -depthLinearizeAdd;
		m_Camera.DepthUnpackConsts = { depthLinearizeMul, depthLinearizeAdd };
	}
}

bool Camera::OnMouseScroll(MouseScrolledEvent& e)
{
	if (Input::IsMousePressed(MouseButton::Right))
	{
		float change = m_Change * e.GetYOffset();
		m_Radius -= change;
	}

	return false;
}