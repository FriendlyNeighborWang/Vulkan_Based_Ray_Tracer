// ==================== Vector3f Implementation ====================

inline Vector3f::Vector3f(Float v) : data(v) {}

inline Vector3f::Vector3f(Float x, Float y, Float z) : data(x, y, z) {}

inline Vector3f::Vector3f(const Normal& n) : data(n.data) {}

inline Vector3f::Vector3f(const Vector3f& v) : data(v.data) {}

inline Vector3f::Vector3f(Vector3f&& v) noexcept : data(std::move(v.data)) {}

#ifdef USE_DOUBLE_AS_FLOAT
inline Vector3f::Vector3f(glm::dvec3 data) : data(data) {}
#else
inline Vector3f::Vector3f(glm::vec3 data) : data(data) {}
#endif

inline Vector3f& Vector3f::operator=(const Vector3f& v) {
	if (this == &v) return *this;
	data = v.data;
	return *this;
}

inline Vector3f& Vector3f::operator=(Vector3f&& v) noexcept {
	if (this == &v) return *this;
	data = std::move(v.data);
	return *this;
}

inline Vector3f Vector3f::operator+(const Vector3f& v) const {
	return { x() + v.x(), y() + v.y(), z() + v.z() };
}

inline Vector3f& Vector3f::operator+=(const Vector3f& v) {
	x() += v.x();
	y() += v.y();
	z() += v.z();
	return *this;
}

inline Vector3f Vector3f::operator-(const Vector3f& v) const {
	return { x() - v.x(), y() - v.y(), z() - v.z() };
}

inline Vector3f& Vector3f::operator-=(const Vector3f& v) {
	x() -= v.x();
	y() -= v.y();
	z() -= v.z();
	return *this;
}

inline Vector3f Vector3f::operator*(const Float& f) const {
	return { x() * f, y() * f, z() * f };
}

inline Vector3f& Vector3f::operator*=(const Float& f) {
	x() *= f;
	y() *= f;
	z() *= f;
	return *this;
}

inline Vector3f Vector3f::operator/(const Float& f) const {
	Float inv = Float(1) / f;
	return { x() * inv, y() * inv, z() * inv };
}

inline Vector3f& Vector3f::operator/=(const Float& f) {
	Float inv = Float(1) / f;
	x() *= inv;
	y() *= inv;
	z() *= inv;
	return *this;
}

inline Vector3f Vector3f::operator-() const {
	return { -x(), -y(), -z() };
}

inline bool Vector3f::operator==(const Vector3f& v) const {
	return x() == v.x() && y() == v.y() && z() == v.z();
}

inline bool Vector3f::operator!=(const Vector3f& v) const {
	return !(*this == v);
}

inline Float& Vector3f::x() { return data.x; }
inline const Float& Vector3f::x() const { return data.x; }
inline Float& Vector3f::y() { return data.y; }
inline const Float& Vector3f::y() const { return data.y; }
inline Float& Vector3f::z() { return data.z; }
inline const Float& Vector3f::z() const { return data.z; }

inline Float& Vector3f::operator[](int i) {
	assert(i >= 0 && i < 3 && "Vector3f::operator[]: index out of range");
	return data[i];
}

inline const Float& Vector3f::operator[](int i) const {
	assert(i >= 0 && i < 3 && "Vector3f::operator[]: index out of range");
	return data[i];
}

inline Float Vector3f::norm2() const {
	return x() * x() + y() * y() + z() * z();
}

inline Float Vector3f::norm() const {
	return std::sqrt(norm2());
}

// ==================== Point2f Implementation ====================

inline Point2f::Point2f(Float p) : data(p) {}

inline Point2f::Point2f(Float x, Float y) : data(x, y) {}

inline Point2f::Point2f(const Point2f& v) : data(v.data) {}

inline Point2f::Point2f(Point2f&& v) noexcept : data(std::move(v.data)) {}

#ifdef USE_DOUBLE_AS_FLOAT
inline Point2f::Point2f(glm::dvec2 data) : data(data) {}
#else
inline Point2f::Point2f(glm::vec2 data) : data(data) {}
#endif

inline Point2f& Point2f::operator=(const Point2f& v) {
	if (this == &v) return *this;
	data = v.data;
	return *this;
}

inline Point2f& Point2f::operator=(Point2f&& v) noexcept {
	if (this == &v) return *this;
	data = std::move(v.data);
	return *this;
}

inline Float& Point2f::x() { return data.x; }
inline const Float& Point2f::x() const { return data.x; }
inline Float& Point2f::y() { return data.y; }
inline const Float& Point2f::y() const { return data.y; }
inline Float& Point2f::u() { return data.x; }
inline const Float& Point2f::u() const { return data.x; }
inline Float& Point2f::v() { return data.y; }
inline const Float& Point2f::v() const { return data.y; }

inline Point2f Point2f::operator+(const Point2f& v) const {
	return { x() + v.x(), y() + v.y() };
}

inline Point2f& Point2f::operator+=(const Point2f& v) {
	x() += v.x();
	y() += v.y();
	return *this;
}

inline Point2f Point2f::operator-(const Point2f& v) const {
	return { x() - v.x(), y() - v.y() };
}

inline Point2f& Point2f::operator-=(const Point2f& v) {
	x() -= v.x();
	y() -= v.y();
	return *this;
}

inline Point2f Point2f::operator*(const Float& f) const {
	return { x() * f, y() * f };
}

inline Point2f& Point2f::operator*=(const Float& f) {
	x() *= f;
	y() *= f;
	return *this;
}

inline Point2f Point2f::operator/(const Float& f) const {
	Float inv = Float(1) / f;
	return { x() * inv, y() * inv };
}

inline Point2f& Point2f::operator/=(const Float& f) {
	Float inv = Float(1) / f;
	x() *= inv;
	y() *= inv;
	return *this;
}

inline Point2f Point2f::operator-() const {
	return { -x(), -y() };
}

inline bool Point2f::operator==(const Point2f& point) const {
	return x() == point.x() && y() == point.y();
}

inline bool Point2f::operator!=(const Point2f& point) const {
	return !(*this == point);
}

inline Float& Point2f::operator[](int i) {
	assert(i >= 0 && i < 2 && "Point2f::operator[]: index out of range");
	return data[i];
}

inline const Float& Point2f::operator[](int i) const {
	assert(i >= 0 && i < 2 && "Point2f::operator[]: index out of range");
	return data[i];
}

// ==================== Point2i Implementation ====================

inline Point2i::Point2i(int p) : data(p) {}

inline Point2i::Point2i(int x, int y) : data(x, y) {}

inline Point2i::Point2i(const Point2i& v) : data(v.data) {}

inline Point2i::Point2i(Point2i&& v) noexcept : data(std::move(v.data)) {}

inline Point2i::Point2i(glm::ivec2 data) : data(data) {}

inline Point2i::Point2i(const Point2f& p) : data(static_cast<int>(p.x()), static_cast<int>(p.y())) {}

inline Point2i& Point2i::operator=(const Point2i& v) {
	if (this == &v) return *this;
	data = v.data;
	return *this;
}

inline Point2i& Point2i::operator=(Point2i&& v) noexcept {
	if (this == &v) return *this;
	data = std::move(v.data);
	return *this;
}

inline int& Point2i::x() { return data.x; }
inline const int& Point2i::x() const { return data.x; }
inline int& Point2i::y() { return data.y; }
inline const int& Point2i::y() const { return data.y; }

inline Point2i Point2i::operator+(const Point2i& v) const {
	return { x() + v.x(), y() + v.y() };
}

inline Point2i& Point2i::operator+=(const Point2i& v) {
	x() += v.x();
	y() += v.y();
	return *this;
}

inline Point2i Point2i::operator-(const Point2i& v) const {
	return { x() - v.x(), y() - v.y() };
}

inline Point2i& Point2i::operator-=(const Point2i& v) {
	x() -= v.x();
	y() -= v.y();
	return *this;
}

inline Point2i Point2i::operator*(int f) const {
	return { x() * f, y() * f };
}

inline Point2i& Point2i::operator*=(int f) {
	x() *= f;
	y() *= f;
	return *this;
}

inline Point2i Point2i::operator/(int f) const {
	return { x() / f, y() / f };
}

inline Point2i& Point2i::operator/=(int f) {
	x() /= f;
	y() /= f;
	return *this;
}

inline Point2i Point2i::operator-() const {
	return { -x(), -y() };
}

inline bool Point2i::operator==(const Point2i& point) const {
	return x() == point.x() && y() == point.y();
}

inline bool Point2i::operator!=(const Point2i& point) const {
	return !(*this == point);
}

inline int& Point2i::operator[](int i) {
	assert(i >= 0 && i < 2 && "Point2i::operator[]: index out of range");
	return data[i];
}

inline const int& Point2i::operator[](int i) const {
	assert(i >= 0 && i < 2 && "Point2i::operator[]: index out of range");
	return data[i];
}

// ==================== Point3f Implementation ====================

inline Point3f::Point3f(Float v) : data(v) {}

inline Point3f::Point3f(Float x, Float y, Float z) : data(x, y, z) {}

inline Point3f::Point3f(const Point3f& v) : data(v.data) {}

inline Point3f::Point3f(Point3f&& v) noexcept : data(std::move(v.data)) {}

#ifdef USE_DOUBLE_AS_FLOAT
inline Point3f::Point3f(glm::dvec3 data) : data(data) {}
#else
inline Point3f::Point3f(glm::vec3 data) : data(data) {}
#endif

inline Point3f& Point3f::operator=(const Point3f& v) {
	if (this == &v) return *this;
	data = v.data;
	return *this;
}

inline Point3f& Point3f::operator=(Point3f&& v) noexcept {
	if (this == &v) return *this;
	data = std::move(v.data);
	return *this;
}

inline Float& Point3f::x() { return data.x; }
inline const Float& Point3f::x() const { return data.x; }
inline Float& Point3f::y() { return data.y; }
inline const Float& Point3f::y() const { return data.y; }
inline Float& Point3f::z() { return data.z; }
inline const Float& Point3f::z() const { return data.z; }

inline Point3f Point3f::operator+(const Vector3f& v) const {
	return { x() + v.x(), y() + v.y(), z() + v.z() };
}

inline Point3f& Point3f::operator+=(const Vector3f& v) {
	x() += v.x();
	y() += v.y();
	z() += v.z();
	return *this;
}

inline Point3f Point3f::operator-(const Vector3f& v) const {
	return { x() - v.x(), y() - v.y(), z() - v.z() };
}

inline Point3f& Point3f::operator-=(const Vector3f& v) {
	x() -= v.x();
	y() -= v.y();
	z() -= v.z();
	return *this;
}

inline Point3f Point3f::operator+(const Point3f& point) const {
	return { x() + point.x(), y() + point.y(), z() + point.z() };
}

inline Vector3f Point3f::operator-(const Point3f& point) const {
	return { x() - point.x(), y() - point.y(), z() - point.z() };
}

inline Point3f Point3f::operator*(const Float& f) const {
	return { x() * f, y() * f, z() * f };
}

inline Point3f& Point3f::operator*=(const Float& f) {
	x() *= f;
	y() *= f;
	z() *= f;
	return *this;
}

inline Point3f Point3f::operator/(const Float& f) const {
	Float inv = Float(1) / f;
	return { x() * inv, y() * inv, z() * inv };
}

inline Point3f& Point3f::operator/=(const Float& f) {
	Float inv = Float(1) / f;
	x() *= inv;
	y() *= inv;
	z() *= inv;
	return *this;
}

inline Point3f Point3f::operator-() const {
	return { -x(), -y(), -z() };
}

inline bool Point3f::operator==(const Point3f& point) const {
	return x() == point.x() && y() == point.y() && z() == point.z();
}

inline bool Point3f::operator!=(const Point3f& point) const {
	return !(*this == point);
}

inline Float& Point3f::operator[](int i) {
	assert(i >= 0 && i < 3 && "Point3f::operator[]: index out of range");
	return data[i];
}

inline const Float& Point3f::operator[](int i) const {
	assert(i >= 0 && i < 3 && "Point3f::operator[]: index out of range");
	return data[i];
}

// ==================== Normal Implementation ====================

inline Normal::Normal(Float v) : data(v) {}

inline Normal::Normal(Float x, Float y, Float z) : data(x, y, z) {}

inline Normal::Normal(const Vector3f& v) : data(v.data) {}

inline Normal::Normal(const Normal& v) : data(v.data) {}

inline Normal::Normal(Normal&& v) noexcept : data(std::move(v.data)) {}

#ifdef USE_DOUBLE_AS_FLOAT
inline Normal::Normal(glm::dvec3 data) : data(data) {}
#else
inline Normal::Normal(glm::vec3 data) : data(data) {}
#endif

inline Normal& Normal::operator=(const Normal& v) {
	if (this == &v) return *this;
	data = v.data;
	return *this;
}

inline Normal& Normal::operator=(Normal&& v) noexcept {
	if (this == &v) return *this;
	data = std::move(v.data);
	return *this;
}

inline Normal Normal::operator+(const Vector3f& v) const {
	return Normal{ x() + v.x(), y() + v.y(), z() + v.z() };
}

inline Normal& Normal::operator+=(const Normal& v) {
	x() += v.x();
	y() += v.y();
	z() += v.z();
	return *this;
}

inline Normal Normal::operator-(const Normal& v) const {
	return Normal{ x() - v.x(), y() - v.y(), z() - v.z() };
}

inline Normal& Normal::operator-=(const Normal& v) {
	x() -= v.x();
	y() -= v.y();
	z() -= v.z();
	return *this;
}

inline Normal Normal::operator*(const Float& f) const {
	return Normal{ x() * f, y() * f, z() * f };
}

inline Normal& Normal::operator*=(const Float& f) {
	x() *= f;
	y() *= f;
	z() *= f;
	return *this;
}

inline Normal Normal::operator/(const Float& f) const {
	Float inv = Float(1) / f;
	return Normal{ x() * inv, y() * inv, z() * inv };
}

inline Normal& Normal::operator/=(const Float& f) {
	Float inv = Float(1) / f;
	x() *= inv;
	y() *= inv;
	z() *= inv;
	return *this;
}

inline Normal Normal::operator-() const {
	return Normal{ -x(), -y(), -z() };
}

inline bool Normal::operator==(const Normal& n) const {
	return x() == n.x() && y() == n.y() && z() == n.z();
}

inline bool Normal::operator!=(const Normal& n) const {
	return !(*this == n);
}

inline Float& Normal::x() { return data.x; }
inline const Float& Normal::x() const { return data.x; }
inline Float& Normal::y() { return data.y; }
inline const Float& Normal::y() const { return data.y; }
inline Float& Normal::z() { return data.z; }
inline const Float& Normal::z() const { return data.z; }

inline Float& Normal::operator[](int i) {
	assert(i >= 0 && i < 3 && "Normal::operator[]: index out of range");
	return data[i];
}

inline const Float& Normal::operator[](int i) const {
	assert(i >= 0 && i < 3 && "Normal::operator[]: index out of range");
	return data[i];
}

inline Float Normal::norm2() const {
	return x() * x() + y() * y() + z() * z();
}

inline Float Normal::norm() const {
	return std::sqrt(norm2());
}

// ==================== Global Functions Implementation ====================

// Point2f functions
inline Point2f operator*(const Float& f, const Point2f& p) {
	return p * f;
}

inline Point2f Lerp(Float t, const Point2f& p1, const Point2f& p2) {
	return (Float(1) - t) * p1 + t * p2;
}

// Point2i functions
inline Point2i operator*(int f, const Point2i& p) {
	return p * f;
}

inline Point2i Min(const Point2i& p1, const Point2i& p2) {
	return Point2i{ std::min(p1.x(), p2.x()), std::min(p1.y(), p2.y()) };
}

inline Point2i Max(const Point2i& p1, const Point2i& p2) {
	return Point2i{ std::max(p1.x(), p2.x()), std::max(p1.y(), p2.y()) };
}

inline Point2i Abs(const Point2i& p) {
	return Point2i{ std::abs(p.x()), std::abs(p.y()) };
}

// Point3f functions
inline Point3f operator*(const Float& f, const Point3f& p) {
	return p * f;
}

inline Float Distance(const Point3f& p1, const Point3f& p2) {
	return (p1 - p2).norm();
}

inline Float DistanceSquared(const Point3f& p1, const Point3f& p2) {
	return (p1 - p2).norm2();
}

inline Point3f Lerp(Float t, const Point3f& p1, const Point3f& p2) {
	return (Float(1) - t) * p1 + t * p2;
}

inline Point3f Max(const Point3f& p1, const Point3f& p2) {
	return Point3f{ std::max(p1.x(), p2.x()), std::max(p1.y(), p2.y()), std::max(p1.z(), p2.z()) };
}

inline Point3f Min(const Point3f& p1, const Point3f& p2) {
	return Point3f{ std::min(p1.x(), p2.x()), std::min(p1.y(), p2.y()), std::min(p1.z(), p2.z()) };
}

inline Point3f Floor(const Point3f& p) {
	return Point3f{ pstd::floor(p.x()), pstd::floor(p.y()), pstd::floor(p.z()) };
}

inline Point3f Ceil(const Point3f& p) {
	return Point3f{ pstd::ceil(p.x()), pstd::ceil(p.y()), pstd::ceil(p.z()) };
}

inline Point3f Abs(const Point3f& p) {
	return Point3f{ pstd::abs(p.x()), pstd::abs(p.y()), pstd::abs(p.z()) };
}

// Vector3f functions
inline Vector3f operator*(const Float& f, const Vector3f& v) {
	return v * f;
}

inline Float Dot(const Vector3f& v1, const Vector3f& v2) {
	return v1.x() * v2.x() + v1.y() * v2.y() + v1.z() * v2.z();
}

inline Vector3f Cross(const Vector3f& v1, const Vector3f& v2) {
		Float x = v1.y() * v2.z() - v1.z() * v2.y();
		Float y = v1.z() * v2.x() - v1.x() * v2.z();
		Float z = v1.x() * v2.y() - v1.y() * v2.x();
		return Vector3f{ x,y,z };
}

inline Float MinComponentValue(const Vector3f& v) {
	return std::min(std::min(v.x(), v.y()), v.z());
}

inline Float MaxComponentValue(const Vector3f& v) {
	return std::max(std::max(v.x(), v.y()), v.z());
}

inline size_t MinComponentValueIndex(const Vector3f& v) {
	return (v.x() < v.y()) ? ((v.x() < v.z()) ? 0 : 2) : ((v.y() < v.z()) ? 1 : 2);
}

inline size_t MaxComponentValueIndex(const Vector3f& v) {
	return (v.x() > v.y()) ? ((v.x() > v.z()) ? 0 : 2) : ((v.y() > v.z()) ? 1 : 2);
}

inline Vector3f Min(const Vector3f& v1, const Vector3f& v2) {
	return Vector3f(std::min(v1.x(), v2.x()), std::min(v1.y(), v2.y()), std::min(v1.z(), v2.z()));
}

inline Vector3f Max(const Vector3f& v1, const Vector3f& v2) {
	return Vector3f(std::max(v1.x(), v2.x()), std::max(v1.y(), v2.y()), std::max(v1.z(), v2.z()));
}

inline Vector3f Abs(const Vector3f& v) {
	return Vector3f(pstd::abs(v.x()), pstd::abs(v.y()), pstd::abs(v.z()));
}

inline Vector3f Reflect(const Vector3f& wi, const Vector3f& n) {
	return -wi + Float(2) * Dot(wi, n) * n;
}

// Normal functions
inline Normal operator*(const Float& f, const Normal& n) {
	return n * f;
}

// Template functions
template<typename T>
inline T& Normalize(T& v) {
	v /= v.norm();
	return v;
}

template<typename T>
inline T Normalize(const T& v) {
	return v / v.norm();
}

// ==================== Vector2f Implementation ====================

inline Vector2f::Vector2f(Float v) : data(v) {}

inline Vector2f::Vector2f(Float x, Float y) : data(x, y) {}

inline Vector2f::Vector2f(const Vector2f& v) : data(v.data) {}

inline Vector2f::Vector2f(Vector2f&& v) noexcept : data(std::move(v.data)) {}

inline Vector2f::Vector2f(const Point2f& p) : data(p.x(), p.y()) {}

#ifdef USE_DOUBLE_AS_FLOAT
inline Vector2f::Vector2f(glm::dvec2 data) : data(data) {}
#else
inline Vector2f::Vector2f(glm::vec2 data) : data(data) {}
#endif

inline Vector2f& Vector2f::operator=(const Vector2f& v) {
	if (this == &v) return *this;
	data = v.data;
	return *this;
}

inline Vector2f& Vector2f::operator=(Vector2f&& v) noexcept {
	if (this == &v) return *this;
	data = std::move(v.data);
	return *this;
}

inline Vector2f Vector2f::operator+(const Vector2f& v) const {
	return { x() + v.x(), y() + v.y() };
}

inline Vector2f& Vector2f::operator+=(const Vector2f& v) {
	x() += v.x();
	y() += v.y();
	return *this;
}

inline Vector2f Vector2f::operator-(const Vector2f& v) const {
	return { x() - v.x(), y() - v.y() };
}

inline Vector2f& Vector2f::operator-=(const Vector2f& v) {
	x() -= v.x();
	y() -= v.y();
	return *this;
}

inline Vector2f Vector2f::operator*(const Float& f) const {
	return { x() * f, y() * f };
}

inline Vector2f& Vector2f::operator*=(const Float& f) {
	x() *= f;
	y() *= f;
	return *this;
}

inline Vector2f Vector2f::operator/(const Float& f) const {
	Float inv = Float(1) / f;
	return { x() * inv, y() * inv };
}

inline Vector2f& Vector2f::operator/=(const Float& f) {
	Float inv = Float(1) / f;
	x() *= inv;
	y() *= inv;
	return *this;
}

inline Vector2f Vector2f::operator-() const {
	return { -x(), -y() };
}

inline bool Vector2f::operator==(const Vector2f& v) const {
	return x() == v.x() && y() == v.y();
}

inline bool Vector2f::operator!=(const Vector2f& v) const {
	return !(*this == v);
}

inline Float& Vector2f::x() { return data.x; }
inline const Float& Vector2f::x() const { return data.x; }
inline Float& Vector2f::y() { return data.y; }
inline const Float& Vector2f::y() const { return data.y; }
inline Float& Vector2f::u() { return data.x; }
inline const Float& Vector2f::u() const { return data.x; }
inline Float& Vector2f::v() { return data.y; }
inline const Float& Vector2f::v() const { return data.y; }

inline Float& Vector2f::operator[](int i) {
	assert(i >= 0 && i < 2 && "Vector2f::operator[]: index out of range");
	return data[i];
}

inline const Float& Vector2f::operator[](int i) const {
	assert(i >= 0 && i < 2 && "Vector2f::operator[]: index out of range");
	return data[i];
}

inline Float Vector2f::norm2() const {
	return x() * x() + y() * y();
}

inline Float Vector2f::norm() const {
	return std::sqrt(norm2());
}

// ==================== Global Functions Implementation ====================

// Vector2f functions
inline Vector2f operator*(const Float& f, const Vector2f& v) {
	return v * f;
}

inline Float Dot(const Vector2f& v1, const Vector2f& v2) {
	return v1.x() * v2.x() + v1.y() * v2.y();
}

inline Float Cross(const Vector2f& v1, const Vector2f& v2) {
	return v1.x() * v2.y() - v1.y() * v2.x();
}

inline Float MinComponentValue(const Vector2f& v) {
	return std::min(v.x(), v.y());
}

inline Float MaxComponentValue(const Vector2f& v) {
	return std::max(v.x(), v.y());
}

inline size_t MinComponentValueIndex(const Vector2f& v) {
	return (v.x() < v.y()) ? 0 : 1;
}

inline size_t MaxComponentValueIndex(const Vector2f& v) {
	return (v.x() > v.y()) ? 0 : 1;
}

inline Vector2f Min(const Vector2f& v1, const Vector2f& v2) {
	return Vector2f(std::min(v1.x(), v2.x()), std::min(v1.y(), v2.y()));
}

inline Vector2f Max(const Vector2f& v1, const Vector2f& v2) {
	return Vector2f(std::max(v1.x(), v2.x()), std::max(v1.y(), v2.y()));
}

inline Vector2f Abs(const Vector2f& v) {
	return Vector2f(pstd::abs(v.x()), pstd::abs(v.y()));
}

inline Vector2f Lerp(Float t, const Vector2f& v1, const Vector2f& v2) {
	return (Float(1) - t) * v1 + t * v2;
}

// Point2f functions

// ==================== Bounds Implementation ====================

inline Bounds::Bounds() {
	constexpr Float minNum = std::numeric_limits<Float>::lowest();
	constexpr Float maxNum = std::numeric_limits<Float>::max();
	min = Point3f(maxNum);
	max = Point3f(minNum);
}

inline Bounds::Bounds(const Point3f& p1, const Point3f& p2) 
	: min(Min(p1, p2)), max(Max(p1, p2)) {}

inline Bounds::Bounds(const Point3f& p) : min(p), max(p) {}

inline bool Bounds::operator==(const Bounds& other) const {
	return this->max == other.max && this->min == other.min;
}

inline bool Bounds::operator!=(const Bounds& other) const {
	return !(*this == other);
}

inline Point3f Bounds::operator[](int i) const {
	assert((i == 0 || i == 1) && "Bounds::operator[]: i must be 0 or 1");
	return (i == 0) ? min : max;
}

inline Point3f Bounds::operator[](int i) {
	assert((i == 0 || i == 1) && "Bounds::operator[]: i must be 0 or 1");
	return (i == 0) ? min : max;
}

inline Vector3f Bounds::diagonal() const {
	return max - min;
}

inline Float Bounds::surface_area() const {
	Vector3f d = diagonal();
	return 2 * (d.x() * d.y() + d.x() * d.z() + d.y() * d.z());
}

inline Float Bounds::volume() const {
	Vector3f d = diagonal();
	return d.x() * d.y() * d.z();
}

inline int Bounds::maxDimension() const {
	Vector3f d = diagonal();
	return MaxComponentValueIndex(d);
}

inline bool Bounds::isInside(const Point3f& p) const {
	return (
		p.x() >= min.x() && p.x() <= max.x() &&
		p.y() >= min.y() && p.y() <= max.y() &&
		p.z() >= min.z() && p.z() <= max.z()
	);
}

inline bool Bounds::isEmpty() const {
	return min.x() > max.x() || min.y() > max.y() || min.z() > max.z();
}

inline Point3f Bounds::center() const {
	return Point3f(
		(min.x() + max.x()) * Float(0.5),
		(min.y() + max.y()) * Float(0.5),
		(min.z() + max.z()) * Float(0.5)
	);
}

inline Vector3f Bounds::offset(Point3f p) const {
	Vector3f o = p - min;
	if (max.x() > min.x())
		o.x() /= max.x() - min.x();
	if (max.y() > min.y())
		o.y() /= max.y() - min.y();
	if (max.z() > min.z())
		o.z() /= max.z() - min.z();
	return o;
}

inline Point3f Bounds::Lerp(Point3f t) const {
	return Point3f(
		pstd::Lerp(t.x(), min.x(), max.x()),
		pstd::Lerp(t.y(), min.y(), max.y()),
		pstd::Lerp(t.z(), min.z(), max.z())
	);
}

inline bool Bounds::IntersectP(Point3f o, Vector3f d, Float tMax, 
      Float* hitt0, Float* hitt1) const {
	Float t0 = 0, t1 = tMax;
	for (int i = 0; i < 3; ++i) {
		Float invRayDir = 1 / d[i];

		if (pstd::abs(d[i]) < pstd::Sigma) {
			if (o[i] < min[i] || o[i] > max[i])
				return false;
			continue;
		}

		Float tNear = (min[i] - o[i]) * invRayDir;
		Float tFar = (max[i] - o[i]) * invRayDir;
		if (tNear > tFar) pstd::swap(tNear, tFar);

		t0 = tNear > t0 ? tNear : t0;
		t1 = tFar < t1 ? tFar : t1;
		if (t0 > t1)
			return false;
	}

	if (hitt0) *hitt0 = t0;
	if (hitt1) *hitt1 = t1;

	return true;
}

inline bool Bounds::IntersectP(Point3f o, Vector3f d, Float raytMax, 
       Vector3f invDir, const int dirIsNeg[3]) const {
	const Bounds& bounds = *this;

	Float tMin = (bounds[dirIsNeg[0]].x() - o.x()) * invDir.x();
	Float tMax = (bounds[1 - dirIsNeg[0]].x() - o.x()) * invDir.x();
	Float tyMin = (bounds[dirIsNeg[1]].y() - o.y()) * invDir.y();
	Float tyMax = (bounds[1 - dirIsNeg[1]].y() - o.y()) * invDir.y();

	if (tMin > tyMax || tyMin > tMax)
		return false;
	if (tyMin > tMin)
		tMin = tyMin;
	if (tyMax < tMax)
		tMax = tyMax;

	Float tzMin = (bounds[dirIsNeg[2]].z() - o.z()) * invDir.z();
	Float tzMax = (bounds[1 - dirIsNeg[2]].z() - o.z()) * invDir.z();

	if (tMin > tzMax || tzMin > tMax)
		return false;
	if (tzMin > tMin)
		tMin = tzMin;
	if (tzMax < tMax)
		tMax = tzMax;

	return (tMin < raytMax) && (tMax > 0);
}

inline Bounds Union(const Bounds& b1, const Bounds& b2) {
	return Bounds(Min(b1.min, b2.min), Max(b1.max, b2.max));
}

inline Bounds Union(const Bounds& b, const Point3f& p) {
	return Bounds(Min(b.min, p), Max(b.max, p));
}
