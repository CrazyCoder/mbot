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
#include "../functions.h"

ZEND_FUNCTION(mb_MenuAdd)
{
	zval*	cb = NULL;
	zval*	pb = NULL;
	char*	name = NULL;
	char*	proto = NULL;
	char	fname[24];
	char	root = 0;
	void*	temp = NULL;
	long	nl = 0;
	long	pl = 0;
	long	param = 0;
	long	icon = 0;
	long	flags = 0;
	long	position = 0x7ffffff;
	char	cm = 0;
	CLISTMENUITEM mi={0};
	sMFSync* mfs;

	mb_event* mbe = (mb_event*)(((sPHPENV*)SG(server_context))->c_param);

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slz|llbslz!b",&name,&nl,&param,&cb,&icon,
		&flags,&cm,&proto,&pl,&position,&pb,&root)==FAILURE 
		|| !nl ||  cb->type != IS_STRING){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!zend_is_callable(cb,0,NULL)){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(pb && (!cm || !zend_is_callable(pb,0,NULL))){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(mbe->event == MBT_AUTOLOAD){
		mbe->php = sman_getbyfile((const char*)mbe->p3);
	}
	if(!mbe->php){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	mfs = (sMFSync*)my_malloc(sizeof(sMFSync));
	if(!mfs){RETURN_FALSE;}
	memset(mfs,0,sizeof(sMFSync));

	mfs->php = mbe->php;

	strncpy(mfs->pszFunction,cb->value.str.val,sizeof(mfs->pszFunction));
	_snprintf(fname,sizeof(fname),"MBot/Menu/%.8x",mfs);

	temp = help_makefunct(mfs,(void*)help_callmenu);
	if(!temp){
		goto Error;
	}
	if(!CreateServiceFunction(fname,(MIRANDASERVICE)temp)){
		goto Error;
	}
	mi.cbSize = sizeof(mi);
	mi.position = position;
	mi.hIcon = (icon)?((HICON)icon):(hMBotIcon);
	mi.pszName = name;
	mi.pszService = fname;
	mi.flags = flags;

	if(cm){
		if(pl){
			mi.pszContactOwner = proto;
		}
		if(pb){
			strncpy(mfs->pszPrebuild,pb->value.str.val,sizeof(mfs->pszPrebuild));
			lm_flags |= MBOT_FLAG_WANTPREBUILD;
		}
	}else{
		mi.pszPopupName = (!root)?"MBot":NULL;
	}

	mfs->hMenu = CallService((cm)?(MS_CLIST_ADDCONTACTMENUITEM):(MS_CLIST_ADDMAINMENUITEM),0,(LPARAM)&mi);
	if(mfs->hMenu==NULL){
		DestroyServiceFunction(fname);
		goto Error;
	}
	g_mlist.Add(mfs);
	RETURN_LONG(mfs->hMenu);
Error:
	if(mbe->php){sman_decref(mbe->php);}
	my_memfree(mfs);
	if(temp){my_memfree(temp);}
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_MenuModify)
{
	long  mid = 0;
	long  flags = 0xffffffff;
	char* name = 0;
	long  nl = 0;
	long  ico = 0;
	zval* cb = NULL;
	CLISTMENUITEM mi = {0};

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|sllz",&mid,&name,&nl,&ico,&flags,&cb)==FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!mid){
		RETURN_NULL();
	}

	if(cb)
	{
		sMFSync* mfs;
		if(!zend_is_callable(cb,0,NULL)){
			PHP_FALSE_AND_ERRORS("$cb is expected to be a valid callback function!");
		}

		g_mlist.Lock();
		mfs = (sMFSync*)g_mlist.m_head;
		while(mfs)
		{
			if(mfs->hMenu == mid){
				strncpy(mfs->pszFunction,cb->value.str.val,sizeof(mfs->pszFunction)-1);
				break;
			}
			mfs = (sMFSync*)mfs->next;
		}
		g_mlist.Unlock();
	}

	mi.cbSize = sizeof(mi);

	if(ico){
		mi.hIcon = (HICON)ico;
		mi.flags |= CMIM_ICON;
	}
	if(nl){
		mi.pszName = name;
		mi.flags |= CMIM_NAME;
	}
	if(flags != 0xffffffff){
		mi.flags |= flags | CMIM_FLAGS;
	}

	RETURN_LONG(CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)mid,(LPARAM)&mi));
}