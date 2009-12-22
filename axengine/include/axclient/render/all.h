/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#ifndef AX_RENDER_ALL_H
#define AX_RENDER_ALL_H

namespace Axon {

	class Material;
	class Font;

	enum Spec {
		VeryLow, Low, Middle, High, VeryHigh
	};

} // namespace Axon

#include "query.h"
#include "texture.h"
#include "textureatlas.h"
#include "shader.h"
#include "material.h"
#include "camera.h"
#include "target.h"
#include "interaction.h"
#include "primitive.h"
#include "renderentity.h"
#include "light.h"
#include "fog.h"
#include "terrain.h"
#include "driver.h"
#include "font.h"
#include "wind.h"
#include "timeofday.h"
#include "outdoorenv.h"
#include "renderworld.h"
#include "queue.h"
#include "rendersystem.h"

#endif // end guardian

