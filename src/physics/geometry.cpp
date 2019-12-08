#include "physics/geometry.h"

#include "core/core.h"

namespace {

b2Body *makeBody(Core &core, Point centre, b2Shape *shape, bool dynamic, bool rotates) {
    b2BodyDef definition;
    definition.type = dynamic ? b2_dynamicBody : b2_staticBody;
    definition.position.Set(centre.x(), centre.y());
    if (!rotates) {
        definition.fixedRotation = true;
    }

    b2FixtureDef fixture;
    fixture.density = 15.0f;
    fixture.friction = 0.7f;
    fixture.shape = shape;

    b2Body *body;
    core.b2world.locked([&](){
        body = core.b2world.b2w->CreateBody(&definition);
        body->CreateFixture(&fixture);
    });

    return body;
}

}

b2Body *makeCircle(Core &core, Point centre, double radius, bool dynamic, bool rotates) {
    b2CircleShape circle;
    circle.m_radius = radius;
    return makeBody(core, centre, &circle, dynamic, rotates);
}

b2Body *makeRect(Core &core, Point centre, double width, double height, bool dynamic, bool rotates) {
    b2PolygonShape box;
    box.SetAsBox(width / 2.0, height / 2.0);
    return makeBody(core, centre, &box, dynamic, rotates);
}
