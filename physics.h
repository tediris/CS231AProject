#ifndef _PHYSICS_H
#define _PHYSICS_H

#include "reactphysics3d.h"

class BoxCollider {
public:
	BoxCollider(rp3d::DynamicsWorld *world, float length, float width, float height, float x, float y, float z);
private:
	rp3d::RigidBody* body;
	MeshRenderer renderer;
};

BoxCollider::BoxCollider(rp3d::DynamicsWorld *world, float length, float width, float height, float x, float y, float z) {
	rp3d::Vector3 initPosition(x, y, z);
	rp3d::Quaternion initOrientation = rp3d::Quaternion::identity();
	rp3d::Transform transform(initPosition, initOrientation);
	body = world->createRigidBody(transform);
	body->setType(rp3d::KINEMATIC);

}


#endif
