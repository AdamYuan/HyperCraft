#version 450

layout(set = 0, binding = 0) uniform sampler2D uPreviousLod;

layout(location = 0) out float oCurrentLod;

void main() {
	ivec2 current_pos = ivec2(gl_FragCoord.xy);
	ivec2 previous_size = textureSize(uPreviousLod, 0), previous_base_pos = current_pos << 1;

	vec4 d4 = vec4(texelFetch(uPreviousLod, previous_base_pos, 0).r,
	               texelFetch(uPreviousLod, previous_base_pos + ivec2(1, 0), 0).r,
	               texelFetch(uPreviousLod, previous_base_pos + ivec2(1, 1), 0).r,
	               texelFetch(uPreviousLod, previous_base_pos + ivec2(0, 1), 0).r);
	float depth = max(max(d4.x, d4.y), max(d4.z, d4.w));

	// deal with edge cases
	bool odd_width = (previous_size.x & 1) == 1, odd_height = (previous_size.y & 1) == 1;
	if (odd_width) {
		depth = max(depth, max(texelFetch(uPreviousLod, previous_base_pos + ivec2(2, 0), 0).r,
		                       texelFetch(uPreviousLod, previous_base_pos + ivec2(2, 1), 0).r));
		if (odd_height)
			depth = max(depth, texelFetch(uPreviousLod, previous_base_pos + ivec2(2, 2), 0).r);
	}
	if (odd_height) {
		depth = max(depth, max(texelFetch(uPreviousLod, previous_base_pos + ivec2(0, 2), 0).r,
		                       texelFetch(uPreviousLod, previous_base_pos + ivec2(1, 2), 0).r));
	}
	oCurrentLod = depth;
}
