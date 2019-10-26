#include "visual/camera.h"

#include "visual/renderer.h"
#include "physics/physics.h"
#include "entities/exec.h"
#include "core/core.h"
#include "game/npc.h"

#include <algorithm>
#include <memory>

CameraSystem::CameraSystem()
    : BaseSystem("Camera", Entity::getSignature< const PhysBody, const Seeker, Camera >()) {
}

CameraSystem::~CameraSystem() { }

void CameraSystem::init(Core &core) {
    core.tracker.addSource(std::make_unique< CameraData >());
}

void CameraSystem::execute(Core &core, double) {
    Entity::Exec< Entity::Packs< const PhysBody >, Entity::Packs< PhysBody, Camera > >::run(core.tracker,
    [&](const auto &bodyPack, auto &cameraPack) {
        const auto &bodies = bodyPack.first.template get< const PhysBody >();
        auto &cameraBods = cameraPack.first.template get< PhysBody >();
        auto &cameras = cameraPack.first.template get< Camera >();
        b2Vec2 botleft(infty< double >(), infty< double >());
        b2Vec2 toprite(-infty< double  >(), -infty< double >());
        for (auto &bod : bodies) {
            const auto pos = bod.body->GetPosition();
            botleft.x = std::min(botleft.x, pos.x);
            toprite.x = std::max(toprite.x, pos.x);
            botleft.y = std::min(botleft.y, pos.y);
            toprite.y = std::max(toprite.y, pos.y);
            if (pos.x > -0.1) {
            }
        }

        b2Vec2 central = botleft + toprite;
        central.x /= 2.0;
        central.y /= 2.0;
        double radius = std::max(toprite.x - botleft.x, toprite.y - botleft.y) / 2.0;
        radius = std::max(radius * 1.2, 10.0);

        const double lerper = 0.2;
        central = VPC< b2Vec2 >(lerp(VPC< Vec >(core.camera), VPC< Vec >(central), lerper));
        radius = lerp(core.radius, radius, lerper);

        for (size_t i = 0; i < cameras.size(); ++i) {
            if (!core.tracker.hasComponent< Seeker >(cameraPack.second[i])) {
                cameraBods[i].body->SetTransform(central, 0);
            }
            cameras[i].radius = radius * 1.01;
        }
    });
}
