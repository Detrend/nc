// Project Nucledian Source File
#pragma once

#include <math/vector.h>
#include <math/matrix.h>

#include <glm/trigonometric.hpp>
#include <glm/exponential.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vector_relational.hpp>
//#include <glm/packing.hpp>
//#include <glm/matrix.hpp>
//#include <glm/integer.hpp>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_access.hpp>

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>

namespace nc
{

using namespace glm;

// This one is extra because it does not have different SSE/NON-SSE implementation.
// Checks if the vector has length of 1
template<typename T>
bool is_normal(const T& v, typename T::value_type threshold = static_cast<T::value_type>(0.01));

template<typename T>
bool is_zero(const T& v, typename T::value_type threshold = static_cast<T::value_type>(0.01));

// Returns a vector rotated 90 degrees to the left
// (flips the components and negates x of the new vector)
vec2 flipped(const vec2& v);

f32 cross(const vec2& a, const vec2& b);

// Normalize vector, if vector cant be normalized return vector with all zeros.
template<typename T>
T normalize_or_zero(const T& vec);

// Clamps the length of the vector. Returns the input vector clamped by the min
// and max length.
template<typename T>
T clamp_length(const T& vec, f32 min, f32 max);

// Normalizes the vector if non-zero and returns it. Otherwise returns the other
template<typename T>
T normalize_or(const T& vec, const T& other);

template<typename VT, typename FT>
VT with_x(const VT& v, const FT& x);

template<typename VT, typename FT>
VT with_y(const VT& v, const FT& x);

template<typename VT, typename FT>
VT with_z(const VT& v, const FT& x);

template<typename VT, typename FT>
VT with_w(const VT& v, const FT& x);

// Creates a translation matrix. Alternative for glm::translate which always
// does an unnecessary matrix multiplication.
mat4 translation(vec3 how_much);

// Creates a scaling matrix.
mat4 scaling(vec3 how_much);

}

#include <math/lingebra.inl>

