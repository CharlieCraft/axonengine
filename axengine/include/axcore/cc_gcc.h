/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#ifndef AX_CC_GCC_H
#define AX_CC_GCC_H

namespace Axon {

typedef long long longlong_t;
typedef unsigned long long ulonglong_t;

} // namespace Axon

#define STDCALL

#ifndef CDECL
#   define CDECL
#endif

#ifndef FASTCALL
#	define FASTCALL
#endif

#define AX_DLL_IMPORT
#define AX_DLL_EXPORT

#define Alloca alloca

#define __AX_ASSUME assume

#endif // AX_CC_GCC_H
