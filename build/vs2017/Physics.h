#pragma once
#include "btBulletDynamicsCommon.h"

#define PHYSICS_DEFAULT_TIMESTEP 0.0166666675f //(1.f / 60.f)
#define PHYSICS_DEFAULT_MAXSUBSTEPS 1

namespace gef
{
	class Matrix44;
	class Mesh;
	class MeshInstance;
}

struct PhysicsMesh
{
	gef::Mesh* Gef_Mesh;
	gef::MeshInstance* Gef_MeshInstance;
	const btRigidBody* Physics_Body;
};

class Physics
{
public:
	Physics(float updateTimeStep = PHYSICS_DEFAULT_TIMESTEP, int maxSubSteps = PHYSICS_DEFAULT_MAXSUBSTEPS);
	~Physics();

	void Init();
	void Update();

	btDiscreteDynamicsWorld* GetWorld();
	const btRigidBody* CreateBoxBody(const btVector3& halfSize, btScalar mass = 0.f);

	static gef::Matrix44 btTransform2Matrix(const btTransform& transform, bool convertUnits = false);
	static btTransform Matrix2btTransform(const gef::Matrix44& mtx, bool convertUnits = false);
private:
	float update_time_step_;
	int max_sub_steps_;

	btDiscreteDynamicsWorld* dynamics_world_;
	btSequentialImpulseConstraintSolver* solver_;
	btBroadphaseInterface* overlapping_pair_cache_;
	btCollisionDispatcher* dispatcher_;
	btAlignedObjectArray<btCollisionShape*> collision_shapes_;
};