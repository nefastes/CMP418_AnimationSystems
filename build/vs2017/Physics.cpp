#include "Physics.h"
#include "maths/matrix44.h"
#include "maths/quaternion.h"

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

btDiscreteDynamicsWorld* Physics::GetWorld()
{
	return dynamics_world_;
}

const btRigidBody* Physics::CreateBoxBody(const btVector3& halfSize, btScalar mass)
{
	btCollisionShape* groundShape = new btBoxShape(halfSize);

	collision_shapes_.push_back(groundShape);

	btTransform groundTransform;
	groundTransform.setIdentity();
	groundTransform.setOrigin(btVector3(0, -halfSize.y(), 0));

	//rigidbody is dynamic if and only if mass is non zero, otherwise static
	bool isDynamic = (mass != 0.f);

	btVector3 localInertia(0, 0, 0);
	if (isDynamic)
		groundShape->calculateLocalInertia(mass, localInertia);

	//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
	btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);

	//add the body to the dynamics world
	dynamics_world_->addRigidBody(body);

	return body;
}

gef::Matrix44 Physics::btTransform2Matrix(const btTransform & transform, bool convertUnits)
{
	gef::Matrix44 result;

	btQuaternion rotation = transform.getRotation();
	btVector3 position = transform.getOrigin();

	result.Rotation(gef::Quaternion(rotation.x(), rotation.y(), rotation.z(), rotation.w()));
	const float m_to_cm = 100.f;	// GEF uses centimeters and bullet uses meters, so convert to cm using this multiplicator
	result.SetTranslation(gef::Vector4(position.x() * (convertUnits ? m_to_cm : 1.f), position.y() * (convertUnits ? m_to_cm : 1.f), position.z() * (convertUnits ? m_to_cm : 1.f)));

	return result;
}

btTransform Physics::Matrix2btTransform(const gef::Matrix44 & mtx, bool convertUnits)
{
	gef::Vector4 mtx_position = mtx.GetTranslation();

	gef::Quaternion mtx_rot;
	mtx_rot.SetFromMatrix(mtx);

	btTransform result;
	const float cm_to_m = .01f;	// GEF uses centimeters and bullet uses meters, so convert to cm using this multiplicator
	result.setOrigin(btVector3(mtx_position.x() * (convertUnits ? cm_to_m : 1.f), mtx_position.y() * (convertUnits ? cm_to_m : 1.f), mtx_position.z() * (convertUnits ? cm_to_m : 1.f)));
	result.setRotation(btQuaternion(mtx_rot.x, mtx_rot.y, mtx_rot.z, mtx_rot.w));

	return result;
}
