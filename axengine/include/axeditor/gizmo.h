/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/

#ifndef AX_MANIPULAR_H
#define AX_MANIPULAR_H

namespace Axon { namespace Editor {

	class Gizmo {
	public:
		typedef Render::Camera Camera;

		virtual ~Gizmo() = 0 {}
		virtual void doRender() = 0;
		virtual int doSelect(View* view, int x, int y) = 0; // return -1 if not intersected
		virtual void setHighlight(int) = 0;
	};

	class TransformGizmo : public Gizmo {
	public:
		virtual ~TransformGizmo() = 0 {}
		virtual void setup(const Camera& camera, const Vector3& pos, const Matrix3& axis, float scale) = 0;
	};

	//--------------------------------------------------------------------------
	// class MoveGizmo, move gizmo
	//--------------------------------------------------------------------------

	class MoveGizmo : public TransformGizmo {
	public:
		enum SelectId {
			None = -1, X = 0, Y, Z, XY, YZ, XZ, XYZ, NumberId
		};

		MoveGizmo();
		virtual ~MoveGizmo();

		void setup(const Camera& camera, const Vector3& pos, const Matrix3& axis, float scale);
		void doRender();
		int doSelect(View* view, int x, int y); // return -1 if not intersected
		int doSelect(Render::Camera* camera, int x, int y,int selectedSize); // return -1 if not intersected
		void setHighlight(int);
		int getHighlight() { return m_highlit; }

	protected:
		void clear();
		void setupAxis(int axis);
		void setupPlane(int axis);
		void setupXYZ(const Camera& camera);
		Rgba getColor(int axis, Rgba c);

	protected:
		Render::Line* m_lines[NumberId];
		Render::Mesh* m_meshs[NumberId];
		SelectId m_highlit;
		MaterialPtr m_material;
		
		Vector3 m_pos;
		Matrix3 m_axis;
		float m_scale;
		float m_length;
	};

	//--------------------------------------------------------------------------
	// class RotateGizmo, rotate gizmo
	//--------------------------------------------------------------------------

	class RotateGizmo : public TransformGizmo {
	public:
		enum SelectId {
			None = -1, X = 0, Y, Z, Screen
		};

		RotateGizmo();
		~RotateGizmo();

		void setup(const Camera& camera, const Vector3& pos, const Matrix3& axis, float scale);
		void doRender();
		int doSelect(View* view, int x, int y);		
		void setHighlight(int id);
		void setCrank(float start, float end);
		void disableCrank();

	protected:
		void setupCrank(const Camera& camera);
		Rgba getColor(int id);
		Rgba getCenterColor(int id);

	private:
		enum { CirculSubdivided = 64, CrankSubdivided=360 };
		Render::Line* m_centerLine;
		Render::Line* m_circles[3];
		Render::Line* m_innerBound;
		Render::Line* m_outerBound;
		Render::Mesh* m_crank;
		SelectId m_highlit;
		MaterialPtr m_material;

		Vector3 m_pos;
		Matrix3 m_axis;
		float m_scale;
		float m_length;
		bool m_enabledCrank;
		float m_crankStart;
		float m_crankEnd;
	};

	//--------------------------------------------------------------------------
	// class ScaleGizmo, rotate gizmo
	//--------------------------------------------------------------------------
	class ScaleGizmo : public TransformGizmo {
	public:
		enum SelectId {
			None = -1, X = 0, Y, Z, XYZ, NumberId // ���ε�һ������
		};

		ScaleGizmo();
		~ScaleGizmo();

		void setup(const Camera& camera, const Vector3& pos, const Matrix3& axis, float scale);
		void doRender();
		int doSelect(View* view, int x, int y);		
		void setHighlight(int id);

	protected:
		Rgba getColor(int id);
		void setupScreenQuad(const Camera& camera, RenderMesh*& mesh, const Vector3& pos, Rgba color);

	private:
		Render::Line* m_lines[NumberId];
		Render::Mesh* m_meshs[NumberId];
		SelectId m_highlit;
		MaterialPtr m_material;

		Vector3 m_pos;
		Matrix3 m_axis;
		float m_scale;
		float m_length;
	};

}} // namespace Axon::Editor

#endif // AX_MANIPULAR_H
