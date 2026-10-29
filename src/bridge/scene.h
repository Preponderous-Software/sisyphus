// Uploads the hill to Strata and keeps it in step with the world: the
// terrain, the town and the stones (once), the terrace markers (again when a
// release ships), the boulder (again when a module's colour changes, and
// moved, scaled and rolled every frame), and Sisyphus himself while the
// camera is outside him.
#pragma once

#include <cstddef>
#include <cstdint>

#include "bridge/geometry.h"
#include "sim/world.h"
#include "strata/strata.hpp"

namespace sisyphus::bridge {

class Scene {
public:
    explicit Scene(strata::Engine& engine);
    ~Scene();
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // Brings the meshes up to date. `avatar` shows Sisyphus where he stands.
    void update(const World& w, bool avatar);

    size_t meshCount() const { return meshes_; }

private:
    uint32_t upload(const MeshData& m);
    void remove(uint32_t& id);

    strata::Engine& engine_;
    uint32_t town_ = 0, markers_ = 0, cairns_ = 0, boulder_ = 0, avatar_ = 0, helper_ = 0;
    int markersBest_ = -1;
    uint64_t cairnsKey_ = 0;
    uint64_t boulderKey_ = 0;
    bool avatarPushing_ = false;
    size_t meshes_ = 0;
};

}  // namespace sisyphus::bridge
