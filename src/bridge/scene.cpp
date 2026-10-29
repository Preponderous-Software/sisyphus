#include "bridge/scene.h"

#include "sim/hill.h"

namespace sisyphus::bridge {

Scene::Scene(strata::Engine& engine) : engine_(engine) {
    const TerrainData t = buildTerrain();
    StrataTerrainTile tile{};
    for (int i = 0; i < 3; ++i) tile.origin[i] = t.origin[i];
    tile.cell_size = t.cell;
    tile.samples_x = t.samplesX;
    tile.samples_z = t.samplesZ;
    tile.heights = t.heights.data();
    tile.colors = t.colors.data();
    tile.color[3] = 1;
    engine_.setTerrain(&tile);
    town_ = upload(buildTown());
}

void Scene::update(const World& w, bool avatar) {
    // The markers count the releases of this major version (all of them
    // while it rolls back down from the summit).
    const int best = w.falling ? hill::kSummit : w.best;
    if (best != markersBest_) {
        remove(markers_);
        markers_ = upload(buildMarkers(best));
        markersBest_ = best;
    }

    const uint64_t ck = cairnsKey(w);
    if (ck != cairnsKey_) {
        remove(cairns_);
        cairns_ = upload(buildCairns(w));
        cairnsKey_ = ck;
    }

    const uint64_t key = boulderKey(w);
    if (!boulder_ || key != boulderKey_) {
        remove(boulder_);
        boulder_ = upload(buildBoulder(w));
        boulderKey_ = key;
    }
    // Scaled to its radius, rolled about x, and set on the path.
    const float R = boulderRadius(w);
    const Vec3 c = boulderCentre(w);
    const float co = detCos(w.boulder.roll), si = detSin(w.boulder.roll);
    const float model[16] = {R, 0, 0, 0, 0, R * co, -R * si, 0, 0, R * si, R * co, 0, c.x, c.y, c.z, 1};
    engine_.setMeshTransform(boulder_, model);

    // Whoever is helping: the same figure in a blue tunic, facing the boulder.
    if (w.helper.name < 0) {
        remove(helper_);
    } else {
        if (!helper_) helper_ = upload(buildAvatar(false, {0.25f, 0.38f, 0.58f}));
        const Helper& h = w.helper;
        float fx = c.x - h.x, fz = c.z - h.z;
        const float d = std::sqrt(fx * fx + fz * fz);
        const float yaw = d > 1e-3f ? detAtan2(fx, -fz) : 0.0f;
        const float hc = detCos(yaw), hs = detSin(yaw);
        const float hm[16] = {hc, 0, hs, 0, 0, 1, 0, 0, -hs, 0, hc, 0, h.x, hill::groundHeight(h.x, h.z), h.z, 1};
        engine_.setMeshTransform(helper_, hm);
    }

    if (!avatar) {
        remove(avatar_);
        return;
    }
    if (avatar_ && avatarPushing_ != w.pushing) remove(avatar_);
    if (!avatar_) {
        avatar_ = upload(buildAvatar(w.pushing));
        avatarPushing_ = w.pushing;
    }
    // Turned to face the way he faces (yaw 0 is -z, positive turns right);
    // pushing back down a terrace he faces downhill.
    const Player& p = w.player;
    const float cy = detCos(p.yaw), sy = detSin(p.yaw);
    const float y = hill::groundHeight(p.x, p.z) - (p.sitting ? 0.6f : 0.0f);
    const float at[16] = {cy, 0, sy, 0, 0, 1, 0, 0, -sy, 0, cy, 0, p.x, y, p.z, 1};
    engine_.setMeshTransform(avatar_, at);
}

void Scene::remove(uint32_t& id) {
    if (!id) return;
    engine_.removeMesh(id);
    id = 0;
    --meshes_;
}

Scene::~Scene() {
    try {
        remove(town_);
        remove(markers_);
        remove(cairns_);
        remove(boulder_);
        remove(avatar_);
        remove(helper_);
        engine_.setTerrain(nullptr);
    } catch (const strata::Error&) {
        // The engine is going away with us; nothing to tidy.
    }
}

uint32_t Scene::upload(const MeshData& m) {
    if (m.empty()) return 0;
    StrataMesh mesh{};
    mesh.positions = m.positions.data();
    mesh.normals = m.normals.data();
    mesh.colors = m.colors.data();
    mesh.indices = m.indices.data();
    mesh.vertex_count = static_cast<int32_t>(m.vertexCount());
    mesh.index_count = static_cast<int32_t>(m.indices.size());
    mesh.color[3] = 1;
    ++meshes_;
    return engine_.addMesh(mesh);
}

}  // namespace sisyphus::bridge
