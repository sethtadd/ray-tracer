#version 430 core

layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) uniform image2D framebuffer;

layout (std430, binding = 0) buffer shader_data {
	vec4 shape[];
} data;

// for normals // broken!!!
layout (std430, binding = 1) buffer shader_data2 {
	vec4 normals[];
} data2;

const float maxDist = 1000000;
const float epsilon = 0.00001;

uniform vec3 camPos;
uniform vec3 camFront;
uniform vec3 camRight;
uniform vec3 camUp;
uniform vec3 camLight;

uniform int rayBounces;
uniform vec3 bgColor;

uniform int numShapeVerts;
uniform int numNormals;

vec3 lightSource1 = vec3(8,6,0);

// Light Properties
///////////////////////////
float diffuse = 0.22;
float ambient = 0.1;
float shininess = 0.15;
float reflectiveness = 0.3;
///////////////////////////

vec3 traceRay(vec3 orig, vec3 dir);
vec3 castRay(vec3 orig, vec3 dir, out vec3 inter, out vec3 normal);
bool rayTriDist(vec3 orig, vec3 dir, vec3 v0, vec3 v0, vec3 v2, out float t);
bool rayTriInter(vec3 orig, vec3 dir, vec3 v0, vec3 v0, vec3 v2, out float t, out float u, out float v, out bool backFacing);
float distToInter(vec3 orig, vec3 dir);

vec3 invSqrLight(vec3 interVec, vec3 dir, vec3 normal);

void main() {
	ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(framebuffer);
	if (pixelCoord.x >= size.x || pixelCoord.y >= size.y) return;
	vec2 posOnLens = pixelCoord / vec2(size.x - 1, size.y - 1) - vec2(0.5); // scale to between -1 and +1
	vec3 orig = camPos + posOnLens.x * camRight + posOnLens.y * camUp; // position of pixel in 3D space
	vec3 dir = normalize(orig - camLight); // light ray direction

	// determing new color
	vec4 color = vec4(traceRay(orig, dir), 1.0f);
	// draw new color to pixel
	imageStore(framebuffer, pixelCoord, color);
}

vec3 traceRay(vec3 orig, vec3 dir)
{
	vec3 mult1 = vec3(0), mult2 = vec3(0), mult3 = vec3(0);

	// cast first ray
	vec3 interVec1;
	vec3 normal1;
	mult1 = castRay(orig, dir, interVec1, normal1);
	if ( length(interVec1) >= (maxDist-100) ) return bgColor; // no intersection
	else if (rayBounces >= 1)
	{
		// cast second ray
		vec3 orig2 = interVec1;
		vec3 dir2 = normalize(reflect(dir, normal1));
		vec3 interVec2;
		vec3 normal2;
		mult2 = castRay(orig2 + 0.001*dir2, dir2, interVec2, normal2);
		if ( length(interVec2) >= (maxDist-100) ) mult2 = bgColor; // no intersection
		else if (rayBounces == 2)
		{
			// cast third ray
			vec3 orig3 = interVec2;
			vec3 dir3 = normalize(reflect(dir2, normal2));
			vec3 interVec3;
			vec3 normal3;
			mult3 = castRay(orig3 + 0.001*dir3, dir3, interVec3, normal3);
			if ( length(interVec3) >= (maxDist-100) ) mult3 = bgColor; // no intersection
		}
	}

	return ambient + (1-shininess)*mult1 + shininess*reflectiveness*mult2 + shininess*reflectiveness*mult3;
}

// cast ray (orig, dir) and return the coordinates of the next intersection and it's normal and vec3 of rgb invSqr multiplier (<1)
vec3 castRay(vec3 orig, vec3 dir, out vec3 interVec, out vec3 normal)
{
	bool interBool;
	int i, i_closest;
	bool backFacing; // assuming right-hand triangle-winding
	float t = maxDist, t_temp;
	float u;
	float v;
	// ASSUME TRIANGLES ARE GROUPS OF 3 VERTICES
	for (i = 0; i < numShapeVerts - 2; i += 3)
	{
		interBool = rayTriDist(orig, dir, data.shape[i].xyz, data.shape[i+1].xyz, data.shape[i+2].xyz, t_temp);
		if (interBool && t_temp < t)
		{
			t = t_temp;
			i_closest = i;
		}
	}

	
	// againt at index of closest intersection to orig
	vec3 v0 = data.shape[i_closest].xyz;
	vec3 v1 = data.shape[i_closest+1].xyz;
	vec3 v2 = data.shape[i_closest+2].xyz;
	interBool = rayTriInter(orig, dir, v0, v1, v2, t, u, v, backFacing);
	interVec = orig + t*dir;
	// inter stores if intersection occured
	// intersection is stored at t,u,v
	// index of first of 3 vertices of intersection with shape is i
	if (interBool)
	{
		if (!backFacing) 
		{
			////////////////////////////////////////////////////////////////////////////////////////
			// HERE we have intersection (t,u,v) and front facing triangle, DO LIGHTING!!!!!!!!!!!!!
			////////////////////////////////////////////////////////////////////////////////////////
			
			vec3 n1 = u*data2.normals[i_closest + 1].xyz;
			vec3 n2 = v*data2.normals[i_closest + 2].xyz;
			vec3 n3 = (1-u-v)*data2.normals[i_closest].xyz;
			normal = normalize(n1 + n2 + n3 /* + cross( (v1-v0),(v2-v1) ) */);
			
			//normal = normalize(cross( (v1-v0),(v2-v1) ));
			return invSqrLight(interVec, dir, normal);
		}
		else return vec3(0,0,0);
	}
	else return bgColor;

	//return vec3(data.shape[i_closest].x, 1.0f, data.shape[i_closest].z);
}

float distToInter(vec3 orig, vec3 dir)
{
	bool inter;
	int i, i_closest;
	bool backFacing; // assuming right-hand triangle-winding
	float t = maxDist, t_temp;
	float u;
	float v;
	// ASSUME TRIANGLES ARE GROUPS OF 3 VERTICES
	for (i = 0; i < numShapeVerts - 2; i += 3)
	{
		inter = rayTriDist(orig, dir, data.shape[i].xyz, data.shape[i+1].xyz, data.shape[i+2].xyz, t_temp);
		if (inter && t_temp < t) t = t_temp;
	}

	return t;
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
	if (t < 0 || t >= maxDist-100) return false;

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
	if (t < 0 || t >= maxDist-100) return false;

	return true;
}

vec3 invSqrLight(vec3 interVec, vec3 dir, vec3 normal)
{
	vec3 rayToLight1 = normalize(lightSource1-interVec);
	float rayIntDistToLight1 = length(rayToLight1);

	// the 'interVec + 0.001*rayToLight1' is to prevent artifacts where reflected ray intersects same surface
	float bounceDist = distToInter(interVec + 0.001*rayToLight1, rayToLight1);
	// ------ check if path from light to surface point is blocked ------
	if (bounceDist < rayIntDistToLight1) return vec3(0);

	vec3 reflectedRay = reflect(dir, normal);
	float light1 = max(0, dot(normalize(lightSource1), reflectedRay));
	float lightTravelDist = length(lightSource1 - interVec);
	light1 = (light1 + diffuse) / ( 0.01*lightTravelDist*lightTravelDist );
	light1 = min(light1, 1);

	return vec3(light1);
}