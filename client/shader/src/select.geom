#version 450

layout(points) in;
layout(line_strip, max_vertices = 16) out;

layout(location = 0) in uint vExtent[];

layout(set = 0, binding = 0) uniform uuCamera {
	vec4 uViewPosition;
	mat4 uViewProjection;
};

void main() {
	vec3 aabb_min = gl_in[0].gl_Position.xyz - 0.004;
	vec3 extent = vec3(uvec3(vExtent[0].x & 0x1fu, (vExtent[0].x >> 5u) & 0x1fu, vExtent[0].x >> 10u)) * 0.0625 + 0.008;

	vec4 px = uViewProjection[0] * extent.x;
	vec4 py = uViewProjection[1] * extent.y;
	vec4 pz = uViewProjection[2] * extent.z;

	vec4 p0 = uViewProjection * vec4(aabb_min, 1);
	vec4 p1 = p0 + pz;
	vec4 p2 = p0 + py;
	vec4 p3 = p2 + pz;
	vec4 p4 = p0 + px;
	vec4 p5 = p4 + pz;
	vec4 p6 = p4 + py;
	vec4 p7 = p6 + pz;

	gl_Position = p0;
	EmitVertex();
	gl_Position = p1;
	EmitVertex();
	gl_Position = p3;
	EmitVertex();
	gl_Position = p2;
	EmitVertex();
	gl_Position = p0;
	EmitVertex();
	gl_Position = p4;
	EmitVertex();
	gl_Position = p5;
	EmitVertex();
	gl_Position = p7;
	EmitVertex();
	gl_Position = p6;
	EmitVertex();
	gl_Position = p4;
	EmitVertex();
	EndPrimitive();

	gl_Position = p1;
	EmitVertex();
	gl_Position = p5;
	EmitVertex();
	EndPrimitive();

	gl_Position = p2;
	EmitVertex();
	gl_Position = p6;
	EmitVertex();
	EndPrimitive();

	gl_Position = p3;
	EmitVertex();
	gl_Position = p7;
	EmitVertex();
	EndPrimitive();
}
