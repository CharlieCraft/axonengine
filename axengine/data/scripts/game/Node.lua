--[[
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
]]--

module( ..., package.seeall );

--[[
Node callbacks:

onSpawn					-- spawn to game world
onRemove				-- remove from game world

--]]--

AX_DECLARE_CLASS( _M, Object )

objNameSpace = "Game"

print( objNameSpace )