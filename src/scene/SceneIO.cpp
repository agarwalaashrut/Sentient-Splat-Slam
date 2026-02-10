#include "SceneIO.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

namespace sss {

using json = nlohmann::json;

Scene SceneIO::load_scene_json(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    json j;
    in >> j;

    Scene scene;
    std::vector<Gaussian3D> gaussians;

    if (!j.contains("gaussians") || !j["gaussians"].is_array()) {
        throw std::runtime_error("JSON missing 'gaussians' array: " + filepath);
    }

    for (const auto& item : j["gaussians"]) {
        if (!item.contains("color")) {
            throw std::runtime_error("Gaussian missing 'color' field");
        }

        Gaussian3D g;

        // Default values for all fields
        g.mean = glm::vec3(0.0f, 0.0f, 0.0f);
        g.scale = glm::vec3(1.0f, 1.0f, 1.0f);
        g.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        g.opacity = 1.0f;
        g.color = glm::vec3(item["color"][0], item["color"][1], item["color"][2]);

        // Parse optional fields if present
        if (item.contains("mean") && item["mean"].is_array() && item["mean"].size() >= 3) {
            g.mean = glm::vec3(item["mean"][0], item["mean"][1], item["mean"][2]);
        } else if (item.contains("position") && item["position"].is_array() && item["position"].size() >= 3) {
            g.mean = glm::vec3(item["position"][0], item["position"][1], item["position"][2]);
        }

        if (item.contains("scale") && item["scale"].is_array() && item["scale"].size() >= 3) {
            g.scale = glm::vec3(item["scale"][0], item["scale"][1], item["scale"][2]);
        }

        if (item.contains("rotation") && item["rotation"].is_array() && item["rotation"].size() >= 4) {
            g.rotation = glm::quat(item["rotation"][0], item["rotation"][1], item["rotation"][2], item["rotation"][3]);
        }

        if (item.contains("opacity") && item["opacity"].is_number()) {
            g.opacity = item["opacity"].get<float>();
        }

        gaussians.push_back(g);
    }

    scene.set_gaussians(gaussians);
    return scene;
}

void SceneIO::save_scene_json(const std::string& filepath, const Scene& scene) {
    json j;
    j["gaussians"] = json::array();

    for (const auto& g : scene.get_gaussians()) {
        json item;
        item["mean"] = {g.mean.x, g.mean.y, g.mean.z};
        item["scale"] = {g.scale.x, g.scale.y, g.scale.z};
        item["rotation"] = {g.rotation.x, g.rotation.y, g.rotation.z, g.rotation.w};
        item["opacity"] = g.opacity;
        item["color"] = {g.color.r, g.color.g, g.color.b};
        j["gaussians"].push_back(item);
    }

    std::ofstream out(filepath);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filepath);
    }

    out << j.dump(2);
}

}  // namespace sss
