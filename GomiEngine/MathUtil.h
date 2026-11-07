#pragma once
#include "Vec2.h"
#include "Vec3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

/// <summary>
/// 3D の座標系は右手系・Y が上(OpenGL・glTF と同じ)。カメラは -Z を向く。
/// 2D(スプライト・UI)は画面のピクセル座標で Y が下。
/// 回転はコンポーネント上は度(エディタで扱いやすいように)、計算はラジアン
/// </summary>
namespace MathUtil
{
	constexpr float PI_F = 3.14159265358979f;

	inline float toRadians(float degrees) { return degrees * (PI_F / 180.0f); }
	inline float toDegrees(float radians) { return radians * (180.0f / PI_F); }

	inline glm::vec3 toGlm(const Vec3& v) { return glm::vec3(v.x, v.y, v.z); }
	inline Vec3 fromGlm(const glm::vec3& v) { return Vec3(v.x, v.y, v.z); }

	/// <summary>
	/// 度の回転 (x: ピッチ, y: ヨー, z: ロール) を行列に。Y → X → Z の順に回す
	/// </summary>
	inline glm::mat4 rotationMatrix(const Vec3& rotationDegrees)
	{
		return glm::yawPitchRoll(toRadians(rotationDegrees.y), toRadians(rotationDegrees.x), toRadians(rotationDegrees.z));
	}

	/// <summary>
	/// 位置・回転(度)・拡大からモデル行列を作る
	/// </summary>
	inline glm::mat4 modelMatrix(const Vec3& position, const Vec3& rotationDegrees, const Vec3& scale)
	{
		glm::mat4 m = glm::translate(glm::mat4(1.0f), toGlm(position));
		m = m * rotationMatrix(rotationDegrees);
		return glm::scale(m, toGlm(scale));
	}

	/// <summary>
	/// modelMatrix の逆。行列を位置・回転(度、Y→X→Z)・拡大に戻す(せん断の無い行列に限る)
	/// </summary>
	inline void decompose(const glm::mat4& m, Vec3& position, Vec3& rotationDegrees, Vec3& scale)
	{
		glm::vec3 s(glm::length(glm::vec3(m[0])), glm::length(glm::vec3(m[1])), glm::length(glm::vec3(m[2])));
		glm::mat4 rot(1.0f);
		for (int i = 0; i < 3; i++) rot[i] = glm::vec4(glm::vec3(m[i]) / (s[i] > 0 ? s[i] : 1.0f), 0);
		float yaw, pitch, roll;
		glm::extractEulerAngleYXZ(rot, yaw, pitch, roll);
		position = fromGlm(glm::vec3(m[3]));
		rotationDegrees = Vec3(toDegrees(pitch), toDegrees(yaw), toDegrees(roll));
		scale = fromGlm(s);
	}

	/// <summary>
	/// from から to を向く回転(度、ロールは 0)
	/// </summary>
	inline Vec3 lookRotation(const Vec3& from, const Vec3& to)
	{
		glm::vec3 d = toGlm(to) - toGlm(from);
		float len = glm::length(d);
		if (len <= 0.0f) return Vec3(0, 0, 0);
		d /= len;
		float yaw = std::atan2(-d.x, -d.z);
		float pitch = std::asin(glm::clamp(d.y, -1.0f, 1.0f));
		return Vec3(toDegrees(pitch), toDegrees(yaw), 0.0f);
	}
}
