/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#ifndef AX_CLIENT_SPEEDTREE_ASSET_H
#define AX_CLIENT_SPEEDTREE_ASSET_H

// forward declaration external library's type
class CSpeedTreeRT;

namespace Axon { namespace Render {

	class TreeActor;
	class TreeManager;

	// NOTE: TreeAsset not managed by AssetManager, but managed by TreeManager
	class TreeAsset : public RefObject {
	public:
		enum {
			MAX_LODS = 6
		};

		TreeAsset(TreeManager* forest);
		~TreeAsset();

		bool load(const String& filename, int seed);
		String getKey() const;

		BoundingBox getBoundingBox() const;
		Primitives getAllPrimitives(float lod) const;

		void addActor(TreeActor* actor);
		void removeActor(TreeActor* actor);

		void issueToQueue(TreeActor* actor, QueuedScene* qscene);
#if 0
		void issueToQueueInstancing(QueuedScene* qscene);
#endif
		static String genKey(const String& filename, int seed);

	protected:
		void loadMaterials();
		void buildPrimitives();
		void buildBranch();
		void buildFrond();
		void buildLeafCard();
		void buildLeafMesh();
//		void setMaterialColor(Material* mat, const float* f, float scaler);

	private:
		TreeManager* m_manager;
		CSpeedTreeRT* m_treeRt;
		String m_filename;
		String m_filepath;
		int m_seed;
		String m_key;

		// primitives
		int m_numBranchLods;
		Primitive* m_branchPrims[MAX_LODS];

		int m_numFrondLods;
		Primitive* m_frondPrims[MAX_LODS];

		int m_numLeafLods;
		Primitive* m_leafPrims[MAX_LODS];

		// material
		MaterialPtr m_branchMat;
		MaterialPtr m_frondMat;
		MaterialPtr m_leafCardMat;
		MaterialPtr m_leafMeshMat;
		
		// actors use this asset
		TreeDict m_treeActors;
	};

	inline String TreeAsset::genKey(const String& filename, int seed) {
		String result;
		StringUtil::sprintf(result, "%s_%d", filename.c_str(), seed);
		return result;
	}

	inline void TreeAsset::addActor(TreeActor* actor) {
		m_treeActors.insert(actor);
	}

	inline void TreeAsset::removeActor(TreeActor* actor) {
		m_treeActors.erase(actor);
	}


}} // namespace Axon::Render

#endif // end guardian

