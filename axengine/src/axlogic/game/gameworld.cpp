/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/



#include "../private.h"

namespace Axon { namespace Game {

	World::World() {
		m_onlyClient = false;
		m_onlyServer = false;
		m_multiPlayer = false;
		TypeZeroArray(m_entities);
		TypeZeroArray(m_spawnIds);

		m_numEntities = EntityNum::MAX_CLIENTS;
		m_firstFreeEntity = EntityNum::MAX_CLIENTS;
		m_numClients = 0;

		Landscape* landscape = new Landscape(this);
		m_entities[EntityNum::LANDSCAPE] = landscape;

		m_renderWorld = new RenderWorld();
		m_renderWorld->initialize(2048);
		m_outdoorEnv = m_renderWorld->getOutdoorEnv();

		m_physicsWorld = new PhysicsWorld();
		m_physicsWorld->attachObserver(this);

		m_soundWorld = new SoundWorld();

		m_mapEnvDef = new Map::EnvDef();

		m_lasttime = OsUtil::milliseconds();
		m_frametime = 0;

		g_gameSystem->setGameWorld(this);
	}

	World::~World() {
		g_gameSystem->setGameWorld(0);

		m_physicsWorld->detachObserver(this);
		SafeDelete(m_physicsWorld);
		SafeDelete(m_renderWorld);
	}

	void World::runFrame(int what, int frametime)
	{
		m_frametime = frametime;
		m_lasttime += m_frametime;

		if (what & Think_physics) {
			m_physicsWorld->step(m_frametime);
		}

		for (int i = 0; i < m_numEntities; i++) {
			if (!m_entities[i]) {
				continue;
			}

			m_entities[i]->doThink();
		}
	}

	void World::drawFrame()
	{
		m_lastCamera.setTime(m_lasttime);

		if (m_entities[0]) {
			Player* player = static_cast<Player*>(m_entities[0]);
			AffineMat player3rd = player->getThirdPersonMatrix();
			m_lastCamera.setOrigin(player3rd.origin);
			m_lastCamera.setViewAxis(player3rd.axis);
		}

		g_renderSystem->beginFrame(m_lastCamera.getTarget());
		drawScene(m_lastCamera);
		g_renderSystem->endFrame();
	}

	void World::drawScene(const Render::Camera& camera)
	{
		m_lastCamera = camera;

		m_soundWorld->setListener(AffineMat(camera.getViewAxis(), camera.getOrigin()), Vector3(0,0,0));
		g_soundSystem->setWorld(m_soundWorld);

		if (m_entities[0] && !g_gameSystem->isRunning()) {
			Player* player = static_cast<Player*>(m_entities[0]);
			player->setMatrix(AffineMat(camera.getViewAxis(), camera.getOrigin()));
		}

		g_renderSystem->beginScene(camera);
		g_renderSystem->addToScene(m_renderWorld);
		g_renderSystem->endScene();
	}

	void World::addEntity(Entity* entity) {
		int start = m_firstFreeEntity;
		int end = EntityNum::MAX_NORMAL;

		if (entity->isPlayer()) {
			start = m_numClients;
			end = EntityNum::MAX_CLIENTS;
		}

		int i = start;
		while (i < end) {
			if (!m_entities[i])
				break;

			i++;
		}

		if (i == end) {
			Errorf("no free entity's slot to add entity");
			return;
		}

		m_entities[i] = entity;
		entity->m_entityNum = i;
		entity->m_world = this;
		entity->doSpawn();

		if (entity->isPlayer()) {
			m_numClients = std::max(i+1, m_numClients);
		} else {
			m_numEntities = std::max(i+1, m_numEntities);
			m_firstFreeEntity = m_numEntities;
		}
	}

	void World::removeEntity(Entity* entity) {
		int num = entity->m_entityNum;
		if (num < 0) {
			Errorf("not a valid entity number");
			return;
		}
		AX_ASSERT(num >= 0 && num < EntityNum::MAX_ENTITIES);

		entity->doRemove();

		m_entities[num] = nullptr;
		entity->m_entityNum = -1;
		entity->m_world = nullptr;

		m_firstFreeEntity = num;
	}

	Entity* World::createEntity(const char* clsname) {
		Object* obj = g_scriptSystem->createObject(clsname);

		if (!obj) {
			return nullptr;
		}

		Entity* ent = object_cast<Entity*>(obj);

		if (!ent) {
			delete obj;
			return nullptr;
		}
#if 0
		String objname = clsname;
		StringUtil::strlwr(&objname[0]);
		objname = gScriptSystem->generateObjectName(objname);

		ent->set_objectName(objname);
#endif
		return ent;
	}

	void World::addFixed(Fixed* fixed) {
		getLandscape()->addFixed(fixed);
	}

	void World::removeFixed(Fixed* fixed) {
		getLandscape()->removeFixed(fixed);
	}

	void World::doNotify(IObservable* subject, int arg) {

	}

	void World::updateEnvdef() {
		m_outdoorEnv->setHaveFarSky(m_mapEnvDef->m_haveSkyBox);
		m_outdoorEnv->setHaveOcean(m_mapEnvDef->m_haveOcean);
		m_outdoorEnv->setFog(m_mapEnvDef->m_fogColor.toVector(), m_mapEnvDef->m_fogDensity);
		m_outdoorEnv->setOceanFog(m_mapEnvDef->m_oceanFogColor.toVector(), m_mapEnvDef->m_oceanFogDensity);
		m_outdoorEnv->setHaveGlobalLight(m_mapEnvDef->m_haveGlobalLight);
		m_outdoorEnv->setSunColor(m_mapEnvDef->m_sunColor, m_mapEnvDef->m_sunColorX, 1);
		m_outdoorEnv->setSkyColor(m_mapEnvDef->m_skyColor, m_mapEnvDef->m_skyColorX);
		m_outdoorEnv->setEnvColor(m_mapEnvDef->m_envColor, m_mapEnvDef->m_envColorX);
		m_outdoorEnv->setCastShadow(m_mapEnvDef->m_castShadow);
		m_outdoorEnv->setSkyBoxTexture(m_mapEnvDef->m_textureSkyBox);

		float alpha = Math::d2r(m_mapEnvDef->m_dayTime / 24 * 360 - 90);
		float beta = Math::d2r(Math::clamp(m_mapEnvDef->m_latitude,-90.0f, 90.0f));

		Vector3 sundir;
		float bc;
		Math::sincos(alpha, sundir.z, sundir.x);
		Math::sincos(-beta, sundir.y, bc);

		sundir.x *= bc;
		sundir.z *= bc;

		m_outdoorEnv->setSunDir(sundir);
	}

	void World::setWindow(Render::Target* targetWin)
	{
		m_targetWindow = targetWin;
	}

	void World::restoreEntities()
	{
		for (int i = EntityNum::MAX_CLIENTS; i < m_numEntities; i++) {
			if (m_entities[i]) {
				m_entities[i]->doPropertyChanged();
			}
		}

	}

	void World::reset()
	{

	}

	void World::addNode(Node* node)
	{
		if (node->isFixed())
			addFixed((Fixed*)node);
		else
			addEntity((Entity*)node);
	}

	void World::removeNode(Node* node)
	{
		if (node->isFixed())
			removeFixed((Fixed*)node);
		else
			removeEntity((Entity*)node);
	}

	AffineMat World::getLastViewMatrix() const
	{
		return AffineMat(m_lastCamera.getViewAxis(), m_lastCamera.getOrigin());
	}

}} // namespace Axon::Game

