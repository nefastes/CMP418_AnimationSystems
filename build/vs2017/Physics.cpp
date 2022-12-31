#include "Physics.h"

Physics::Physics(float updateTimeStep, int maxSubSteps) :
	update_time_step_(updateTimeStep),
	max_sub_steps_(maxSubSteps),
	dynamics_world_(nullptr),
	solver_(nullptr),
	overlapping_pair_cache_(nullptr),
	dispatcher_(nullptr)
{
}

Physics::~Physics()
{
	for (int i = dynamics_world_->getNumConstraints() - 1; i >= 0; --i)
	{
		btTypedConstraint* constraint = dynamics_world_->getConstraint(i);
		dynamics_world_->removeConstraint(constraint);
		delete constraint;
	}

	btCollisionObjectArray& objectArray = dynamics_world_->getCollisionObjectArray();
	for (int i = dynamics_world_->getNumCollisionObjects() - 1; i >= 0; --i)
	{
		btCollisionObject* obj = objectArray[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState()) delete body->getMotionState();
		dynamics_world_->removeCollisionObject(obj);
		delete obj;
	}

	for (uint32_t i = 0u; i < collision_shapes_.size(); ++i)
		if (collision_shapes_[i]) delete collision_shapes_[i], collision_shapes_[i] = nullptr;
	collision_shapes_.clear();

	if (dynamics_world_)			delete dynamics_world_, dynamics_world_ = nullptr;
	if (solver_)					delete solver_, solver_ = nullptr;
	if (overlapping_pair_cache_)	delete overlapping_pair_cache_, overlapping_pair_cache_ = nullptr;
	if (dispatcher_)				delete dispatcher_, dispatcher_ = nullptr;
}

void Physics::Init()
{
	/// collision configuration contains default setup for memory , collision setup . Advanced users can create their own configuration .
	btDefaultCollisionConfiguration * collision_configuration = new btDefaultCollisionConfiguration();

	/// use the default collision dispatcher . For parallel processing you can use a diffent dispatcher(see Extras / BulletMultiThreaded)
	dispatcher_ = new btCollisionDispatcher(collision_configuration);

	/// btDbvtBroadphase is a good general purpose broadphase . You can also try out btAxis3Sweep .
	overlapping_pair_cache_ = new btDbvtBroadphase();

	/// the default constraint solver . For parallel processing you can use a different solver (see Extras / BulletMultiThreaded)
	solver_ = new btSequentialImpulseConstraintSolver;

	dynamics_world_ = new btDiscreteDynamicsWorld(dispatcher_, overlapping_pair_cache_, solver_, collision_configuration);
	dynamics_world_->setGravity(btVector3(0, -9.8f, 0));
}

void Physics::Update()
{
	dynamics_world_->stepSimulation(update_time_step_, max_sub_steps_);
}
