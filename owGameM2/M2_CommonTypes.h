#pragma once

#include __PACK_BEGIN

enum Interpolations : uint16_t
{
	INTERPOLATION_NONE = 0,
	INTERPOLATION_LINEAR,
	INTERPOLATION_HERMITE
};

template<typename T>
struct M2Array
{
	uint32 size;
	uint32 offset;
};

#if (VERSION == VERSION_Vanila)
struct M2TrackBase
{
	Interpolations						interpolation_type;
	int16_t								global_sequence;
	M2Array<pair<uint32_t, uint32_t>>	interpolation_ranges;
	M2Array<uint32_t>					timestamps;
};

template<typename T>
struct M2Track : public M2TrackBase
{
	M2Array<T>							values;
};

template<typename T>
struct M2TrackFake
{
	M2Array<pair<uint32_t, uint32_t>>	interpolation_ranges;
	M2Array<uint32_t>					timestamps;
	M2Array<T>							values;
};
#else
struct M2TrackBase
{
	Interpolations						interpolation_type;
	int16_t								global_sequence;
	M2Array<M2Array<uint32>>			timestamps;
};

template<typename T>
struct M2Track : public M2TrackBase
{
	M2Array<M2Array<T>>					values;
};

template<typename T>
struct M2TrackFake
{
	M2Array<M2Array<uint32>>			timestamps;
	M2Array<M2Array<T>>					values;
};
#endif

struct M2CompQuat
{
	int16 x, y, z, w;
};

struct M2Bounds
{
	CAaBox extent;
	float radius;
};

template<typename T>
struct M2SplineKey
{
	T value;
	T inTan;
	T outTan;
};

struct M2Box
{
	vec3 ModelRotationSpeedMin;
	vec3 ModelRotationSpeedMax;
};

struct M2Range
{
	uint32 minimum;
	uint32 maximum;
};

#include __PACK_END