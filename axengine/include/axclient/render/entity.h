/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/

#ifndef AX_RENDER_ACTOR_H
#define AX_RENDER_ACTOR_H

namespace Axon { namespace Render {

	// forward declaration
	struct QueuedScene;
	typedef Sequence<QueuedScene*>	QueuedSceneSeq;

	struct QueuedEntity {
		enum {
			MAX_TARGETS = 8
		};

		AffineMat m_matrix;
		Vector4 instanceParam;
		int flags;			// Actor::Flag

		float distance;

		// used by interaction
		int m_chainId;
		Interaction* m_headInteraction;

		void interactionChain(Interaction* last, int chainId);
	};

	struct QueuedLight;

	inline void QueuedEntity::interactionChain(Interaction* last, int chainId) {
		if (m_chainId != chainId) {
			m_headInteraction = last;
			m_chainId = chainId;
			return;
		}

		last->actorNext = m_headInteraction;
		m_headInteraction = last;
	}

	class QuadNode;

	class AX_API Entity {
	public:
		friend class World;

		enum Flag {
			DepthHack = 1, OutsideOnly = 2
		};

		enum Kind {
			kNone, kModel, kSpeedTree, kAnchor, kPartical, kUserInterface,
			kLight, kFog, kVisArea, kPortal, kOccluder, kTerrain, kOutdoorEnv
		};

		Entity(Kind type );
		virtual ~Entity();

		Kind getKind() const { return m_kind; }

		// read and writable
		const Vector3& getOrigin() const;
		void setOrigin(const Vector3& origin);
		const Matrix3& getAxis() const;
		void setAxis(const Angles& angles);
		void setAxis(const Angles& angles, float scale);
		const AffineMat& getMatrix() const;
		void setMatrix(const AffineMat& mat);

		void setInstanceColor(const Vector3& color);
		Vector3 getInstanceColor() const;

		bool isPresented() const { return m_world != 0; }
		void updateToWorld();

		int getFlags() const;
		void setFlags(int flags);
		void addFlags(int flags);
		bool isFlagSet(Flag flag);

		bool isVisable() const;
		bool isLight() const { return m_kind == kLight; }

		// queued
		void setQueued(QueuedEntity* queued);
		QueuedEntity* getQueued() const;

		// read only
		Matrix4 getModelMatrix() const;

		void update(QueuedScene* qscene, Plane::Side side);
		void updateCsm(QueuedScene* qscene, Plane::Side side);
		bool isCsmCulled() const;
		float getVisSize() const { return m_visSize; }
		World* getWorld() const { return m_world; }
		void setWorld(World* world) { m_world = world; }

		// helper prims
		void clearHelperPrims();
		void addHelperPrim(Primitive* prim);
		const Primitives& getHelperPrims() const;

		// generate render primitive
		virtual BoundingBox getLocalBoundingBox() = 0;
		virtual BoundingBox getBoundingBox() = 0;
		virtual Primitives getAllPrimitives() { return Primitives(); }
		virtual Primitives getSelectionPrims() { return getAllPrimitives(); }

		// new interface
		virtual void doUpdate(QueuedScene* qscene);
		virtual void doCalculateLod(QueuedScene* qscene);
		virtual Vector4 getInstanceParam() const;
		virtual void issueToQueue(QueuedScene* qscene) {}

	protected:
		const Kind m_kind;
		AffineMat m_affineMat;
		int m_flags;
		Vector4 m_instanceParam;

		QueuedEntity* m_queued;

		int m_visFrameId;
		BoundingBox m_linkedBbox;
		float m_linkedExtends;
		float m_distance;
		float m_lodRatio;
		float m_lod;
		float m_visSize;
		bool m_viewDistCulled;
		bool m_queryCulled;

		// linked info
		Link<Entity>			m_nodeLink;
		QuadNode* m_linkedNode;
		int m_linkedFrame;

		// helper primitives
		Primitives m_helperPrims;

		// used by world
		World* m_world;
		Query* m_visQuery;
		Query* m_shadowQuery;
		Plane::Side m_cullSide;
	};

	inline void Entity::clearHelperPrims()
	{
		m_helperPrims.clear();
	}

	inline void Entity::addHelperPrim(Primitive* prim)
	{
		m_helperPrims.push_back(prim);
	}

	inline const Primitives& Entity::getHelperPrims() const
	{
		return m_helperPrims;
	}



	typedef Sequence<Entity*>		ActorSeq;

	//--------------------------------------------------------------------------
	// class IEntityManager
	//--------------------------------------------------------------------------

	class AX_API IEntityManager {
	public:
		virtual bool isSupportExt(const String& ext) const = 0;
		virtual Entity* create(const String& name, intptr_t arg = 0) = 0;
		virtual void updateForFrame(QueuedScene* qscene ) {}
		virtual void issueToQueue(QueuedScene* qscene) {}
	};

}} // namespace Axon::Render

#endif // AX_RENDER_ACTOR_H
