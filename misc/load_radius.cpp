#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <unordered_set>
#include <vector>

#include <cstdio>

#include "../client/include/client/Config.hpp"

std::vector<glm::ivec3> pos_vec;
std::unordered_set<glm::ivec3> pos_set;

std::vector<int> radius_ptr;

int main() {
	for (int r = 0; r <= kWorldMaxLoadRadius; ++r) {
		glm::ivec3 b, d, p;
		for (b.x = -r; b.x <= r; ++b.x)
			for (b.y = -r; b.y <= r; ++b.y)
				for (b.z = -r; b.z <= r; ++b.z) {
					if (b.x * b.x + b.y * b.y + b.z * b.z <= r * r) {
						for (d.x = 0; d.x <= 2; ++d.x)
							for (d.y = 0; d.y <= 2; ++d.y)
								for (d.z = 0; d.z <= 2; ++d.z) {
									p = b + glm::ivec3{d.x <= 1 ? d.x : -1, d.y <= 1 ? d.y : -1, d.z <= 1 ? d.z : -1};
									if (pos_set.find(p) == pos_set.end()) {
										pos_set.insert(p);
										pos_vec.push_back(p);
									}
								}
					}
				}
		radius_ptr.push_back(pos_vec.size());
	}

	printf("constexpr ChunkPos3 kWorldLoadingList[] = {\n\t");
	uint32_t line_cnt{};
	for (const auto &i : pos_vec) {
		printf("{%d, %d, %d}, ", i.x, i.y, i.z);
		if (++line_cnt == 5) {
			printf("\n\t");
			line_cnt = 0;
		}
	}
	printf("};\n");

	printf("constexpr const ChunkPos3 *kWorldLoadingRadiusEnd[kWorldMaxLoadRadius + 1] = {\n\t");
	for (uint32_t i : radius_ptr) {
		printf("kWorldLoadingList + %u, ", i);
	}
	printf("};\n");

	return 0;
}
