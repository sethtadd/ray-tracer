#version 430 core

#define MAX_SCENE_BOUNDS 100.0
#define NUM_BOXES 2

layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) uniform image2D framebuffer;

uniform vec3 bgColor;

uniform vec3 camPos;
uniform vec3 camFront;
uniform vec3 camRight;
uniform vec3 camUp;
uniform vec3 camLight;

vec3 lightSource1 = vec3(1,1,1);

struct box {
  vec3 min;
  vec3 max;
};

const box boxes[] = {
  /* The ground */
  {vec3(-5.0, -0.1, -5.0), vec3(5.0, 0.0, 5.0)},
  /* Box in the middle */
  {vec3(-0.5, 0.0, -0.5), vec3(0.5, 1.0, 0.5)}
};

struct HitInfo {
  vec2 lambda;
  int index;
};

vec2 intersectBox(vec3 origin, vec3 dir, const box b) {
  vec3 tMin = (b.min - origin) / dir;
  vec3 tMax = (b.max - origin) / dir;
  vec3 t1 = min(tMin, tMax);
  vec3 t2 = max(tMin, tMax);
  float tNear = max(max(t1.x, t1.y), t1.z);
  float tFar = min(min(t2.x, t2.y), t2.z);
  return vec2(tNear, tFar);
}

bool intersectBoxes(vec3 origin, vec3 dir, out HitInfo info) {
  float smallest = MAX_SCENE_BOUNDS;
  bool found = false;
  for (int i = 0; i < NUM_BOXES; i++) {
    vec2 lambda = intersectBox(origin, dir, boxes[i]);
    if (lambda.x > 0.0 && lambda.x < lambda.y && lambda.x < smallest) {
      info.lambda = lambda;
      info.index = i;
      smallest = lambda.x;
      found = true;
    }
  }
  return found;
}

vec4 trace(vec3 origin, vec3 dir) {
  HitInfo info;
  if (intersectBoxes(origin, dir, info)) {
    vec3 gray = vec3(info.index / 10.0 + 0.8);
    return vec4(gray, 1);
  }
  return vec4(bgColor, 1);
}

void main() {
  ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
  ivec2 size = imageSize(framebuffer);
  if (pixelCoord.x >= size.x || pixelCoord.y >= size.y) return;
  vec2 posOnLens = pixelCoord / vec2(size.x - 1, size.y - 1) - vec2(0.5); // scale to between -1 and +1
  vec3 pos = camPos + posOnLens.x * camRight + posOnLens.y * camUp; // position of pixel in 3D space
  vec3 dir = normalize(pos - camLight); // light ray direction
  vec4 color = trace(pos, dir);
  imageStore(framebuffer, pixelCoord, color);
}
