/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/

#ifndef AX_CLIENT_RENDER_SELECTION_H
#define AX_CLIENT_RENDER_SELECTION_H

AX_BEGIN_NAMESPACE

struct SelectionVertex;
struct Record;

// ��ά�������ϵ�еĲü�, ��Ҫ�������κ��߶εĲü�.
// ������ø��������, �����<<�����ͼ��ѧ(������)>>��7.11��ά�ü��㷨һ��.

// �ü�����: ģ������->��������->������->��һ��ͶӰ����->�ü�->���ؽ��
// ��Ҫע�����,����Z�ü�,����������������ZֵͶӰ����һ�������,
// ��õ�����ȷ�Ľ��.���Զ�Z�ü�Ҫ��������������жϲ��ü�,�ü�����ת���ɹ�һ��ͶӰ����.

class Selection
{

public:
	Selection(void);
	~Selection(void);


	void beginSelect(const RenderCamera& view);
	void loadSelectId(int id);
	void testEntity(RenderEntity* re);
	void testPrimitive(Primitive* prim);
	void testPrimitive(Primitive* prim, const AffineMat& matrix);
	HitRecords endSelect();

	bool isSelectMode()	{return m_isSelectMode;}

	// ��ָ���Ķ���ת����������
	void translateToEyeCoor(const Vector3& inVertex, Vector3& outVertex);
	// ��ָ���Ķ���ת���ɹ�һ��ͶӰ����
	static void translateToProCoor(SelectionVertex& vertex);

	// ��ʼ��������λ��־λ
	static void initVertexFlags(SelectionVertex& vertex);


private:

	void testLine(const LinePrim* line);
	void testMesh(const MeshPrim* mesh);
	void testChunk(const ChunkPrim* chunk);

	void aliasClipTriangle(const SelectionVertex& vertex0,
							const SelectionVertex& vertex1, const SelectionVertex& vertex2);
	void aliasClipLine(const SelectionVertex& vertexStart, const SelectionVertex& vertexEnd);

	int aliasClip(SelectionVertex *in, SelectionVertex *out, int flag, int count,
							void (*clip) (SelectionVertex *pfv0, SelectionVertex *pfv1, SelectionVertex *out));
	
	void addSelectionRecord(const HitRecord &record);


private:

	static RenderCamera m_selectionCamera;
	Matrix4 m_selectModelViewMatrix;

	int m_selectTime;

	bool m_isSelectMode;
	HitRecords m_selectRecSeq;

	int m_currentTestId;	// ��ǰ����id
	int m_idPos;			// �洢�ü������λ��
	bool m_isActor;
};

AX_END_NAMESPACE

#endif