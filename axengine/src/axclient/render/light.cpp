/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/

#include "../private.h"

namespace Axon { namespace Render {

	class Light::ShadowInfo {
	public:
		class SplitInfo {
		public:
			SplitInfo(Light::ShadowInfo* shadowInfo, int shadowSize) {
				m_target = 0;
				m_updateFrame = -1;

				if (shadowInfo->m_light->getLightType() != Light::kGlobal)
					m_target = g_targetManager->allocTarget(Target::PooledAlloc, shadowSize, shadowSize, TexFormat::D16);
			}
			~SplitInfo() {
				if (m_target)
					g_targetManager->freeTarget(m_target);
			}

		public:
			Light::ShadowInfo* m_shadowInfo;
			Target* m_target;
			Camera m_camera;
			Vector3 m_volume[8];
			int m_updateFrame;
			float m_csmExtend;
			BoundingBox m_csmBbox;	// calc scale and offset
		};

		ShadowInfo(Light* light, int numSplits, int shadowSize) {
			m_light = light;
			m_shadowMapSize = Math::nearestPowerOfTwo(shadowSize);

			if (m_light->getLightType() == kGlobal) {
				int maxsize = g_renderDriver->getDriverInfo()->maxTextureSize;

				if (m_shadowMapSize > maxsize / 2) {
					m_shadowMapSize = maxsize / 2;
				}
				int csmSize = m_shadowMapSize * 2;

				m_csmTarget = g_targetManager->allocTarget(Target::PooledAlloc, csmSize, csmSize, TexFormat::D16);
			} else {
				m_csmTarget = 0;
			}

			TypeZeroArray(m_splits);
			m_numSplits = numSplits;
			for (int i = 0; i < m_numSplits; i++) {
				m_splits[i] = new SplitInfo(this, shadowSize);
			}
			m_updateFrame = -1;
			light->m_shadowMemoryUsed = m_numSplits * m_shadowMapSize * m_shadowMapSize * 2;

			m_origin.clear();
			m_axis.clear();
			m_radius = 0;

			init();
		}

		~ShadowInfo() {
			for (int i = 0; i < m_numSplits; i++) {
				SafeDelete(m_splits[i]);
			}
		}

		bool init()
		{
			Type t = m_light->getLightType();

			if (t == Light::kGlobal) {
				return initGlobal();
			} else if (t == Light::kPoint) {
				return initPoint();
			} else if (t == Light::kSpot) {
				return initSpot();
			}

			return false;
		}

		void update(QueuedScene* qscene)
		{
			switch (m_light->getLightType()) {
				case Light::kGlobal:
					updateGlobal(qscene);
					break;
				case Light::kPoint:
					updatePoint(qscene);
					break;
				case Light::kSpot:
					updateSpot(qscene);
					break;
			}
		}

		void issueQueuedShadow(QueuedScene* qscene)
		{
			if (m_updateFrame < 0)
				return;

			QueuedShadow* qshadow = g_renderQueue->allocType<QueuedShadow>(1);
			m_light->m_queuedLight->shadowInfo = qshadow;

			if (m_light->isGlobal()) {
				qshadow->numSplitCamera = m_numCsmSplits;
				memcpy(qshadow->splitScaleOffsets, m_splitScaleOffsets, sizeof(qshadow->splitScaleOffsets));
			} else {
				qshadow->numSplitCamera = m_numSplits;
			}

			for (int i = 0; i < qshadow->numSplitCamera; i++) {
				SplitInfo* si = m_splits[i];
				qshadow->splitCameras[i] = si->m_camera;
				memcpy(qshadow->splitVolumes[i], si->m_volume, sizeof(qshadow->splitVolumes[i]));
			}

		}

		void updatePoint(QueuedScene* qscene)
		{
			bool needRegen = initPoint();

			if (!needRegen && m_light->m_linkedNode->lastUpdateFrame < m_updateFrame) {
				issueQueuedShadow(qscene);
				return;
			}

			useShadowMap();

			int oldestFrame = -1;
			for (int i = 0; i < m_numSplits; i++) {
				SplitInfo* si = m_splits[i];

				QueuedScene* subscene = qscene->addSubScene();

				if (!subscene)
					break;

				g_statistic->incValue(stat_shadowPoolUpdate);

				subscene->sceneType = QueuedScene::ShadowGen;

				subscene->camera = si->m_camera;
				subscene->sourceLight = m_light->m_queuedLight;
				subscene->splitIndex = i;

				si->m_updateFrame = m_light->getWorld()->getVisFrameId();

				if (i == 0) {
					oldestFrame = si->m_updateFrame;
				} else {
					oldestFrame = std::min(oldestFrame, si->m_updateFrame);
				}
			}

			m_updateFrame = oldestFrame;

			issueQueuedShadow(qscene);
		}

		void updateSpot(QueuedScene* qscene)
		{
			bool needRegen = initSpot();

			if (!needRegen && m_light->m_linkedNode->lastUpdateFrame < m_updateFrame) {
				issueQueuedShadow(qscene);
				return;
			}

			useShadowMap();

			SplitInfo* si = m_splits[0];

			QueuedScene* subscene = qscene->addSubScene();
			if (!subscene) {
				issueQueuedShadow(qscene);
				return;
			}

			g_statistic->incValue(stat_shadowPoolUpdate);

			subscene->sceneType = QueuedScene::ShadowGen;

			subscene->camera = si->m_camera;
			subscene->sourceLight = m_light->m_queuedLight;
			subscene->splitIndex = 0;

			m_updateFrame = m_light->getWorld()->getVisFrameId();

			issueQueuedShadow(qscene);
		}

		// updateSplitDist computes the near and far distances for every frustum slice
		// in camera eye space - that is, at what distance does a slice start and end
		int updateSplitDist(float f[Light::MAX_CSM_SPLITS+1], float nd, float fd)
		{
			float lambda = r_csmLambda->getFloat();
			float ratio = fd/nd;
			int numsplits = r_csmSplits->getInteger();

			numsplits = Math::clamp<int>(numsplits, 1, Light::MAX_CSM_SPLITS);

			f[0] = nd;

			for (int i=1; i<numsplits; i++) {
				float si = i / (float)numsplits;

				f[i] = lambda*(nd*powf(ratio, si)) + (1-lambda)*(nd + (fd - nd)*si);
			}
			f[numsplits] = fd;

			if (lambda != m_csmLambda || numsplits != m_numCsmSplits || fd != m_csmRange) {
				m_csmLambda = lambda;
				m_numCsmSplits = numsplits;
				m_csmRange = fd;

				for (int i = 0; i < m_numCsmSplits; i++) {
					m_splits[i]->m_csmExtend = 0;
				}
			}

			return numsplits;
		}

		bool checkIfNeedUpdateSplit(int index)
		{
			static int d[] = {1,3,9,27,16,32};

			SplitInfo* si = m_splits[index];

			if (si->m_updateFrame < 0)
				return true;

			if (m_light->getWorld()->getVisFrameId() - si->m_updateFrame >= d[index])
				return true;
			else
				return false;
		}

		void updateGlobalSplit(QueuedScene* scene, int index, float* f)
		{
			QueuedScene* subscene = scene->addSubScene();
			if (!subscene)
				return;

			SplitInfo* si = m_splits[index];

			Vector3* points = si->m_volume;
			BoundingBox& bbox = si->m_csmBbox;

			const Camera& visCamera = scene->camera;

			visCamera.calcPointsAlongZdist(points, f[index]);
			visCamera.calcPointsAlongZdist(points + 4, f[index+1]);

			bbox.clear();

			si->m_camera.setTime(visCamera.getTime());

			const Matrix4& lightmatrix = si->m_camera.getViewMatrix();

			size_t s = sizeof(QueuedLight);

			for (int j = 0; j < 8; j++) {
				Vector3 lightspace = lightmatrix * points[j];
				bbox.expandBy(lightspace);
			}

			float zfar = -bbox.min.z;
			float znear = zfar - 4000;

			bbox.max.z = zfar;
			bbox.min.z = znear;

			Vector3 extends = bbox.getExtends();

			// fix sub-pixel correct
			if (si->m_csmExtend < std::max(extends.x, extends.y)) {
				si->m_csmExtend = std::max(extends.x, extends.y);
			}

			float pixeldist = si->m_csmExtend / 512.0f;

			bbox.min.x = floor(bbox.min.x / pixeldist) * pixeldist;
			bbox.max.x = bbox.min.x + si->m_csmExtend;

			bbox.min.y = floor(bbox.min.y / pixeldist) * pixeldist;
			bbox.max.y = bbox.min.y + si->m_csmExtend;

			si->m_camera.setOrtho(bbox.min.x, bbox.max.x, bbox.min.y, bbox.max.y, m_csmZfar - 512, m_csmZfar);

			subscene->sceneType = QueuedScene::ShadowGen;

			// setup atlas info
			static Point s_viewOffsets[4] = { Point(0,1), Point(1,1), Point(0,0), Point(1,0) };

			subscene->camera = si->m_camera;
			subscene->camera.setViewRect(Rect(s_viewOffsets[index].x * m_shadowMapSize, s_viewOffsets[index].y * m_shadowMapSize, m_shadowMapSize, m_shadowMapSize));
			subscene->sourceLight = m_light->m_queuedLight;
			subscene->splitIndex = index;

			si->m_updateFrame = m_light->getWorld()->getVisFrameId();
		}

		void updateZfar(QueuedScene* scene, float* f)
		{
			SplitInfo* si = m_splits[m_numCsmSplits-1];
			if (si->m_updateFrame > 0) {
				m_csmZfar = si->m_csmBbox.max.z + 16;
				return;
			}

			Vector3* points = si->m_volume;
			BoundingBox& bbox = si->m_csmBbox;

			const Camera& visCamera = scene->camera;

			visCamera.calcPointsAlongZdist(points, f[m_numCsmSplits-1]);
			visCamera.calcPointsAlongZdist(points + 4, f[m_numCsmSplits]);

			bbox.clear();

			si->m_camera.setTime(visCamera.getTime());

			const Matrix4& lightmatrix = si->m_camera.getViewMatrix();

			size_t s = sizeof(QueuedLight);

			for (int j = 0; j < 8; j++) {
				Vector3 lightspace = lightmatrix * points[j];
				bbox.expandBy(lightspace);
			}

			float zfar = -bbox.min.z;
			float znear = zfar - 4000;

			m_csmZfar = zfar + 16;
		}

		void updateGlobal(QueuedScene* scene)
		{
			if (!r_shadowGen->getBool()) {
				issueQueuedShadow(scene);
				return;
			}

			initGlobal();

			float neard = scene->camera.getZnear();
			neard = std::max(neard, 1.0f);
			float fard = r_csmRange->getFloat();

			float f[Light::MAX_CSM_SPLITS+1];

			// calculate split dist
			m_numCsmSplits= updateSplitDist(f, neard, fard);

			int statindexes[4] = {
				stat_csmSplit0Dist, stat_csmSplit1Dist, stat_csmSplit2Dist, stat_csmSplit3Dist,
			};

			for (int i = 0; i < m_numCsmSplits; i++) {
				g_statistic->setValue(statindexes[i], f[i+1]);
			}

			checkCsmTarget();

			bool allupdate = true;
			for (int i = 0; i < m_numCsmSplits; i++) {
				if (!checkIfNeedUpdateSplit(i)) {
					allupdate = false;
					break;
				}
			}

			if (allupdate) {
				updateZfar(scene, f);
			}

			for (int i = 0; i < m_numCsmSplits; i++) {
				if (checkIfNeedUpdateSplit(i)) {
					updateGlobalSplit(scene, i, f);
				}
			}

			// setup atlas info
			static const Vector3 s_offsets[4] = { Vector3(0,0,0), Vector3(0.5,0,0), Vector3(0,0.5,0), Vector3(0.5,0.5,0) };

			int oldestFrame = -1;
			for (int i = 0; i < m_numCsmSplits; i++) {
				if (i == 0) {
					oldestFrame = m_splits[i]->m_updateFrame;
					m_splitScaleOffsets[0].set(0.5, 0.5, 0, 0);
				} else {
					const BoundingBox& bbox0 = m_splits[0]->m_csmBbox;
					const BoundingBox& bbox = m_splits[i]->m_csmBbox;
					Vector3 scale = bbox0.getExtends() / bbox.getExtends() * 0.5f;
					Vector3 offset =(bbox0.min - bbox.min) / bbox0.getExtends() * scale + s_offsets[i];
					m_splitScaleOffsets[i].set(scale.x, scale.y, offset.x, offset.y);
					oldestFrame = std::min(oldestFrame, m_splits[i]->m_updateFrame);
				}
			}

			m_updateFrame = oldestFrame;

			issueQueuedShadow(scene);
		}

		bool initGlobal()
		{
			Vector3 origin = m_light->getGlobalLightDirection();

			if (m_origin == origin)
				return false;

			m_origin = origin;

			Vector3 forward = -m_light->getGlobalLightDirection();
			Vector3 up(0, 0, 1);

			if (forward.x == 0.0f) {
				up.set(1, 0, 0);
			}

			Vector3 left = up ^ forward;
			up = forward ^ left;

			left.normalize();
			up.normalize();

			for (int i = 0; i < 4; i++) {
				SplitInfo* si = m_splits[i];
				si->m_camera.setTarget(m_csmTarget);
				si->m_camera.setOrigin(origin);
				si->m_camera.setViewAxis(Matrix3(forward, left, up));
				si->m_updateFrame = -1;
			}

			return true;
		}

		void checkCsmTarget()
		{
			int maxsize = g_renderDriver->getDriverInfo()->maxTextureSize;

			int desiredSize = r_shadowMapSize->getInteger();
			if (desiredSize > maxsize / 2) {
				desiredSize = maxsize / 2;
			}
			int csmSize = desiredSize * 2;

			if (desiredSize != m_shadowMapSize) {
				if (m_csmTarget) {
					g_targetManager->freeTarget(m_csmTarget);
				}
				m_csmTarget = g_targetManager->allocTarget(Target::PooledAlloc, csmSize, csmSize, TexFormat::D16);

				for (int i = 0; i < 4; i++) {
					SplitInfo* si = m_splits[i];
					si->m_camera.setTarget(m_csmTarget);
				}
			}

			if (!m_csmTarget->m_realAllocated) {
				m_csmTarget->allocReal();
				for (int i = 0; i < 4; i++) {
					m_splits[i]->m_updateFrame = -1;
				}
			}
		}

		bool initPoint()
		{
			const AffineMat& mtx = m_light->getMatrix();

			if (m_origin == mtx.origin && m_radius == m_light->getRadius()) {
				return false;
			}

			m_origin = mtx.origin;
			m_radius = m_light->getRadius();

			// create camera
			Camera cameras[6];
			Camera::createCubemapCameras(mtx, cameras, 0.5f, m_light->getRadius());

			// calculate volumes
			for (int i=0; i<6; i++) {
				SplitInfo* si = m_splits[i];

				si->m_volume[0] = mtx.origin;
				si->m_volume[1] = mtx.origin;
				si->m_volume[2] = mtx.origin;
				si->m_volume[3] = mtx.origin;

				// TODO: fix this
				si->m_camera = cameras[i];
				si->m_camera.calcPointsAlongZdist(&si->m_volume[4], m_light->getRadius());
				si->m_camera.setTarget(si->m_target);
				si->m_camera.setViewRect(Rect(0,0,m_shadowMapSize,m_shadowMapSize));
			}

			return true;
		}

		bool initSpot()
		{
			const AffineMat& mtx = m_light->getMatrix();

			if (m_origin == mtx.origin && m_axis == mtx.axis && m_radius == m_light->getRadius()) {
				return false;
			}

			m_origin = mtx.origin;
			m_axis = mtx.axis;
			m_radius = m_light->getRadius();

			Camera& camera = m_splits[0]->m_camera;
			SplitInfo* si = m_splits[0];

			camera.setOrigin(mtx.origin);
			const Vector3 forward = -mtx.axis[2];
			const Vector3 left = mtx.axis[0];
			const Vector3 up = -mtx.axis[1];
			Matrix3 axis(forward,left,up);
			camera.setViewAxis(axis);
			camera.setFov(m_light->getSpotAngle(), m_light->getSpotAngle(), 0.5f, m_light->getRadius());

			// calculate volumes
			si->m_volume[0] = mtx.origin;
			si->m_volume[1] = mtx.origin;
			si->m_volume[2] = mtx.origin;
			si->m_volume[3] = mtx.origin;

			// TODO: fix this
			camera.calcPointsAlongZdist(&si->m_volume[4], m_light->getRadius());

			si->m_camera.setTarget(si->m_target);
			si->m_camera.setViewRect(Rect(0,0,m_shadowMapSize,m_shadowMapSize));

			return true;
		}

		void useShadowMap() {
			for (int i = 0; i < m_numSplits; i++) {
				m_splits[i]->m_target->allocReal();
			}
		}

		void unuseShadowMap() {
			for (int i = 0; i < m_numSplits; i++) {
				m_splits[i]->m_target->freeReal();
				m_splits[i]->m_updateFrame = -1;
				m_updateFrame = -1;
			}
		}

	public:
		Light* m_light;
		int m_numSplits;
		SplitInfo* m_splits[Light::MAX_SPLITS];
		int m_updateFrame;
		int m_shadowMapSize;
		Target* m_csmTarget;

		// some cached info for check if need update
		Vector3 m_origin;
		Matrix3 m_axis;
		float m_radius;

		Vector4 m_splitScaleOffsets[Light::MAX_CSM_SPLITS];
		float m_csmRange;
		float m_csmLambda;
		int m_csmZfar;
		int m_numCsmSplits;
	};



	Light::Light() : Entity(kLight)
	{
		m_type = kGlobal;
		m_castShadowMap = 0;
		m_radius = 0;
		m_spotAngle = 60;
		m_color = Vector4::One;
		m_skyColor = Vector3::One * 0.5f;
		m_envColor = Vector3::Zero;
		m_hdrStops = 0;

		m_shadowLink.setOwner(this);
		m_shadowInfo = 0;
		m_queuedLight = 0;

		m_preferShadowMapSize = 256;
	}

	Light::Light(Type t, const Vector3& pos, Rgb color) : Entity(kLight)
	{
		m_type = t;
		setOrigin(pos);
		m_color = Vector4::One;
		m_skyColor = Vector3::One * 0.5f;
		m_envColor = Vector3::Zero;
		m_spotAngle = 60;
		m_castShadowMap = 0;
		m_radius = 0;
		m_hdrStops = 0;

		m_shadowLink.setOwner(this);
		m_shadowInfo = 0;
		m_queuedLight = 0;

		m_preferShadowMapSize = 256;
	}

	Light::Light(Type t, const Vector3& pos, Rgb color, float radius) : Entity(kLight)
	{
		m_type = t;
		setOrigin(pos);
		m_color = Vector4::One;
		m_skyColor = Vector3::One * 0.5f;
		m_envColor = Vector3::Zero;
		m_spotAngle = 60;
		m_castShadowMap = 0;
		m_radius = radius;
		m_hdrStops = 0;

		m_shadowLink.setOwner(this);
		m_shadowInfo = 0;
		m_queuedLight = 0;

		m_preferShadowMapSize = 256;
	}

	Light::~Light()
	{
		m_shadowLink.remove();
	}

	BoundingBox Light::getLocalBoundingBox()
	{
		if (m_type == kGlobal) {
			return BoundingBox::LargestBox;
		}

		if (m_type == kPoint) {
			return BoundingBox(-m_radius,-m_radius,-m_radius,m_radius,m_radius,m_radius);
		}

		if (m_type == kSpot) {
			float angle = Math::d2r(m_spotAngle * 0.5f);
			float len, height;
			Math::sincos(angle,len,height);
			len *= m_radius / height;
			height = m_radius;
			return BoundingBox(-len,-len,-height,len,len,0);
		}

		return BoundingBox::UnitBox;
	}

	BoundingBox Light::getBoundingBox()
	{
		if (m_type == kGlobal) {
			return BoundingBox::LargestBox;
		}

		return getLocalBoundingBox().getTransformed(m_affineMat);
	}


	void Light::fillQueued(QueuedLight* queued)
	{
		m_queuedLight = queued;

		if (!queued) {
			return;
		}

		queued->preQueued = this;

		queued->type = m_type;
		queued->matrix = m_affineMat;

		queued->pos = m_affineMat.origin;
		if (m_type == Light::kGlobal) {
			queued->pos.xyz().normalize();
			queued->pos.w = 0;
		}

		queued->color = m_color;

		queued->skyColor = m_skyColor;
		queued->envColor = m_envColor;
		queued->radius = m_radius;
	}

	void Light::issueToQueue(QueuedScene* qscene)
	{
		if (!m_queuedLight) {
			return;
		}

		if (qscene->sceneType != QueuedScene::WorldMain)
			return;

		prepareLightBuffer(qscene);

#if 0
		bool shadowed = this->prepareShadow(qscene);

		if (!shadowed) {
			return;
		}

		TexFormat smf = gTargetManager->getSuggestFormat(Target::SHADOW_MAP);

		QueuedShadow* qshadow = m_queuedLight->shadowInfo;
		if (0 &&!r_csmAtlas->getBool()) {
			int mapsize = r_shadowMapSize->getInteger();

			for (int i = 0; i < qshadow->numSplitCamera; i++) {
				QueuedScene* subscene = gRenderQueue->allocQueuedScene();
				qscene->subScenes[qscene->numSubScenes++] = subscene;
				subscene->sceneType = QueuedScene::ShadowGen;

				subscene->camera = qshadow->splitCameras[i];
				Target* target = gTargetManager->allocTarget(Target::FrameAlloc, mapsize, mapsize, smf);
				qshadow->shadowGenScenes[i] = subscene;

				subscene->camera.setTarget(target);
				subscene->camera.setViewRect(Rect(0, 0, mapsize, mapsize));
				subscene->sourceLight = m_queuedLight;
				subscene->splitIndex = i;
			}
		} else {
			int mapsize = r_shadowMapSize->getInteger();
			int maxsize = gRenderDriver->getDriverInfo()->maxTextureSize;

			if (mapsize > maxsize / 2) {
				mapsize = maxsize / 2;
			}

			Target* target = gTargetManager->allocTarget(Target::FrameAlloc, mapsize*2, mapsize*2, smf);
#if 0
			qshadow->useCsmAtlas = true;
#endif
			static Point s_offsets[4] = { Point(0,1), Point(1,1), Point(0,0), Point(1,0) };

			for (int i = 0; i < qshadow->numSplitCamera; i++) {
				QueuedScene* subscene = g_renderQueue->allocQueuedScene();
				qscene->subScenes[qscene->numSubScenes++] = subscene;
				TypeZero(subscene);
				subscene->sceneType = QueuedScene::ShadowGen;
				qshadow->shadowGenScenes[i] = subscene;

				subscene->camera = qshadow->splitCameras[i];
				subscene->camera.setTarget(target);
				subscene->camera.setViewRect(Rect(s_offsets[i].x * mapsize, s_offsets[i].y * mapsize, mapsize, mapsize));
				subscene->sourceLight = m_queuedLight;
				subscene->splitIndex = i;
			}
		}
#endif
	}

	void Light::prepareLightBuffer(QueuedScene* scene)
	{
		if (m_type == kGlobal) {
			prepareLightBuffer_Global(scene);
		} else if (m_type == kPoint) {
			prepareLightBuffer_Point(scene);
		} else if (m_type == kSpot) {
			prepareLightBuffer_Spot(scene);
		}
	}

	void Light::prepareLightBuffer_Global( QueuedScene* scene )
	{
		const Camera& camera = scene->camera;
		float znear = camera.getZnear();
		float zfar = camera.getZfar();

		camera.calcPointsAlongZdist(m_queuedLight->lightVolume, znear);
		camera.calcPointsAlongZdist(&m_queuedLight->lightVolume[4], zfar-50);
	}

	void Light::prepareLightBuffer_Point(QueuedScene* scene)
	{
		// calculate light volume
		const Vector3& origin = getOrigin();
		const Matrix3& viewaxis = getAxis();

		for (int i = 0; i < 2; i++) {
			Vector3 back_forward = viewaxis[0] *((i==0) ? -1.0f : 1.0f);
			for (int j = 0; j < 2; j++) {
				Vector3 left_right = viewaxis[1] *((j==0) ? -1.0f : 1.0f);
				for (int k = 0; k < 2; k++) {
					Vector3 down_up = viewaxis[2] *((k==0) ? -1.0f : 1.0f);

					m_queuedLight->lightVolume[i*4+j*2+k] = origin +(back_forward+left_right+down_up) * m_radius;
				}
			}
		}

		// calculate texture projection matrix
		m_queuedLight->projMatrix.fromAxisInverse(m_affineMat.axis, m_affineMat.origin);
		m_queuedLight->projMatrix.scale(1.0f/m_radius, 1.0f/m_radius, 1.0f/m_radius);

		// check if intersert near clip plane
		if (m_linkedBbox.pointDistance(scene->camera.getOrigin()) > scene->camera.getZnear() * 4.0f) {
			m_queuedLight->isIntersectsNearPlane = false;
		} else {
			m_queuedLight->isIntersectsNearPlane = true;
		}
	}

	void Light::prepareLightBuffer_Spot(QueuedScene* scene)
	{
		// calculate light volume
		const Vector3& origin = getOrigin();
		const Matrix3& viewaxis = getAxis();
		const Vector3& back_forward = -viewaxis[2] * m_radius;

		float extend = m_radius * atanf(Math::d2r(m_spotAngle*0.5f));

		m_queuedLight->lightVolume[0] = origin;
		m_queuedLight->lightVolume[1] = origin;
		m_queuedLight->lightVolume[2] = origin;
		m_queuedLight->lightVolume[3] = origin;

		for (int j = 0; j < 2; j++) {
			Vector3 left_right = viewaxis[0] *((j==0) ? 1.0f : -1.0f);
			for (int k = 0; k < 2; k++) {
				Vector3 down_up = viewaxis[1] *((k==0) ? -1.0f : 1.0f);

				m_queuedLight->lightVolume[4+j*2+k] = origin + back_forward + (left_right+down_up) * extend;
			}
		}

		// calculate texture projection matrix
		m_queuedLight->projMatrix.fromAxisInverse(m_affineMat.axis, m_affineMat.origin);
		m_queuedLight->projMatrix.scale(1.0f/extend, 1.0f/extend, 1.0f/m_radius);

		const Matrix4& m = m_queuedLight->projMatrix;
		Vector3 p1 = origin;
		Vector3 p2 = m_queuedLight->lightVolume[4];

		p1 = m * p1;
		p2 = m * p2;

		// check if intersect near clip plane
		if (m_linkedBbox.pointDistance(scene->camera.getOrigin()) > scene->camera.getZnear() * 4.0f) {
			m_queuedLight->isIntersectsNearPlane = false;
		} else {
			m_queuedLight->isIntersectsNearPlane = true;
		}
	}

	void Light::initShadowInfo()
	{
		if (!m_castShadowMap)
			return;

		int csmSize = r_shadowMapSize->getInteger();
		csmSize = Math::nearestPowerOfTwo(csmSize);

		int localShadowSize = Math::nearestPowerOfTwo(m_preferShadowMapSize);

		if (m_type == kGlobal) {
			m_shadowInfo = new ShadowInfo(this, 4, csmSize);
		} else if (m_type == kPoint) {
			m_shadowInfo = new ShadowInfo(this, 6, localShadowSize);
		} else {
			m_shadowInfo = new ShadowInfo(this, 1, localShadowSize);
		}
	}

	void Light::clearShadowInfo()
	{
		SafeDelete(m_shadowInfo);
	}

	bool Light::checkShadow( QueuedScene* qscene )
	{
		if (!m_castShadowMap) {
			return false;
		}

		if (!m_shadowInfo) {
			initShadowInfo();
		}

		if (isGlobal()) {
			genShadowMap(qscene);
			return false;
		}

		return true;
	}

	void Light::linkShadow()
	{
		m_shadowLink.addToFront(m_world->m_shadowLink);
	}

	Light* Light::unlinkShadow()
	{
		if (m_shadowLink.isInList()) {
			Light* l = m_shadowLink.getNext();
			m_shadowLink.remove();
			return l;
		}

		return 0;
	}

	void Light::freeShadowMap()
	{
		AX_ASSERT(m_shadowInfo);

		m_shadowInfo->unuseShadowMap();
	}

	bool Light::genShadowMap(QueuedScene* qscene)
	{
		AX_ASSERT(m_shadowInfo);

		if (isVisable() || isGlobal()) {
			m_shadowInfo->update(qscene);
			return true;
		}

		return false;
	}

	int Light::getShadowMemoryUsed() const
	{
		if (!m_shadowInfo)
			return 0;

		return m_shadowMemoryUsed;
	}

	void Light::setLightColor( Rgb color, float intensity, float specularX )
	{
		m_color.xyz() = color.toVector() * intensity;
		m_color.w = m_color.xyz().getLength() * specularX;
	}

	void Light::setSkyColor( Rgb color, float skyIntensity /*= 1.0f*/ )
	{
		m_skyColor = color.toVector() * skyIntensity;
	}

	void Light::setEnvColor( Rgb color, float envIntensity /*= 1.0f*/ )
	{
		m_envColor = color.toVector() * envIntensity;
	}


}} // namespace Axon::Render
