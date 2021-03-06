/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/

#ifndef AX_EDITOR_VIEW_H
#define AX_EDITOR_VIEW_H

namespace Axon { namespace Editor {

	// editor view container
	struct IFrame {
		virtual Render::Target* getRenderTarget() = 0;
		virtual Rect getRect() = 0;
		virtual void setCursor(CursorType cursor_type) = 0;
		virtual void resetCursor() = 0;
		virtual void setCursorPos(const Point& pos) = 0;
		virtual void setNeedUpdate() = 0;
		virtual void setAutoUpdate(bool update) = 0;
		virtual void registerEventSource() = 0;
		virtual void removeEventSource() = 0;
	};

	//------------------------------------------------------------------------------
	// class View
	//------------------------------------------------------------------------------

	class AX_API View : public Input::IEventHandler {
	public:
		enum {
			MaxView = 16
		};

		enum AutoUpdate {
			Force, Default, Disabled
		};

		View(Context* context);
		virtual ~View();

		virtual void bindFrame(IFrame* container);

		Context* getContext() { return m_context; }
		String& getTitle();
		void setCursor(CursorType cursor_type) { if (m_frame) m_frame->setCursor(cursor_type); }
		void resetCursor() { if (m_frame) m_frame->resetCursor(); }
		Vector3 getCursorPos() { return m_cursorPos; }

		void setAutoUpdate(AutoUpdate update);
		void doUpdate();

		// select helper
		// from screen pos trace world, if not interacted, then trace to water zero plane, if even not intersected, returned false
		bool traceWorld(int x, int y, Vector3& result, int part);
		bool traceWorld(Vector3& result); // use screen center to trace
		void beginSelect(const Rect& r);
		int endSelect();

		bool selectRegion(const Rect& rect, SelectPart part, OUT Vector3& pos, OUT ActorList& retlist, bool onlynearest = false);
		bool selectRegion(const Rect& rect, SelectPart part, OUT Vector3& pos);

		const RenderCamera& getCamera() const { return m_camera; }

		void setEyeMatrix(const AffineMat& eyeMatrix) {m_eyeMatrix = eyeMatrix; }
		const AffineMat& getEyeMatrix() const { return m_eyeMatrix; }

		// event handler
		virtual void handleEvent(Input::Event* e);

	protected:
		virtual void doRender() = 0;
		virtual void preUpdate() {}

	protected:
		void drawAxis();
		void drawFrameNum();

	protected:
		Context* m_context;
		IFrame* m_frame;
		String m_title;
		Render::Camera m_camera;
		int m_frameNumCur;
		unsigned int m_frameOldTime;
		uint_t m_frameTime;
		float m_fps;
		Vector3 m_cursorPos;
		AffineMat m_eyeMatrix;
		bool m_autoUpdate;
		Render::Font* m_font;
	};

}} // namespace Axon::Editor

#endif // AX_EDITOR_VIEW_H
