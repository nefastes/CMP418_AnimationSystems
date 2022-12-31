#include "ragdoll.h"
#include "..\Extras\Serialize\BulletWorldImporter\btBulletWorldImporter.h"
#include "system/debug_log.h"
#include "Physics.h"

// This file is the result of a lab worksheet from CMP418 on ragdolls
// Some bits of this code were already setup for us to use, the actual ragdoll logic was added on top of it
// Minor tweaks have also been made to the original code

Ragdoll::Ragdoll()
{
}

Ragdoll::~Ragdoll()
{
}

void Ragdoll::Init(const gef::SkeletonPose& bind_pose, btDiscreteDynamicsWorld* dynamics_world, const char* physics_filename)
{
	bind_pose_ = bind_pose;
	pose_ = bind_pose;

	gef::Matrix44 identity;
	identity.SetIdentity();

	bone_rb_offset_matrices_.resize(bind_pose_.skeleton()->joint_count(), identity);
	bone_rbs_.resize(bind_pose_.skeleton()->joint_count(), NULL);
	bone_world_matrices_.resize(bind_pose_.skeleton()->joint_count());
	bone_world_matrices_test_.resize(bind_pose_.skeleton()->joint_count(), identity);

	btBulletWorldImporter* fileLoader = new btBulletWorldImporter(dynamics_world);
	fileLoader->loadFile(physics_filename);

	int numRigidBodies = fileLoader->getNumRigidBodies();
	for (int i = 0; i < fileLoader->getNumRigidBodies(); i++)
	{
		btCollisionObject* obj = fileLoader->getRigidBodyByIndex(i);
		btRigidBody* body = btRigidBody::upcast(obj);

		// properties

		// The Blender object name
		std::string rb_name(fileLoader->getNameForPointer(body));

		// trim the blender rigid body name to match bone names
		rb_name = std::string(&rb_name.c_str()[sizeof("OBArmature_") - 1]);
		char* new_name = (char*)rb_name.c_str();
		new_name[rb_name.length() - sizeof("_hitbox") + 1] = 0;
		rb_name = std::string(new_name);

		gef::DebugOut("  object name = %s\n", rb_name.c_str());	
		gef::DebugOut("  get position = (%4.3f,%4.3f,%4.3f)\n",
			body->getCenterOfMassPosition().getX(),
			body->getCenterOfMassPosition().getY(),
			body->getCenterOfMassPosition().getZ());			// Blender CoM
		if (body->getInvMass() == 0)
			gef::DebugOut("  static object\n");
		else
		{
			gef::DebugOut("  mass = %4.3f\n", 1 / body->getInvMass());		// Blender Mass
		}
		gef::DebugOut("\n");

		if (bind_pose_.skeleton())
		{
			gef::StringId joint_name_id = gef::GetStringId(rb_name);
			if (joint_name_id != 0)
			{
				// find bone in skeleton that matches the name of the rigid body
				int joint_num = bind_pose_.skeleton()->FindJointIndex(joint_name_id);
				if (joint_num != -1)
				{
					bone_rbs_[joint_num] = body;

					// CALCULATE THE BONE TO RIGID BODY OFFSET MATRIX HERE
					// offset = rb world transform * inverse( anim bone world transform )
					const gef::Matrix44 rigidbody_worldtransform = Physics::btTransform2Matrix(body->getCenterOfMassTransform(), true);

					gef::Matrix44 animbone_worldtransform_inverse, animbone_worldtransform = bind_pose_.global_pose()[joint_num];
					animbone_worldtransform_inverse.AffineInverse(animbone_worldtransform);

					const gef::Matrix44 rigidBodyOffset = rigidbody_worldtransform * animbone_worldtransform_inverse;
					bone_rb_offset_matrices_[joint_num] = rigidBodyOffset;
				}
			}
		}
	}

	delete fileLoader;
	fileLoader = NULL;
}

void Ragdoll::UpdatePoseFromRagdoll()
{
	for (int bone_num = 0; bone_num < bind_pose_.skeleton()->joint_count(); ++bone_num)
	{
		const gef::Joint& joint = bind_pose_.skeleton()->joint(bone_num);
		gef::Matrix44 anim_bone_local_transform;


		btRigidBody* bone_rb = bone_rbs_[bone_num];
		if (bone_rb)
		{
			// REPLACE THIS LINE BELOW TO CALCULATE THE BONE LOCAL TRANSFORM
			// BASED ON THE RIGID BODY WORLD TRANSFORM
			// Get the rigid body transform
			const gef::Matrix44 rigidbody_worldtransform = Physics::btTransform2Matrix(bone_rb->getCenterOfMassTransform(), true);

			// Get the inverse offset transform
			const gef::Matrix44& rigidbody_offsettransform = bone_rb_offset_matrices_[bone_num];
			gef::Matrix44 rigidbody_offsettransform_inv;
			rigidbody_offsettransform_inv.AffineInverse(rigidbody_offsettransform);

			// Get the parent world transform
			const gef::Matrix44& parent_worldtransform = bone_world_matrices_[joint.parent];
			gef::Matrix44 parent_worldtransform_inv;
			parent_worldtransform_inv.AffineInverse(parent_worldtransform);

			// Calculate the local world transform
			// B_LT = inv(RB_OFF) * RB_WT * inv(P_WT)
			// every bone in here has a parent
			anim_bone_local_transform = rigidbody_offsettransform_inv * rigidbody_worldtransform * parent_worldtransform_inv;
		}
		else
		{
			anim_bone_local_transform = bind_pose_.local_pose()[bone_num].GetMatrix();
		}

		// calculate bone world transforms for anim skeleton
		if (joint.parent == -1)
		{
			bone_world_matrices_[bone_num] = anim_bone_local_transform;
		}
		else
		{
			bone_world_matrices_[bone_num] = anim_bone_local_transform * bone_world_matrices_[joint.parent];
		}
	}

	pose_.CalculateLocalPose(bone_world_matrices_);
	pose_.CalculateGlobalPose();
}

void Ragdoll::UpdateRagdollFromPose()
{
	for (int bone_num = 0; bone_num < bind_pose_.skeleton()->joint_count(); ++bone_num)
	{
		const gef::Joint& joint = bind_pose_.skeleton()->joint(bone_num);

		btRigidBody* bone_rb = bone_rbs_[bone_num];
		if (bone_rb)
		{
			// CALCULATE THE RIGID BODY WORLD TRANSFORM BASED ON THE CURRENT SKELETON POSE
			// Get the bone world transform of the current animation (B_WT)
			const gef::Matrix44& bone_worldtransform = pose_.global_pose()[bone_num];

			// Get the rigid body offset transform (RB_OFF)
			const gef::Matrix44& rigidbody_offsettransform = bone_rb_offset_matrices_[bone_num];

			// Calculate the new rigid body transform (RB_WT)
			// RB_WT = RB_OFF * B_WT
			const gef::Matrix44 rigidbody_worldtransform = rigidbody_offsettransform * bone_worldtransform;

			// Convert it to a btTransform
			const btTransform engineTransform = Physics::Matrix2btTransform(rigidbody_worldtransform, true);

			// Set it to the rigid bdoy and reset velocities
			bone_rb->setCenterOfMassTransform(engineTransform);
			gef::Vector4 test = rigidbody_worldtransform.GetTranslation() - bone_world_matrices_test_[bone_num].GetTranslation();
			bone_rb->setLinearVelocity(btVector3(test.x(), test.y(), test.z()));
			bone_rb->setAngularVelocity(btVector3(0.f, 0.f, 0.f));

			bone_world_matrices_test_[bone_num] = rigidbody_worldtransform;
		}
	}
}