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
#include "../dialogs.h"

int   g_res_dlg_id = 0;
char* g_res_dlg_name = "mb_rsc_dlg";

void help_dlg_destruction_handler(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	sDialog* dlg = (sDialog*)rsrc->ptr;
	if(dlg && (dlg->lFlags & 0x01)==0)
	{
		if(dlg->hDlg){
			DestroyWindow(dlg->hDlg);
			dlg->hDlg = NULL;
		}
		DlgFree(dlg);
	}
}

ZEND_FUNCTION(mb_DlgGetFile)
{
	char  open = 0;
	char *ext = NULL;
	char *std = NULL;
	char *e;
	long  el = 0;
	long  sl = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sb|s",&ext,&el,&open,&std,&sl) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}else if(el < 2 || ext[el-1]!='|'){
		RETURN_FALSE;
	}

	e = ext;
	while(*e){
		if(*e == '|'){
			*e = '\0';
		}e++;
	};

	e = help_getfilename(open,ext,std);
	if(e && *e){
		RETURN_STRING(e,1);
	}else{
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mb_DlgGetFileMultiple)
{
	char *ext = NULL;
	char *fn;
	char *e;
	char  out[2048];
	char  path[MAX_PATH];
	long  el = 0;
	long  n = 0;
	long  offset = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&ext,&el) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}else if(el < 2 || ext[el-1]!='|'){
		RETURN_FALSE;
	}

	e = ext;
	while(*e){
		if(*e == '|'){
			*e = '\0';
		}e++;
	};

	*out = 0;

	fn = help_getfilenamemultiple(ext,out,sizeof(out)-1,&offset);

	if(!fn || array_init(return_value)==FAILURE)
	{
		RETURN_FALSE;
	}
	else
	{
		//add_index_string(return_value,i,proto[i]->szName,1);
		if(offset>0){
			out[offset-1]='\0';
		}

		fn = out + offset;
		while(*fn)
		{
			_snprintf(path,sizeof(path)-1,"%s\\%s",out,fn);
			add_index_string(return_value,n++,path,1);
			fn += strlen(fn) + 1;
		}
		return;
	}
}

ZEND_FUNCTION(mb_DlgCreate)
{
	char* title=NULL;
	unsigned long tl=0;
	unsigned long cx=CW_USEDEFAULT;
	unsigned long cy=CW_USEDEFAULT;
	long rs_id = 0;
	long param = 0;
	long flags = 0;
	zval* cb;
	sDialog* dlg = NULL;

	mb_event* mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "szll|ll",&title,&tl,&cb,&cx,&cy,&param,&flags) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(mbe->php == NULL){
		RETURN_FALSE;
	}else if(!zend_is_callable(cb,0,NULL)){
		PHPWSE("$cb is expected to be a valid callback function!");
	}

	if(!tl){title="MBot";}
	if(cx < 100 || cx > 1024){
		cx = 256;
	}
	if(cy < 100 || cy > 1024){
		cy = 256;
	}

	dlg = (sDialog*)my_malloc(sizeof(sDialog));
	if(!dlg){
		RETURN_NULL();
	}
	memset(dlg,0,sizeof(sDialog));
	dlg->php = mbe->php;
	dlg->param = param;

	if(!(dlg->hDlg = CreateDialogParam(hInst,MAKEINTRESOURCE(IDD_DIALOG1),NULL,(DLGPROC)DlgProcedure,(LPARAM)dlg))){
		goto Error;
	}

	if(flags & 0x01){
		ShowWindow(GetDlgItem(dlg->hDlg,IDCANCEL),SW_HIDE);
	}

	SetWindowPos(dlg->hDlg,NULL,0,0,cx,cy,SWP_NOMOVE | SWP_NOZORDER);
	SetWindowText(dlg->hDlg,title);
	SendMessage(dlg->hDlg,WM_USER + 2,0,0);
	strncpy(dlg->pszCallback,cb->value.str.val,sizeof(dlg->pszCallback)-1);

	rs_id = zend_list_insert(dlg,g_res_dlg_id);
	if(rs_id){
		RETURN_RESOURCE(rs_id);
	}
Error:
	if(dlg){
		if(dlg->hDlg)DestroyWindow(dlg->hDlg);
		my_memfree(dlg);
	}
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgRun)
{
	zval* res = 0;
	sDialog* dlg = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r",&res) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	if(!res)goto Error;
	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg || (dlg->lFlags & 0x01))goto Error;

	ShowWindow(dlg->hDlg,SW_SHOWNORMAL);
	dlg->lFlags |= 1;

	RETURN_TRUE;
Error:
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_DlgSetCallbacks)
{
	zval* res = 0;
	zval* wmc=NULL;
	zval* wmn=NULL;
	zval* wmt=NULL;
	sDialog* dlg = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|z!z!z!",&res,&wmc,&wmn,&wmt) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	if(!res)goto Error;
	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg)goto Error;

	if(wmc && zend_is_callable(wmc,0,NULL)){
		strncpy(dlg->pszWmCommand,wmc->value.str.val,sizeof(dlg->pszWmCommand));
	}else{
		*dlg->pszWmCommand = 0;
	}

	if(wmn && zend_is_callable(wmn,0,NULL)){
		strncpy(dlg->pszWmNotify,wmn->value.str.val,sizeof(dlg->pszWmNotify));
	}else{
		*dlg->pszWmNotify = 0;
	}

	if(wmt && zend_is_callable(wmt,0,NULL)){
		strncpy(dlg->pszWmTimer,wmt->value.str.val,sizeof(dlg->pszWmTimer));
	}else{
		*dlg->pszWmTimer = 0;
	}

	RETURN_TRUE;
Error:
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_DlgSetTimer)
{
	unsigned long id,ts;
	zval* res = 0;
	zval* cb = 0;
	sDialog* dlg = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rllz",&res,&id,&ts,&cb) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!zend_is_callable(cb,0,NULL)){
		PHPWSE("$cb must be a valid callback!");
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg)RETURN_FALSE;

	strncpy(dlg->pszWmTimer,cb->value.str.val,sizeof(dlg->pszWmTimer));

	RETURN_LONG(SetTimer(dlg->hDlg,id,ts,NULL));
}

ZEND_FUNCTION(mb_DlgKillTimer)
{
	unsigned long id;
	zval* res = 0;
	sDialog* dlg = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl",&res,&id) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg)RETURN_FALSE;
	RETURN_LONG(KillTimer(dlg->hDlg,id));
}

ZEND_FUNCTION(mb_DlgGet)
{
	sDialog* dlg = NULL;
	long rs_id = 0;
	mb_event* mbe = (mb_event*)((sPHPENV*)SG(server_context))->c_param;

	if(mbe->event != MBT_DIALOG){
		RETURN_NULL();
	}

	if(mbe->p1 != NULL){
		RETURN_RESOURCE((long)mbe->p1);
	}

	dlg = (sDialog*)mbe->p3;
	rs_id = zend_list_insert(dlg,g_res_dlg_id);
	if(rs_id){
		mbe->p1 = (void*)(rs_id);
		RETURN_RESOURCE(rs_id);
	}
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgGetText)
{
	unsigned long id=0;
	char text[2048];
	zval* res = 0;
	sDialog* dlg = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl",&res,&id) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg)goto Error;


	if(id > (dlg->lNum + 1000)){
		goto Error;
	}else{
		*text = 0;
		GetDlgItemText(dlg->hDlg,id,text,sizeof(text)-1);
		RETURN_STRING(text,1);
	}
Error:
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgSetText)
{
	char* text="";
	unsigned long id=0,tl=0;
	zval* res = 0;
	sDialog* dlg = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rls",&res,&id,&text,&tl) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg)goto Error;


	if(id > (dlg->lNum + 1000)){
		goto Error;
	}else{
		SetDlgItemText(dlg->hDlg,id,text);
	}
	RETURN_TRUE;
Error:
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_DlgGetInt)
{
	unsigned long id=0;
	char  isss=0;
	int   ok = 0;
	zval* res = 0;
	sDialog* dlg = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl|b",&res,&id,&isss) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg)goto Error;


	if(id > (dlg->lNum + 1000)){
		goto Error;
	}else{
		id = GetDlgItemInt(dlg->hDlg,id,&ok,isss);
		if(ok){
			RETURN_LONG(id);
		}else{
			RETURN_FALSE;
		}
	}
Error:
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgSendMsg)
{
	zval* res=0;
	zval* lparam=0;
	zval* wparam=0;
	unsigned long id=0;
	unsigned long msg=0;
	sDialog* dlg = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rll|z!z!",&res,&id,&msg,&wparam,&lparam) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg || !msg || msg == WM_DESTROY)goto Error;

	if(id > (dlg->lNum + 1000)){
		goto Error;
	}else{
		try{
			void* lp = (lparam->type==IS_STRING)?((void*)lparam->value.str.val):((void*)lparam->value.lval);
			void* wp = (lparam->type==IS_STRING)?((void*)wparam->value.str.val):((void*)wparam->value.lval);
			RETURN_LONG(SendDlgItemMessage(dlg->hDlg,id,msg,(WPARAM)wp,(LPARAM)lp));
		}catch(...){
			RETURN_FALSE;
		}
	}
Error:
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgGetIdByParam)
{
	long param=0;
	zval* res = 0;
	sDialog* dlg = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl",&res,&param) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg)goto Error;

	for(unsigned long i=0;i<dlg->lNum;i++)
	{
		if(dlg->table[i]->param == param){
			RETURN_LONG(1000 + i);
		}
	}
Error:
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgGetHWND)
{
	long id=0;
	zval* res = 0;
	sDialog* dlg = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl",&res,&id) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg)goto Error;
	if(id == NULL){
		RETURN_LONG((long)dlg->hDlg);
	}else{
		RETURN_LONG((long)GetDlgItem(dlg->hDlg,id));
	}
Error:
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgMove)
{
	sDialog* dlg = NULL;
	long id,x,y,cx=-1,cy=-1;
	HWND hWnd;
	zval* res = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rlll|ll",&res,&id,&x,&y,&cx,&cy) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg)goto Error;

	hWnd = (id == NULL)?(dlg->hDlg):GetDlgItem(dlg->hDlg,id);
	RETURN_LONG(SetWindowPos(hWnd,NULL,x,y,cx,cy,SWP_NOZORDER | ((cx == -1)?(SWP_NOSIZE):0)));
Error:
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgGetString)
{
	sStdInDlg di = {0};
	zval* tmp = NULL;
	long  ilen;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|sllls",&di.info,&ilen,&di.title,&ilen,&di.flags,&di.x,&di.y,&di.def,&ilen) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}


	//zend_alter_ini_entry("max_execution_time",sizeof("max_execution_time"),"600",3,PHP_INI_USER,PHP_INI_STAGE_RUNTIME);

	if(DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_DIALOG2),NULL,(DLGPROC)DlgStdInProc,(LPARAM)&di)==IDOK)
	{
		RETURN_STRING(di.buffer,1);
	}else{
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(mb_DlgListAddItem)
{
	unsigned long id=0;
	unsigned long nl=0;
	unsigned long cx=0;
	unsigned long i=0;
	zval* res = 0;
	char* values[5]={0};
	sDialog* dlg = NULL;
	sDlgControl* ct;
	LVITEM li={0};
	HWND hList;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rls|ssss",&res,&id,&values[0],&nl,
		&values[1],&nl,&values[2],&nl,&values[3],&nl,&values[4],&nl) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg || id < 1000 || (id > (dlg->lNum + 1000)) || *values[0]=='\0')goto Error;

	ct = dlg->table[id - 1000];
	if(ct->type != 6)goto Error;

	li.pszText = values[0];
	li.lParam = (LPARAM)ct->cs2;
	li.iSubItem = 0;
	li.iItem = ct->cs2;
	li.mask = LVIF_PARAM | LVIF_TEXT;

	hList = GetDlgItem(dlg->hDlg,id);
	nl = ListView_InsertItem(hList,&li);
	if(nl != -1)
	{
		for(char** v=(values + 1);*v;v++)
		{
			li.mask = LVIF_TEXT;
			li.pszText = *v;
			li.iItem = nl;
			li.iSubItem = ++i;
			ListView_SetItem(hList,&li);
		}
		ct->cs2++;
		RETURN_LONG(nl);
	}
Error:
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgListDelItem)
{
	unsigned long item=0;
	unsigned long id=0;
	zval* res = 0;
	sDialog* dlg = NULL;
	sDlgControl* ct;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rll",&res,&id,&item) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg || id < 1000 || (id > (dlg->lNum + 1000)))goto Error;

	ct = dlg->table[id - 1000];
	if(ct->type != 6)goto Error;

	RETURN_LONG(ListView_DeleteItem(ct->hWnd,item));
Error:
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgListSetItem)
{
	unsigned long id=0,it=0,si=0,tl=0;
	zval* res = 0;
	char* txt = 0;
	sDialog* dlg = NULL;
	sDlgControl* ct;
	LVITEM li={0};

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rllls",&res,&id,&it,&si,&txt,&tl) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg || id < 1000 || (id > (dlg->lNum + 1000)))goto Error;

	ct = dlg->table[id - 1000];
	if(ct->type != 6 || it > ct->cs2 || si > ct->cs1)goto Error;

	li.mask = LVIF_TEXT;
	li.pszText = txt;
	li.iItem = it;
	li.iSubItem = si;
	ListView_SetItem(ct->hWnd,&li);
	RETURN_TRUE;
Error:
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgListGetItem)
{
	unsigned long id=0,it=0,si=0;
	char buffer[1024];
	zval* res = 0;
	sDialog* dlg = NULL;
	sDlgControl* ct;
	LVITEM li={0};

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rlll",&res,&id,&it,&si) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg || id < 1000 || (id > (dlg->lNum + 1000)))goto Error;

	ct = dlg->table[id - 1000];
	if(ct->type != 6 || it > ct->cs2 || si > ct->cs1)goto Error;

	*buffer = 0;

	li.mask = LVIF_TEXT;
	li.iItem = it;
	li.iSubItem = si;
	li.pszText = buffer;
	li.cchTextMax = sizeof(buffer)-1;

	if(ListView_GetItem(ct->hWnd,&li) == FALSE)goto Error;
	RETURN_STRING(buffer,1);
Error:
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgListGetSel)
{
	unsigned long id=0;
	zval* res = 0;
	sDialog* dlg = NULL;
	sDlgControl* ct;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl",&res,&id) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg || id < 1000 || (id > (dlg->lNum + 1000)))goto Error;

	ct = dlg->table[id - 1000];
	if(ct->type != 6)goto Error;

	id = ListView_GetSelectionMark(ct->hWnd);
	if(id !=-1){
		RETURN_LONG(id);
	}
Error:
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgListAddCol)
{
	unsigned long id=0;
	unsigned long nl=0;
	unsigned long cx=0;
	zval* res = 0;
	char* name;
	sDialog* dlg = NULL;
	sDlgControl* ct;
	LVCOLUMN lvc={0};

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rlsl",&res,&id,&name,&nl,&cx) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg || id < 1000 || (id > (dlg->lNum + 1000)))goto Error;

	ct = dlg->table[id - 1000];
	if(ct->type != 6 || ct->cs1 > 5)goto Error;

	lvc.cx = cx;
	lvc.fmt = LVCFMT_CENTER;
	lvc.pszText = name;
	lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;

	nl = ListView_InsertColumn(GetDlgItem(dlg->hDlg,id),ct->cs1,&lvc) != -1;
	ct->cs1 += nl;
	if(nl != -1){
		RETURN_LONG(nl);
	}
Error:
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgComboAddItem)
{
	unsigned long id;
	unsigned long il;
	unsigned long param = -1;
	char* item;
	zval* res = 0;
	sDialog* dlg = NULL;
	sDlgControl* ct;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rls|l",&res,&id,&item,&il,&param) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg || id < 1000 || (id > (dlg->lNum + 1000)))goto Error;

	ct = dlg->table[id - 1000];
	if(ct->type != 4)goto Error;

	id = SendMessage(ct->hWnd,CB_ADDSTRING,NULL,(LPARAM)item);
	if(id >= 0){
		if(param != -1){
			SendMessage(ct->hWnd,CB_SETITEMDATA,id,param);
		}
		RETURN_LONG(id);
	}
Error:
	RETURN_NULL();
}
ZEND_FUNCTION(mb_DlgComboDelItem)
{
	unsigned long id;
	unsigned long iid;
	zval* res = 0;
	sDialog* dlg = NULL;
	sDlgControl* ct;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rll",&res,&id,&iid) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg || id < 1000 || (id > (dlg->lNum + 1000)))goto Error;

	ct = dlg->table[id - 1000];
	if(ct->type != 4)goto Error;

	RETURN_LONG(SendMessage(ct->hWnd,CB_DELETESTRING,iid,0)>=0);
Error:
	RETURN_NULL();
}

ZEND_FUNCTION(mb_DlgComboGetItem)
{
	unsigned long id;
	unsigned long iid;
	zval* res = 0;
	char* str = 0;
	sDialog* dlg = NULL;
	sDlgControl* ct;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rll",&res,&id,&iid) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg || id < 1000 || (id > (dlg->lNum + 1000)))goto Error;

	ct = dlg->table[id - 1000];
	if(ct->type != 4)goto Error;

	id = SendMessage(ct->hWnd,CB_GETLBTEXTLEN,iid,0);
	if(id < 0)goto Error;

	str = (char*)emalloc(id + 1);
	if(!str)goto Error;
	str[id] = 0;
	
	id = SendMessage(ct->hWnd,CB_GETLBTEXT,iid,(LPARAM)str);
	if(id<=0){
		efree(str);
		goto Error;
	}
	RETURN_STRING(str,0);
Error:
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_DlgComboGetSel)
{
	unsigned long id;
	zval* res = 0;
	sDialog* dlg = NULL;
	sDlgControl* ct;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl",&res,&id) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg || id < 1000 || (id > (dlg->lNum + 1000)))goto Error;

	ct = dlg->table[id - 1000];
	if(ct->type != 4)goto Error;

	id = SendMessage(ct->hWnd,CB_GETCURSEL,0,0);
	if(id != CB_ERR){
		RETURN_LONG(id);
	}
Error:
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_DlgComboGetItemData)
{
	unsigned long id;
	unsigned long iid;
	zval* res = 0;
	sDialog* dlg = NULL;
	sDlgControl* ct;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rll",&res,&id,&iid) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg || id < 1000 || (id > (dlg->lNum + 1000)))goto Error;

	ct = dlg->table[id - 1000];
	if(ct->type != 4)goto Error;

	id = SendMessage(ct->hWnd,CB_GETITEMDATA,iid,0);
	if(id == CB_ERR)goto Error;
	RETURN_LONG(id);
Error:
	RETURN_FALSE;
}

ZEND_FUNCTION(mb_DlgAddControl)
{
	extern HFONT hVerdanaFont;

	zval* res = 0;
	zval* cb = 0;
	long type=0;
	long param=0;
	long style=0;
	long x,y,cx,cy;
	char* name;
	long nl=0;
	sDialog* dlg = NULL;
	sDlgControl* ctrl = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rlsllllz!|ll",&res,&type,&name,&nl,
		&x,&y,&cx,&cy,&cb,&style,&param) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}

	if(!res){
		goto Error;
	}

	if(cb){
		if(cb->type!=IS_STRING || !zend_is_callable(cb,0,NULL)){
			goto Error;
		}
	}

	ZEND_FETCH_RESOURCE(dlg,sDialog*,&res,-1,g_res_dlg_name,g_res_dlg_id);
	if(!dlg || dlg->lNum >= DLG_NUM_CONTROLS)goto Error;

	ctrl = (sDlgControl*)my_malloc(sizeof(sDlgControl));
	if(!ctrl)goto Error;
	memset(ctrl,0,sizeof(sDlgControl));

	if(type != 1){
		style |= WS_TABSTOP;
	}

	ctrl->hWnd = CreateWindow(DlgGetCtrlClass(type),name,(WS_CHILD|WS_VISIBLE|style),x,y,cx,cy,
		dlg->hDlg,(HMENU)(1000 + dlg->lNum),hInst,NULL);

	if(!ctrl->hWnd)goto Error;

	if(type == 6){
		ListView_SetExtendedListViewStyle(ctrl->hWnd,LVS_EX_FULLROWSELECT);
	}

	SendMessage(ctrl->hWnd,WM_SETFONT,(WPARAM)hVerdanaFont,0);

	if(cb){
		strncpy(ctrl->pszCallback,cb->value.str.val,sizeof(ctrl->pszCallback)-1);
	}

	ctrl->type = (char)(0xFF & type);
	ctrl->param = param;
	dlg->table[dlg->lNum] = ctrl;

	RETURN_LONG(1000 + dlg->lNum++);
Error:
	if(ctrl){
		my_memfree((void*)ctrl);
	}
	RETURN_FALSE;
}