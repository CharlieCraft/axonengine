/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/

#ifdef AX_CONFIG_OPTION_USE_SPEEDTREE_40

#include "../private.h"

namespace Axon { namespace Render {

	TreeManager::TreeManager() {
		m_defaulted = nullptr;
	}

	TreeManager::~TreeManager() {

	}

	TreeAsset* TreeManager::findAsset(const String& name, int seed) {
		String key = TreeAsset::genKey(name, seed);
		TreeAssetDict::iterator it = m_treeAssetDict.find(key);

		if (it != m_treeAssetDict.end()) {
			it->second->addref();
			return it->second;
		}

		TreeAsset* result = new TreeAsset(this);
		bool v = result->load(name, seed);

		if (!v) {
			delete result;
			m_defaulted->addref();
			return m_defaulted;
		}

		m_treeAssetDict[key] = result;
		return result;
	}

	void TreeManager::addAsset(TreeAsset* wrapper) {
		m_treeAssetDict[wrapper->getKey()] = wrapper;
	}

	void TreeManager::removeAsset(TreeAsset* wrapper) {
		m_treeAssetDict.erase(wrapper->getKey());
	}

	void TreeManager::addTree(TreeActor* tree) {
		m_treeDict.insert(tree);
	}

	void TreeManager::removeTree(TreeActor* tree) {
		m_treeDict.erase(tree);
	}

	bool TreeManager::isSupportExt(const String& ext) const {
		if (ext == "spt") {
			return true;
		}

		if (ext == "SPT") {
			return true;
		}

		return false;
	}

	Entity* TreeManager::create(const String& name, intptr_t arg) {
		return new TreeActor(name, arg);
	}

	void TreeManager::updateForFrame(QueuedScene* qscene ) {
		// do nothing
	}

	void TreeManager::issueToQueue(QueuedScene* qscene) {
#if 0
		if (!r_geoInstancing->getInteger()) {
			return;
		}
		// gen instance
		TreeAssetDict::iterator it = m_treeAssetDict.begin();
		for (; it != m_treeAssetDict.end(); ++it) {
			it->second->issueToQueueInstancing(qscene);
		}
#endif
	}

}} // namespace Axon::Render

#endif // AX_CONFIG_OPTION_USE_SPEEDTREE_40
