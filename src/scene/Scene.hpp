#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace sss {

// Gaussian structure for temporary storage
struct Gaussian {
    glm::vec3 position;
    glm::vec3 color;
};

// Full 3D Gaussian representation
struct Gaussian3D {
    glm::vec3 mean;
    glm::vec3 scale;
    glm::quat rotation;
    float opacity;
    glm::vec3 color;
};

// GPU-packed Gaussian instance data (64 bytes)
struct GaussianInstanceGPU {
    glm::vec4 mean_opacity;      // xyz = position, w = opacity
    glm::vec4 quat;              // xyzw quaternion
    glm::vec4 scale_colorx;      // xyz = scale, w = color.x
    glm::vec4 coloryz_pad;       // xy = color.yz, zw = padding
};

static_assert(sizeof(GaussianInstanceGPU) == 64, "GaussianInstanceGPU must be 64 bytes");

// GPU point structure for simple rendering
struct GPUPoint {
    float x, y, z;
    float r, g, b;
};

// Scene container
class Scene {
public:
    Scene() = default;
    ~Scene() = default;

    const std::vector<Gaussian3D>& get_gaussians() const { return m_gaussians; }
    std::vector<Gaussian3D>& get_gaussians_mut() { return m_gaussians; }
    
    size_t gaussian_count() const { return m_gaussians.size(); }
    
    void set_gaussians(const std::vector<Gaussian3D>& gaussians) {
        m_gaussians = gaussians;
    }

    void add_gaussian(const Gaussian3D& gaussian) {
        m_gaussians.push_back(gaussian);
    }

    void clear() {
        m_gaussians.clear();
    }

private:
    std::vector<Gaussian3D> m_gaussians;
};

}  // namespace sss
