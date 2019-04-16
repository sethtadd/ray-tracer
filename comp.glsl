#version 430 core

layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) uniform image2D framebuffer;

const float epsilon = 0.000001;

uniform vec3 bgColor;

uniform vec3 camPos;
uniform vec3 camFront;
uniform vec3 camRight;
uniform vec3 camUp;
uniform vec3 camLight;

vec3 lightSource1 = vec3(1,1,1);

// Shape
const int numShapeVerts = 12; // vertex count
const vec3 shape[numShapeVerts] = {
	// 0xz
	vec3(0,0,0),
	vec3(1,0,0),
	vec3(0,0,1),
	// 0zy
	vec3(0,0,0),
	vec3(0,0,1),
	vec3(0,1,0),
	// 0yx
	vec3(0,0,0),
	vec3(0,1,0),
	vec3(1,0,0),
	// xyz
	vec3(1,0,0),
	vec3(0,1,0),
	vec3(0,0,1),
};

vec3 castRay(vec3 orig, vec3 dir);
bool rayTriDist(vec3 orig, vec3 dir, vec3 v0, vec3 v0, vec3 v2, out float t);
bool rayTriInter(vec3 orig, vec3 dir, vec3 v0, vec3 v0, vec3 v2, out float t, out float u, out float v, out bool backFacing);

void main() {
	ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(framebuffer);
	if (pixelCoord.x >= size.x || pixelCoord.y >= size.y) return;
	vec2 posOnLens = pixelCoord / vec2(size.x - 1, size.y - 1) - vec2(0.5); // scale to between -1 and +1
	vec3 orig = camPos + posOnLens.x * camRight + posOnLens.y * camUp; // position of pixel in 3D space
	vec3 dir = normalize(orig - camLight); // light ray direction

	// determing new color
	vec4 color = vec4(castRay(orig, dir), 1.0f);
	// draw new color to pixel
	imageStore(framebuffer, pixelCoord, color);
}

vec3 castRay(vec3 orig, vec3 dir)
{
	bool inter;
	int i, i_closest;
	bool backFacing;
	float t = 1000000, t_temp;
	float u;
	float v;
	// ASSUME TRIANGLES ARE GROUPS OF 3 VERTICES
	for (i = 0; i < numShapeVerts - 2; i += 3)
	{
		inter = rayTriDist(orig, dir, shape[i], shape[i+1], shape[i+2], t_temp);
		if (inter && t_temp < t)
		{
			t = t_temp;
			i_closest = i;
		}
	}
	// againt at index of closest intersection to orig
	inter = rayTriInter(orig, dir, shape[i_closest], shape[i_closest+1], shape[i_closest+2], t, u, v, backFacing);
	// inter stores if intersection occured
	// intersection is stored at t,u,v
	// index of first of 3 vertices of intersection with shape is i
	if (inter)
	{
		if (!backFacing) return vec3(1,1,1)/max((t*t),1);
		else return vec3(0,0,0);
	}
	else return bgColor;
}

bool rayTriDist(vec3 orig, vec3 dir, vec3 v0, vec3 v1, vec3 v2, out float t)
{
	vec3 v0v1 = v1 - v0;
	vec3 v0v2 = v2 - v0;
	vec3 pvec = cross(dir,v0v2);
	float det = dot(v0v1,pvec);

	// assume ray and triangle parallel if triple-scalar-product (det) small enough FRONT CULLING
	if (abs(det) < epsilon) return false;

	vec3 tvec = orig - v0;
	float u = dot(tvec,pvec) / det;
	if (u < 0 || u > 1) return false;

	vec3 qvec = cross(tvec,v0v1);
	float v = dot(dir,qvec) / det;
	if (v < 0 || u + v > 1) return false;

	t = dot(v0v2,qvec) / det;
	if (t < 0) return false;

	return true;
}

bool rayTriInter(vec3 orig, vec3 dir, vec3 v0, vec3 v1, vec3 v2, out float t, out float u, out float v, out bool backFacing)
{
	vec3 v0v1 = v1 - v0;
	vec3 v0v2 = v2 - v0;
	vec3 pvec = cross(dir,v0v2);
	float det = dot(v0v1,pvec);

	// assume ray and triangle parallel if triple-scalar-product (det) small enough FRONT CULLING
	if (abs(det) < epsilon) return false;

	// determine if triangle is back-facing
	if (det < epsilon) backFacing = true;
	else backFacing = false;

	vec3 tvec = orig - v0;
	u = dot(tvec,pvec) / det;
	if (u < 0 || u > 1) return false;

	vec3 qvec = cross(tvec,v0v1);
	v = dot(dir,qvec) / det;
	if (v < 0 || u + v > 1) return false;

	t = dot(v0v2,qvec) / det;
	if (t < 0) return false;

	return true;
}