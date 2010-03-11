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
#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#include "mbot.h"
#pragma once

extern void* mv_module_entry;

#define MB_EVENT_MSG_IN 0x01
#define MB_EVENT_MSG_OUT 0x02
#define MB_EVENT_URL_IN 0x04
#define MB_EVENT_AUTH_IN 0x08
#define MB_EVENT_AWAY_MSG_OUT 0x10
#define MB_EVENT_AWAY_MSG_REQ 0x20
#define MB_EVENT_NEW_CSTATUS 0x40
#define MB_EVENT_NEW_MYSTATUS 0x80
#define MB_EVENT_COMMAND 0x0100
#define MB_EVENT_STARTUP 0x0200
#define MB_EVENT_SHUTDOWN 0x0400
#define MB_EVENT_USER_TYPING 0x0800
#define MB_EVENT_AWAY_MSG_ICQ 0x1000
#define MB_EVENT_MENU_COMMAND 0x2000
#define MB_EVENT_EXTERNAL 0x4000
#define MB_EVENT_FILE_IN 0x8000
#define IRC_EVENT_GUI_IN 0x010000
#define IRC_EVENT_GUI_OUT 0x020000
#define IRC_EVENT_RAW_IN 0x040000
#define IRC_EVENT_RAW_OUT 0x080000
#define MB_EVENT_CONFIG 0x100000

extern const char* PHP_ERROR_MSG;
extern const char* PHP_ERROR_EXECUTION;
extern const char* PHP_ERROR_TITLE;
extern const char* PHP_WARN_INVALID_PARAMS;

#define PHP_FALSE_AND_ERROR php_error_docref(NULL TSRMLS_CC,E_ERROR,PHP_ERROR_MSG);return
#define PHP_FALSE_AND_ERRORS(s) php_error_docref(NULL TSRMLS_CC,E_ERROR,s);return
#define PHP_FALSE_AND_WARNS(s) php_error_docref(NULL TSRMLS_CC,E_WARNING,s);return
#define PHP_WARN php_error_docref(NULL TSRMLS_CC,E_WARNING,

#define PHPWS php_error_docref(NULL TSRMLS_CC,E_WARNING,
#define PHPWE );return
#define PHPWSE(s) php_error_docref(NULL TSRMLS_CC,E_WARNING,s);return

extern HICON	hMBotIcon;
extern CSyncList g_mlist;
extern CSyncList g_elist;
extern long lm_flags;
extern MM_INTERFACE mmi;

void MB_Popup(const char* title,const char* error,unsigned long flags = NIIF_ERROR,unsigned long timeout=15000);

/* function definition */
ZEND_FUNCTION(mb_SelfRegister);
ZEND_FUNCTION(mb_SelfEnable);
ZEND_FUNCTION(mb_SelfSetInfo);

ZEND_FUNCTION(mb_SchReg);
ZEND_FUNCTION(mb_SchModify);
ZEND_FUNCTION(mb_SchUnreg);
ZEND_FUNCTION(mb_SchEnable);
ZEND_FUNCTION(mb_SchList);
//messages
ZEND_FUNCTION(mb_MsgSend);
ZEND_FUNCTION(mb_MsgSetBody);
//stdin/out
ZEND_FUNCTION(mb_MsgBox);
ZEND_FUNCTION(mb_Echo);
//menu
ZEND_FUNCTION(mb_MenuAdd);
ZEND_FUNCTION(mb_MenuModify);
//clist
ZEND_FUNCTION(mb_CListEventAdd);
//system
ZEND_FUNCTION(mb_SysTranslate);
ZEND_FUNCTION(mb_SysGetMirandaDir);
ZEND_FUNCTION(mb_SysGetProfileName);
ZEND_FUNCTION(mb_SysQuit);
ZEND_FUNCTION(mb_SysEnumModules);
ZEND_FUNCTION(mb_SysEnumHandlers);
ZEND_FUNCTION(mb_SysEnumProtocols);
ZEND_FUNCTION(mb_SysCallService);
ZEND_FUNCTION(mb_SysCallProtoService);
ZEND_FUNCTION(mb_SysCallContactService);
ZEND_FUNCTION(mb_SysManageScript);
ZEND_FUNCTION(mb_SysShallDie);

ZEND_FUNCTION(mb_SysGetNumber);
ZEND_FUNCTION(mb_SysGetString);
ZEND_FUNCTION(mb_SysGetPointer);
ZEND_FUNCTION(mb_SysPutString);
ZEND_FUNCTION(mb_SysPutNumber);
ZEND_FUNCTION(mb_SysMemCpy);
ZEND_FUNCTION(mb_SysMalloc);
ZEND_FUNCTION(mb_SysFree);
ZEND_FUNCTION(mb_SysGlobalAlloc);
ZEND_FUNCTION(mb_SysGlobalFree);
ZEND_FUNCTION(mb_SysBeginThread);
ZEND_FUNCTION(mb_SysLoadModule);
ZEND_FUNCTION(mb_SysUnLoadModule);

ZEND_FUNCTION(mb_SysCreateService);
ZEND_FUNCTION(mb_SysHookEvent);
ZEND_FUNCTION(mb_SysGetProcAddr);
ZEND_FUNCTION(mb_SysCallProc);
//console
ZEND_FUNCTION(mb_ConsoleShow);
ZEND_FUNCTION(mb_ConsoleClear);
//protocols
ZEND_FUNCTION(mb_PGetMyStatus);
ZEND_FUNCTION(mb_PSetMyStatus);
ZEND_FUNCTION(mb_PSetMyAwayMsg);
ZEND_FUNCTION(mb_PGetCaps);
//contacts/settings
ZEND_FUNCTION(mb_CSettingGet);
ZEND_FUNCTION(mb_CSettingSet);
ZEND_FUNCTION(mb_CSettingAdd);
ZEND_FUNCTION(mb_CSettingDel);
ZEND_FUNCTION(mb_CSettingEnum);
ZEND_FUNCTION(mb_CGetDisplayName);
ZEND_FUNCTION(mb_CGetUIN);
ZEND_FUNCTION(mb_CGetProto);
ZEND_FUNCTION(mb_CDelete);
ZEND_FUNCTION(mb_CGetAwayMsg);
ZEND_FUNCTION(mb_CGetStatus);
ZEND_FUNCTION(mb_CIsOnList);
ZEND_FUNCTION(mb_CFindFirst);
ZEND_FUNCTION(mb_CFindNext);
ZEND_FUNCTION(mb_CFindByUIN);
ZEND_FUNCTION(mb_CSetApparentMode);
ZEND_FUNCTION(mb_CAddNew);
ZEND_FUNCTION(mb_CAddAuth);
ZEND_FUNCTION(mb_CAddSearch);
ZEND_FUNCTION(mb_CGetInfo);
ZEND_FUNCTION(mb_CSendTypingInfo);
//auth in
ZEND_FUNCTION(mb_AuthGetInfo);
ZEND_FUNCTION(mb_AuthDeny);
ZEND_FUNCTION(mb_AuthAccept);
ZEND_FUNCTION(mb_AuthStore);
//history
ZEND_FUNCTION(mb_EventFindFirst);
ZEND_FUNCTION(mb_EventFindFirstUnread);
ZEND_FUNCTION(mb_EventFindNext);
ZEND_FUNCTION(mb_EventFindPrev);
ZEND_FUNCTION(mb_EventFindLast);
ZEND_FUNCTION(mb_EventGetCount);
ZEND_FUNCTION(mb_EventGetData);
ZEND_FUNCTION(mb_EventDel);
ZEND_FUNCTION(mb_EventAdd);
ZEND_FUNCTION(mb_EventMarkRead);
//skin/sounds
ZEND_FUNCTION(mb_SoundPlay);
ZEND_FUNCTION(mb_SoundAdd);
ZEND_FUNCTION(mb_SoundAddEx);
ZEND_FUNCTION(mb_SoundDel);
ZEND_FUNCTION(mb_SoundSet);
//popups
ZEND_FUNCTION(mb_PUMsg);
ZEND_FUNCTION(mb_PUAdd);
ZEND_FUNCTION(mb_PUAddEx);
ZEND_FUNCTION(mb_PUSystem);
//icons
ZEND_FUNCTION(mb_IconLoadSys);
ZEND_FUNCTION(mb_IconLoadSkin);
ZEND_FUNCTION(mb_IconLoadProto);
ZEND_FUNCTION(mb_IconLoadSkinnedProto);
ZEND_FUNCTION(mb_IconDestroy);
//search
ZEND_FUNCTION(mb_SearchBasic);
ZEND_FUNCTION(mb_SearchByEmail);
ZEND_FUNCTION(mb_SearchByName);

//dialogs
void help_dlg_destruction_handler(zend_rsrc_list_entry *rsrc TSRMLS_DC);

ZEND_FUNCTION(mb_DlgGetFile);
ZEND_FUNCTION(mb_DlgGetFileMultiple);
ZEND_FUNCTION(mb_DlgCreate);
ZEND_FUNCTION(mb_DlgRun);
ZEND_FUNCTION(mb_DlgAddControl);
ZEND_FUNCTION(mb_DlgGet);
ZEND_FUNCTION(mb_DlgGetText);
ZEND_FUNCTION(mb_DlgSetText);
ZEND_FUNCTION(mb_DlgGetInt);
ZEND_FUNCTION(mb_DlgGetIdByParam);
ZEND_FUNCTION(mb_DlgSendMsg);
ZEND_FUNCTION(mb_DlgSetCallbacks);
ZEND_FUNCTION(mb_DlgSetTimer);
ZEND_FUNCTION(mb_DlgKillTimer);
//list view
ZEND_FUNCTION(mb_DlgListAddItem);
ZEND_FUNCTION(mb_DlgListDelItem);
ZEND_FUNCTION(mb_DlgListSetItem);
ZEND_FUNCTION(mb_DlgListGetItem);
ZEND_FUNCTION(mb_DlgListGetSel);
ZEND_FUNCTION(mb_DlgListAddCol);
//combo box
ZEND_FUNCTION(mb_DlgComboAddItem);
ZEND_FUNCTION(mb_DlgComboDelItem);
ZEND_FUNCTION(mb_DlgComboGetItem);
ZEND_FUNCTION(mb_DlgComboGetItemData);
ZEND_FUNCTION(mb_DlgComboGetSel);

//general
ZEND_FUNCTION(mb_DlgGetHWND);
ZEND_FUNCTION(mb_DlgMove);
ZEND_FUNCTION(mb_DlgGetString);

//file transfers
ZEND_FUNCTION(mb_FileInitSend);
ZEND_FUNCTION(mb_FileAccept);
ZEND_FUNCTION(mb_FileCancel);
ZEND_FUNCTION(mb_FileDeny);
ZEND_FUNCTION(mb_FileStore);
ZEND_FUNCTION(mb_FileGetInfo);

ZEND_FUNCTION(mb_IrcGetGuiDataIn);//ok
ZEND_FUNCTION(mb_IrcSetGuiDataIn);//ok
ZEND_FUNCTION(mb_IrcSetGuiDataOut);
ZEND_FUNCTION(mb_IrcInsertRawIn);
ZEND_FUNCTION(mb_IrcInsertRawOut);
ZEND_FUNCTION(mb_IrcInsertGuiIn);
ZEND_FUNCTION(mb_IrcInsertGuiOut);
ZEND_FUNCTION(mb_IrcGetData);
ZEND_FUNCTION(mb_IrcPostMessage);

//asus
ZEND_FUNCTION(mb_AsusExt);

#endif //_FUNCTIONS_H_