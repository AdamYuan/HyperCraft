#version 450
layout(set = 0, binding = 0) uniform sampler2D uResult;
layout(set = 0, binding = 1) uniform sampler2D uDepth;

layout(location = 0) out vec4 oColor;
layout(location = 1) out float oDepth;

#define Z_NEAR 0.01
#define Z_FAR 640.0
float linearize_depth(in const float d) { return Z_NEAR * Z_FAR / (Z_FAR + d * (Z_NEAR - Z_FAR)); }
vec2 linearize_depth(in const vec2 d) { return Z_NEAR * Z_FAR / (Z_FAR + d * (Z_NEAR - Z_FAR)); }
float nonlinearize_depth(in const float ld) { return ((ld - Z_NEAR) * Z_FAR) / (ld * (Z_FAR - Z_NEAR)); }

void main() {
	ivec2 coord = ivec2(gl_FragCoord.xy);

	float depth = texelFetch(uDepth, coord, 0).r;

	float judger = linearize_depth(depth) - 10.0;
	vec2 neighbour_judger = linearize_depth(
	    vec2(texelFetch(uDepth, coord - ivec2(1, 0), 0).r, texelFetch(uDepth, coord + ivec2(1, 0), 0).r));
	if (neighbour_judger.x < judger && neighbour_judger.y < judger) {
		oColor = (texelFetch(uResult, coord - ivec2(1, 0), 0) + texelFetch(uResult, coord + ivec2(1, 0), 0)) * 0.5;
		oColor.xyz = pow(oColor.xyz, vec3(1.0 / 2.2));
		oDepth = nonlinearize_depth((neighbour_judger.x + neighbour_judger.y) * 0.5);
		return;
	}
	neighbour_judger = linearize_depth(
	    vec2(texelFetch(uDepth, coord - ivec2(0, 1), 0).r, texelFetch(uDepth, coord + ivec2(0, 1), 0).r));
	if (neighbour_judger.x < judger && neighbour_judger.y < judger) {
		oColor = (texelFetch(uResult, coord - ivec2(0, 1), 0) + texelFetch(uResult, coord + ivec2(0, 1), 0)) * 0.5;
		oColor.xyz = pow(oColor.xyz, vec3(1.0 / 2.2));
		oDepth = nonlinearize_depth((neighbour_judger.x + neighbour_judger.y) * 0.5);
		return;
	}

	oColor = texelFetch(uResult, coord, 0);
	oColor.xyz = pow(oColor.xyz, vec3(1.0 / 2.2));
	oDepth = depth;
}
