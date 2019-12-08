#include "physics/geometry.h"

#include "core/core.h"

namespace {

b2Body *makeBody(Core &core, Point centre, b2Shape *shape) {
    b2BodyDef definition;
    definition.type = b2_dynamicBody;
    definition.position.Set(centre.x(), centre.y());

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

b2Body *makeCircle(Core &core, Point centre, double radius) {
    b2CircleShape circle;
    circle.m_radius = radius;
    return makeBody(core, centre, &circle);
}

b2Body *makeRect(Core &core, Point centre, double width, double height) {
    b2PolygonShape box;
    box.SetAsBox(width / 2.0, height / 2.0);
    return makeBody(core, centre, &box);
}
