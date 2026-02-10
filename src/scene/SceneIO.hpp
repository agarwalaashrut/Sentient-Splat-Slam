#pragma once

#include "Scene.hpp"
#include <string>

namespace sss {

class SceneIO {
public:
    /**
     * Load a scene from a JSON file.
     * Expected format: { "gaussians": [ { "color": [r,g,b], "mean"?: [x,y,z], ... } ] }
     */
    static Scene load_scene_json(const std::string& filepath);

    /**
     * Save a scene to a JSON file.
     */
    static void save_scene_json(const std::string& filepath, const Scene& scene);
};

}  // namespace sss
