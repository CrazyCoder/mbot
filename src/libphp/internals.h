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
#ifndef _LIBPHP_I_H_
#define _LIBPHP_I_H_

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <time.h>

#include <list>
#include <vector>
#include <hash_map>
#include <map>
#include <string>

#include "libphp.h"

struct lphp_funct
{
	char name[64];
	void* ptr;
	long lp;
	long pinfo;
public:
	lphp_funct(){}
	lphp_funct(const char* n,void* p,long l,long pi){
		strncpy(name,n,63);
		ptr = p;
		lp = l;
		pinfo = pi;
	}
};

struct STR{
	char* val;
	unsigned long  len;
};

class ccLock
{
public:
	ccLock(CRITICAL_SECTION& cs) : m_cs(cs){lock();}
	~ccLock(){unlock();}
public:
	void unlock(){LeaveCriticalSection(&m_cs);}
	void lock(){EnterCriticalSection(&m_cs);}
protected:
	CRITICAL_SECTION& m_cs;
};
#define cLock(c) ccLock _lockObj(c)
#define cULock() _lockObj.unlock();
#define cLLock() _lockObj.lock();

typedef enum {
	MT_LOG_INFO = 0,
	MT_LOG_ERROR = 1,
	MT_LOG_FATAL_ERROR = 2,
	MT_LOG_WARNING = 3,
	MT_LOG_DATA_ERROR = 4,
	MT_LOG_EXCEPTION = 5,
	MT_LOG_DATABASE = 6
}MTLOG_ENUM;

void lphp_error(MTLOG_ENUM log_class,void* _ep,const char* info,...);
const char* g_pref(const char* name);
const char* g_pref_def(const char* name,const char* def);
unsigned long g_pref_ul(const char* name, long base = 0, unsigned long def=0);

typedef void* (*LPHP_GEN0)(void* cp);
typedef void* (*LPHP_GEN1)(void* cp,void* p1);
typedef void* (*LPHP_GEN2)(void* cp,void* p1,void* p2);
typedef void* (*LPHP_GEN3)(void* cp,void* p1,void* p2,void* p3);
typedef void* (*LPHP_GEN4)(void* cp,void* p1,void* p2,void* p3,void* p4);

extern void* g_php_module;

#endif