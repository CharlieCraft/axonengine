/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#ifndef AX_PHYSICS_PUBLIC_H
#define AX_PHYSICS_PUBLIC_H

// forward declaration for havok engine
class hkpShape;
class hkpEntity;
class hkpRigidBody;
class hkpWorld;
class hkaMeshBinding;
class hkLoader;
class hkRootLevelContainer;
class hkVisualDebugger;
class hkpMultithreadingUtil;
class hkaSkeleton;
class hkxScene;
class hkaAnimatedSkeleton;
class hkaAnimationControl;
class hkaSkeletalAnimation;
class hkxNode;
class hkMatrix4;
class hkxMesh;
class hkpPhysicsData;
class hkaAnimationContainer;
class hkaDefaultAnimationControl;
class hkaAnimationBinding;
class hkaPose;
class hkaRagdollInstance;
class hkaSkeletonMapper;
class hkxMaterial;
class hkDataWorldDict;
class hkpCharacterProxy;

namespace Axon { namespace Game {
	class Entity;
}} // namespace Axon::Game

namespace Axon { namespace Physics {

	using namespace Axon::Logic;

	// forward declaration
	class Entity;
	class RigidBody;
	class Terrain;
	class Ragdoll;
	class PhysicsPlayer;

	typedef Game::Entity gameEntity;

}} // namespace Axon::Physics

#include "shape.h"
#include "package.h"
#include "world.h"
#include "system.h"

namespace Axon {

	typedef Physics::System PhysicsSystem;
	typedef Physics::World PhysicsWorld;
	typedef Physics::Entity PhysicsEntity;
	typedef Physics::RigidBody physicsRigid;
	typedef Physics::Rig PhysicsRig;
	typedef Physics::Pose PhysicsPose;
	typedef Physics::SkeletalModel PhysicsModel;
	typedef Physics::Animation PhysicsAnimation;
	typedef Physics::Animator PhysicsAnimator;
	typedef Physics::Package PhysicsPackage;
	typedef Physics::PackageManager PhysicsPackageManager;

	typedef Physics::Ragdoll PhysicsRagdoll;

	typedef Physics::PhysicsPlayer PhysicsPlayer;

	extern PhysicsSystem* g_physicsSystem;
	extern PhysicsPackageManager* g_physicsPackageManager;

} // namespace Axon

#endif // end guardian



