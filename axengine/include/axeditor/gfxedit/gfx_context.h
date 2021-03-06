/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#ifndef AX_GFXEDIT_CONTEXT_H
#define AX_GFXEDIT_CONTEXT_H

namespace Axon { namespace Editor { namespace GfxEdit {

	class GfxContext : public Context {
	public:
		GfxContext();
		virtual ~GfxContext();

		// implement Context
		virtual void doRender(const Render::Camera& camera, bool world = false);
		virtual void doSelect(const Render::Camera& camera, int part);

	private:
		GfxView* m_view;
	};

}}} // namespace Axon::Editor::GfxEdit

#endif
