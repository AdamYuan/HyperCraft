#version 450
layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput uAccum;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput uReveal;

layout(location = 0) out vec4 oColor;

void main() {
	vec4 accum = subpassLoad(uAccum);
	oColor = vec4(accum.rgb / accum.a, subpassLoad(uReveal).r);
}
