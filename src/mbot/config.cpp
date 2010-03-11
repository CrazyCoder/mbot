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
#include "mbot.h"
#include "config.h"

char			pszCmdTag[8] = "m>";
char			pszPhpTag[8] = "?>";
const char*		pszMonospacedFont = "Courier";
long			lCmdTagLen = 2;
long			lPhpTagLen = 2;
long			lDebugOut = 0;
long			lConTopMost = 0;
long			lErrorLog = 0;
long			lConFontSize = 13;
long			lConToFile = 0;
cXmlDoc			cSettings;

static php_entry php_ini[] =
{
	{"memory_limit","8M",0},
	{"max_execution_time","45",0},
	{"display_startup_errors","On",0},
	{"precision","14",0},
	{"display_errors","On",0},
	{"asp_tags","Off",0},
	{"error_reporting","E_ALL",0},
	{"cgi.force_redirect","0",0},
	{"safe_mode","Off",0},
	{"implicit_flush","On",0},
	{"html_errors","Off",0},
	{"register_argc_argv","On",0},
	{"register_globals","On",0},
	{"variables_order","GPCES",1},
	{"magic_quotes_gpc","Off",0},
	{"file_uploads","On",0},
	{"upload_tmp_dir","$(mroot)\\mbot\\temp",1},
	{"upload_max_filesize","2M",0},
	{"session.use_cookies","1",1},
	{"session.cookie_path","/",1},
	{"session.name","phpsessid",1},
	{"session.save_path","$(mroot)\\mbot\\temp",1},
	{"include_path","$(mroot)\\mbot;$(mroot)\\mbot\\scripts;$(mroot)\\mbot\\scripts\\autoload",1},
	{"doc_root","$(mroot)\\mbot\\scripts\\autoload\\",1},
	{"extension_dir","$(mroot)\\mbot\\extensions",1},
	{NULL,NULL,0}
};

int php_generate_ini()
{
	char path[MAX_PATH + 2]={0};
	string ss;
	sXmlNode* xn;
	php_entry* ce;
	long  res = 0;
	FILE* pini = NULL;

	_snprintf(path,sizeof(path)-1,"%s\\mbot\\config\\php.ini",g_root);
	LPHP_NewVar("/cfg/php/ini",path,SV_STRING,1);
	LPHP_NewVar("/cfg/php/noupd",(void*)1,SV_LONG,1);
	LPHP_NewVar("/cfg/php/debug",(void*)1,SV_LONG,1);
	LPHP_NewVar("/cfg/php/root",g_root,SV_STRING,1);

	_snprintf(path,sizeof(path)-1,"%s\\mbot\\config\\mbot.xml",g_root);
	try{
		res = cSettings.ParseFile(path,cXmlDoc::PARSE_IGNORE_WHITESPACES | cXmlDoc::PARSE_COMMENTS);
	}catch(...){
		res = 0;
	}

	if(!res)
	{
		MessageBox(0,"Could not parse 'mbot/mbot.xml' configuration file!","MBot",MB_ICONERROR);
		return 1;
	}
	//generate php.ini
	_snprintf(path,sizeof(path)-1,"%s\\mbot\\config\\php.ini",g_root);
	pini = fopen(path,"wb");
	fprintf(pini,";(auto-generated) Please don't ever modify this file! Modify mbot.xml instead!\r\n");
	for(ce = php_ini;(ce->name);ce++)
	{
		_snprintf(path,sizeof(path)-1,"mbot/php/%s",ce->name);
		sXmlNode* xn = cSettings.GetNode(path,NULL);
		ss = (xn && xn->value)?(xn->value):(ce->def_val);
		ut_str_replace("$(default)", ce->def_val, ss);
		ut_str_replace("$(mroot)", g_root, ss);

		if(xn){
			fprintf(pini,"%s=%c%s%c\r\n",ce->name,(ce->str)?('\"'):(' '),ss.data(),(ce->str)?('\"'):(' '));
		}else{
			fprintf(pini,"%s=%c%s%c\r\n",ce->name,(ce->str)?('\"'):(' '),ss.data(),(ce->str)?('\"'):(' '));
		}
	}
	xn = cSettings.GetNode("mbot/add_php",NULL);
	if(xn && xn->f_child)
	{
		xn = xn->f_child;
		while(xn)
		{
			if(xn->value && xn->name)
			{
				fprintf(pini, "%s=%s\r\n", xn->name, (xn && xn->value)?(xn->value):(ce->def_val));
			}
			xn = xn->next;
		}
	}
	fclose(pini);

	xn = cSettings.GetNode("mbot/thread_switch",NULL);
	if(!xn || !xn->value){
		LPHP_NewVar("/cfg/php/thread_fork",(void*)0,SV_LONG,1);
	}else{
		LPHP_NewVar("/cfg/php/thread_fork",(void*)strtoul(xn->value,0,0),SV_LONG,1);
	}

	_snprintf(path,sizeof(path)-1,"%s\\mbot\\extensions",g_root);
	if(!help_direxists(path)){
		CreateDirectory(path,NULL);
	}

	xn = cSettings.GetNode("mbot/debug");
	if(xn && xn->value && *xn->value == '1'){
		lDebugOut = 1;
	}

	xn = cSettings.GetNode("mbot/error_log");
	if(xn && xn->value && *xn->value == '1'){
		lErrorLog = 1;
	}

	xn = cSettings.GetNode("mbot/console/font");
	if(xn && xn->value){
		pszMonospacedFont = xn->value;
	}
	xn = cSettings.GetNode("mbot/console/topmost");
	if(xn && xn->value && *xn->value == '1'){
		lConTopMost = 1;
	}
	xn = cSettings.GetNode("mbot/console/tofile");
	if(xn && xn->value && *xn->value == '1'){
		lConToFile = 1;
	}
	xn = cSettings.GetNode("mbot/console/fsize");
	if(xn && xn->value){
		lConFontSize = strtoul(xn->value,NULL,0);
	}

#ifndef _NOHTTPD_
	xn = cSettings.GetNode("mbot/httpd/port",NULL);
	if(xn && xn->value && (res = strtoul(xn->value,NULL,10))){
		DBWriteContactSettingWord(NULL,MBOT,"WWWPort",(WORD)(res&0xffff));
	}

	xn = cSettings.GetNode("mbot/httpd/ip_mask",NULL);
	if(xn && xn->value && *xn->value){
		DBWriteContactSettingString(NULL,MBOT,"WWWIPMask",xn->value);
	}

	xn = cSettings.GetNode("mbot/httpd/auth_req",NULL);
	if(xn && xn->value && (res = strtoul(xn->value,NULL,10))){
		DBWriteContactSettingByte(NULL,MBOT,"WWWAuthR",1);
	}else{
		DBWriteContactSettingByte(NULL,MBOT,"WWWAuthR",0);
	}

	if(res)
	{
		xn = cSettings.GetNode("mbot/httpd/auth_usr",NULL);
		if(xn && xn->value && *xn->value){
			DBWriteContactSettingString(NULL,MBOT,"WWWUser",xn->value);
		}
		xn = cSettings.GetNode("mbot/httpd/auth_pwd",NULL);
		if(xn && xn->value && *xn->value){
			DBWriteContactSettingString(NULL,MBOT,"WWWPass",xn->value);
		}
	}

	xn = cSettings.GetNode("mbot/httpd/wwwroot",NULL);
	if(xn && xn->value && *xn->value)
	{
		ss = xn->value;
		ut_str_replace("$(mroot)", g_root, ss);
		DBWriteContactSettingString(NULL,MBOT,"WWWRoot",ss.data());
	}
#endif
	return 0;
}

int mbot_our_own(const char* s){
	for(const char** x=config_table;*x;x++){
		if(s == *x){return 1;}
	}return 0;
}

const char* mbot_replace_with_our_own(const char* s)
{
	for(const char** x=config_table;*x;x++){
		if(strcmp(*x,s)==0){return *x;}
	}return s;
}
