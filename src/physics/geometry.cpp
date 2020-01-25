#include "physics/geometry.h"

#include "core/core.h"

namespace {

b2Body *makeBody(Core &core, Point centre, b2Shape *shape, PhysProperties properties) {
    b2BodyDef definition;
    definition.type = properties.dynamic ? b2_dynamicBody : b2_staticBody;
    definition.position.Set(centre.x(), centre.y());
    definition.fixedRotation = !properties.rotates;

    b2FixtureDef fixture;
    fixture.density = 15.0f;
    fixture.friction = 0.7f;
    fixture.shape = shape;
    fixture.isSensor = properties.sensor;
    fixture.filter.categoryBits = properties.category;
    fixture.filter.maskBits = properties.mask;

    b2Body *body;
    core.b2world.locked([&](){
        body = core.b2world.b2w->CreateBody(&definition);
        body->CreateFixture(&fixture);
    });

    return body;
}

}

b2Body *makeCircle(Core &core, Point centre, double radius, PhysProperties properties) {
    b2CircleShape circle;
    circle.m_radius = radius;
    return makeBody(core, centre, &circle, properties);
}

b2Body *makeRect(Core &core, Point centre, double width, double height, PhysProperties properties) {
    b2PolygonShape box;
    box.SetAsBox(width / 2.0, height / 2.0);
    return makeBody(core, centre, &box, properties);
}
