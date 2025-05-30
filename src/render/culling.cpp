#include <glm/glm.hpp>
#include <vector>
#include <gsl/span>

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
    glm::vec3 corner(int i) const {
        return glm::vec3(
            (i & 1) ? max.x : min.x,
            (i & 2) ? max.y : min.y,
            (i & 4) ? max.z : min.z);
    }
};

struct DrawCmd {
    glm::mat4 modelMatrix;
    uint32_t meshRef;
    uint32_t materialRef;
    uint32_t materialUBOOffset;
};

extern std::vector<AABB> aabbs;

std::vector<DrawCmd> frustumCull(const glm::mat4& VP,
                                 gsl::span<const DrawCmd> in)
{
    glm::vec4 planes[6];
    const glm::mat4 m = VP;
    planes[0] = m[3] + m[0];
    planes[1] = m[3] - m[0];
    planes[2] = m[3] + m[1];
    planes[3] = m[3] - m[1];
    planes[4] = m[3] + m[2];
    planes[5] = m[3] - m[2];
    for(auto& p: planes) p /= glm::length(glm::vec3(p));

    std::vector<DrawCmd> out;
    out.reserve(in.size());

    for(const DrawCmd& dc : in)
    {
        const AABB& box = aabbs[dc.meshRef];
        bool inside = true;
        for(const glm::vec4& pl : planes)
        {
            int inCount = 0;
            for(int i=0;i<8 && inCount==0;++i){
                glm::vec4 pt = dc.modelMatrix * glm::vec4(box.corner(i),1);
                if(glm::dot(pl, pt) >= 0) inCount = 1;
            }
            if(!inCount){ inside=false; break; }
        }
        if(inside) out.push_back(dc);
    }
    return out;
}
