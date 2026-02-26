#pragma once
#include <math.h>

struct Vec2
{
	float x;
	float y;

	Vec2() : x(0), y(0) {}
	Vec2(float x, float y) : x(x), y(y) {}

	inline Vec2 normalize()
	{
		float length = sqrt(x * x + y * y);
		if (length == 0) return Vec2(0, 0);
		return Vec2(x / length, y / length);
	}

	inline float length()
	{
		return sqrt(x * x + y * y);
	}
	inline float lengthSq()
	{
		return x * x + y * y;
	}

	inline float dot(const Vec2& other) const
	{
		return x * other.x + y * other.y;
	}

	inline Vec2 fromAngle(float angle)
	{
		return Vec2(cos(angle), sin(angle));
	}

	inline Vec2 operator+(const Vec2& other) const
	{
		return Vec2(x + other.x, y + other.y);
	}
	inline Vec2 operator-(const Vec2& other) const
	{
		return Vec2(x - other.x, y - other.y);
	}
	inline Vec2 operator*(float scalar) const
	{
		return Vec2(x * scalar, y * scalar);
	}
	inline Vec2 operator/(float scalar) const
	{
		return Vec2(x / scalar, y / scalar);
	}
	inline void operator+=(const Vec2& other)
	{
		x += other.x;
		y += other.y;
	}
	inline void operator-=(const Vec2& other)
	{
		x -= other.x;
		y -= other.y;
	}
	inline void operator*=(float scalar)
	{
		x *= scalar;
		y *= scalar;
	}
	inline void operator/=(float scalar)
	{
		x /= scalar;
		y /= scalar;
	}
	inline bool operator==(const Vec2& other) const
	{
		return x == other.x && y == other.y;
	}
	inline bool operator!=(const Vec2& other) const
	{
		return !(*this == other);
	}
};