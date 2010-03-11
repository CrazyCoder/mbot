/*

Miranda Scripting Plugin for Miranda-IM
Copyright 2004-2006 Piotr Pawluczuk (www.pawluczuk.info)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef _LIBPHP_H__
#define _LIBPHP_H__

#ifndef _LPHP_STATIC_
	#ifdef LIBPHP_EXPORTS
		#define LPHPAPI __declspec(dllexport)
	#else
		#define LPHPAPI __declspec(dllimport)
	#endif
#else  //_LPHP_STATIC_
	#define LPHPAPI 
#endif //_LPHP_STATIC_

#include <stdarg.h>


enum LPHP_TYPE{LPHP_NUMBER=0,LPHP_STRING=1,LPHP_VOID=2};

enum SVAR_TYPE{
	SV_NULL,SV_LONG=1,SV_WORD=2,SV_DOUBLE=3,
	SV_STRING=4,SV_LPOINTER=5,SV_ARRAY=11};

enum LPHP_CB{
		LPHP_CB_SETHDR=0, //const char* "header: value", int replace
		LPHP_CB_OUTSTARTED=1, //NULL NULL
		LPHP_CB_GETMETHOD=2, //NULL NULL, must return "GET" or "POST"
		LPHP_CB_GETCOOKIE=3, //NULL NULL, must return "string" or NULL
		LPHP_CB_POST_LENGTH=5, //NULL NULL, must return length
		LPHP_CB_POST_DATA=6, //NULL NULL, must return pointer or NULL
		LPHP_CB_GETENV=7,//const char* name NULL, must return pointer or "" if empty!
		LPHP_CB_GETCL=8,//NULL NULL
		LPHP_CB_GETCT=9,//NULL NULL
		LPHP_CB_GETVARS=10//NULL NULL must return pointer to a "\0\0" terminated var array;
};

struct lphp_vparam{
	void* data;
	long  length;
	long  type;
};

typedef void  (*LPHP_OUTPUT)(const char* data,long length);
typedef int   (*LPHP_ENVCB)(LPHP_CB code,void* p1,void* p2,void* cparam);
typedef void* (*LPHP_MALLOC)(unsigned long amount);
typedef void  (*LPHP_FREE)(void* ptr);

LPHPAPI int LPHP_PreInit(unsigned long vt_size,unsigned long ft_size, LPHP_MALLOC fp_malloc, LPHP_FREE fp_free);
LPHPAPI int LPHP_Init(LPHP_OUTPUT fp_output, LPHP_OUTPUT fp_error, void* php_module = 0);

LPHPAPI int LPHP_DeInit();
LPHPAPI int LPHP_Initialized();

LPHPAPI int LPHP_RegisterFunction(const char* name,void* fptr,long lp,
	LPHP_TYPE rval,LPHP_TYPE p1 = LPHP_VOID,LPHP_TYPE p2 = LPHP_VOID,LPHP_TYPE p3 = LPHP_VOID,LPHP_TYPE p4 = LPHP_VOID);
LPHPAPI	int LPHP_UnregisterFunction(const char* name);

LPHPAPI int LPHP_Free(void* ptr);

LPHPAPI int LPHP_ExecutePage(const char* filepath,const char* querystring,const char** output,void* cparam,LPHP_ENVCB cb,long flags=0);
LPHPAPI int LPHP_ExecuteFile(const char* path,const char* funct,const char** output,void* cparam,const char* ptypes, ...);
LPHPAPI int LPHP_ExecuteFileVA(const char* path,const char* funct,const char** output,void* cparam,const char* ptypes, va_list args);

LPHPAPI int LPHP_ExecuteScript(const char* body,const char* funct,const char** output,void* cparam,const char* ptypes, ...);
LPHPAPI int LPHP_ExecuteScriptVA(const char* body,const char* funct,const char** output,void* cparam,const char* ptypes, va_list args);

LPHPAPI int LPHP_ExecuteDirect(const char* body,const char** output,void* cparam);

LPHPAPI int LPHP_GetVar(const char* name,void** value,SVAR_TYPE* cType);
LPHPAPI int LPHP_DelVar(const char* name);
LPHPAPI int LPHP_SetVar(const char* name,void* value,SVAR_TYPE cType);
LPHPAPI int LPHP_NewVar(const char* name,void* value,SVAR_TYPE cType,char locked=0);
LPHPAPI int LPHP_IsVar(const char* name);

#endif