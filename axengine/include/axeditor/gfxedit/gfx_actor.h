/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#ifndef AX_GFXEDIT_ACTOR_H
#define AX_GFXEDIT_ACTOR_H

namespace Axon { namespace Editor { namespace GfxEdit {

	class GfxContext;

	class GfxActor : public Actor {
	public:
		GfxActor(GfxContext* ctx);
		virtual ~GfxActor();

	private:
	};

}}} // namespace Axon::Editor::GfxEdit

#endif
