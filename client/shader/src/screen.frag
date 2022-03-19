#version 450
layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput uOpaque;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput uAccum;
layout(input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput uReveal;

layout(location = 0) out vec4 oColor;

void main() {
	vec4 opaque = subpassLoad(uOpaque);
	vec4 accum = subpassLoad(uAccum);
	if (accum.a == 0.0) {
		oColor = vec4(opaque.xyz, 1);
	} else {
		float reveal = subpassLoad(uReveal).r;
		oColor = vec4(accum.rgb * (1.0 - reveal) / accum.a + opaque.xyz * reveal, 1);
	}
	oColor = vec4(pow(oColor.xyz, vec3(1.0 / 2.2)), 1);
}
