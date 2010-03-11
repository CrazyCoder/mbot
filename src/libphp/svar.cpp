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
#include "libphp.h"
#include "internals.h"
#include "svar.h"

extern LPHP_MALLOC g_malloc;
extern LPHP_FREE g_free;

void* svar_malloc(unsigned long amount)
{
	return g_malloc(amount);
}
char* svar_strdup(const char* str)
{
	unsigned long len = strlen(str)+1;
	char* tmp = (char*)g_malloc(len);
	if(tmp){
		memcpy(tmp,str,len);
	}
	return tmp;
}
void svar_free(void* ptr)
{
	try{//g_heap,0,
		(ptr)?(g_free(ptr)):(0);
	}catch(...){
		return;
	}
}

void svar_freevar(sVar* ptr)
{
	try{
		if(ptr)
		{
			if(ptr->type == SV_STRING || ptr->type > 10){
				svar_free(ptr->str.val);
				ptr->str.val = 0;
			}
		}
	}catch(...){
		return;
	}
}

int svar_set(sVar* v1, sVar* v2, void* param){

	if(!v2)
	{//release
		if(v1->locked && param!=(void*)1)return 0;
		svar_freevar(v1);
		v1->type = SV_NULL;
	}
	else
	{
		if(v1->locked){
			return 1;
		}
		if(v1->type == SV_STRING || v1->type > 10){
			svar_free(v1->str.val);
			v1->str = v2->str;
		}else{
			v1->dval = v2->dval;
		}
		v1->type = v2->type;
	}
	return 1;
}

void sVariable(sVar* out,char cType,void* pValue,unsigned long pLen,char cLocked)
{
	out->type = cType;
	out->locked = cLocked;

	switch(cType){
		case SV_STRING:
			out->str.val = (pLen)?((char*)pValue):(svar_strdup((const char*)pValue));
			out->str.len = (pLen)?(pLen):(strlen(out->str.val));
			break;
		case SV_LONG:
		case SV_WORD:
			out->lval = (long)pValue;
			break;
		case SV_DOUBLE:
			out->dval = *((double*)pValue);
			break;
		case SV_ARRAY:
			out->str.val = (char*)pValue;
			out->str.len = pLen;
			break;
		case SV_LPOINTER:
			out->lval = (long)pValue;
			break;
		default:
			out->type = SV_NULL;
			out->lval = 0;
	}
};

void sVariable(sVar* out,double val,char cLocked)
{
	out->type = SV_DOUBLE;
	out->dval = val;
	out->locked = cLocked;
}