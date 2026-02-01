#ifndef PBRT_GRAPHICS_VEC_UTILS_H
#define PBRT_GRAPHICS_VEC_UTILS_H

#include "base/base.h"
#include "util/pstd.h"
#include "util/math.h"
#include "glm.hpp"

#include <algorithm>
#include <cmath>

// ==================== Vector2f Declaration ====================
struct Vector2f {
	// Constructors
	Vector2f() = default;
	Vector2f(Float v);
	Vector2f(Float x, Float y);
	Vector2f(const Vector2f& v);
	Vector2f(Vector2f&& v) noexcept;
	explicit Vector2f(const Point2f& p);
	
#ifdef USE_DOUBLE_AS_FLOAT
	explicit Vector2f(glm::dvec2 data);
#else
	explicit Vector2f(glm::vec2 data);
#endif

	// Assignment operators
	Vector2f& operator=(const Vector2f& v);
	Vector2f& operator=(Vector2f&& v) noexcept;

	// Arithmetic operators
	Vector2f operator+(const Vector2f& v) const;
	Vector2f& operator+=(const Vector2f& v);
	Vector2f operator-(const Vector2f& v) const;
	Vector2f& operator-=(const Vector2f& v);
	Vector2f operator*(const Float& f) const;
	Vector2f& operator*=(const Float& f);
	Vector2f operator/(const Float& f) const;
	Vector2f& operator/=(const Float& f);
	Vector2f operator-() const;

	// Comparison operators
	bool operator==(const Vector2f& v) const;
	bool operator!=(const Vector2f& v) const;

	// Accessors
	Float& x();
	const Float& x() const;
	Float& y();
	const Float& y() const;
	Float& u();
	const Float& u() const;
	Float& v();
	const Float& v() const;
	Float& operator[](int i);
	const Float& operator[](int i) const;

	// Math methods
	Float norm2() const;
	Float norm() const;

#ifdef USE_DOUBLE_AS_FLOAT
	glm::dvec2 data;
#else
	glm::vec2 data;
#endif
};

// ==================== Vector3f Declaration ====================
struct Vector3f {
	// Constructors
	Vector3f() = default;
	Vector3f(Float v);
	Vector3f(Float x, Float y, Float z);
	explicit Vector3f(const Normal& n);
	Vector3f(const Vector3f& v);
	Vector3f(Vector3f&& v) noexcept;
	
#ifdef USE_DOUBLE_AS_FLOAT
	explicit Vector3f(glm::dvec3 data);
#else
	explicit Vector3f(glm::vec3 data);
#endif

	// Assignment operators
	Vector3f& operator=(const Vector3f& v);
	Vector3f& operator=(Vector3f&& v) noexcept;

	// Arithmetic operators
	Vector3f operator+(const Vector3f& v) const;
	Vector3f& operator+=(const Vector3f& v);
	Vector3f operator-(const Vector3f& v) const;
	Vector3f& operator-=(const Vector3f& v);
	Vector3f operator*(const Float& f) const;
	Vector3f& operator*=(const Float& f);
	Vector3f operator/(const Float& f) const;
	Vector3f& operator/=(const Float& f);
	Vector3f operator-() const;

	// Comparison operators
	bool operator==(const Vector3f& v) const;
	bool operator!=(const Vector3f& v) const;

	// Accessors
	Float& x();
	const Float& x() const;
	Float& y();
	const Float& y() const;
	Float& z();
	const Float& z() const;
	Float& operator[](int i);
	const Float& operator[](int i) const;

	// Math methods
	Float norm2() const;
	Float norm() const;

#ifdef USE_DOUBLE_AS_FLOAT
	glm::dvec3 data;
#else
	glm::vec3 data;
#endif
};

// ==================== Vector4f Declaration ====================
struct Vector4f {
	// Constructors
	Vector4f() = default;
	Vector4f(Float v);
	Vector4f(Float x, Float y, Float z, Float w);
	Vector4f(const Vector3f& v, Float w);
	Vector4f(const Vector4f& v);
	Vector4f(Vector4f&& v) noexcept;

#ifdef USE_DOUBLE_AS_FLOAT
	explicit Vector4f(glm::dvec4 data);
#else
	explicit Vector4f(glm::vec4 data);
#endif

	// Assignment operators
	Vector4f& operator=(const Vector4f& v);
	Vector4f& operator=(Vector4f&& v) noexcept;

	// Arithmetic operators
	Vector4f operator+(const Vector4f& v) const;
	Vector4f& operator+=(const Vector4f& v);
	Vector4f operator-(const Vector4f& v) const;
	Vector4f& operator-=(const Vector4f& v);
	Vector4f operator*(const Float& f) const;
	Vector4f& operator*=(const Float& f);
	Vector4f operator/(const Float& f) const;
	Vector4f& operator/=(const Float& f);
	Vector4f operator-() const;

	// Comparison operators
	bool operator==(const Vector4f& v) const;
	bool operator!=(const Vector4f& v) const;

	// Accessors
	Float& x();
	const Float& x() const;
	Float& y();
	const Float& y() const;
	Float& z();
	const Float& z() const;
	Float& w();
	const Float& w() const;
	Float& operator[](int i);
	const Float& operator[](int i) const;

	// Math methods
	Float norm2() const;
	Float norm() const;
	Vector3f xyz() const;

#ifdef USE_DOUBLE_AS_FLOAT
	glm::dvec4 data;
#else
	glm::vec4 data;
#endif
};

// ==================== Point2f Declaration ====================
struct Point2f {
	Point2f() = default;
	Point2f(Float p);
	Point2f(Float x, Float y);
	Point2f(const Point2f& v);
	Point2f(Point2f&& v) noexcept;
	
#ifdef USE_DOUBLE_AS_FLOAT
	explicit Point2f(glm::dvec2 data);
#else
	explicit Point2f(glm::vec2 data);
#endif

	Point2f& operator=(const Point2f& v);
	Point2f& operator=(Point2f&& v) noexcept;

	Float& x();
	const Float& x() const;
	Float& y();
	const Float& y() const;
	Float& u();
	const Float& u() const;
	Float& v();
	const Float& v() const;

	Point2f operator+(const Point2f& v) const;
	Point2f& operator+=(const Point2f& v);
	Point2f operator-(const Point2f& v) const;
	Point2f& operator-=(const Point2f& v);
	Point2f operator*(const Float& f) const;
	Point2f& operator*=(const Float& f);
	Point2f operator/(const Float& f) const;
	Point2f& operator/=(const Float& f);
	Point2f operator-() const;
	bool operator==(const Point2f& point) const;
	bool operator!=(const Point2f& point) const;
	Float& operator[](int i);
	const Float& operator[](int i) const;

#ifdef USE_DOUBLE_AS_FLOAT
	glm::dvec2 data;
#else
	glm::vec2 data;
#endif
};

// ==================== Point2i Declaration ====================
struct Point2i {
	Point2i() = default;
	Point2i(int p);
	Point2i(int x, int y);
	Point2i(const Point2i& v);
	Point2i(Point2i&& v) noexcept;
	explicit Point2i(glm::ivec2 data);
	explicit Point2i(const Point2f& p);

	// Assignment operators
	Point2i& operator=(const Point2i& v);
	Point2i& operator=(Point2i&& v) noexcept;

	int& x();
	const int& x() const;
	int& y();
	const int& y() const;

	Point2i operator+(const Point2i& v) const;
	Point2i& operator+=(const Point2i& v);
	Point2i operator-(const Point2i& v) const;
	Point2i& operator-=(const Point2i& v);
	Point2i operator*(int f) const;
	Point2i& operator*=(int f);
	Point2i operator/(int f) const;
	Point2i& operator/=(int f);
	Point2i operator-() const;
	bool operator==(const Point2i& point) const;
	bool operator!=(const Point2i& point) const;
	int& operator[](int i);
	const int& operator[](int i) const;

	glm::ivec2 data;
};

// ==================== Point3f Declaration ====================
struct Point3f {
	Point3f() = default;
	Point3f(Float v);
	Point3f(Float x, Float y, Float z);
	Point3f(const Point3f& v);
	Point3f(Point3f&& v) noexcept;
	
#ifdef USE_DOUBLE_AS_FLOAT
	explicit Point3f(glm::dvec3 data);
#else
	explicit Point3f(glm::vec3 data);
#endif

	Point3f& operator=(const Point3f& v);
	Point3f& operator=(Point3f&& v) noexcept;

	Float& x();
	const Float& x() const;
	Float& y();
	const Float& y() const;
	Float& z();
	const Float& z() const;

	Point3f operator+(const Vector3f& v) const;
	Point3f& operator+=(const Vector3f& v);
	Point3f operator-(const Vector3f& v) const;
	Point3f& operator-=(const Vector3f& v);
	Point3f operator+(const Point3f& point) const;
	Vector3f operator-(const Point3f& point) const;
	Point3f operator*(const Float& f) const;
	Point3f& operator*=(const Float& f);
	Point3f operator/(const Float& f) const;
	Point3f& operator/=(const Float& f);
	Point3f operator-() const;
	bool operator==(const Point3f& point) const;
	bool operator!=(const Point3f& point) const;
	Float& operator[](int i);
	const Float& operator[](int i) const;

#ifdef USE_DOUBLE_AS_FLOAT
	glm::dvec3 data;
#else
	glm::vec3 data;
#endif
};



// ==================== Normal Declaration ====================
struct Normal {
	Normal() = default;
	Normal(Float v);
	Normal(Float x, Float y, Float z);
	explicit Normal(const Vector3f& v);
	Normal(const Normal& v);
	Normal(Normal&& v) noexcept;
	
#ifdef USE_DOUBLE_AS_FLOAT
	explicit Normal(glm::dvec3 data);
#else
	explicit Normal(glm::vec3 data);
#endif

	Normal& operator=(const Normal& v);
	Normal& operator=(Normal&& v) noexcept;

	Normal operator+(const Vector3f& v) const;
	Normal& operator+=(const Normal& v);
	Normal operator-(const Normal& v) const;
	Normal& operator-=(const Normal& v);
	Normal operator*(const Float& f) const;
	Normal& operator*=(const Float& f);
	Normal operator/(const Float& f) const;
	Normal& operator/=(const Float& f);
	Normal operator-() const;
	bool operator==(const Normal& point) const;
	bool operator!=(const Normal& point) const;
	Float& operator[](int i);
	const Float& operator[](int i) const;

	Float& x();
	const Float& x() const;
	Float& y();
	const Float& y() const;
	Float& z();
	const Float& z() const;

	Float norm2() const;
	Float norm() const;

#ifdef USE_DOUBLE_AS_FLOAT
	glm::dvec3 data;
#else
	glm::vec3 data;
#endif
};

// ==================== Global Function Declarations ====================
// Vector2f
Vector2f operator*(const Float& f, const Vector2f& v);
Float Dot(const Vector2f& v1, const Vector2f& v2);
Float Cross(const Vector2f& v1, const Vector2f& v2);
Float MinComponentValue(const Vector2f& v);
Float MaxComponentValue(const Vector2f& v);
size_t MinComponentValueIndex(const Vector2f& v);
size_t MaxComponentValueIndex(const Vector2f& v);
Vector2f Min(const Vector2f& v1, const Vector2f& v2);
Vector2f Max(const Vector2f& v1, const Vector2f& v2);
Vector2f Abs(const Vector2f& v);
Vector2f Lerp(Float t, const Vector2f& v1, const Vector2f& v2);

// Point2f
Point2f operator*(const Float& f, const Point2f& p);
Point2f Lerp(Float t, const Point2f& p1, const Point2f& p2);

// Point2i
Point2i operator*(int f, const Point2i& p);
Point2i Min(const Point2i& p1, const Point2i& p2);
Point2i Max(const Point2i& p1, const Point2i& p2);
Point2i Abs(const Point2i& p);

// Point3f
Point3f operator*(const Float& f, const Point3f& p);
Float Distance(const Point3f& p1, const Point3f& p2);
Float DistanceSquared(const Point3f& p1, const Point3f& p2);
Point3f Lerp(Float t, const Point3f& p1, const Point3f& p2);
Point3f Max(const Point3f& p1, const Point3f& p2);
Point3f Min(const Point3f& p1, const Point3f& p2);
Point3f Floor(const Point3f& p);
Point3f Ceil(const Point3f& p);
Point3f Abs(const Point3f& p);

// Vector3f
Vector3f operator*(const Float& f, const Vector3f& v);
Float Dot(const Vector3f& v1, const Vector3f& v2);
Vector3f Cross(const Vector3f& v1, const Vector3f& v2);
Float MinComponentValue(const Vector3f& v);
Float MaxComponentValue(const Vector3f& v);
size_t MinComponentValueIndex(const Vector3f& v);
size_t MaxComponentValueIndex(const Vector3f& v);
Vector3f Min(const Vector3f& v1, const Vector3f& v2);
Vector3f Max(const Vector3f& v1, const Vector3f& v2);
Vector3f Abs(const Vector3f& v);
Vector3f Reflect(const Vector3f& wi, const Vector3f& n);

// Vector4f
Vector4f operator*(const Float& f, const Vector4f& v);
Float Dot(const Vector4f& v1, const Vector4f& v2);
Float MinComponentValue(const Vector4f& v);
Float MaxComponentValue(const Vector4f& v);
size_t MinComponentValueIndex(const Vector4f& v);
size_t MaxComponentValueIndex(const Vector4f& v);
Vector4f Min(const Vector4f& v1, const Vector4f& v2);
Vector4f Max(const Vector4f& v1, const Vector4f& v2);
Vector4f Abs(const Vector4f& v);
Vector4f Lerp(Float t, const Vector4f& v1, const Vector4f& v2);

// Normal
Normal operator*(const Float& f, const Normal& n);

// Type traits
template<typename N>
struct is_vector_or_normal : std::false_type {};
template<>
struct is_vector_or_normal<Vector3f> : std::true_type {};
template<>
struct is_vector_or_normal<Normal> : std::true_type {};

// Normalize functions
template<typename T>
T& Normalize(T& v);

template<typename T>
T Normalize(const T& v);


class Bounds {
public:
	Bounds();
	
	Bounds(const Bounds&) = default;
	Bounds& operator=(const Bounds&) = default;
	Bounds(Bounds&&) = default;
	Bounds& operator=(Bounds&&) = default;

	Bounds(const Point3f& p1, const Point3f& p2);
	explicit Bounds(const Point3f& p);

	bool operator==(const Bounds& other) const;
	bool operator!=(const Bounds& other) const;

	Point3f operator[](int i) const;
	Point3f operator[](int i);

	Vector3f diagonal() const;
	Float surface_area() const;
	Float volume() const;
	size_t maxDimension() const;
	bool isInside(const Point3f& p) const;
	bool isEmpty() const;
	Point3f center() const;
	Vector3f offset(Point3f p) const;
	Point3f Lerp(Point3f t) const;
	
	bool IntersectP(Point3f o, Vector3f d, Float tMax = INFINITY, 
	           Float* hitt0 = nullptr, Float* hitt1 = nullptr) const;
	
	bool IntersectP(Point3f o, Vector3f d, Float raytMax, 
	    Vector3f invDir, const int dirIsNeg[3]) const;

	Point3f min, max;
};

Bounds Union(const Bounds& b1, const Bounds& b2);
Bounds Union(const Bounds& b, const Point3f& p);


#include "Vec.inl"

#endif // PBRT_GRAPHICS_VEC_UTILS_H

