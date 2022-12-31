#pragma once
#include "btBulletDynamicsCommon.h"

#define PHYSICS_DEFAULT_TIMESTEP (1.f / 60.f)
#define PHYSICS_DEFAULT_MAXSUBSTEPS 1

class Physics
{
public:
	Physics(float updateTimeStep = PHYSICS_DEFAULT_TIMESTEP, int maxSubSteps = PHYSICS_DEFAULT_MAXSUBSTEPS);
	~Physics();

	void Init();
	void Update();

private:
	float update_time_step_;
	int max_sub_steps_;

	btDiscreteDynamicsWorld* dynamics_world_;
	btSequentialImpulseConstraintSolver* solver_;
	btBroadphaseInterface* overlapping_pair_cache_;
	btCollisionDispatcher* dispatcher_;
	btAlignedObjectArray<btCollisionShape*> collision_shapes_;
};
