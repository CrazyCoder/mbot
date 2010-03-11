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
#include "functions.h"
#include "window.h"
#include "config.h"

extern CRITICAL_SECTION csConsole;
extern cXmlDoc			cSettings;
extern CSyncList		sm_list;
extern HICON			hMBotIcon;
extern FILE*			dbgout;
extern const char*		pszMonospacedFont;
extern long				lConFontSize;

HIMAGELIST	hImagelist = NULL;
HBITMAP		hBitmap = NULL;
WNDPROC		fpOldWndProc = NULL;
HFONT		hVerdanaFont = NULL;
HFONT		hMonospacedFont = NULL;
HMENU		hMenu = NULL;
BOOL		bLocked = FALSE;

const char* rtf_start = "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0\\fmodern Lucida Console;}}\n\
{\\colortbl;\\red255\\green0\\blue0;\\red63\\green172\\blue51;\\red47\\green86\\blue176;}\\fs21 ";
const char* rtf_stop = "}";

struct mbot_es
{
	const char* ptr;
	long left;
	long cb;
	const char* buff[3];
	unsigned short length[3];
};

void MBotDragAccept(HDROP wParam);

static DWORD CALLBACK MBotESCallback(mbot_es* es,LPBYTE pbBuff,long cb,long *pcb)
{
	if(!es->ptr && es->cb == 0){
		es->ptr = es->buff[0];
		es->left = es->length[0];
	}

	if(!es->left){
		*pcb = 0;
		return 0;
	}

	if(es->left > cb){
		memcpy(pbBuff,es->ptr,cb);
		es->left -= cb;
		es->ptr += cb;
		*pcb = cb;
	}else{
		memcpy(pbBuff,es->ptr,es->left);
		*pcb = es->left;
		es->ptr += es->left;
		es->left = 0;
	}

	if(!es->left && es->cb < 3){
		es->ptr = es->buff[es->cb];
		es->left = es->length[es->cb++];
	}

	return 0;
}

void MBotConsoleAppendText(const char* txt,long formatted)
{
	GETTEXTLENGTHEX stGTL = {GTL_NUMCHARS,CP_ACP};
	EDITSTREAM es = {0};
	SCROLLINFO si = {0};
	CHARRANGE sel;
	mbot_es mbes = {0,0,0,{rtf_start,txt,rtf_stop},{strlen(rtf_start),strlen(txt),1}};
	long stTxtLength = 0;

	if(!(formatted=0)){
		mbes.cb = 3;
		mbes.ptr = mbes.buff[1];
		mbes.left = mbes.length[1];
	}

	if(!hConsole){
		return;
	}

	try{
		EnterCriticalSectionX(&csConsole);
	}catch(...){
		return;
	}

	stGTL.codepage = CP_ACP;
	stGTL.flags = GTL_PRECISE;

	SendMessage(hConsole,EM_HIDESELECTION, TRUE, 0);
	stTxtLength = SendMessage(hConsole,EM_GETTEXTLENGTHEX,(WPARAM)&stGTL,0);
	sel.cpMin = sel.cpMax = stTxtLength;
	SendMessage(hConsole,EM_EXSETSEL,0,(LPARAM) &sel);

	es.dwCookie = (DWORD)&mbes;
	es.pfnCallback = (EDITSTREAMCALLBACK)MBotESCallback;
	SendMessage(hConsole,EM_STREAMIN,(WPARAM)formatted?(SF_RTF | SFF_SELECTION):(SF_TEXT | SFF_SELECTION),(LPARAM)&es);


	SendMessage(hConsole,EM_HIDESELECTION, TRUE, 0);
	SendMessage(hConsole,EM_SETSEL,-1,0);
	SendMessage(hConsole,WM_KILLFOCUS,0,0);

	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_RANGE;
	GetScrollInfo(hConsole,SB_VERT, &si);
	si.fMask = SIF_POS;
	si.nPos = si.nMax - si.nPage + 1;
	SetScrollInfo(hConsole,SB_VERT, &si, TRUE);

	PostMessage(hConsole,WM_VSCROLL,SB_BOTTOM,0);
	LeaveCriticalSectionX(&csConsole);
}

void MBotConsoleClear()
{
	static mbot_es mbes = {0,0,0,{rtf_start,rtf_stop,0},{strlen(rtf_start),1,0}};
	EDITSTREAM es = {0};

	if(!hConsole){
		return;
	}

	try{
		EnterCriticalSectionX(&csConsole);
	}catch(...){
		return;
	}
	es.dwCookie = (DWORD)&mbes;
	es.pfnCallback = (EDITSTREAMCALLBACK)MBotESCallback;
	SendMessage(hConsole,EM_STREAMIN,(WPARAM)SF_RTF,(LPARAM)&es);
	LeaveCriticalSectionX(&csConsole);
}

void MBotDlgOnSize(HWND hWnd)
{
	RECT rc;
	long h;
	long tbh;

	GetClientRect(hToolbar,&rc);
	tbh = rc.bottom - rc.top;
	GetClientRect(hWnd,&rc);
	h = ((rc.bottom - rc.top)*72)/100 - tbh;

	SetWindowPos(hConsole,NULL,0,0,(rc.right - rc.left),h,SWP_NOZORDER);
	SetWindowPos(hToolbar,NULL,0,h+1,(rc.right - rc.left),tbh,SWP_NOZORDER);
	SetWindowPos(hCommandBox,NULL,0,h+tbh+1,(rc.right - rc.left),(rc.bottom - rc.top) - h - tbh - 1,SWP_NOZORDER);
}

static TBBUTTON tbb[]=
{
	{2,NULL,TBSTATE_ENABLED,TBSTYLE_SEP,NULL,NULL},
	{0,ID_EXECUTE,TBSTATE_ENABLED,TBSTYLE_BUTTON, NULL,NULL},
	{1,ID_CLEAR,TBSTATE_ENABLED,TBSTYLE_BUTTON, NULL,NULL},
	{2,ID_LINEWRAP,TBSTATE_ENABLED,TBSTYLE_BUTTON|TBSTYLE_CHECK, NULL,NULL},
	//{3,ID_MBHELP,TBSTATE_ENABLED,TBSTYLE_BUTTON, NULL,NULL},
	{4,ID_MBINSTALL,TBSTATE_ENABLED,TBSTYLE_DROPDOWN|BTNS_WHOLEDROPDOWN, NULL,NULL},
};

static char pszCommand[4096];

void WINAPI MBotDlgExecute(void* cut)
{
	static PHPR retval = PHPR_UNKNOWN;
	static char* cmd;
	static SYSTEMTIME st;
	static LONG len=0;

	string body;

	if(GetWindowTextLength(hCommandBox) >= sizeof(pszCommand)){
		MBotConsoleAppendText("Command too long!\r\n",1);
		SendDlgItemMessage(hDialog,IDC_TOOLBAR,TB_ENABLEBUTTON,ID_EXECUTE,TRUE);
		EnableWindow(hCommandBox,TRUE);
		bLocked = FALSE;
		return;
	}

	len = GetWindowText(hCommandBox,pszCommand,sizeof(pszCommand)-1);

	if(cut && len > 4){
		pszCommand[len - 4]='\0';
	}

	if(strncmp(pszCmdTag, pszCommand, lCmdTagLen)==0)
	{
		if(!(lh_flags & MB_EVENT_COMMAND))
		{
			MBotConsoleAppendText("No command handler registered!\r\n",1);
		}
		else
		{
			cmd = pszCommand + lCmdTagLen;
			while(*cmd && isspace(*cmd))cmd++;
			if(!(*cmd)){
				MBotConsoleAppendText("Command is empty!\r\n",1);
				goto End;
			}

			retval = MBCommandExecute(cmd,0,NULL);
			if(retval != PHPR_DROP && retval != PHPR_BREAK){
				MBotConsoleAppendText("Command not found!\r\n",1);
			}
			else
			{
				GetSystemTime(&st);

				/*body.Format("%.2d:%.2d:%.2d  \'%s%s\'\r\n",
					st.wHour,st.wMinute,st.wSecond,pszCmdTag,cmd);

				MBotConsoleAppendText(body,1);*/
				SetWindowText(hCommandBox, pszCmdTag);
			}
		}
	}
	else if(strncmp(pszPhpTag,pszCommand,lPhpTagLen)==0)
	{
		mb_event mbe = {MBT_CCSCRIPT,0,0};

		cmd = pszCommand + lPhpTagLen;
		while(*cmd && isspace(*cmd))cmd++;
		if(!(*cmd))goto End;

		body = "$cid=0;\r\n";
		body.append(cmd);

		if(LPHP_ExecuteDirect(body.data(), NULL, (void*)&mbe) == FALSE){
			MB_Popup(PHP_ERROR_TITLE,PHP_ERROR_EXECUTION);
		}else{
			SetWindowText(hCommandBox,"");
		}
	}else{
		MBotConsoleAppendText("Remember to use command/script tags!\r\n",1);
	}

End:
	SendDlgItemMessage(hDialog,IDC_TOOLBAR,TB_ENABLEBUTTON,ID_EXECUTE,TRUE);
	EnableWindow(hCommandBox,TRUE);
	bLocked = FALSE;
	return;
}

void MBotDlgSetWrap(long set)
{
	SendMessage(hConsole,EM_SETTARGETDEVICE,NULL,(set)?(0):(1));
}

LRESULT WINAPI MBotNewWndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static DWORD lr = 0;

	switch(uMsg)
	{
	case WM_KEYUP:
		if(wParam == VK_RETURN)
		{
			if(GetTickCount() - lr < 600){
				SendMessage(hDialog,WM_USER + 3,0,0);
			}else{
				lr = GetTickCount();
			}
		}else{
			if(wParam == VK_DOWN){
				PostMessage(hDialog,WM_USER + 5,0,0);
			}
			lr = 0;
		}
		break;
	case WM_DROPFILES:
		MBotDragAccept((HDROP)wParam);
		break;
	}
	return CallWindowProc(fpOldWndProc,hWnd, uMsg, wParam, lParam);
}

long MBotInstallScript(char* path)
{
	char* ff;
	char tmpp[MAX_PATH + 1];
	int length;

	length = strlen(path);
	ff = path + length - 4;

	while(ff != path && *ff!='\\' && *ff!='/'){
		ff--;
	}
	if(ff!=path)ff++;

	if(_snprintf(tmpp,MAX_PATH,"%s\\mbot\\scripts\\autoload\\%s",g_root,ff) < 0){
		MBotConsoleAppendText("File path is too long!\r\n",1);
		return 0;
	}

	if(help_fileexists(tmpp))
	{
		char msg[MAX_PATH + 64];
		_snprintf(msg,MAX_PATH + 63,"Following file already exists:\n'%s'\nOverwrite?",tmpp);
		if(MessageBox(NULL,msg,"MBot",MB_ICONQUESTION | MB_YESNO) == IDNO){
			return -1;
		}
	}

	if(!CopyFile(path,tmpp,FALSE))
	{
		MBotConsoleAppendText("Could not copy the file!\r\n",0);
		return 0;
	}
	else
	{
		if(DBGetContactSettingByte(NULL,MBOT,"LoadOnDrop",1)==1)
		{
			if(!help_loadscript(tmpp)){
				DeleteFile(tmpp);
				_snprintf(tmpp,MAX_PATH,"Could not load the script: %s\r\n",ff);
				
			}else{
				_snprintf(tmpp,MAX_PATH,"New script installed: %s\r\n",ff);
				MBHookEvents();
			}
			MBotConsoleAppendText(tmpp,1);
		}
		return 1;
	}
}

long MBotInstallExtension(char* path)
{
	char* ff;
	char tmpp[MAX_PATH + 1];
	int length;
	sXmlNode* node;

	length = strlen(path);
	ff = path + length - 4;

	while(ff != path && *ff!='\\' && *ff!='/'){
		ff--;
	}
	if(ff != path)ff++;

	if(*(DWORD*)ff != '_php'){
		MBotConsoleAppendText("This is not a valid php module!\r\n",1);
		return 0;
	}

	if(_snprintf(tmpp,MAX_PATH,"%s\\mbot\\extensions\\%s",g_root,ff) < 0){
		MBotConsoleAppendText("File path is too long!\r\n",1);
		return 0;
	}

	if(help_fileexists(tmpp))
	{
		char msg[MAX_PATH + 64];
		_snprintf(msg,MAX_PATH + 63,"Following file already exists:\n'%s'\nOverwrite?",tmpp);
		if(MessageBox(NULL,msg,"MBot",MB_ICONQUESTION | MB_YESNO) == IDNO){
			return -1;
		}
	}

	if(!CopyFile(path,tmpp,FALSE))
	{
		MBotConsoleAppendText("Could not copy the file!\r\n",0);
		return 0;
	}
	else
	{
		node = cSettings.GetNode("mbot/add_php",NULL);
		if(!node){
			MBotConsoleAppendText("Could not found appropriate node!\r\n",1);
			return 0;
		}

		strlwr(ff);
		node = cSettings.AddNewNode("extension",ff,node);
		if(!node){
			MBotConsoleAppendText("Could add a new node to the config file!\r\n",1);
			return 0;
		}

		_snprintf(tmpp,MAX_PATH,"New extension installed: %s\r\n",ff);
		MBotConsoleAppendText(tmpp,1);
		return 1;
	}
}

void MBotUpdateConfigFile()
{
	char path[MAX_PATH + 1]={0};
	_snprintf(path,MAX_PATH,"%s\\mbot\\config\\mbot.xml",g_root);
	cSettings.SaveToFile(path);
	_snprintf(path,MAX_PATH,"Extensions will be loaded next time you restart miranda\r\n");
	MBotConsoleAppendText(path,1);
}

void MBotDragAccept(HDROP wParam)
{
	static int count;
	static int ok1;
	static int ok2;
	static int length;
	char path[MAX_PATH + 1]={0};

	count = DragQueryFile((HDROP)wParam,0xFFFFFFFF,NULL,NULL);
	ok1 = 0;
	ok2 = 0;

	for(int i=0;i<count;i++)
	{
		if(!DragQueryFile((HDROP)wParam,i,path,MAX_PATH)){
			MBotConsoleAppendText("Could not accept the file!\r\n",1);
			continue;
		}
		length = strlen(path);

		if(length > 4 && *(DWORD*)(path + length - 4) == 'php.'){//script
			if(MBotInstallScript(path)==1)ok1++;
		}
		else if(length > 8 && *(DWORD*)(path + length - 4) == 'lld.'){//extension
			if(MBotInstallExtension(path)==1)ok2++;
		}
		if(ok2 > 0){
			MBotUpdateConfigFile();
		}
	}
}

INT_PTR WINAPI MBotDlgProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static POINT pt;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			TBADDBITMAP tbab;
			RECT rc;
			unsigned long tmp = 0;

			InitializeCriticalSectionAndSpinCount(&csConsole,0x80000100);
			hConsole = GetDlgItem(hWnd,IDC_MBCONSOLE);
			hCommandBox = GetDlgItem(hWnd,IDC_MBCOMMANDBOX);
			hImagelist = ImageList_Create(21,16,ILC_COLOR24|ILC_MASK,10,0);

			SendMessage(hWnd,WM_SETICON,ICON_SMALL,(LPARAM)hMBotIcon);

			hBitmap = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP1),
				IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);

			ImageList_AddMasked(hImagelist,hBitmap,RGB(255,255,255));

			hToolbar = CreateWindowEx(0,TOOLBARCLASSNAME,NULL,WS_CHILD | WS_VISIBLE |
				TBSTYLE_TOOLTIPS | CCS_NOPARENTALIGN | CCS_NODIVIDER | CCS_NORESIZE,
				0,0,100,26,hWnd,(HMENU)IDC_TOOLBAR,hInst,NULL);

			if(!hConsole || !hCommandBox || !hToolbar){
				return -1;
			}

			hVerdanaFont = CreateFont(13,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
				OUT_OUTLINE_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,0,"Verdana");

			hMonospacedFont = CreateFont(lConFontSize,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
				OUT_OUTLINE_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FIXED_PITCH,pszMonospacedFont);

			hMenu = LoadMenu(hInst,MAKEINTRESOURCE(IDR_MENU1));

			SendMessage(hConsole,EM_SETBKGNDCOLOR,FALSE,(LPARAM)(RGB(240,240,240)));

			SendMessage(hConsole,WM_SETFONT,(WPARAM)hMonospacedFont,0);
			SendMessage(hCommandBox,WM_SETFONT,(WPARAM)hMonospacedFont,0);

			tbab.hInst = NULL;
			tbab.nID = (INT_PTR)hBitmap;

			SendMessage(hToolbar,TB_SETIMAGELIST,(WPARAM)0,(LPARAM)hImagelist);
			SendMessage(hToolbar,TB_BUTTONSTRUCTSIZE,(WPARAM) sizeof(TBBUTTON), 0);

			SendMessage(hToolbar,TB_ADDBUTTONS,sizeof(tbb)/sizeof(TBBUTTON),(LPARAM)tbb);
			SendMessage(hToolbar,TB_SETEXTENDEDSTYLE,0,TBSTYLE_EX_DRAWDDARROWS);

			MBotConsoleClear();
			MBotConsoleAppendText("Miranda Scripting Plugin 0.0.3.2" "\r\nhttp://www.piopawlu.net\r\nBuilt on: "__TIMESTAMP__"\r\n",0);

			SendMessage(hConsole,EM_SETWORDWRAPMODE,WBF_WORDBREAK,0);

			fpOldWndProc = (WNDPROC)SetWindowLong(hCommandBox,GWL_WNDPROC,(LONG)MBotNewWndProc);
			DragAcceptFiles(hCommandBox,TRUE);

			if(DBGetContactSettingByte(NULL,MBOT,"Wrap",0)){
				SendMessage(hToolbar,TB_CHECKBUTTON,ID_LINEWRAP,TRUE);
				MBotDlgSetWrap(1);
			}

			tmp = DBGetContactSettingWord(NULL,MBOT,"wX",10);
			if(tmp >= (ULONG)GetSystemMetrics(SM_CXSCREEN))tmp = 0;
			rc.left = rc.right = tmp;

			tmp = DBGetContactSettingWord(NULL,MBOT,"wY",10);
			if(tmp >= (ULONG)GetSystemMetrics(SM_CYSCREEN))tmp = 0;
			rc.top = rc.bottom = tmp;

			tmp = DBGetContactSettingWord(NULL,MBOT,"wCx",400);
			if(tmp >= (ULONG)GetSystemMetrics(SM_CXSCREEN))tmp = 200;
			rc.right += tmp;

			tmp = DBGetContactSettingWord(NULL,MBOT,"wCy",300);
			if(tmp >= (ULONG)GetSystemMetrics(SM_CYSCREEN))tmp = 300;
			rc.bottom += tmp;

			MoveWindow(hWnd,rc.left,rc.top,rc.right - rc.left,rc.bottom - rc.top,FALSE);
			MBotDlgOnSize(hWnd);
		}
		return 1;
	case WM_CLOSE:
		ShowWindow(hWnd,SW_HIDE);
		return 1;
	case WM_GETMINMAXINFO:
		{
			MINMAXINFO* mmi = (MINMAXINFO*)lParam;
			mmi->ptMinTrackSize.x = 300;
			mmi->ptMinTrackSize.y = 200;
			return TRUE;
		}
		break;
	case WM_SIZE:
		MBotDlgOnSize(hWnd);
		return 1;
	case (WM_USER + 2):
		SendMessage(hToolbar,TB_CHECKBUTTON,ID_LINEWRAP,wParam);
		MBotDlgSetWrap(wParam);
		return 1;
	case (WM_USER + 3):
		{
			if(!bLocked){
				SendDlgItemMessage(hDialog,IDC_TOOLBAR,TB_ENABLEBUTTON,ID_EXECUTE,FALSE);
				EnableWindow(hCommandBox,FALSE);
				bLocked = TRUE;
				CallFunctionAsync(MBotDlgExecute,(void*)1);
			}
			return 1;
		}
	case (WM_USER + 5):
		{
			if(!bLocked && *pszCommand && GetWindowTextLength(hCommandBox) <= 1){
				SetWindowText(hCommandBox,pszCommand);
			}
			return 1;
		}
		break;
	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
			case ID_CLEAR:
				MBotConsoleClear();
				break;
			case ID_EXECUTE:
				{
					if(!bLocked)
					{
						SendDlgItemMessage(hDialog,IDC_TOOLBAR,TB_ENABLEBUTTON,ID_EXECUTE,FALSE);
						EnableWindow(hCommandBox,FALSE);
						bLocked = TRUE;
						CallFunctionAsync(MBotDlgExecute,NULL);
					}
				}
				break;
			case ID_MBHELP:
				{
					_snprintf(pszCommand,sizeof(pszCommand)-1,"%s\\mbot\\help\\mbot.chm",g_root);
					ShellExecute(NULL,"open","hh.exe",pszCommand,0,SW_SHOWNORMAL);
				}
				break;
			case ID_LINEWRAP:
				if(SendMessage(hToolbar,TB_ISBUTTONCHECKED,(ID_LINEWRAP),0))
				{
					DBWriteContactSettingByte(NULL,MBOT,"Wrap",1);
					MBotDlgSetWrap(1);
				}else{
					DBWriteContactSettingByte(NULL,MBOT,"Wrap",0);
					MBotDlgSetWrap(0);
				}
				break;
			case ID_POPUP2_INSTALLSCRIPT:
				{
					char* fn = (char*)help_getfilename(1,"PHP Files (*.php)\0*.php\0\0",NULL);
					if(!fn)break;
					MBotInstallScript(fn);
				}
				break;
			case ID_POPUP2_INSTALLEXTENSION:
				{
					char* fn = (char*)help_getfilename(1,"PHP Extensions (php_*.dll)\0php_*.dll\0\0",NULL);
					if(!fn)break;
					if(MBotInstallExtension(fn)){
						MBotUpdateConfigFile();
					}
				}
				break;
			}
		}
		break;
	case WM_DESTROY:
		{
			MBLOGEX("WM_DESTROY START");
			RECT rc;

			if(help_iswin2k())
			{
				MBLOGEX("GetWindowRect...");
				GetWindowRect(hWnd,&rc);
				MBLOGEX("DBWriteContactSettingWord (wCx)");
				DBWriteContactSettingWord(NULL,MBOT,"wCx",(WORD)(rc.right - rc.left));
				MBLOGEX("DBWriteContactSettingWord (wCy)");
				DBWriteContactSettingWord(NULL,MBOT,"wCy",(WORD)(rc.bottom - rc.top));
				MBLOGEX("DBWriteContactSettingWord (wX)");
				DBWriteContactSettingWord(NULL,MBOT,"wX",(WORD)rc.left);
				MBLOGEX("DBWriteContactSettingWord (wY)");
				DBWriteContactSettingWord(NULL,MBOT,"wY",(WORD)rc.top);
			}

			MBLOGEX("Destroying toolbar...");
			DestroyWindow(hToolbar);
			MBLOGEX("Destroying toolbar icons...");
			DeleteObject((HGDIOBJ)hBitmap);
			MBLOGEX("Destroying toolbar images...");
			DeleteObject((HGDIOBJ)hImagelist);
			MBLOGEX("Destroying fonts...");
			DeleteObject((HGDIOBJ)hVerdanaFont);
			DeleteObject((HGDIOBJ)hMonospacedFont);
			MBLOGEX("WM_DESTROY END");
		}
		return 1;
	case WM_NOTIFY:
		{
			#define lpnm   ((LPNMHDR)lParam)

			if(wParam == IDC_MBCOMMANDBOX)
			{
				LPNMHDR nmhdr = (LPNMHDR)lParam;
				if(nmhdr->code == EN_DROPFILES){
					return 1;
				}
			}
			else if(lpnm->code == TTN_GETDISPINFO)
			{
				NMTTDISPINFO* tt = (NMTTDISPINFO*)lParam;
				switch(tt->hdr.idFrom)
				{
				case ID_CLEAR: tt->lpszText = "Clear the console window"; break;
				case ID_MBHELP: tt->lpszText = "Opens the help file"; break;
				case ID_EXECUTE: tt->lpszText = "Executes the command or a piece of script entered in the box below"; break;
				case ID_LINEWRAP: tt->lpszText = "Changes the output wrap mode (on/off)"; break;
				case ID_MBINSTALL: tt->lpszText = "Install new script or php extension"; break;
				default:
					return FALSE;
				}
				return TRUE;
			}else if(lpnm->code == TBN_DROPDOWN){
				
				GetCursorPos(&pt);
				TrackPopupMenu(GetSubMenu(hMenu,1),
					TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL,
					pt.x,pt.y,0,hWnd,NULL);

				return FALSE;
			}
		}
		break;
	default:
		break;
	}
	return(FALSE);
}

int MBotGetPrefString(const char *szSetting, char* out, int len, char* def)
{
	DBVARIANT dbv;
	if((DBGetContactSetting(NULL, MBOT,szSetting, &dbv) == FALSE) && dbv.type==DBVT_ASCIIZ)
	{
		strncpy(out,dbv.pszVal,len);
		DBFreeVariant(&dbv);
		return 1;
	}
	else
	{
		strncpy(out,def,len);
		return FALSE;
	}
}

int MBotDlgFillHandlers(HWND hList,int* sch)
{
	extern CSECTION	sm_sync;
	extern CSyncList g_cron;
	PHANDLER ph;
	PPHP php;
	sCronSync* cs;
	LVITEM li={0};
	char temp[MAX_PATH];
	long n = 1;
	long it = 0;
	long adv = DBGetContactSettingByte(NULL,MBOT,"AdvConfig",0);

	if(!hConsole)return 0;

	ListView_DeleteAllItems(hList);

	if(!adv)
	{
		sm_list.Lock();
		php = (PPHP)sm_list.m_head;
		while(php)
		{
			li.pszText = strrchr(php->szFilePath,'/');
			if(!li.pszText)li.pszText = php->szFilePath;

			li.lParam = (LPARAM)php;
			li.iSubItem = 0;
			li.iItem = n;
			li.mask = LVIF_PARAM | LVIF_TEXT;
			it = ListView_InsertItem(hList,&li);

			li.pszText = php->szDescription?php->szDescription:"no description";
			li.mask = LVIF_TEXT;
			li.iItem = it;
			li.iSubItem = 1;
			ListView_SetItem(hList,&li);

			php = (PPHP)php->next;
		}
		sm_list.Unlock();
		return 0;
	}

	EnterCriticalSectionX(&sm_sync);

	for(int i=0;i<32;i++)
	{
		ph = sman_handler_get(1 << i);
		while(ph)
		{
			if(!(ph->lFlags & MBOT_FLAG_DELETE))
			{
				_snprintf(temp,sizeof(temp)-1,"%.2u",n++);
				li.pszText = temp;
				li.lParam = (LPARAM)ph;
				li.iSubItem = 0;
				li.iItem = n;
				li.mask = LVIF_PARAM | LVIF_TEXT;
				it = ListView_InsertItem(hList,&li);
				if(it!=-1)
				{
					li.mask = LVIF_TEXT;
					li.pszText = ph->szFcnName + 4;
					li.iItem = it;
					li.iSubItem = 1;
					ListView_SetItem(hList,&li);

					li.pszText = (ph->lFlags & MBOT_FLAG_INACTIVE)?("no"):("yes");
					if(ph->php->lFlags & MBOT_FLAG_SHALLDIE)li.pszText = "DEL";

					li.iSubItem = 2;
					ListView_SetItem(hList,&li);

					li.pszText = (ph->php->szBuffered)?("yes"):("no");
					li.iSubItem = 3;
					ListView_SetItem(hList,&li);

					li.pszText = strrchr(ph->php->szFilePath,'/');
					if(!li.pszText)li.pszText = ph->php->szFilePath;

					li.iSubItem = 4;
					ListView_SetItem(hList,&li);

				}
			}
			ph = ph->next;
		}
	}
	///////////////////////////////
	*sch = n;
	///////////////////////////////
	li.iSubItem = 0;
	li.iItem = n;
	li.lParam = NULL;
	li.pszText = "---";
	it = ListView_InsertItem(hList,&li);
	if(it != -1){
		li.mask = LVIF_TEXT;
		li.pszText = "#scheduler#";
		li.iItem = it;
		li.iSubItem = 1;
		ListView_SetItem(hList,&li);

		li.pszText = "---";
		li.iSubItem = 2;
		ListView_SetItem(hList,&li);
		li.iSubItem = 3;
		ListView_SetItem(hList,&li);
		li.iSubItem = 4;
		ListView_SetItem(hList,&li);
	}
	///////////////////////////////
	g_cron.Lock();
	cs = (sCronSync*)g_cron.m_head;
	while(cs)
	{
		if(!(cs->lFlags & MBOT_FLAG_DELETE))
		{
			_snprintf(temp,sizeof(temp)-1,"%.2u",n++);
			li.pszText = temp;
			li.lParam = (LPARAM)cs;
			li.iSubItem = 0;
			li.iItem = n;
			li.mask = LVIF_PARAM | LVIF_TEXT;
			it = ListView_InsertItem(hList,&li);
			if(it!=-1)
			{
				li.mask = LVIF_TEXT;
				li.pszText = cs->name;
				li.iItem = it;
				li.iSubItem = 1;
				ListView_SetItem(hList,&li);

				li.pszText = (cs->lFlags & MBOT_FLAG_INACTIVE)?("no"):("yes");
				li.iSubItem = 2;
				ListView_SetItem(hList,&li);

				li.pszText = (cs->data->szBuffered)?("yes"):("no");
				if(cs->data->lFlags & MBOT_FLAG_SHALLDIE)li.pszText = "DEL";
				li.iSubItem = 3;
				ListView_SetItem(hList,&li);

				_snprintf(temp,sizeof(temp)-1,"%s\\mbot\\scripts\\autoload",g_root);
				if(strncmp(cs->data->szFilePath,temp,strlen(temp))==0){
					_snprintf(temp,sizeof(temp)-1,"%s",cs->data->szFilePath + strlen(temp));
					li.pszText = temp;
				}else{
					li.pszText = (cs->data->szFilePath);
				}
				li.iSubItem = 4;
				ListView_SetItem(hList,&li);
			}
		}
		cs = (sCronSync*)cs->next;
	}
	g_cron.Unlock();
	LeaveCriticalSectionX(&sm_sync);
	return 0;
}

void xx_configure_script(PPHP php)
{
	mb_event mbe={MBT_CALLBACK,0,0};
	mbe.php = php;
	mbe.event = MBT_CALLBACK;
	MBMultiParam(php,"mbe_config",&mbe,NULL);
}

INT_PTR WINAPI MBotDlgProcOption(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static int tmp = 0;
	static char stmp[64];
	static int initialized = 0;
	static HWND  hList;
	static PHANDLER phc;
	static sCronSync* cs;
	static int sch_id;
	static int adv = 0;
	static int type;

	static PPHP xphp = NULL;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			LVCOLUMN lvc = {0};

			phc = NULL;
			initialized = 0;
			type = 0;
			TranslateDialogDefault(hWnd);

			tmp = DBGetContactSettingByte(NULL,MBOT,"Enable",1);
			SendDlgItemMessage(hWnd,IDC_MBENABLE,BM_SETCHECK,(tmp)?(BST_CHECKED):(BST_UNCHECKED),0);
			tmp = DBGetContactSettingByte(NULL,MBOT,"NoCache",0);
			SendDlgItemMessage(hWnd,IDC_MBDISABLECACHE,BM_SETCHECK,(tmp)?(BST_CHECKED):(BST_UNCHECKED),0);
			tmp = DBGetContactSettingByte(NULL,MBOT,"NoEvents",0);
			SendDlgItemMessage(hWnd,IDC_MBDISABLEEVENTS,BM_SETCHECK,(tmp)?(BST_CHECKED):(BST_UNCHECKED),0);
			tmp = DBGetContactSettingByte(NULL,MBOT,"SWOnStartup",0);
			SendDlgItemMessage(hWnd,IDC_MBSHOWCONSOLE,BM_SETCHECK,(tmp)?(BST_CHECKED):(BST_UNCHECKED),0);
			tmp = DBGetContactSettingByte(NULL,MBOT,"SWOnError",0);
			SendDlgItemMessage(hWnd,IDC_MBSHOWCONSOLEERROR,BM_SETCHECK,(tmp)?(BST_CHECKED):(BST_UNCHECKED),0);
			tmp = DBGetContactSettingByte(NULL,MBOT,"Wrap",0);
			SendDlgItemMessage(hWnd,IDC_WRAPCONSOLE,BM_SETCHECK,(tmp)?(BST_CHECKED):(BST_UNCHECKED),0);
			tmp = DBGetContactSettingByte(NULL,MBOT,"NoScheduler",0);
			SendDlgItemMessage(hWnd,IDC_DISABLESCHEDULER,BM_SETCHECK,(tmp)?(BST_CHECKED):(BST_UNCHECKED),0);
			tmp = DBGetContactSettingByte(NULL,MBOT,"WWWEnabled",1);
			SendDlgItemMessage(hWnd,IDC_ENABLEWWW,BM_SETCHECK,(tmp)?(BST_CHECKED):(BST_UNCHECKED),1);
			tmp = DBGetContactSettingByte(NULL,MBOT,"WWWLog",1);
			SendDlgItemMessage(hWnd,IDC_ENABLEADVOPT,BM_SETCHECK,(tmp)?(BST_CHECKED):(BST_UNCHECKED),1);
			adv = DBGetContactSettingByte(NULL,MBOT,"AdvConfig",0);
			SendDlgItemMessage(hWnd,IDC_ENABLEADVOPT,BM_SETCHECK,(adv)?(BST_CHECKED):(BST_UNCHECKED),1);


			MBotGetPrefString("CmdTag",stmp,2,"m>");
			SetDlgItemText(hWnd,IDC_ADVCMDTAG,stmp);

			MBotGetPrefString("ScriptTag",stmp,2,"?>");
			SetDlgItemText(hWnd,IDC_ADVSCRIPTTAG,stmp);

			hList = GetDlgItem(hWnd,IDC_LIST1);
			if(!hList)return 0;

			sm_list.Lock();
			xphp = (PPHP)sm_list.m_head;
			while(xphp){
				sman_inc(xphp);
				xphp = (PPHP)xphp->next;
			}
			sm_list.Unlock();

			if(adv)
			{
				lvc.cx = 25;
				lvc.fmt = LVCFMT_CENTER;
				lvc.pszText = "#";
				lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
				ListView_InsertColumn(hList,0,&lvc);

				lvc.cx = 100;
				lvc.pszText = "Event";
				lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
				ListView_InsertColumn(hList,1,&lvc);

				lvc.cx = 60;
				lvc.pszText = "Active";
				ListView_InsertColumn(hList,2,&lvc);

				lvc.cx = 60;
				lvc.pszText = "Cached";
				ListView_InsertColumn(hList,3,&lvc);

				lvc.cx = 150;
				lvc.pszText = "File";
				lvc.fmt = LVCFMT_LEFT;
				ListView_InsertColumn(hList,4,&lvc);

				ListView_SetExtendedListViewStyle(hList,LVS_EX_FULLROWSELECT);
				MBotDlgFillHandlers(hList,&sch_id);
				SetDlgItemText(hWnd,IDC_ADV2,Translate("Registered handlers"));
			}else{
				lvc.cx = 100;
				lvc.pszText = "File";
				lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
				lvc.fmt = LVCFMT_LEFT;
				ListView_InsertColumn(hList,0,&lvc);

				lvc.cx = 300;
				lvc.pszText = "Description";
				ListView_InsertColumn(hList,1,&lvc);

				ListView_SetExtendedListViewStyle(hList,LVS_EX_FULLROWSELECT);
				MBotDlgFillHandlers(hList,&sch_id);
				SetDlgItemText(hWnd,IDC_ADV2,Translate("Installed scripts (double click to configure)"));
			}
			SendDlgItemMessage(hWnd,IDC_ADVSCRIPTTAG,EM_LIMITTEXT,2,0);
			SendDlgItemMessage(hWnd,IDC_ADVCMDTAG,EM_LIMITTEXT,2,0);
			initialized = 1;
		}
		return TRUE;
	case WM_NOTIFY:
		if(((LPNMHDR)lParam)->idFrom == 0 && ((LPNMHDR)lParam)->code == PSN_APPLY)
		{
			tmp = IsDlgButtonChecked(hWnd,IDC_MBENABLE);
			DBWriteContactSettingByte(NULL,MBOT,"Enable",(tmp)?(1):(0));
			tmp = IsDlgButtonChecked(hWnd,IDC_MBDISABLECACHE);
			DBWriteContactSettingByte(NULL,MBOT,"NoCache",(tmp)?(1):(0));
			tmp = IsDlgButtonChecked(hWnd,IDC_MBDISABLEEVENTS);
			DBWriteContactSettingByte(NULL,MBOT,"NoEvents",(tmp)?(1):(0));
			tmp = IsDlgButtonChecked(hWnd,IDC_MBSHOWCONSOLE);
			DBWriteContactSettingByte(NULL,MBOT,"SWOnStartup",(tmp)?(1):(0));
			tmp = IsDlgButtonChecked(hWnd,IDC_MBSHOWCONSOLEERROR);
			DBWriteContactSettingByte(NULL,MBOT,"SWOnError",(tmp)?(1):(0));
			tmp = IsDlgButtonChecked(hWnd,IDC_DISABLESCHEDULER);
			DBWriteContactSettingByte(NULL,MBOT,"NoScheduler",(tmp)?(1):(0));
			tmp = IsDlgButtonChecked(hWnd,IDC_ENABLEWWW);
			DBWriteContactSettingByte(NULL,MBOT,"WWWEnabled",(tmp)?(1):(0));
			tmp = IsDlgButtonChecked(hWnd,IDC_ENABLEWWW2);
			DBWriteContactSettingByte(NULL,MBOT,"WWWLog",(tmp)?(1):(0));

			tmp = IsDlgButtonChecked(hWnd,IDC_WRAPCONSOLE);
			DBWriteContactSettingByte(NULL,MBOT,"Wrap",(tmp)?(1):(0));
			PostMessage(hDialog,WM_USER + 2,tmp,0);

			tmp = GetDlgItemText(hWnd,IDC_ADVCMDTAG,stmp,64);
			if(!tmp || tmp>2){
				SetDlgItemText(hWnd,IDC_ADVCMDTAG,"m>");
			}else{
				strncpy(pszCmdTag,stmp,tmp+1);
				lCmdTagLen = tmp;
				DBWriteContactSettingString(NULL,MBOT,"CmdTag",pszCmdTag);
			}

			tmp = GetDlgItemText(hWnd,IDC_ADVSCRIPTTAG,stmp,64);
			if(!tmp || tmp>2){
				SetDlgItemText(hWnd,IDC_ADVSCRIPTTAG,"?>");
			}else{
				strncpy(pszPhpTag,stmp,tmp+1);
				lPhpTagLen = tmp;
				DBWriteContactSettingString(NULL,MBOT,"ScriptTag",pszPhpTag);
			}
			return (TRUE);
		}
		else if(((LPNMHDR)lParam)->code == NM_DBLCLK && wParam == IDC_LIST1)
		{
			LVITEM lvi = {0};

			lvi.mask = LVIF_PARAM;
			lvi.iItem = ((LPNMLISTVIEW)lParam)->iItem;
			if(!ListView_GetItem(GetDlgItem(hWnd,IDC_LIST1),&lvi) || !lvi.lParam)break;
			xphp = (PPHP)lvi.lParam;

			if(xphp && sman_handler_find(xphp,MB_EVENT_CONFIG)){
				xx_configure_script(xphp);
			}else{
				MessageBox(hList,"No configuration available","MSP",MB_ICONINFORMATION);
			}
		}
		else if(((LPNMHDR)lParam)->code == NM_RCLICK && wParam == IDC_LIST1)
		{
			LVITEM lvi = {0};
			POINT pt;
			LONG  lFlags;
			PPHP  php;
			lvi.mask = LVIF_PARAM;
			lvi.iItem = ((LPNMLISTVIEW)lParam)->iItem;

			if(!ListView_GetItem(GetDlgItem(hWnd,IDC_LIST1),&lvi) || !lvi.lParam)break;
			GetCursorPos(&pt);

			if(!adv){
				phc = NULL;
				cs = NULL;
				xphp = (PPHP)lvi.lParam;
				lFlags = xphp->lFlags;

				if(lFlags & (MBOT_FLAG_INACTIVE|MBOT_FLAG_SHALLDIE|MBOT_FLAG_DELETE)){
					EnableMenuItem(hMenu,ID_POPUP3_ENABLE,MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu,ID_POPUP3_DISABLE,MF_BYCOMMAND | MF_GRAYED);
				}else{
					EnableMenuItem(hMenu,ID_POPUP3_DISABLE,MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu,ID_POPUP3_ENABLE,MF_BYCOMMAND | MF_GRAYED);
				}

				if(sman_handler_find(xphp,MB_EVENT_CONFIG)){
					EnableMenuItem(hMenu,ID_POPUP3_CONFIGURE,MF_BYCOMMAND | MF_ENABLED);
				}else{
					EnableMenuItem(hMenu,ID_POPUP3_CONFIGURE,MF_BYCOMMAND | MF_GRAYED);
				}

			}else{
				xphp = NULL;
				if(lvi.iItem < sch_id){
					phc = (PHANDLER)lvi.lParam;
					lFlags = phc->lFlags;
					php = phc->php;
					cs = NULL;
				}else{
					cs = (sCronSync*)lvi.lParam;
					lFlags = cs->lFlags;
					php = cs->data;
					phc = NULL;
				}

				if(lFlags & MBOT_FLAG_INACTIVE){
					EnableMenuItem(hMenu,ID_POPUP_ACTIVATE,MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu,ID_POPUP_DEACTIVATE,MF_BYCOMMAND | MF_GRAYED);
				}else{
					EnableMenuItem(hMenu,ID_POPUP_DEACTIVATE,MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu,ID_POPUP_ACTIVATE,MF_BYCOMMAND | MF_GRAYED);
				}

				if(!(php->szBuffered)){
					EnableMenuItem(hMenu,ID_POPUP_UNCACHE,MF_BYCOMMAND | MF_GRAYED);
				}else{
					EnableMenuItem(hMenu,ID_POPUP_UNCACHE,MF_BYCOMMAND | MF_ENABLED);
				}
			}
		
			TrackPopupMenu(GetSubMenu(hMenu,adv?0:2),0,pt.x,pt.y,0,hWnd,NULL);
		}
		break;
	case WM_COMMAND:
		{
			tmp = 0;
			switch(LOWORD(wParam))
			{
			case IDC_MBENABLE:
			case IDC_MBDISABLECACHE:
			case IDC_ENABLEWWW:
			case IDC_ENABLEWWW2:
				MessageBox(NULL,"This option requires Miranda restart!","MBot",MB_ICONINFORMATION);
			case IDC_MBDISABLEEVENTS:
			case IDC_MBSHOWCONSOLE:
			case IDC_MBSHOWCONSOLEERROR:
			case IDC_WRAPCONSOLE:
				SendMessage(GetParent(hWnd),PSM_CHANGED,0,0);
				break;
			case IDC_ADVCMDTAG:
			case IDC_ADVSCRIPTTAG:
				if(HIWORD(wParam) == EN_CHANGE && initialized==1){
					SendMessage(GetParent(hWnd),PSM_CHANGED,0,0);
				}
				break;
			case ID_ACTIVATE_EVENT:
				if(phc){
					sman_handler_enable(phc);
				}else if(cs){
					cron_enable(cs->name,1);
				}
				MBotDlgFillHandlers(hList,&sch_id);
				break;
			case ID_DEACTIVATE_EVENT:
				if(phc){
					sman_handler_disable(phc);
				}else if(cs){
					cron_enable(cs->name,0);
				}
				MBotDlgFillHandlers(hList,&sch_id);
				break;
			case ID_POPUP3_ENABLE:
			case ID_ACTIVATE_FILE:
				tmp = 1;
			case ID_POPUP3_DISABLE:
			case ID_DEACTIVATE_FILE:
				xphp = (!adv)?(xphp):((phc)?(phc->php):(cs->data));
				sman_dec(xphp);
				help_enable_script(xphp,tmp==1);
				sman_inc(xphp);
				MBotDlgFillHandlers(hList,&sch_id);
				break;
			case ID_POPUP_UNCACHE:
				{
					PPHP php = (phc)?(phc->php):(cs->data);
					sman_dec(php);
					if(!sman_uncache(php)){
						sman_inc(php);
						MessageBox(hWnd,"File is in use!",MBOT,MB_ICONINFORMATION);
					}else{
						sman_inc(php);
						MBotDlgFillHandlers(hList,&sch_id);
					}
				}
				break;
			case ID_POPUP_RECACHE:
				{
					PPHP php = (phc)?(phc->php):(cs->data);
					sman_dec(php);
					if(!sman_recache(php)){
						sman_inc(php);
						MessageBox(hWnd,"File is in use or could not cache the file!",MBOT,MB_ICONINFORMATION);
					}else{
						sman_inc(php);
					}
					MBotDlgFillHandlers(hList,&sch_id);
				}
				break;
			case ID_POPUP_UNLOAD:
				{
					PPHP php = (phc)?(phc->php):(cs->data);
					sman_dec(php);
					if(!sman_uninstall(php,0)){
						sman_inc(php);
						MessageBox(hWnd,"File is in use or could not find the file!",MBOT,MB_ICONINFORMATION);
					}else{
						MBotDlgFillHandlers(hList,&sch_id);
					}
				}
				break;
			case ID_POPUP3_CONFIGURE:
				if(xphp){
					xx_configure_script(xphp);
				}
				break;
			case ID_POPUP3_UNINSTALL:
			case ID_POPUP_UNINSTALL:
				{
					PPHP php = (!adv)?(xphp):((phc)?(phc->php):(cs->data));

					if(MessageBox(hWnd,"Are you sure you want to unload selected script?",
						MBOT,MB_ICONQUESTION | MB_YESNO)==IDYES)
					{
						if(!sman_uninstall(php,1)){
							MessageBox(hWnd,"File is in use or could not find the file!",MBOT,MB_ICONINFORMATION);
						}else{
							MBotDlgFillHandlers(hList,&sch_id);
						}
					}
				}
				break;
			default:
				break;
			}
		}
		break;
	case WM_DESTROY:
		tmp = IsDlgButtonChecked(hWnd,IDC_ENABLEADVOPT);
		DBWriteContactSettingByte(NULL,MBOT,"AdvConfig",(tmp)?(1):(0));

		sm_list.Lock();
		xphp = (PPHP)sm_list.m_head;
		while(xphp){
			sman_dec(xphp);
			xphp = (PPHP)xphp->next;
		}
		sm_list.Unlock();
		hList = NULL;
		break;
	default:
		break;
	}
	return(FALSE);
}