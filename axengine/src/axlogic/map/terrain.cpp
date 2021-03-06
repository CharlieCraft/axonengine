/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#include "../private.h"

namespace Axon { namespace Map {

	static const float ALTITUDE_BLUR = 4.0f;
	static const float SLOPE_BLUR = 16.0f;

	//--------------------------------------------------------------------------
	// class LayerGen
	//--------------------------------------------------------------------------
	AlphaBlock* const AlphaBlock::One = (AlphaBlock* const)Map::AlphaOneIndex;
	AlphaBlock* const AlphaBlock::Zero = (AlphaBlock* const)Map::AlphaZeroIndex;

	AlphaBlock::AlphaBlock() {
		m_isDirty = false;
	}

	AlphaBlock::~AlphaBlock() {
	}

	const AlphaBlock::Data& AlphaBlock::getData() const {
		return m_data;
	}

	AlphaBlock::Data& AlphaBlock::lock() {
		return m_data;
	}
	void AlphaBlock::unlock() {
		m_isDirty = true;
	}

	void AlphaBlock::setAll(byte_t d) {
		memset(m_data, d, sizeof(m_data));
		m_isDirty = true;
	}


	Texture* AlphaBlock::getTexture() {
		if (m_isDirty)
			updateTexture();

		m_isDirty = false;
		return m_texture;
	}


	void AlphaBlock::updateTexture() {
		if (!m_texture) {
			m_texture << (Texture*)g_assetManager->createEmptyAsset(Asset::kTexture);
			m_texture->initialize(TexFormat::A8, Map::ChunkPixels, Map::ChunkPixels);
			String key;
			StringUtil::sprintf(key, "_alphablock_%d", g_system->generateId());
			g_assetManager->addAsset(Asset::kTexture, key, m_texture.get());

			m_texture->setClampMode(Texture::CM_ClampToEdge);
		}

		m_texture->uploadSubTexture(Rect(0,0,Map::ChunkPixels,Map::ChunkPixels), &m_data[0][0]);
	}

	LayerGen::LayerGen(Terrain* terrain, int layerId) {
		m_terrain = terrain;
		m_layerId = layerId;

		m_pixelIndexOffset = m_terrain->getChunkIndexOffset() * Map::ChunkPixels;

		Map::LayerDef* l = getLayerDef();

		// init alphablocks
		int numchunks = m_terrain->getNumChunks();
		m_alphaBlocks = new AlphaBlock*[numchunks*numchunks];
		memset(m_alphaBlocks, 0, sizeof(AlphaBlock*)*numchunks*numchunks);

		// init color template
		m_colorTemplate = nullptr;
		m_detailMat = nullptr;

		update();
#if 0
		if (!l->baseImage.empty()) {
			m_colorTemplate = new Image();
			if (!m_colorTemplate->loadFile_tga(l->baseImage)) {
				SafeDelete(m_colorTemplate);
			}
		}

		if (l->isUseDetail) {
			m_detailMat = UniqueAsset_<renderMaterial>("terrain");

			String fn = PathUtil::removeExt(l->detailMat);

			Texture* diffuse = FindAsset_<Texture>(fn);
			Texture* normal = FindAsset_<Texture>(fn+"_n");
			Texture* specular = FindAsset_<Texture>(fn+"_s");

			m_detailMat->setTexture(SamplerType::Diffuse, diffuse);
			if (!normal->isDefaulted()) {
				m_detailMat->setTexture(SamplerType::Normal, normal);
			}
			if (!specular->isDefaulted()) {
				m_detailMat->setTexture(SamplerType::Specular, specular);
			}

			if (m_colorTemplate)
				return;

#if 0
			Image image;
			bool v = image.loadFile(l->detailMat, Image::NoCompressed);
			if (!v) {
				Errorf("%s: can't load diffuse texture", __func__);
				return;
			}

			float meterpixels = 4.0f / m_terrain->getTileMeters();
			Vector2 imagesize(meterpixels, meterpixels);
			imagesize /= l->uvScale;

			m_colorTemplate = image.resize(imagesize.x, imagesize.y);
			AX_ASSURE(m_colorTemplate);
#endif
		}
#endif
	}

	LayerGen::~LayerGen() {
		// TODO free image and blocks
		SafeDelete(m_colorTemplate);

		int chunkCount = m_terrain->getNumChunks() * m_terrain->getNumChunks();
		for (int i = 0; i<chunkCount; i++) {
			if (m_alphaBlocks[i] == AlphaBlock::One)
				continue;

			if (m_alphaBlocks[i] == AlphaBlock::Zero)
				continue;

			// free block
			m_terrain->freeAlphaBlock(m_alphaBlocks[i]);
		}

		SafeDeleteArray(m_alphaBlocks);
	}

	void LayerGen::update() {
		m_detailMat = 0;

		// TODO free image and blocks
		SafeDelete(m_colorTemplate);

		Map::LayerDef* l = getLayerDef();

		if (!l->baseImage.empty()) {
			m_colorTemplate = new Image();
			if (!m_colorTemplate->loadFile_tga(l->baseImage)) {
				SafeDelete(m_colorTemplate);
			}
		}

		if (l->isUseDetail) {
			m_detailMat = UniqueAsset_<RenderMaterial>("terrain");

			String fn = PathUtil::removeExt(l->detailMat);

			TexturePtr diffuse = FindAsset_<Texture>(fn);
			TexturePtr normal = FindAsset_<Texture>(fn+"_n");
			TexturePtr specular = FindAsset_<Texture>(fn+"_s");

			m_detailMat->setTexture(SamplerType::Diffuse, diffuse.get());
			if (!normal->isDefaulted()) {
				m_detailMat->setTexture(SamplerType::Normal, normal.get());
			}
			if (!specular->isDefaulted()) {
				m_detailMat->setTexture(SamplerType::Specular, specular.get());
			}

#if 0
			SafeRelease(diffuse);
			SafeRelease(normal);
			SafeRelease(specular);

			if (m_colorTemplate)
				return;

			Image image;
			bool v = image.loadFile(l->detailMat, Image::NoCompressed);
			if (!v) {
				Errorf("%s: can't load diffuse texture", __func__);
				return;
			}

			float meterpixels = 4.0f / m_terrain->getTileMeters();
			Vector2 imagesize(meterpixels, meterpixels);
			imagesize /= l->uvScale;

			m_colorTemplate = image.resize(imagesize.x, imagesize.y);
			AX_ASSURE(m_colorTemplate);
#endif
		}
	}



	void LayerGen::load(File* f) {
//		m_layerId = f->readInt();
		int numchunks = m_terrain->getNumChunks();

		for (int y = 0; y < numchunks; y++) {
			for (int x = 0; x < numchunks; x++) {
				int type = f->readInt();
				AlphaBlock* b = 0;
				if (type == 0) {
					b = AlphaBlock::Zero;
				} else if (type == 1) {
					b = AlphaBlock::One;
				} else if (type == 2) {
					b = m_terrain->allocAlphaBlock();
					AlphaBlock::Data& d = b->lock();
					f->read(d, sizeof(AlphaBlock::Data));
					b->unlock();
				} else {
					Errorf("%s: error alpha block", __func__);
				}
				m_alphaBlocks[y*numchunks+x] = b;
			}
		}
	}

	void LayerGen::save(File* f) {
//		f->writeInt(m_layerId);
		int numchunks = m_terrain->getNumChunks();

		for (int y = 0; y < numchunks; y++) {
			for (int x = 0; x < numchunks; x++) {
				AlphaBlock* b = m_alphaBlocks[y*numchunks+x];
				if (b == AlphaBlock::Zero) {
					f->writeInt(0);
				} else if (b == AlphaBlock::One) {
					f->writeInt(1);
				} else {
					f->writeInt(2);
					f->write(b->getData(), sizeof(AlphaBlock::Data));
				}
			}
		}
	}


	Terrain* LayerGen::getTerrain() const {
		return m_terrain;
	}

	int LayerGen::getLayerId() const {
		return m_layerId;
	}

	Map::LayerDef* LayerGen::getLayerDef() const {
		return m_terrain->getMaterialDef()->findLayerDefById(m_layerId);
	}

	Image* LayerGen::getColorTemplate() const {
		return m_colorTemplate;
	}

	AlphaBlock* LayerGen::getBlock(const Point& index) const {
		Point offset = index + m_terrain->getChunkIndexOffset();
		int idx = offset.y * m_terrain->getNumChunks() + offset.x;

		return m_alphaBlocks[idx];
	}

	void LayerGen::setBlock(const Point& index, AlphaBlock* block) {
		Point offset = index + m_terrain->getChunkIndexOffset();
		int idx = offset.y * m_terrain->getNumChunks() + offset.x;

		m_alphaBlocks[idx] = block;
	}


	bool LayerGen::isAlphaZero(const Point& index) const {
		intptr_t p = (intptr_t)getBlock(index);
		return p == Map::AlphaZeroIndex;
	}

	bool LayerGen::isAlphaOne(const Point& index) const {
		intptr_t p = (intptr_t)getBlock(index);
		return p == Map::AlphaOneIndex;
	}

	void LayerGen::autoGenerate() {
		int xchunk, ychunk;

		for (xchunk = -m_terrain->getChunkIndexOffset(); xchunk < m_terrain->getChunkIndexOffset(); xchunk++) {
			for (ychunk = -m_terrain->getChunkIndexOffset(); ychunk < m_terrain->getChunkIndexOffset(); ychunk++) {
				generateBlock(Point(xchunk, ychunk));
			}
		}
	}

	void LayerGen::generateBlock(const Point& index) {
		Map::LayerDef* l = getLayerDef();
		Chunk* chunk = m_terrain->getChunk(index);
		BoundingRange chunkAltiRange = chunk->getAltitudeRange();
		BoundingRange chunkSlopeRange = chunk->getSlopeRange();

		BoundingRange altiRange(l->altitudeRange.x, l->altitudeRange.y);
		BoundingRange slopeRange(l->slopeRange.x, l->slopeRange.y);

		BoundingRange maxAltiRange = altiRange; maxAltiRange.inflate(ALTITUDE_BLUR);
		BoundingRange maxSlopeRange = slopeRange; maxSlopeRange.inflate(SLOPE_BLUR);

		AlphaBlock* block = nullptr;
		if (slopeRange.contains(chunkSlopeRange) && altiRange.contains(chunkAltiRange)) {
			block = AlphaBlock::One;
		} else if (!maxSlopeRange.intersects(chunkSlopeRange) || !maxAltiRange.intersects(chunkAltiRange)) {
			block = AlphaBlock::Zero;
		} else {
			Point pixelindex = index * Map::ChunkPixels;
			block = m_terrain->allocAlphaBlock();
			AlphaBlock::Data& data = block->lock();

			bool isZero = true; bool isOne = true;

			for (int y = 0; y < Map::ChunkPixels; y++) {
				for (int x = 0; x < Map::ChunkPixels; x++) {
					int xindex = x + pixelindex.x;
					int yindex = y + pixelindex.y;
					float h = m_terrain->getPixelHeight(xindex, yindex);
					float slope = m_terrain->getPixelSlope(xindex, yindex);

					float hblur = 1.0f - Math::saturate(altiRange.distance(h) / ALTITUDE_BLUR);
					float sblur = 1.0f - Math::saturate(slopeRange.distance(slope) / SLOPE_BLUR);
					float blur = hblur * sblur;

					int alpha = Math::clampByte(blur * 255);
					data[y][x] = alpha;
					if (alpha != 255) {
						isOne = false;
					} 
					
					if (alpha != 0) {
						isZero = false;
					}
				}
			}

			block->unlock();

			if (isZero || isOne) {
				m_terrain->freeAlphaBlock(block);
				if (isZero) {
					block = AlphaBlock::Zero;
				} else {
					block = AlphaBlock::One;
				}

			}
		}

		setBlock(index, block);
	}

	Vector2 LayerGen::getDetailScale() const {
		Map::LayerDef* l = getLayerDef();
		return l->uvScale;
	}

	float LayerGen::getAlpha(int x, int y) const {
		return getByteAlpha(x, y) / 255.0f;
	}

	bool LayerGen::isVerticalProjection() const {
		Map::LayerDef* l = getLayerDef();
		return l->isVerticalProjection;
	}


	byte_t LayerGen::getByteAlpha(int x, int y) const {
		Point chunkIndex(x, y);

		chunkIndex = (chunkIndex + m_pixelIndexOffset) / Map::ChunkPixels - m_terrain->getChunkIndexOffset();

		AlphaBlock* block = getBlock(chunkIndex);

		if (block == AlphaBlock::Zero)
			return 0;

		if (block == AlphaBlock::One)
			return 255;

		x = Math::mapToBound(x, Map::ChunkPixels);
		y = Math::mapToBound(y, Map::ChunkPixels);

		return block->getData()[y][x];
	}


	void LayerGen::setAlpha(int x, int y, float alpha) {
		setByteAlpha(x, y, Math::clampByte(alpha * 255.0f + 0.5f));
	}

	void LayerGen::setByteAlpha(int x, int y, byte_t alpha) {
		Point chunkIndex(x, y);

		chunkIndex = (chunkIndex + m_pixelIndexOffset) / Map::ChunkPixels - m_terrain->getChunkIndexOffset();

		AlphaBlock* block = getBlock(chunkIndex);

		if (block == AlphaBlock::Zero) {
			if (alpha == 0)
				return;

			block = m_terrain->allocAlphaBlock();
			block->setAll(0);
			setBlock(chunkIndex, block);
		}

		if (block == AlphaBlock::One) {
			if (alpha == 255)
				return;
			block = m_terrain->allocAlphaBlock();
			block->setAll(255);
			setBlock(chunkIndex, block);
		}

		x = Math::mapToBound(x, Map::ChunkPixels);
		y = Math::mapToBound(y, Map::ChunkPixels);

		block->lock()[y][x] = alpha;
		block->unlock();
	}

	Image* LayerGen::copyAlpha(const Rect& r) const {
		Image* result = new Image;

		result->initImage(TexFormat::A8, r.width, r.height);
		byte_t* dst = result->getWritablePointer(0);

		for (int y = r.y; y < r.yMax(); y++) {
			for (int x = r.x; x < r.xMax(); x++) {
				*dst++ = getByteAlpha(x, y);
			}
		}

		return result;
	}

	void LayerGen::writeAlpha(const Rect& r, Image* image) {
		const byte_t* dst = image->getData(0);

		for (int y = r.y; y < r.yMax(); y++) {
			for (int x = r.x; x < r.xMax(); x++) {
				setByteAlpha(x, y, *dst++);
			}
		}
	}

	//--------------------------------------------------------------------------
	// class Chunk
	//--------------------------------------------------------------------------

	static bool IsDegenerate(int a, int b, int c) {
		return a == b || b == c || c == a;
	}

	Chunk::Chunk() {
		m_terrain = nullptr;
		m_zone = nullptr;
		m_prim = nullptr;
		m_material = nullptr;
		m_lod = -1;
		m_lastLod = -1;
		m_neighborLod.i = -1;
		m_lastNeighborLod.i = -1;
	}

	Chunk::~Chunk() {
		finalize();
	}

	inline Vector4 Chunk::getChunkRect() const {
		Vector4 result(m_tilerect.x, m_tilerect.y, m_tilerect.width, m_tilerect.height);

		return result * m_terrain->getTileMeters();
	}


	void Chunk::initialize(Zone* zone, int x, int y) {
		finalize();

		m_zone = zone;
		m_terrain = m_zone->getTerrain();
		m_index.set(x, y);
		m_heightChanged = true;

		m_tilerect.x = x * Map::ChunkTiles;
		m_tilerect.y = y * Map::ChunkTiles;
		m_tilerect.width = Map::ChunkTiles;
		m_tilerect.height = Map::ChunkTiles;

		m_material = FindAsset_<Render::Material>("materials/terrain");
		AX_ASSERT(!m_material->isDefaulted());

		allocatePrimitive();
	}

	void Chunk::finalize() {
		m_lod = -1;
		m_neighborLod.i = -1;

		SafeDelete(m_prim);
		m_material.clear();
	}

	void Chunk::onHeightChanged() {
		const Rect rect = m_tilerect;

		// calculate boundingbox and slope range
		m_heightRange.clear();
		m_slopeRange.clear();

		for (int y = rect.y; y <= rect.yMax(); y++) {
			for (int x = rect.x; x <= rect.xMax(); x++) {
				float h = m_terrain->getHeight(x, y);
				m_heightRange.expandBy(h);
			}
		}
		m_bbox.min.x = m_index.x * Map::ChunkTiles * m_terrain->getTileMeters();
		m_bbox.min.y = m_index.y * Map::ChunkTiles * m_terrain->getTileMeters();
		m_bbox.min.z = m_heightRange.min;
		m_bbox.max.x = m_bbox.min.x + Map::ChunkTilesP1 * m_terrain->getTileMeters();
		m_bbox.max.y = m_bbox.min.y + Map::ChunkTilesP1 * m_terrain->getTileMeters();
		m_bbox.max.z = m_heightRange.max;

		// update slope
		for (int y = rect.y; y < rect.yMax(); y++) {
			for (int x = rect.x; x < rect.xMax(); x++) {
				float slope = m_terrain->getSlope(x, y);
				m_slopeRange.expandBy(slope);
			}
		}

		// calculate lod error
		int step = 2;
		int halfstep = step >> 1;
		int totalerr = 0;
		int count = 0;
		float err = 0;
		lodErrors[0] = 0;
		int lod = 1;

		while (step < Map::ChunkTiles) {
			float curloderr = 0;
			for (int y = rect.y; y < rect.yMax(); y+= step) {
				for (int x = rect.x; x < rect.xMax(); x += step) {
					int average = m_terrain->getHeight(x, y);
					average += m_terrain->getHeight(x+step, y);
					average += m_terrain->getHeight(x, y+step);
					average += m_terrain->getHeight(x+step, y+step);
					average >>= 2;
					float center = m_terrain->getHeight(x+halfstep, y+halfstep);
					float cur_err = Math::abs(average - center);
					totalerr += cur_err;
					count++;

					err = std::max(cur_err, err);
					curloderr = std::max(cur_err, curloderr);
				}
			}
			lodErrors[lod] = curloderr;
			lodErrors[lod] = std::max(lodErrors[lod], lodErrors[lod-1]);
			lod++;
			step <<= 1;
			halfstep <<= 1;
		}
		m_maxerror = err;
		totalerr /= count;
		m_averageError = totalerr;

		m_heightChanged = true;
	}

	void Chunk::onCalculateLOD(TerrainEvent* e) {
		if (r_lockLOD->getBool())
			return;

#if 1
		const Chunk* eyechunk = m_terrain->getPosChunk(e->camera->getOrigin());

		if (eyechunk == this) {
			AX_ASSERT(eyechunk);
		}
#endif
		float ref = r_terrainLOD->getFloat();
		ref = fabs(ref);
//		const renderCamera& camera = gRenderSystem->getActiveCamera();
		const RenderCamera& camera = *e->camera;

		Vector3 center = m_bbox.getCenter();
		float dist = m_bbox.pointDistance(camera.getOrigin()); // (center - camera.getOrigin()).getLength();
		float factor = camera.getZnear() * camera.getViewRect().height / (dist * camera.getTop());

		int i;
		for (i = Map::NumChunkLod - 1; i >= 0; i--) {
			float pixelerr = lodErrors[i] * factor;
			if (pixelerr < ref)
				break;
		}

		if (0 && m_lastLod >= 0)
			m_lod = Math::clamp(i, m_lastLod - 1, m_lastLod + 1);
		else
			m_lod = Math::clamp<sbyte_t>(i, 0, Map::NumChunkLod);

		if (dist > r_terrainDetailDist->getFloat()) {
			m_needDrawLayer = false;
			m_lod = std::max(m_lod, m_zone->m_zonePrimLod);
		} else {
			m_needDrawLayer = true;
		}

		m_prim->setLayerVisible(m_needDrawLayer);

		if (!m_needDrawLayer && m_zone->m_zonePrimLod <= m_lod) {
			if (r_terrainUseZonePrim->getBool()) {
				m_useZonePrim = true;
			} else {
				m_useZonePrim = false;
			}
		} else {
			m_useZonePrim = false;
		}
	}

	void Chunk::onUpdateLayer() {
		updateLayers();
	}


	void Chunk::onLayerPainted() {
		updateLayers();
		updateColorTexture();
	}

	void Chunk::allocatePrimitive() {
		m_prim = new RenderChunk(RenderPrim::Dynamic);
	}

	void Chunk::updatePrimitive() {
		if (!m_heightChanged && (m_lod == m_lastLod) && (m_neighborLod.i == m_lastNeighborLod.i))
			return;

		g_statistic->incValue(stat_chunkUpdated);

		m_lastLod = m_lod;
		m_lastNeighborLod.i = m_neighborLod.i;
		m_heightChanged = false;

		int num_tiles = (Map::ChunkTiles >> m_lod);
		int num_verts = (num_tiles + 1) * (num_tiles + 1);
		int num_tris = num_tiles * num_tiles * 2;

		if (num_tiles > 1) {
			num_tris = (num_tiles - 2) * (num_tiles - 2) * 2;
			for (int i = 0; i < 4; i++) {
				int n_lod = m_neighborLod.lods[i];
				int n_tiles = Map::ChunkTiles >> n_lod;
				if (n_lod <= m_lod)
					num_tris += (num_tiles - 1) * 2;
				else
					num_tris += (num_tiles - 2) + n_tiles;
			}
		}

		int c = m_prim->getNumIndexes() / 3;
		g_statistic->addValue(stat_terrainTris, num_tris - c);
		g_statistic->addValue(stat_terrainVerts, num_verts - m_prim->getNumVertexes());
		m_prim->initialize(num_verts, num_tris * 3);
		m_prim->setMaterial(m_material.get());
		m_prim->setTerrainRect(m_terrain->getTerrainRect());
		m_prim->setColorTexture(m_zone->getColorTexture());
		m_prim->setNormalTexture(m_zone->getNormalTexture());
		m_prim->setZoneRect(m_zone->getZoneRect());
		m_prim->setChunkRect(getChunkRect());

		float mpt = m_terrain->getTileMeters();
		int tilegridsize = 1 << m_lod;
		int i, j;
		Rect rect = m_tilerect;
		Point offset = m_tilerect.getMins() - rect.getMins();

		ChunkVertex* verts = m_prim->lockVertexes();
		for (j = rect.y; j <= rect.yMax(); j+=tilegridsize) {
			for (i = rect.x; i <= rect.xMax(); i+=tilegridsize) {
				// vertexes
				verts->xyz.x = (i + offset.x)* mpt;
				verts->xyz.y = (j + offset.y) * mpt;
				verts->xyz.z = m_terrain->getHeight(i, j);

				verts++;
			}
		}
		m_prim->unlockVertexes();
		AX_ASSERT(i == rect.xMax() + tilegridsize);
		AX_ASSERT(j == rect.yMax() + tilegridsize);
		AX_ASSERT((verts - m_prim->getVertexesPointer()) == m_prim->getNumVertexes());

		ushort_t* idxes = m_prim->lockIndexes();
		int mapping[Map::ChunkTilesP1][Map::ChunkTilesP1];

		for (j = 0; j <= num_tiles; j++) {
			for (i = 0; i <= num_tiles; i++) {
				mapping[i][j] = j * (num_tiles + 1) + i;
			}
		}
		for (int side = 0; side < 4; side++) {
			int n_lod = m_neighborLod.lods[side];

			if (n_lod <= m_lod)
				continue;

			int n_tiles = Map::ChunkTiles >> n_lod;
			int num_div = num_tiles / n_tiles;
			Point index;
			Point delta;

			if (side == 0) {
				index = Point(num_tiles, 0);
				delta = Point(0, 1);
			} else if (side == 1) {
				index = Point(num_tiles, num_tiles);
				delta = Point(-1, 0);
			} else if (side == 2) {
				index = Point(0, num_tiles);
				delta = Point(0, -1);
			} else {
				index = Point(0, 0);
				delta = Point(1, 0);
			}

			for (i = 0; i < n_tiles; i++) {
				for (j = 1; j < num_div; j++) {
					Point to = index + delta * j;
					mapping[to.x][to.y] = mapping[index.x][index.y];
				}
				index += delta * num_div;
			}
		}

		for (j = 0; j < num_tiles; j++) {
			for (i = 0; i < num_tiles; i++) {
				int p0 = mapping[i][j];
				int p1 = mapping[i][j+1];
				int p2 = mapping[i+1][j];
				int p3 = mapping[i+1][j+1];

				if ((i + j) & 1) {
					if (!IsDegenerate(p0, p1, p2)) {
						idxes[0] = p0;	idxes[1] = p1;	idxes[2] = p2;
						idxes += 3;
					}
					if (!IsDegenerate(p2, p1, p3)) {
						idxes[0] = p2;	idxes[1] = p1;	idxes[2] = p3;
						idxes += 3;
					}
				} else {
					if (!IsDegenerate(p1, p3, p0)) {
						idxes[0] = p1;	idxes[1] = p3;	idxes[2] = p0;
						idxes += 3;
					}
					if (!IsDegenerate(p0, p3, p2)) {
						idxes[0] = p0;	idxes[1] = p3;	idxes[2] = p2;
						idxes += 3;
					}
				}
			}
		}

		m_prim->unlockIndexes();
		AX_ASSERT((idxes - m_prim->getIndexesPointer()) == m_prim->getNumIndexes());
	}

	void Chunk::updateLayers() {
		int count = 0;
		RenderChunk::Layer detaillayers[MaxLayers];

		for (int i = 0; i < m_terrain->getNumLayer(); i++) {
			LayerGen* l = m_terrain->getLayerGen(i);

			if (!l)
				continue;

			RenderMaterial* detail = l->getDetailMat();
			if (!detail)
				continue;

			AlphaBlock* block = l->getBlock(m_index);
			if (block == AlphaBlock::Zero)
				continue;

			if (block == AlphaBlock::One) {
				detaillayers[count].alphaTex = 0;
			} else {
				detaillayers[count].alphaTex = block->getTexture();
			}
			detaillayers[count].detailMat = detail;
			detaillayers[count].scale = l->getDetailScale();
			detaillayers[count].isVerticalProjection = l->isVerticalProjection();

			count++;
		}

		int start = 0;
		int numlayers = count;
		if (count > Render::Chunk::MAX_LAYERS) {
			start = count - Render::Chunk::MAX_LAYERS;
			numlayers = Render::Chunk::MAX_LAYERS;
		}

		for (int i = 0; i < numlayers; i++ ) {
			int index = i+start;
			RenderChunk::Layer *dl = &detaillayers[index];
			m_prim->setLayers(i, dl->alphaTex.get(), dl->detailMat.get(), dl->scale, dl->isVerticalProjection );
		}

		m_prim->setNumLayers(numlayers);
	}

	void Chunk::updateColorTexture() {
		int count = 0;

		Bgr pixelbuf[Map::ChunkPixels][Map::ChunkPixels];
		memset(pixelbuf, 255, sizeof(pixelbuf));

		Point pixeloffset = (m_index + m_terrain->getChunkIndexOffset()) * Map::ChunkPixels;

		for (int i = 0; i < m_terrain->getNumLayer(); i++) {
			LayerGen* l = m_terrain->getLayerGen(i);

			if (!l)
				continue;

			AlphaBlock* block = l->getBlock(m_index);
			if (block == AlphaBlock::Zero)
				continue;

			Bgr lcolor = l->getLayerDef()->color.bgr();
			Image* image = l->getColorTemplate();

			for (int y = 0; y < Map::ChunkPixels; y++) {
				for (int x = 0; x < Map::ChunkPixels; x++) {
					Bgr color = lcolor;
					if (image) {
						const byte_t* p = image->getPixel(0, pixeloffset.x + x, pixeloffset.y + y);
						Bgr pixel(p[2], p[1], p[0]);
						color *= pixel;
					}
					if (block == AlphaBlock::One) {
						pixelbuf[y][x] = color;
					} else {
						int alpha = block->getData()[y][x];
						if (alpha == 0) {
							// do nothing
						} else if (alpha == 255) {
							pixelbuf[y][x] = color;
						} else {
							pixelbuf[y][x] = pixelbuf[y][x] * (255-alpha) + color * alpha;
						}
					}
				}
			}

			count++;
		}

		// upload to texture
		m_zone->uploadColorTexture(m_index, &pixelbuf[0][0][0]);
	}


	void Chunk::onGetViewedPrims(TerrainEvent* e) {
#if _DEBUG
		const Chunk* eyechunk = m_terrain->getPosChunk(e->camera->getOrigin());

		if (eyechunk == this) {
			AX_ASSERT(eyechunk);
		}
#endif
		const RenderCamera& camera = *e->camera;
#if 0
		m_neighborLod.lods[0] = m_terrain->getChunkLod(Point(m_index.x + 1, m_index.y));
		m_neighborLod.lods[1] = m_terrain->getChunkLod(Point(m_index.x, m_index.y + 1));
		m_neighborLod.lods[2] = m_terrain->getChunkLod(Point(m_index.x - 1, m_index.y));
		m_neighborLod.lods[3] = m_terrain->getChunkLod(Point(m_index.x, m_index.y - 1));

		if (r_frustumCull->getBool() && camera.cullBox(m_bbox))
			return;

		if (m_canUseZonePrim) {
			m_zone->addChunkToPrim(this);
		} else {
			updatePrimitive();

			// get neighborLod
			out.push_back(m_prim);
			m_primDrawFrame = m_terrain->m_updateFrame;
		}
#else
		if (!m_useZonePrim) {
			if (r_frustumCull->getBool() && camera.cullBox(m_bbox))
				return;
			e->primSeq.push_back(m_prim);
		}
#endif
	}


	void Chunk::onUpdatePrimitive(TerrainEvent* e)
	{
		const RenderCamera& camera = *e->camera;

		m_neighborLod.lods[0] = m_terrain->getChunkLod(Point(m_index.x + 1, m_index.y));
		m_neighborLod.lods[1] = m_terrain->getChunkLod(Point(m_index.x, m_index.y + 1));
		m_neighborLod.lods[2] = m_terrain->getChunkLod(Point(m_index.x - 1, m_index.y));
		m_neighborLod.lods[3] = m_terrain->getChunkLod(Point(m_index.x, m_index.y - 1));

		if (m_useZonePrim) {
			m_zone->addChunkToPrim(this);
		} else {
			updatePrimitive();
		}
	}

	void Chunk::onSelect(TerrainEvent* e) {
		if (m_useZonePrim) {
			return;
		}

		if (e->camera->cullBox(m_bbox))
			return;

		g_renderSystem->testPrimitive(m_prim);
	}

	void Chunk::doEvent(TerrainEvent* e) {
		if (!m_tilerect.intersects(e->rect))
			return;

		if (e->type == TerrainEvent::CalcLod)
			onCalculateLOD(e);
		else if (e->type == TerrainEvent::HeightChanged)
			onHeightChanged();
		else if (e->type == TerrainEvent::GetViewed)
			onGetViewedPrims(e);
		else if (e->type == TerrainEvent::UpdatePrimitive)
			onUpdatePrimitive(e);
		else if (e->type == TerrainEvent::Select)
			onSelect(e);
		else if (e->type == TerrainEvent::UpdateLayer)
			onUpdateLayer();
		else if (e->type == TerrainEvent::LayerPainted)
			onLayerPainted();

		return;
	}

	//--------------------------------------------------------------------------
	// class Zone
	//--------------------------------------------------------------------------

	Zone::Zone() : m_terrain(nullptr) {
		m_maxerror = 0;
		m_lod = -1;
		m_zonePrimLod = 2;			// 129 * 129
		m_prim = nullptr;
		m_material = nullptr;
		m_normalTexture = nullptr;
		m_colorTexture = nullptr;

		TypeZeroArray(m_chunks);
	}

	Zone::~Zone() {
		finalize();
	}

	void Zone::initialize(Terrain* terrain, int x, int y) {
		finalize();

		m_terrain = terrain;
		m_index.set(x, y);

		m_chunkIndexOffset.x = x * Map::ZoneChunks;
		m_chunkIndexOffset.y = y * Map::ZoneChunks;

		// tilerect
		m_tilerect.x = x * Map::ZoneTiles;
		m_tilerect.y = y * Map::ZoneTiles;
		m_tilerect.width = Map::ZoneTiles;
		m_tilerect.height = Map::ZoneTiles;

		// init chunks
		int halfzonechunks = Map::ZoneChunks >> 1;
		for (int i = 0; i < Map::ZoneChunks; i++) {
			y = m_chunkIndexOffset.y + i;

			for (int j = 0; j < Map::ZoneChunks; j++) {
				x = m_chunkIndexOffset.x + j;
				Chunk* chunk = TypeNew<Chunk>();
				chunk->initialize(this, x, y);
				m_chunks[i][j] = chunk;
			}
		}

		m_material = FindAsset_<Render::Material>("materials/terrain");

		// create normal texture
		m_normalTexture << dynamic_cast<Texture*>(g_assetManager->createEmptyAsset(Asset::kTexture));
		m_normalTexture->initialize(TexFormat::BGRA8, Map::ZoneTiles, Map::ZoneTiles, Texture::IF_AutoGenMipmap);
		m_normalTexture->setClampMode(Texture::CM_ClampToEdge);
		String texname;
		StringUtil::sprintf(texname, "_zone_normal_%d_%d_%d", g_renderSystem->getFrameNum(), m_index.x, m_index.y);
		g_assetManager->addAsset(Asset::kTexture, texname, m_normalTexture.get());

		// create color texture
		m_colorTexture << dynamic_cast<Texture*>(g_assetManager->createEmptyAsset(Asset::kTexture));
		m_colorTexture->initialize(TexFormat::DXT1, Map::ZonePixels, Map::ZonePixels, Texture::IF_AutoGenMipmap);
		m_colorTexture->setClampMode(Texture::CM_ClampToEdge);
		m_colorTexture->setFilterMode(Texture::FM_Bilinear);
		StringUtil::sprintf(texname, "_zone_color_%d_%d_%d", g_renderSystem->getFrameNum(), m_index.x, m_index.y);
		g_assetManager->addAsset(Asset::kTexture, texname, m_colorTexture.get());

		// init primitive
		m_prim = new RenderChunk(RenderPrim::Static);
		int primverts = (Map::ZoneTiles >> m_zonePrimLod) + 1;
		primverts *= primverts;
		int primidxes = (Map::ZoneTiles >> m_zonePrimLod);
		primidxes = primidxes * primidxes * 2 * 3;
		m_prim->initialize(primverts, primidxes);
		m_prim->setMaterial(m_material.get());
		m_prim->setTerrainRect(m_terrain->getTerrainRect());
		m_prim->setColorTexture(getColorTexture());
		m_prim->setNormalTexture(getNormalTexture());
		m_prim->setZoneRect(getZoneRect());
		m_prim->setNumLayers(0);
		m_prim->setIsZonePrim(true);
	}

	void Zone::finalize() {
		// free chunks
		for (int i = 0; i < Map::ZoneChunks; i++) {
			for (int j = 0; j < Map::ZoneChunks; j++) {
				TypeDelete(m_chunks[i][j]);
			}
		}

#if 0
		FreeAsset_(m_normalTexture);
		FreeAsset_(m_colorTexture);
		FreeAsset_(m_material);
#endif
		SafeDelete(m_prim);
	}

	void Zone::updatePrimVertexes(const Rect& rect_in) {
		Rect rect = m_tilerect & rect_in;
		int step = 1 << m_zonePrimLod;
		int mask = (step - 1);
		int stride = (Map::ZoneTiles >> m_zonePrimLod) + 1;
		Point tileoffset = m_index * Map::ZoneTiles;

		ChunkVertex* verts = m_prim->lockVertexes();
		for (int y = rect.y; y < rect.yMax() + 1; y++) {
			if ((y & mask) != 0) {
				continue;
			}
			int lineoffset = ((y - tileoffset.y) >> m_zonePrimLod) * stride;
			for (int x = rect.x; x < rect.xMax() + 1; x++) {
				if ((x & mask) != 0) {
					continue;
				}
				float h = m_terrain->getHeight(x, y);
				int offset = lineoffset + ((x-tileoffset.x)>>m_zonePrimLod);
				ChunkVertex* vert = verts + offset;
				vert->xyz.x = x * m_terrain->getTileMeters();
				vert->xyz.y = y * m_terrain->getTileMeters();
				vert->xyz.z = h;
			}
		}
		m_prim->unlockVertexes();
	}

	void Zone::updateNormalTexture(const Rect& rect_in) {
		// calculate normal texture
		Rect rect = m_tilerect & rect_in;

		if (rect.isEmpty()) {
			return;
		}

		float mpt2 = m_terrain->getTileMeters() * 4.0f;
		byte_t* buf = TypeAlloc<byte_t>(Map::ZoneTiles*Map::ZoneTiles*4);
		byte_t* pbuf = buf;

		for (int y = rect.y; y < rect.yMax(); y++) {
			for (int x = rect.x; x < rect.xMax(); x++) {
				float h1, h2, h3, h4, h5, h6, h7, h8;

				h1 = m_terrain->getHeight(x+1, y);
				h2 = m_terrain->getHeight(x+1, y+1);
				h3 = m_terrain->getHeight(x, y+1);
				h4 = m_terrain->getHeight(x-1, y+1);
				h5 = m_terrain->getHeight(x-1, y);
				h6 = m_terrain->getHeight(x-1, y-1);
				h7 = m_terrain->getHeight(x, y-1);
				h8 = m_terrain->getHeight(x+1, y+1);

				float d1 = h1 - h5;
				float d2 = h3 - h7;
				float d3 = (h2 - h6) * 0.5f;
				float d4 = (h4 - h8) * 0.5f;

				// store in dxt1
				Vector3 t,b,n;
				t.x = mpt2;
				t.y = 0;
				t.z = d1 + d3 - d4;

				b.x = 0;
				b.y = mpt2;
				b.z = d2 + d3 + d4;

				float maxdiff = std::max(abs(t.z), abs(b.z));
				float slope = atan2(maxdiff, mpt2);
				slope = abs(slope);
				slope = Math::r2d(slope);
				m_terrain->setSlope(x, y, slope);

				//				t.normalize();
				//				b.normalize();
				n = t ^ b;
				n.normalize();
				pbuf[0] = (n.z * 0.5f + 0.5f) * 255;
				pbuf[1] = (n.y * 0.5f + 0.5f) * 255;
				pbuf[2] = (n.x * 0.5f + 0.5f) * 255;
				pbuf[3] = 255;
				pbuf += 4;
			}
		}

		Rect texrect = rect;
		texrect.x -= m_tilerect.x;
		texrect.y -= m_tilerect.y;
		m_normalTexture->uploadSubTexture(texrect, buf, TexFormat::BGRA8);

		TypeFree(buf);
	}


	void Zone::onHeightChanged(TerrainEvent* e) {
		updateNormalTexture(e->rect);
		updatePrimVertexes(e->rect);

		forwardEventToChunks(e);

		// calculate bbox from chunks
		float minz = 9999, maxz = -9999;

		// get maxerror from chunks
		for (int i = 0; i < Map::ZoneChunks; i++) {
			for (int j = 0; j < Map::ZoneChunks; j++) {
				m_maxerror = std::max(m_maxerror, m_chunks[i][j]->m_maxerror);
				minz = std::min(minz, m_chunks[i][j]->m_bbox.min.z);
				maxz = std::max(maxz, m_chunks[i][j]->m_bbox.max.z);
			}
		}

		// calculate bounding box
		m_bbox.min.x = m_tilerect.x * m_terrain->getTileMeters();
		m_bbox.min.y = m_tilerect.y * m_terrain->getTileMeters();
		m_bbox.min.z = minz;

		m_bbox.max.x = m_tilerect.xMax() * m_terrain->getTileMeters();
		m_bbox.max.y = m_tilerect.yMax() * m_terrain->getTileMeters();
		m_bbox.max.z = maxz;
	}

	void Zone::onCalculateLOD(TerrainEvent* e) {
		forwardEventToChunks(e);

		m_lastLod = m_lod;
		m_lod = Map::NumChunkLod;
		for (int i = 0; i < Map::ZoneChunks; i++) {
			for (int j = 0; j < Map::ZoneChunks; j++) {
				m_lod = std::min(m_lod, m_chunks[i][j]->m_lod);
			}
		}
	}

	void Zone::onGetPrimitive(TerrainEvent* e) {
		const RenderCamera& camera = *e->camera;
		if (r_frustumCull->getBool() && camera.cullBox(m_bbox))
			return;
#if 0
		renderPrim* curprim = nullptr;

		if (m_getPrimTimes != 0) {
			sNumIndex = 0;
		} else {
			m_prim->setActivedIndexes(0);
			curprim = m_prim;
		}

		forwardEventToChunks(e);

		if (m_getPrimTimes) {
			if (sNumIndex) {
				Render::RefPrim* ref = new Render::RefPrim(renderPrim::OneFrame);
				ref->initialize(m_prim, sNumIndex);
				int* dst = ref->lockIndexes();
				memcpy(dst, sTempIndexBuf, sNumIndex * sizeof(int));
				ref->unlockIndexes();
				ref->setActivedIndexes(sNumIndex);
				e->primSeq.push_back(ref);
			}
		} else {
			if (m_prim->getActivedIndexes()) {
				e->primSeq.push_back(m_prim);
			}
		}

		m_getPrimTimes++;
#else
		forwardEventToChunks(e);

		if (m_prim->getActivedIndexes())
			e->primSeq.push_back(m_prim);
#endif
	}

	void Zone::onUpdatePrimitive(TerrainEvent* e) {
		m_prim->setActivedIndexes(0);

		forwardEventToChunks(e);
	}

	void Zone::onSelection(TerrainEvent* e) {
		if (e->camera->cullBox(m_bbox))
			return;

		if (m_prim->getActivedIndexes() > 0) {
			g_renderSystem->testPrimitive(m_prim);
		}
		forwardEventToChunks(e);
	}

	void Zone::doEvent(TerrainEvent* e) {
		if (!m_tilerect.intersects(e->rect))
			return;

		switch (e->type) {
		case TerrainEvent::HeightChanged:
			onHeightChanged(e);
			break;
		case TerrainEvent::GetViewed:
			onGetPrimitive(e);
			break;
		case TerrainEvent::UpdatePrimitive:
			onUpdatePrimitive(e);
			break;
		case TerrainEvent::CalcLod:
			onCalculateLOD(e);
			break;
		case TerrainEvent::Select:
			onSelection(e);
			break;
		case TerrainEvent::UpdateColorMapLod:
			updateColorTextureLod();
			break;
		case TerrainEvent::UpdateNormalMapLod:
			updateNormalTextureLod();
			break;
		default:
			forwardEventToChunks(e);
			break;
		}
	}
	
	inline Vector4 Zone::getZoneRect() const {
		Vector4 result(m_tilerect.x, m_tilerect.y, m_tilerect.width, m_tilerect.height);
		return result * m_terrain->getTileMeters();
	}

	void Zone::uploadColorTexture(const Point& chunk_idx, const byte_t* pixelbuf) {
		Point pixelpos = chunk_idx - m_chunkIndexOffset;
		pixelpos *= Map::ChunkPixels;

		Rect rect(pixelpos.x, pixelpos.y, Map::ChunkPixels, Map::ChunkPixels);

		m_colorTexture->uploadSubTexture(rect, pixelbuf, TexFormat::BGR8);
	}

	void Zone::addChunkToPrim(Chunk* chunk) {
		AX_ASSERT(chunk->m_zone == this);
		AX_ASSERT(chunk->m_lod >= m_zonePrimLod);

		int curindexnum;
		ushort_t* idxes;

		curindexnum = m_prim->getActivedIndexes();
		idxes = m_prim->lockIndexes() + curindexnum;

		chunk->m_lastLod = -1;	// let chunk update primitive if cann't use zone prim

		int num_tiles = (Map::ChunkTiles >> chunk->m_lod);
		int num_tris = num_tiles * num_tiles * 2;

		ushort_t* startidxes = idxes;
		static int mapping[Map::ChunkTilesP1][Map::ChunkTilesP1];

		Point chunkindex = chunk->m_index;
		Point chunkoffset = chunkindex - m_index * Map::ZoneChunks;

		int stride = (Map::ZoneTiles >> m_zonePrimLod) + 1;
		int offset = (chunkoffset.y * stride + chunkoffset.x) * (Map::ChunkTiles >> m_zonePrimLod);
		int step = 1 << (chunk->m_lod - m_zonePrimLod);

		for (int j = 0; j <= num_tiles; j++) {
			for (int i = 0; i <= num_tiles; i++) {
				mapping[i][j] = offset + i * step;
			}

			offset += stride * step;
		}

		for (int side = 0; side < 4; side++) {
			int n_lod = chunk->m_neighborLod.lods[side];

			if (n_lod <= chunk->m_lod)
				continue;

			int n_tiles = Map::ChunkTiles >> n_lod;
			int num_div = num_tiles / n_tiles;
			Point index;
			Point delta;

			if (side == 0) {
				index = Point(num_tiles, 0);
				delta = Point(0, 1);
			} else if (side == 1) {
				index = Point(num_tiles, num_tiles);
				delta = Point(-1, 0);
			} else if (side == 2) {
				index = Point(0, num_tiles);
				delta = Point(0, -1);
			} else {
				index = Point(0, 0);
				delta = Point(1, 0);
			}

			for (int i = 0; i < n_tiles; i++) {
				for (int j = 1; j < num_div; j++) {
					Point to = index + delta * j;
					mapping[to.x][to.y] = mapping[index.x][index.y];
				}
				index += delta * num_div;
			}
		}

		for (int j = 0; j < num_tiles; j++) {
			for (int i = 0; i < num_tiles; i++) {
				int p0 = mapping[i][j];
				int p1 = mapping[i][j+1];
				int p2 = mapping[i+1][j];
				int p3 = mapping[i+1][j+1];

				if ((i + j) & 1) {
					if (!IsDegenerate(p0, p1, p2)) {
						idxes[0] = p0;	idxes[1] = p1;	idxes[2] = p2;
						idxes += 3;
					}
					if (!IsDegenerate(p2, p1, p3)) {
						idxes[0] = p2;	idxes[1] = p1;	idxes[2] = p3;
						idxes += 3;
					}
				} else {
					if (!IsDegenerate(p1, p3, p0)) {
						idxes[0] = p1;	idxes[1] = p3;	idxes[2] = p0;
						idxes += 3;
					}
					if (!IsDegenerate(p0, p3, p2)) {
						idxes[0] = p0;	idxes[1] = p3;	idxes[2] = p2;
						idxes += 3;
					}
				}
			}
		}

		curindexnum += idxes - startidxes;

		m_prim->unlockIndexes();
		m_prim->setActivedIndexes(curindexnum);
	}

	void Zone::forwardEventToChunks(TerrainEvent* e) {
		for (int i = 0; i < Map::ZoneChunks; i++) {
			for (int j = 0; j < Map::ZoneChunks; j++) {
				m_chunks[i][j]->doEvent(	e);
			}
		}
	}

	void Zone::updateColorTextureLod() {
		m_colorTexture->generateMipmap();
	}

	void Zone::updateNormalTextureLod() {
		m_normalTexture->generateMipmap();
	}

	//--------------------------------------------------------------------------
	// class Terrain
	//--------------------------------------------------------------------------

	Terrain::Terrain() {
		m_heightmap = nullptr;
		m_oldHeightmap = nullptr;
		m_slopeImage = nullptr;
		m_colorImage = 0;
		m_normalTex = nullptr;
		m_materialDef = nullptr;
		m_zoneCount = 0;
		m_zones = nullptr;
		m_isHeightDirty = false;
		
		m_grassManager = new GrassManager(this);	// timlly add
		m_riverManager = new RiverManager(this);

		m_lastViewOrigin.set(9999,9999,9999);
		m_heightDirtyLastView = true;;
	}

	Terrain::~Terrain() {
		clear();
		SafeDelete(m_grassManager);
	}

	void Terrain::init(int tiles, int tilemeters){
		clear();

		m_tiles = Math::nextPowerOfTwo(tiles);
		m_tilemeters = tilemeters;

		AX_ASSERT(m_tiles >= Map::ZoneTiles*2 && m_tiles <= Map::MaxTiles);
		AX_ASSERT(m_tilemeters > 0);

		// init bbox
		m_bbox.min.x = -(m_tiles / 2 * m_tilemeters);
		m_bbox.min.y = m_bbox.min.x;
		m_bbox.min.z = -1;

		m_bbox.max = -m_bbox.min;

		m_tilerect.x = -m_tiles / 2;
		m_tilerect.y = -m_tiles / 2;
		m_tilerect.width = m_tiles;
		m_tilerect.height = m_tiles;

		m_heightBound.x = -1024;
		m_heightBound.y = 1024;

		// init height map
		m_heightmap = new Image;

		// map f to us
		float defaultHeightF = -2.0f;
		float MapedHeightScaleF = (defaultHeightF - m_heightBound.x) / (m_heightBound.y - m_heightBound.x);
		MapedHeightScaleF = Math::clamp(MapedHeightScaleF, 0.0f, 1.0f);

		float defaultMaxHeightUS = 65535.0f;
		ushort_t defaultHeight = (ushort_t)defaultMaxHeightUS * MapedHeightScaleF;

		m_heightmap->initImage(TexFormat::L16, m_tiles, m_tiles);

		m_heighData = (ushort_t*)m_heightmap->getWritablePointer();

		for (int y = 0; y < m_tiles; y++) {
			for (int x = 0; x < m_tiles; x++) {
				m_heighData[y*m_tiles+x] = defaultHeight;
			}
		}

		m_oldHeightmap = m_heightmap->readSubImage(0, Rect(0,0,m_tiles,m_tiles));

		m_slopeImage = new Image();
		m_slopeImage->initImage(TexFormat::L8, m_tiles, m_tiles);

		m_colorImage = new Image();
		byte_t white[] = {255,255,255};
		m_colorImage->initImage(TexFormat::BGR8, m_tiles, m_tiles, white);

		// init zones
		m_terrainZones = m_tiles / Map::ZoneTiles;
		m_zoneIndexOffset = m_terrainZones >> 1;
		m_chunkIndexOffset = m_zoneIndexOffset * Map::ZoneChunks;
		m_tileOffset = m_tiles >> 1;
		m_zoneCount = m_terrainZones * m_terrainZones;

		m_zones = TypeAlloc<Zone*>(m_zoneCount);

		int count = 0;
		for (int y = -m_zoneIndexOffset; y < m_zoneIndexOffset; y++) {
			for (int x = -m_zoneIndexOffset; x < m_zoneIndexOffset; x++) {
				Zone* zone = TypeNew<Zone>();
				zone->initialize(this, x, y);
				m_zones[count] = zone;
				count++;
			}
		}
		AX_ASSERT(count == m_zoneCount);

		// init materialdef
		Map::MaterialDef* matdef = new Map::MaterialDef();
		matdef->createLayerDef();
		setMaterialDef(matdef, false);

		// fire height changed event
		doHeightChanged(m_tilerect);
		generateZoneColor();
		doLayerPainted(m_tilerect * Map::TilePixels);
		doUpdateColorTextureLod(m_tilerect * Map::TilePixels);

		notify(Render::Terrain::HeightfieldSetted);
	}

	void Terrain::initFromXml(const String& map_name, const TiXmlElement* elem) {
		clear();

		m_tiles = atoi(elem->Attribute("tilesize"));
		m_tilemeters = atoi(elem->Attribute("tilemeters"));
		const char* heightfile = elem->Attribute("heightmap");
		const char* layerfile = elem->Attribute("layergen");

		const TiXmlElement* child = elem->FirstChildElement();

//		init(tilesize, tilemeter, heighfile);

		// init bbox
		m_bbox.min.x = -(m_tiles / 2 * m_tilemeters);
		m_bbox.min.y = m_bbox.min.x;
		m_bbox.min.z = -1;

		m_bbox.max = -m_bbox.min;

		m_tilerect.x = -m_tiles / 2;
		m_tilerect.y = -m_tiles / 2;
		m_tilerect.width = m_tiles;
		m_tilerect.height = m_tiles;

		m_heightBound.x = -1024;
		m_heightBound.y = 1024;

		// init height map
		m_heightmap = new Image;
		m_heightmap->initImage(TexFormat::L16, m_tiles, m_tiles);

		m_heighData = (ushort_t*)m_heightmap->getWritablePointer();

		void* buf; size_t fsize;
		fsize = g_fileSystem->readFile(heightfile, &buf);
		AX_ASSERT(fsize == m_tiles*m_tiles*2);
		memcpy(m_heighData, buf, m_tiles*m_tiles*2);
		g_fileSystem->freeFile(buf);

		m_oldHeightmap = m_heightmap->readSubImage(0, Rect(0,0,m_tiles,m_tiles));

		m_slopeImage = new Image();
		m_slopeImage->initImage(TexFormat::L8, m_tiles, m_tiles);

		m_colorImage = new Image();
		byte_t white[] = {255,255,255};
		m_colorImage->initImage(TexFormat::BGR8, m_tiles, m_tiles, white);

		// init zones
		m_terrainZones = m_tiles / Map::ZoneTiles;
		m_zoneIndexOffset = m_terrainZones >> 1;
		m_chunkIndexOffset = m_zoneIndexOffset * Map::ZoneChunks;
		m_tileOffset = m_tiles >> 1;
		m_zoneCount = m_terrainZones * m_terrainZones;

		m_zones = TypeAlloc<Zone*>(m_zoneCount);

		int count = 0;
		for (int y = -m_zoneIndexOffset; y < m_zoneIndexOffset; y++) {
			for (int x = -m_zoneIndexOffset; x < m_zoneIndexOffset; x++) {
				Zone* zone = TypeNew<Zone>();
				zone->initialize(this, x, y);
				m_zones[count] = zone;
				count++;
			}
		}
		AX_ASSERT(count == m_zoneCount);

		// fire height changed event
		doHeightChanged(m_tilerect);

		if (child && child->ValueTStr() == "materialDef") {
			Map::MaterialDef* matdef = new Map::MaterialDef();
			matdef->parseXml(child);
			setMaterialDef(matdef, false);

			if (m_numLayerGens && layerfile) {
				File* f = g_fileSystem->openFileRead(layerfile);
				int n = f->readInt();
				AX_ASSERT(m_numLayerGens == n);

				for (int i = 0; i < m_numLayerGens; i++) {
					int id = f->readInt();
					LayerGen* lg = getLayerGenById(id);
					AX_ASSERT(lg);
					lg->load(f);
				}

				f->close();
				delete f;
			}

			child = child->NextSiblingElement();	// timlly add.
		} else
		{

		}

		// timlly add
		if (child && child->ValueTStr() == "GrassInfo") 
		{
			m_grassManager->loadFile(map_name, child);
			m_grassManager->init();		// timlly add
			
			child = child->NextSiblingElement();	// timlly add.
		}
		else
		{
			m_grassManager->init();		// timlly add
		}

		if (child && child->ValueTStr() == "RiverInfo") 
		{
			m_riverManager->loadFile(map_name, child);
			m_riverManager->init();		// timlly add

			child = child->NextSiblingElement();	// timlly add.
		}

		if (loadColorTexture(map_name)) {
			doUpdateLayer(m_tilerect * Map::TilePixels);
		} else {
			doLayerPainted(m_tilerect * Map::TilePixels);
			doUpdateColorTextureLod(m_tilerect * Map::TilePixels);
		}

		notify(Render::Terrain::HeightfieldSetted);
	}

	bool Terrain::loadColorTexture(const String& map_name) {
		for (int i = 0; i < m_zoneCount; i++) {
			Zone* z = m_zones[i];
			Point zindex = z->getZoneIndex();

			String texname;
			StringUtil::sprintf(texname, "%s_%d_%d", map_name.c_str(), m_zones[i]->getZoneIndex().x, m_zones[i]->getZoneIndex().y);

			TexturePtr tex = UniqueAsset_<Texture>(texname, Texture::IF_AutoGenMipmap);

			if (!tex->isDefaulted()) {
				z->setColorTexture(tex.get());
				tex->setClampMode(Texture::CM_ClampToEdge);
			} else {
				Errorf("can't load color texture");
			}
		}
		return true;
	}


	void Terrain::writeXml(File* f, int indent) {
#define INDENT if (indent) f->printf("%s", ind.c_str());
		String ind(indent*2, ' ');

		String filename = f->getName();
		filename = PathUtil::removeExt(filename);
		String heightfile = filename + "_height.raw";
		String layerfilename = filename + "_layer.bin";

		INDENT;f->printf("<terrain\n");
		INDENT;f->printf("  tilesize=\"%d\"\n", m_tiles);
		INDENT;f->printf("  tilemeters=\"%d\"\n", m_tilemeters);
		INDENT;f->printf("  heightmap=\"%s\"\n", heightfile.c_str());
		INDENT;f->printf("  layergen=\"%s\"\n", layerfilename.c_str());
		INDENT;f->printf("  >\n");

		if (m_materialDef) {
			m_materialDef->writeToFile(f, indent+1);
		}

		// write grass info --timlly add
		m_grassManager->saveFile(filename, f, indent+1);
		m_riverManager->saveFile(filename, f, indent+1);

		INDENT;f->printf("</terrain>\n");

		m_heightmap->saveFile_raw(heightfile);
		if (m_numLayerGens) {

			File* f = g_fileSystem->openFileWrite(layerfilename);
			f->writeInt(m_numLayerGens);

			for (int i = 0; i < m_numLayerGens; i++) {
				f->writeInt(m_layerGens[i]->getLayerId());
				m_layerGens[i]->save(f);
			}

			f->close();
			delete f;
		}

		// write color texture
		for (int i = 0; i < m_zoneCount; i++) {
			Texture* tex = m_zones[i]->getColorTexture();
			if (!tex) {
				continue;
			}
			tex->generateMipmap();
			String texname;
			StringUtil::sprintf(texname, "%s_%d_%d.dds", filename.c_str(), m_zones[i]->getZoneIndex().x, m_zones[i]->getZoneIndex().y);
			tex->saveToFile(texname);
		}

#undef INDENT
	}

	void Terrain::clear() {
		SafeDelete(m_heightmap);
		SafeDelete(m_oldHeightmap);
		SafeDelete(m_slopeImage);
		SafeDelete(m_colorImage);
		SafeDelete(m_materialDef);

		for (int i = 0; i < m_zoneCount; i++) {
			TypeDelete(m_zones[i]);
		}
		TypeFree(m_zones);

		// clear layer
		LayerGenHash::iterator it = m_layerGenHash.begin();
		while (it != m_layerGenHash.end()) {
			SafeDelete(it->second);
			++it;
		}

		m_layerGenHash.clear();
		m_numLayerGens = 0;

		m_isHeightDirty = false;
	}

	void Terrain::doHeightChanged(const Rect& rect) {
		TerrainEvent e;
		e.type = TerrainEvent::HeightChanged;
		e.rect = rect;

		doEvent(&e);

		m_isHeightDirty = true;
		m_heightDirtyLastView = true;

		// 更新草的高度. timlly add
		m_grassManager->update(m_grassManager->getIndexByCoor(Vector4(rect.x, rect.y, rect.width, rect.height)));	
	}

	void Terrain::doUpdateNormalTextureLod(const Rect& tilerect) {
		TerrainEvent e;
		e.type = TerrainEvent::UpdateNormalMapLod;
		e.rect = tilerect;

		doEvent(&e);
	}

	void Terrain::doUpdateLayer(const Rect& pixelrect) {
		TerrainEvent e;
		e.type = TerrainEvent::UpdateLayer;
		e.rect = pixelrect / Map::TilePixels;

		doEvent(&e);
	}


	void Terrain::doLayerPainted(const Rect& rect) {
		TerrainEvent e;
		e.type = TerrainEvent::LayerPainted;
		e.rect = rect / Map::TilePixels;

		doEvent(&e);
	}

	void Terrain::doUpdateColorTextureLod(const Rect& pixelrect) {
		TerrainEvent e;
		e.type = TerrainEvent::UpdateColorMapLod;
		e.rect = pixelrect / Map::TilePixels;

		doEvent(&e);
	}

	void Terrain::doEvent(TerrainEvent* e) {
		for (int i = 0; i < m_zoneCount; i++) {
			m_zones[i]->doEvent(e);
		}
	}

	void Terrain::doSelect(const RenderCamera& camera) {
		TerrainEvent e;
		e.rect = m_tilerect;
		e.type = TerrainEvent::Select;
		e.camera = &camera;

		doEvent(&e);
	}

	Image* Terrain::copyHeight(const Rect& rect) const {
		Rect tr = rect;
		tr.offset(m_tileOffset, m_tileOffset);

		return m_heightmap->readSubImage(0, tr);
	}

	Image* Terrain::copyOldHeight(const Rect& rect) const {
		Rect tr = rect;
		tr.offset(m_tileOffset, m_tileOffset);

		return m_oldHeightmap->readSubImage(0, tr);
	}

	void Terrain::writeHeight(const Rect& rect, Image* image) {
		Rect tr = rect;
		tr.offset(m_tileOffset, m_tileOffset);

		m_heightmap->writeSubImage(image, Rect(0,0,tr.width,tr.height), Point(tr.x,tr.y));
	}

	void Terrain::writeOldHeight(const Rect& rect, Image* image) {
		Rect tr = rect;
		tr.offset(m_tileOffset, m_tileOffset);

		m_oldHeightmap->writeSubImage(image, Rect(0,0,tr.width,tr.height), Point(tr.x,tr.y));
	}

	Image* Terrain::copyFloatHeight(int size) const {
		AX_ASSERT(size > 0);

		Image* result = new Image();
		result->initImage(TexFormat::R32F, size, size);
		float* pdata = (float*)result->getWritablePointer(0);

		float scale = (float)m_tiles / size;
		for (int y = 0; y < size; y++) {
			for (int x = 0; x < size; x++) {
				int tilex = x * scale;
				int tiley = y * scale;
				
				*pdata = getHeight(tilex, tiley, true);
				pdata++;
			}
		}

		return result;
	}

	RenderPrims Terrain::getPrimsByCircle(float x, float y, float radius) {
		RenderPrims result;

		float scale = 1.0f / (m_tilemeters * Map::ChunkTiles);
		int minx = floorf((x - radius) * scale);
		int miny = floorf((y - radius) * scale);
		int maxx = floorf((x + radius + 1) * scale);
		int maxy = floorf((y + radius + 1) * scale);

		for (int y = miny; y <= maxy; y++) {
			for (int x = minx; x <= maxx; x++) {
				const Chunk* chunk = getChunk(Point(x,y));
				if (!chunk)
					continue;
				if (chunk->m_useZonePrim) {
					const Zone* zone = chunk->m_zone;
					if (!zone->m_prim->getActivedIndexes()) {
						continue;
					}
					if (std::find(result.begin(), result.end(), zone->m_prim) != result.end()) {
						continue;
					}
					result.push_back(zone->m_prim);
					continue;
				}
				result.push_back(chunk->m_prim);
			}
		}

		std::unique(result.begin(), result.end());
		return result;
	}

	void Terrain::setMaterialDef(Map::MaterialDef* matdef, bool undoable) {
		if (0 && undoable) {
			if (m_materialDef == matdef) {
				Errorf("%s: error param", __func__);
			}

//			TerrainMaterialDefHis* his = new TerrainMaterialDefHis(m_materialDef, matdef->clone(), this);
//			gEditorContext->addHistory(his);
			m_materialDef = matdef;
		} else {
			if (m_materialDef != matdef) {
				SafeDelete(m_materialDef);
				m_materialDef = matdef;
			}
		}

		int i;
		for (i = 0; i < m_materialDef->getNumLayers(); i++) {
			Map::LayerDef* l = m_materialDef->getLayerDef(i);
			LayerGen* lg = m_layerGenHash[l->id];

			if (!lg) {
				lg = new LayerGen(this, l->id);
				m_layerGenHash[l->id] = lg;
			} else {
				lg->update();
			}

			m_layerGens[i] = lg;
		}

		m_numLayerGens = i;

#if 0
		if (gEditorContext)
			gEditorContext->notify(Context::TerrainMaterialEdited);
#endif

#if 0
		doLayerPainted(m_tilerect * Map::TilePixels);
		doUpdateColorTextureLod(m_tilerect * Map::TilePixels);
#else
		doUpdateLayer(m_tilerect * Map::TilePixels);
#endif
	}

#if 0
	void Terrain::setMaterialDefNoHistory(Map::MaterialDef* matdef) {
		if (m_materialDef == matdef) {
			Errorf("%s: error param", __func__);
		} else {
			SafeDelete(m_materialDef);
			m_materialDef = matdef;
		}

		gEditorContext->notify(Context::TerrainMaterialEdited);
	}
#endif

	Image* Terrain::getSlopeImage() const {
		return m_slopeImage;
	}

	Image* Terrain::copySlopeImage(int size) const {
		return m_slopeImage->resize(size, size);
	}

	void Terrain::generateZoneColor(bool doprogress) {
		// TODO: free old autogen

		if (!m_materialDef)
			return;

		// fire event
		g_system->beginProgress("Generating zone color...");
		doLayerPainted(m_tilerect * Map::TilePixels);
		doUpdateColorTextureLod(m_tilerect * Map::TilePixels);
		g_system->endProgress();
	}

	void Terrain::generateLayerAlpha(bool doprogress, int id) {
		if (id < 0) {
			g_system->beginProgress("Generating layer alpha...");
			m_numLayerGens = m_materialDef->getNumLayers();

			for (int i = 0; i < m_materialDef->getNumLayers(); i++) {
				String msg;
				StringUtil::sprintf(msg, "generating %d layer...", i);
				g_system->showProgress(i*100 / m_materialDef->getNumLayers(), msg);
				if (m_layerGens[i]->getLayerDef()->isAutoGen) {
					m_layerGens[i]->autoGenerate();
				}
			}

			g_system->endProgress();
			return;
		}

		LayerGen* lg = getLayerGenById(id);
		if (!lg) {
			return;
		}

		if (!lg->getLayerDef()->isAutoGen) {
			return;
		}

		lg->autoGenerate();
	}

	void Terrain::doUpdate(Render::QueuedScene* qscene) {
		m_grassManager->update(qscene);

		const Vector3& org = qscene->camera.getOrigin();
		if ((org - m_lastViewOrigin).getLength() < 32 && !m_heightDirtyLastView) {
			return;
		}

		TerrainEvent e;
		e.rect = m_tilerect;
		e.camera = &qscene->camera;

		e.type = TerrainEvent::CalcLod;
		doEvent(&e);

		e.type = TerrainEvent::UpdatePrimitive;
		doEvent(&e);

		m_lastViewOrigin = org;
		m_heightDirtyLastView = false;
	}

	RenderPrims Terrain::getAllPrimitives() {
		RenderPrims result;

		return result;
	}

#if 0
	RenderPrims Terrain::getViewedPrimitives() {
		ulonglong_t start = OsUtil::microseconds();
		TerrainEvent e;
		e.rect = m_tilerect;

		e.type = TerrainEvent::GetViewed;
		doEvent(&e);

		ulonglong_t end = OsUtil::microseconds();
		gStatistic->addValue(stat_terrainGenPrimsTime, end - start);

		// timlly add
		if (m_grassManager && r_grass->getBool())
		{
			//m_grassMgr->update();
			m_grassManager->uploadRenderData(&(e.primSeq));
		}

		if (m_riverManager/* && r_river->getBool()*/)
		{
			//m_riverMgr->update();
			m_riverManager->uploadRenderData(&(e.primSeq));
		}

		return e.primSeq;
	}
#endif

	void Terrain::issueToQueue( Render::QueuedScene* qscene )
	{
		ulonglong_t start = OsUtil::microseconds();
		TerrainEvent e;
		e.rect = m_tilerect;
		e.camera = &qscene->camera;
		e.type = TerrainEvent::GetViewed;

		doEvent(&e);

		ulonglong_t end = OsUtil::microseconds();
		g_statistic->addValue(stat_terrainGenPrimsTime, end - start);

		// timlly add
		if (m_grassManager && r_grass->getBool())
		{
			//m_grassMgr->update();
			m_grassManager->uploadRenderData(&(e.primSeq));
		}

		if (m_riverManager/* && r_river->getBool()*/)
		{
			//m_riverMgr->update();
			m_riverManager->uploadRenderData(&(e.primSeq));
		}

		for (size_t i = 0; i < e.primSeq.size(); i++) {
			qscene->addInteraction(0, e.primSeq[i]);
		}
	}

#if 0
	RenderPrims Terrain::getLightedPrimitives(Render::QueuedLight* light) {
		RenderPrims result;

		return result;
	}
#endif

	void Terrain::getHeightinfo(ushort_t*& datap, int& size, float& tilemeters) {
		datap = m_heighData;
		size = m_tiles;
		tilemeters = m_tilemeters;
	}



}} // namespace Axon::Map


