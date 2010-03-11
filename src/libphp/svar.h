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
#ifndef _SVAR_H_
#define _SVAR_H_

#include "internals.h"

struct sVar
{
	char  type;
	char  locked;
	union{
		struct {
			char* val;
			unsigned long  len;
		}str;
		long lval;
		unsigned long ulval;
		short sval;
		double dval;
	};

	/*sVar(){
		this->type = 0;
		this->lval = 0;
		this->locked = 0;
	}*/
};

typedef stdext::hash_map<std::string, sVar> sVARmap;
typedef stdext::hash_map<std::string, lphp_funct> sFCNmap;

void sVariable(sVar* out,char cType,void* pValue,unsigned long pLen,char cLocked);
void sVariable(sVar* out,double val,char cLocked);

void*	svar_malloc(unsigned long amount);
char*	svar_strdup(const char* str);
void	svar_free(void* ptr);
void	svar_freevar(sVar* ptr);
int		svar_set(sVar* v1,sVar* v2,void* param);

#endif //_SVAR_H_
