#pragma once
#include "MathUtil.h"

/// <summary>
/// エディタで 3D を見回すカメラ。注目点のまわりを回る(右ドラッグで回す、中ドラッグで動かす、ホイールで寄る)
/// </summary>
struct EditorCamera
{
	glm::vec3 target{ 0, 0, 0 };
	float distance = 16;
	float yaw = 0;     //度
	float pitch = -30; //度(下向きが負)

	glm::vec3 forward() const
	{
		float p = glm::radians(pitch), y = glm::radians(yaw);
		return glm::vec3(-std::cos(p) * std::sin(y), std::sin(p), -std::cos(p) * std::cos(y));
	}
	glm::vec3 position() const { return target - forward() * distance; }
	glm::mat4 view() const { return glm::lookAt(position(), target, glm::vec3(0, 1, 0)); }

	void orbit(float dx, float dy)
	{
		yaw -= dx * 0.3f;
		pitch = glm::clamp(pitch - dy * 0.3f, -89.0f, 89.0f);
	}
	void pan(float dx, float dy)
	{
		glm::vec3 f = forward();
		glm::vec3 right = glm::normalize(glm::cross(f, glm::vec3(0, 1, 0)));
		glm::vec3 up = glm::cross(right, f);
		float k = distance * 0.0015f;
		target += (-right * dx + up * dy) * k;
	}
	void zoom(float wheel)
	{
		distance = glm::clamp(distance * std::pow(0.88f, wheel), 0.5f, 500.0f);
	}
};
