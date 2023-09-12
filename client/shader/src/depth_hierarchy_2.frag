#version 450
#extension GL_KHR_shader_subgroup_quad : enable

layout(set = 0, binding = 0) uniform sampler2D uPreviousLod;
layout(r32f, set = 0, binding = 1) uniform writeonly image2D uNextLod;

layout(location = 0) out float oCurrentLod;

void main() {
    ivec2 current_pos = ivec2(gl_FragCoord.xy);
    ivec2 previous_size = textureSize(uPreviousLod, 0), previous_base_pos = current_pos << 1;

    vec4 d4 = vec4(texelFetch(uPreviousLod, previous_base_pos, 0).r,
    texelFetch(uPreviousLod, previous_base_pos + ivec2(1, 0), 0).r,
    texelFetch(uPreviousLod, previous_base_pos + ivec2(1, 1), 0).r,
    texelFetch(uPreviousLod, previous_base_pos + ivec2(0, 1), 0).r);
    float cur_depth = max(max(d4.x, d4.y), max(d4.z, d4.w));

    // deal with edge cases
    bool prev_edge_width = previous_base_pos.x == previous_size.x - 3,
    prev_edge_height = previous_base_pos.y == previous_size.y - 3;
    if (prev_edge_width) {
        cur_depth = max(cur_depth, max(texelFetch(uPreviousLod, previous_base_pos + ivec2(2, 0), 0).r,
        texelFetch(uPreviousLod, previous_base_pos + ivec2(2, 1), 0).r));
        if (prev_edge_height)
        cur_depth = max(cur_depth, texelFetch(uPreviousLod, previous_base_pos + ivec2(2, 2), 0).r);
    }
    if (prev_edge_height) {
        cur_depth = max(cur_depth, max(texelFetch(uPreviousLod, previous_base_pos + ivec2(0, 2), 0).r,
        texelFetch(uPreviousLod, previous_base_pos + ivec2(1, 2), 0).r));
    }
    oCurrentLod = cur_depth;

    float next_depth = max(cur_depth, subgroupQuadSwapHorizontal(cur_depth));
    next_depth = max(next_depth, subgroupQuadSwapVertical(next_depth));
    if (gl_SubgroupInvocationID == subgroupQuadBroadcast(gl_SubgroupInvocationID, 0))
    imageStore(uNextLod, current_pos >> 1, vec4(next_depth));
}
