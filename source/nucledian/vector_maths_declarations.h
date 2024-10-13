// DO NOT INCLUDE BY ITSELF!!!

// Compares two vector types and returns true if they are same
template<typename T, u64 SIZE>
bool eq(const vec<T, SIZE>& a, const vec<T, SIZE>& b);

// Returns a component-wise minimum of each of the two provided
// input vectors
template<typename T, u64 SIZE>
vec<T, SIZE> min(const vec<T, SIZE>& a, const vec<T, SIZE>& b);

// Returns a component-wise maximum of each of the two provided
// input vectors
template<typename T, u64 SIZE>
vec<T, SIZE> max(const vec<T, SIZE>& a, const vec<T, SIZE>& b);

// Returns component-wise sum of two vectors
template<typename T, u64 SIZE>
vec<T, SIZE> add(const vec<T, SIZE>& a, const vec<T, SIZE>& b);

// Returns component-wise subtraction of two vectors
template<typename T, u64 SIZE>
vec<T, SIZE> sub(const vec<T, SIZE>& a, const vec<T, SIZE>& b);

// Returns component-wise multiplication of two vectors
template<typename T, u64 SIZE>
vec<T, SIZE> mul(const vec<T, SIZE>& a, const vec<T, SIZE>& b);

// Returns component-wise division of two vectors
template<typename T, u64 SIZE>
vec<T, SIZE> div(const vec<T, SIZE>& a, const vec<T, SIZE>& b);

// Returns component-wise inversion of the vector elements.
// Each element of the result will have a value v = 1/e.
template<typename T, u64 SIZE>
vec<T, SIZE> inv(const vec<T, SIZE>& a);

// Returns a dot product of two vectors
template<typename T, u64 SIZE>
T dot(const vec<T, SIZE>& a, const vec<T, SIZE>& b);

// Computes a cross product of 3 dimensional vector
vec3 cross(const vec3& a, const vec3& b);

// Computes a cross product of 4 dimensional vector as if had
// only 3 dimensions
vec4 cross(const vec4& a, const vec4& b);

// Computes a "2D" cross product, returning scalar
f32  cross(const vec2& a, const vec2& b);


