#pragma once
#include <math.h>

struct Vec3
{
	float x;
	float y;
	float z;

	Vec3() : x(0), y(0), z(0) {}
	Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

	inline Vec3 normalize() const
	{
		float length = sqrt(x * x + y * y + z * z);
		if (length == 0) return Vec3(0, 0, 0);
		return Vec3(x / length, y / length, z / length);
	}
	inline float length() const
	{
		return sqrt(x * x + y * y + z * z);
	}
	inline float lengthSq() const
	{
		return x * x + y * y + z * z;
	}
	inline float dot(const Vec3& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}
	/// <summary>
	/// 0: x, 1: y, 2: z(軸ごとに同じ処理をする時に使う)
	/// </summary>
	inline float& operator[](int axis)
	{
		return axis == 0 ? x : axis == 1 ? y : z;
	}
	inline float operator[](int axis) const
	{
		return axis == 0 ? x : axis == 1 ? y : z;
	}

	inline Vec3 operator+(const Vec3& other) const
	{
		return Vec3(x + other.x, y + other.y, z + other.z);
	}
	inline Vec3 operator-(const Vec3& other) const
	{
		return Vec3(x - other.x, y - other.y, z - other.z);
	}
	inline Vec3 operator-() const
	{
		return Vec3(-x, -y, -z);
	}
	inline Vec3 operator*(float scalar) const
	{
		return Vec3(x * scalar, y * scalar, z * scalar);
	}
	inline Vec3 operator/(float scalar) const
	{
		return Vec3(x / scalar, y / scalar, z / scalar);
	}
	inline void operator+=(const Vec3& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
	}
	inline void operator-=(const Vec3& other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
	}
	inline void operator*=(float scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
	}
	inline void operator/=(float scalar)
	{
		x /= scalar;
		y /= scalar;
		z /= scalar;
	}
	inline bool operator==(const Vec3& other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}
	inline bool operator!=(const Vec3& other) const
	{
		return !(*this == other);
	}
};
