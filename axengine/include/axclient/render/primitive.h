/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/

#ifndef AX_RENDER_PRIMITIVE_H
#define AX_RENDER_PRIMITIVE_H

namespace Axon { namespace Render {

	//--------------------------------------------------------------------------
	// class Primitive
	//--------------------------------------------------------------------------

	// virtual base, can't create an instance for this
	class AX_API Primitive {
	public:
		enum Type {
			NoneType = 0,		// for error checks
			PointType,
			LineType,
			MeshType,			// raw mesh
			TextType,
			GuiType,
			ChunkType,
			GroupType,			// grouped primitive, maybe for gui system
			ReferenceType,		// a reference to another primitive, but can use different entity and material etc...
			InstancingType // geometry instancing type, instancing a primitive many time with different parameter
		};

		enum Hint {
			Static,				// will alloc in video memory, never change
			Stream,				// provide once, draw once
			Dynamic,			// 
			OneFrame,			// only draw in one frame, will auto deleted when render driver cached it
			OnStack // allocated on stack, so not need be free. just use for GeoInstancing, otherwise will maybe cause memory/resource leak
		};

		Primitive(Hint hint);
		virtual ~Primitive() = 0;

		inline Hint getHint() const { return m_hint; }
		inline int getCachedId() const { return m_cachedId; }
		inline void setCachedId(int cached_id) { m_cachedId = cached_id; }
		inline int getCachedFrame() const { return m_cachedFrame; }
		inline void setCachedFrame(int frame) { m_cachedFrame = frame; }
		inline Type getType() const { return m_type; }

		inline bool isDirty() const { return m_isDirtied; }
		inline bool isVertexBufferDirty() const { return m_isVertexBufferDirtied; }
		inline bool isIndexBufferDirty() const { return m_isIndexBufferDirtied; }
		inline void clearDirty() { m_isDirtied = m_isVertexBufferDirtied = m_isIndexBufferDirtied = false; }

		inline void setMatrix(const Matrix4& matrix) { m_isMatrixSet = true; m_matrix = matrix; }
		inline bool isMatrixSet() const { return m_isMatrixSet; }
		inline Matrix4 getMatrix() const { return m_matrix; }
		inline void disableMatrix() { m_isMatrixSet = false; }

		inline Material* getMaterial() const;
		inline void setMaterial(Material* material);
		inline void setLightMap(Texture* lm);
		inline Texture* getLightMap() const;

		inline int getActivedIndexes() const { return m_activedIndexes; }
		inline void setActivedIndexes(int val) { m_activedIndexes = val; }

		void interactionChain(Interaction* last, int chainId);
		Interaction* getHeadInteraction() const { return m_headInteraction; }

	protected:
		int m_cachedId;		// used by render driver
		int m_cachedFrame;
		const Hint m_hint;
		bool m_isDirtied;		// dirtied
		bool m_isVertexBufferDirtied;
		bool m_isIndexBufferDirtied;
		Type m_type;
		MaterialPtr m_material;
		TexturePtr m_lightMap;

		bool m_isMatrixSet;
		Matrix4 m_matrix;

		int m_activedIndexes;

		int m_chainId;
		Interaction* m_headInteraction;
	};

	inline void Primitive::interactionChain(Interaction* last, int chainId) {
		if (m_chainId != chainId) {
			m_headInteraction = last;
			m_chainId = chainId;
			return;
		}

		last->primNext = m_headInteraction;
		m_headInteraction = last;
	}

	inline Material* Primitive::getMaterial() const {
		return m_material.get();
	}

	inline void Primitive::setMaterial(Material* material) {
		m_material = material;
	}

	inline void Primitive::setLightMap(Texture* lm) {
		m_lightMap = lm;
	}

	inline Texture* Primitive::getLightMap() const {
		return m_lightMap.get();
	}


	typedef Sequence<Primitive*>	Primitives;

	//--------------------------------------------------------------------------
	// class PointPrim
	//--------------------------------------------------------------------------

	class AX_API PointPrim : public Primitive {
	public:
		typedef DebugVertex VertexType;

		PointPrim(Hint hint);
		virtual ~PointPrim();

		void initialize(int num_points);

		int getNumPoints() const;
		const DebugVertex* getPointsPointer() const;
		DebugVertex* lock();
		void unlock();

		// pointCount == -1, from offset to end
		void setDrawOffsetCount(int pointOffset = 0, int pointCount = -1);
		int getDrawOffset();
		int getDrawCount();

		void setPointSize(float point_size);
		float getPointSize() const;

		// static helper function
	private:
		int m_numPoints;
		DebugVertex* m_points;
		float m_pointSize;

		int m_pointDrawOffset;
		int m_pointDrawCount;
	};

	//--------------------------------------------------------------------------
	// class Line
	//--------------------------------------------------------------------------

	class AX_API Line : public Primitive {
	public:
		typedef DebugVertex VertexType;

		Line(Hint hint);
		virtual ~Line();

		void initialize(int numverts, int numidxes);
		void finalize();

		int getNumVertexes() const;
		const DebugVertex* getVertexesPointer() const;
		DebugVertex* lockVertexes();
		void unlockVertexes();

		int getNumIndexes() const;
		const ushort_t* getIndexPointer() const;
		ushort_t* lockIndexes();
		void unlockIndexes();

		// helper
		ushort_t getIndex(int order) const;
		const VertexType& getVertex(int order) const;

		void lock();
		void setIndex(int order, int index);
		void setVertex(int order, const VertexType& vert);
		VertexType& getVertexRef(int order);
		void unlock();

		void setLineWidth(float line_width);
		float getLineWidth() const;

		// helper static create
		static Line* createAxis(Hint hint, float line_length);
		static Line* createAxis(Hint hint, const Vector3& origin, const Matrix3& axis, float line_length);
		static Line* createScreenRect(Hint hint, const Rect& rect, const Rgba& color);
		static Line* createLine(Hint hint, const Vector3& from, const Vector3& to, const Rgba& color);
		static Line* createWorldBoundingBox(Hint hint, const BoundingBox& bbox, const Rgba& color);

		static bool setupLine(Line*& line, const Vector3& from, const Vector3& to, const Rgba& color);
		static bool setupAxis(Line*& line, const Vector3& origin, const Matrix3& axis, float length, Rgba xcolor, Rgba ycolor, Rgba zcolor);
		static bool setupBoundingBox(Line*& line, const Vector3& origin, const Matrix3& axis, const BoundingBox& inbbox, float scale=1.0f, Hint hint = Primitive::Dynamic);
		static bool setupCircle(Line*& line, const Vector3& origin, const Vector3& p0, const Vector3& p1, const Rgba& color, int subdivided);
		static bool setupCircle(Line*& line, const Vector3& origin, const Vector3& p0, const Vector3& p1, const Rgba& color, int subdivided ,Hint hint);
		static bool setupCircle(Line*& line, const Vector3& origin, const Vector3& p0, const Vector3& p1, const Rgba& color, int subdivided, const Plane& plane);
		static bool setupCircle(Line*& line, const Vector3& origin, const Vector3& p0, const Vector3& p1, const Rgba& color, int subdivided, bool clipplane, const Plane& plane,Hint hint = Primitive::Dynamic);
		static bool setupScreenRect(Line*& line, const Rect& rect, const Rgba& color);

	private:
		int m_numVertexes;
		VertexType* m_vertexes;
		int m_numIndexes;
		ushort_t* m_indexes;
		float m_lineWidth;
	};

	//--------------------------------------------------------------------------
	// class Mesh 
	//--------------------------------------------------------------------------

	class AX_API Mesh : public Primitive {
	public:
		typedef Vertex VertexType;

		Mesh(Hint hint);
		virtual ~Mesh();

		void initialize(int numverts, int numidxes);
		void finalize();

		int getNumVertexes() const;
		const Vertex* getVertexesPointer() const;
		Vertex* lockVertexes();
		void unlockVertexes();

		const ushort_t* getIndexPointer() const;
		int getNumIndexes() const;
		ushort_t* lockIndexes();
		void unlockIndexes();

		Line* getTangentLine(float len) const;
		Line* getNormalLine(float len) const;

		void computeTangentSpace();
		void computeTangentSpaceSlow();

		bool getIsStriped() const { return m_isStriped; }
		void setIsStriped(bool val) { m_isStriped = val; }

		// static helper function
		static Mesh* createScreenQuad(Hint hint, const Rect& rect, const Rgba& color, Material* material=nullptr, const Vector4& st = Vector4(0,0,1,1));
		static Mesh* createQuad(Hint hint, const Vector3& p0, const Vector3& p1);
		static Mesh* createAABB(Hint hint, const BoundingBox& box);
		static Mesh* createSphere(Hint hint, const Vector3& origin, float radius, int subdivided);
		static Mesh* createHalfSphere(Hint hint, const Vector3& origin, float radius, int subdivided, int side);
		static Mesh* createCylinder(Hint hint, const Vector3& p0, const Vector3& p1, float radius);

		static bool setupFan(Mesh*& mesh, const Vector3& center, const Vector3& v0, const Vector3& v1, float start, float end, Rgba color, int subdivided, Material* material = nullptr);
		static bool setupQuad(Mesh*& mesh, const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, const Rgba& color, Material* material = nullptr);
		static bool setupPolygon(Mesh*& mesh, int numverts, const Vector3* verts, Rgba color, Material* material = nullptr);
		static bool setupCone(Mesh*& mesh, const Vector3& center, float radius, const Vector3& top, const Rgba& color, int subdivided = 8);
		static bool setupBox(Mesh*& mesh, const BoundingBox& bbox, const Rgba& color);
		static bool 		setupScreenQuad(Mesh*& mesh, const Rect& rect, const Rgba& color, Material* material=nullptr, const Vector4& st = Vector4(0,0,1,1));
		static bool setupHexahedron(Mesh*& mesh, Vector3 vertes[8]);

	private:
		int m_numVertexes;
		Vertex* m_vertexes;
		int m_numIndexes;
		ushort_t* m_indexes;
		bool m_isStriped;
	};

	//--------------------------------------------------------------------------
	// class Text
	//--------------------------------------------------------------------------

	class AX_API Text : public Primitive {
	public:
		enum Format {
			// style flags
			Bold = 1,
			Underline = 2,
			Italic = 4,
			DropShadow = 8,
			Blink = 0x10,
			ScaleByHorizon = 0x20,		// stretch to full target rect, so don't need align
			ScaleByVertical = 0x40,
		};

		// horizon align
		enum HorizonAlign {
			Center, Left, Right
		};

		// vertical align
		enum VerticalAlign {
			VCenter, Top, Bottom
		};

		Text(Hint hint);
		virtual ~Text();

		void initialize(const Rect& rect, Rgba color, float aspect, int format, Font* font, const String& text);
		void initializeSimple(const Vector3& xyz, Rgba color, const String& text, bool fixedWidth = true);
		void finalize();

		const Rect& getRect() const { return m_rect; }
		const Vector3& getPosition() const { return m_position; }
		const bool getIsSimpleText() const { return m_isSimpleText; }
		const Rgba getColor() const { return m_color; }
		const float getAspect() const { return m_aspect; }
		const int getFormat() const { return m_format; }
		const String& getText() const { return m_text; }
		Font* getFont() const { return m_font; }
		HorizonAlign getHorizonAlign() const { return m_horizonAlign; }
		VerticalAlign getVerticalAlign() const { return m_verticalAlign; }
		void setHorizonAlign(HorizonAlign align) { m_horizonAlign = align; }
		void setVerticalAlign(VerticalAlign align) { m_verticalAlign = align; }

		// static helper function
		static Text* createSimpleText(Hint hint, const Vector3& xyz, const Rgba& color, const String& text, bool fixedWidth = true);
		static Text* createText(Hint hint, const Rect& rect, Font* font, const String& text, const Rgba& color=Rgba::White, HorizonAlign halign=Center, VerticalAlign valign=VCenter, int format=0, float aspect = 1.0f);

	private:
		Rect m_rect;				// draw on this rect
		Vector3 m_position;			// for simple text
		bool m_isSimpleText;
		Rgba m_color;			// text color
		float m_aspect;
		int m_format;			// format flags
		Font* m_font;				// font used
		String m_text;				// string to draw
		HorizonAlign m_horizonAlign;
		VerticalAlign m_verticalAlign;
	};

	//--------------------------------------------------------------------------
	// class Chunk
	//--------------------------------------------------------------------------

	class AX_API Chunk : public Primitive {
	public:
		enum {
			MAX_LAYERS = 4
		};
		typedef ChunkVertex VertexType;

		struct Layer {
			TexturePtr alphaTex;
			MaterialPtr detailMat;
			Vector2 scale;
			bool isVerticalProjection;
		};

		Chunk(Hint hint);
		virtual ~Chunk();

		void initialize(int numverts, int numindexes);
		void finalize();

		int getNumVertexes() const;
		const ChunkVertex* getVertexesPointer() const;
		ChunkVertex* lockVertexes();
		void unlockVertexes();

		int getNumIndexes() const;
		const ushort_t* getIndexesPointer() const;
		ushort_t* lockIndexes();
		void unlockIndexes();

		// terrain properties
		void setTerrainRect(const Vector4& rect);
		Vector4 getTerrainRect() const;

		// zone properties
		void setZoneRect(const Vector4& rect);
		Vector4 getZoneRect() const;
		void setColorTexture(Texture* color_texture);
		Texture* getColorTexture();
		void setNormalTexture(Texture* normal);
		Texture* getNormalTexture();

		// chunk properties
		void setChunkRect(const Vector4& rect);
		Vector4 getChunkRect() const;
		void setNumLayers(int n);
		int getNumLayers() const;
		void setLayers(int index, Texture* alpha, Material* detail, const Vector2& scale, bool isVerticalProjection = false);
		Texture* getLayerAlpha(int index) const;
		Material* getLayerDetail(int index) const;
		Vector2 getLayerScale(int index) const;
		void setLayerVisible(bool visible) { m_layerVisible = visible; m_isDirtied = true; }
		bool getLayerVisible() const { return m_layerVisible; }
		bool isLayerVerticalProjection(int index) const;

		bool isZonePrim() const { return m_isZonePrim; }
		void setIsZonePrim(bool val) { m_isZonePrim = val; }

	private:
		int m_numVertexes;
		ChunkVertex* m_vertexes;
		int m_numIndexes;
		ushort_t* m_indexes;

		Vector4 m_terrainRect;

		Vector4 m_zoneRect;
		TexturePtr m_colorTexture;
		TexturePtr m_normalTexture;

		Vector4 m_chunkRect;
		int m_numLayers;
		Layer m_layers[MAX_LAYERS];
		bool m_layerVisible;
		bool m_isZonePrim;
	};

	inline void Chunk::setZoneRect(const Vector4& rect) {
		m_zoneRect = rect;
	}
	inline Vector4 Chunk::getZoneRect() const {
		return m_zoneRect;
	}

	//--------------------------------------------------------------------------
	// class GroupPrim
	//
	// this prim will not free grouped primitives
	//--------------------------------------------------------------------------

	class AX_API GroupPrim : public Primitive {
	public:
		GroupPrim(Hint hint);
		virtual ~GroupPrim();

		void addPrimitive(Primitive* prim, bool needfree = true);
		int getPrimitiveCount() const;
		Primitive* getPrimitive(int index);
		void clear();

	private:
		typedef Sequence<bool> BoolSeq;
		Primitives m_primitives;
		BoolSeq m_needFrees;
	};

	//--------------------------------------------------------------------------
	// class RefPrim
	//
	// this prim will not free refered primitive
	//--------------------------------------------------------------------------

	class AX_API RefPrim : public Primitive {
	public:
		RefPrim(Hint hint);
		virtual ~RefPrim();

		Primitive* getRefered() const;
		void setRefered(Primitive* refered);

		void initialize(Primitive* refered, int numindexes = 0);

		int getNumIndexes() const;
		const ushort_t* getIndexesPointer() const;
		ushort_t* lockIndexes();
		void unlockIndexes();

	private:
		Primitive* m_refered;

		// override refered primitive's indexes
		int m_numIndexes;
		ushort_t* m_indexes;
	};

	//--------------------------------------------------------------------------
	// class GeoInstancing
	//
	// geometry instance primitive. this primitive will not free instanced primitive
	//--------------------------------------------------------------------------

	class AX_API GeoInstance : public Primitive {
	public:
		// per instance parameter
		struct Param {
			AffineMat worldMatrix;
			Vector4 userDefined;		// user defined param
		};

		typedef Sequence<Param>	ParamSeq;

		GeoInstance(Hint hint);
		virtual ~GeoInstance();

		Primitive* getInstanced() const;
		void setInstanced(Primitive* instanced);

		void clearAllInstances();
		void addInstance(const Param& param);
		void addInstance(const AffineMat& mtx, const Vector4& user);
		void setInstances(const ParamSeq& params);
		int getNumInstance() const;
		const Param& getInstance(int index) const;
		const ParamSeq& getAllInstances() const;

	private:
		Primitive* m_instanced;
		ParamSeq m_params;
	};


	//--------------------------------------------------------------------------
	// class PrimitiveManager
	//--------------------------------------------------------------------------

	class AX_API PrimitiveManager {
	public:
		PrimitiveManager();
		virtual ~PrimitiveManager();

		void hintUncache(Primitive* prim);

	protected:
		bool isStatic(int id) { return(size_t(id) & FRAME_FLAG) == 0; }
		bool isFrameHandle(int id) { return !isStatic(id); }

	protected:
		enum {
			FRAME_FLAG = 0x80000000,
			INDEX_MASK = ~FRAME_FLAG,
		};
		List<int>			m_waitUncache;
	};

}} // namespace Axon::Render

#endif // AX_RENDER_PRIMITIVE_H
