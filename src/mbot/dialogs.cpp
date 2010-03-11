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
#include "helpers.h"
#include "functions.h"
#include "dialogs.h"

extern CSyncList g_dlist;


LPCSTR DlgGetCtrlClass(long type)
{
	const static char* classes[]={
		WC_STATIC,//1
		WC_BUTTON,//2
		WC_EDIT,//3
		WC_COMBOBOX,//4
		WC_IPADDRESS,//5
		WC_LISTVIEW,//6
		PROGRESS_CLASS,//7
		WC_TREEVIEW,//8
		};

	type &= 0x7FffFFff;

	if(type > (sizeof(classes) / sizeof(char*))){
		return "STATIC";
	}else{
		return classes[type-1];
	}
}

VOID DlgFree(sDialog* dlg)
{
	if(dlg){
		while(dlg->lNum)
		{
			//DestroyWindow(dlg->table[dlg->lNum-1]->hWnd);
			my_memfree(dlg->table[dlg->lNum-1]);
			dlg->table[dlg->lNum-1] = NULL;
			dlg->lNum--;
		}
		my_memfree(dlg);
		//memset(dlg,0xAA,sizeof(sDialog));
	}
}

BOOL CALLBACK DlgStdInProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			SetWindowLong(hDlg,GWL_USERDATA,(LONG)lParam);
			sStdInDlg* sin = (sStdInDlg*)lParam;
			if(!sin){
				EndDialog(hDlg,IDCANCEL);
				return (TRUE);
			}

			SetWindowText(hDlg,(sin->title)?(sin->title):("Enter some data"));
			SetDlgItemText(hDlg,IDC_GS_INFO,(sin->info)?(sin->info):("Please enter some data:"));

			if(sin->flags & sStdInDlg::NUMBER_ONLY){
				SetWindowLong(GetDlgItem(hDlg,IDC_GS_EDIT),GWL_STYLE,ES_NUMBER);
			}

			if(sin->flags & sStdInDlg::NO_CANCEL){
				ShowWindow(GetDlgItem(hDlg,IDCANCEL),SW_HIDE);
			}

			if(sin->def){
				SetDlgItemText(hDlg,IDC_GS_EDIT,sin->def);
			}
			return (TRUE);
		}
	case WM_COMMAND:
		{
			UINT uid = LOWORD(wParam);
			UINT ncode = HIWORD(wParam);
			sStdInDlg* sin = (sStdInDlg*)GetWindowLong(hDlg,GWL_USERDATA);
			if(uid == IDOK)
			{
				//do checking
				ncode = GetDlgItemText(hDlg,IDC_GS_EDIT,sin->buffer,sizeof(sin->buffer)-1);
				if(!ncode && (sin->flags & sStdInDlg::MUST_FILL)){
					MessageBox(hDlg,"You must type in some data!",sin->title,MB_ICONWARNING);
					break;
				}
				EndDialog(hDlg,IDOK);
			}else if(uid == IDCANCEL){
				if(sin->flags & sStdInDlg::MUST_FILL){
					MessageBox(hDlg,"You must type in some data!",sin->title,MB_ICONWARNING);
					break;
				}else{
					EndDialog(hDlg,IDCANCEL);
				}
			}
		}
		break;
	default:
		return (FALSE);
	}
	return (TRUE);
}

BOOL CALLBACK DlgProcedure(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	sDialog* dlg = (sDialog*)GetWindowLong(hDlg,GWL_USERDATA);
	PHPR res;

	if((uMsg != WM_INITDIALOG && uMsg != (WM_USER + 2)) && (!dlg || (dlg->hDlg != hDlg) || (dlg->lDepth > 0) || !(dlg->lFlags))){
		return (FALSE);
	}

	if(dlg){dlg->lDepth++;}

	switch(uMsg)
	{
	case WM_INITDIALOG:
		SetWindowLong(hDlg,GWL_USERDATA,lParam);
		dlg = (sDialog*)lParam;
		sman_inc(dlg->php);
		return (TRUE);
	case (WM_USER + 2):
		{
			RECT rc;
			RECT drc;
			HWND hButton;
			//move the OK & CANCEL buttons
			GetClientRect(hDlg,&drc);

			hButton = GetDlgItem(hDlg,IDCANCEL);
			GetClientRect(hButton,&rc);
			SetWindowPos(hButton,NULL,drc.right - 10 - 2*(rc.right - rc.left),drc.bottom - 5 - (rc.bottom - rc.top),
				0,0,SWP_NOZORDER | SWP_NOSIZE);

			hButton = GetDlgItem(hDlg,IDOK);
			GetClientRect(hButton,&rc);
			SetWindowPos(hButton,NULL,drc.right - 5 - (rc.right - rc.left),
				drc.bottom - 5 - (rc.bottom - rc.top),0,0,SWP_NOZORDER | SWP_NOSIZE);
		}
		break;
	case (WM_USER + 3):
		DestroyWindow(hDlg);
		sman_dec(dlg->php);
		DlgFree(dlg);
		SetWindowLong(hDlg,GWL_USERDATA,0);
		return (TRUE);
	case WM_CLOSE:
		wParam = IDCANCEL;
		lParam = (LPARAM)GetDlgItem(dlg->hDlg,IDCANCEL);
	case WM_COMMAND:
		{
			UINT uid = LOWORD(wParam);
			UINT ncode = HIWORD(wParam); 
			if((uid == IDOK || uid == IDCANCEL) && ncode == BN_CLICKED)
			{
				mb_event mbe={MBT_DIALOG,wParam,lParam,0};
				mbe.php = dlg->php;
				mbe.t3 = MBE_DIALOGCB;
				mbe.p3 = (void*)dlg;

				res = (PHPR)MBMultiParam(mbe.php,dlg->pszCallback,&mbe,"lll",(uid==IDOK),dlg->param,0);
				if(res == PHPR_BREAK || res == PHPR_END){
					PostMessage(hDlg,WM_USER + 3,0,0);
				}
				break;
			}
			else if(uid >= 1000 && uid < (dlg->lNum + 1000))
			{
				sDlgControl* dc = dlg->table[uid - 1000];
				ULONG tmp = GetTickCount(); 
				if(((uid == dlg->lid) && (tmp - dlg->ltc < 250)) || dc->pszCallback[0]=='\0'){
					break;
				}

				mb_event mbe={MBT_DIALOG,wParam,lParam,0};
				mbe.php = dlg->php;
				mbe.t3 = MBE_DIALOGCB;
				mbe.p3 = (void*)dlg;
				mbe.lFlags = MBOT_FLAG_NOOUTPUT;

				if(dc->type == 2 && ncode==BN_CLICKED){//button
					res = (PHPR)MBMultiParam(dlg->php,dc->pszCallback,&mbe,"ll",uid,dc->param);
					if(res == PHPR_END){
						PostMessage(hDlg,WM_USER + 3,0,0);
					}
				}
				else if(dc->type == 4 && ncode == CBN_SELENDOK)
				{
					res = (PHPR)MBMultiParam(dlg->php,dc->pszCallback,&mbe,"ll",uid,dc->param);
					if(res == PHPR_END){
						PostMessage(hDlg,WM_USER + 3,0,0);
					}
				}
				else if(*dlg->pszWmCommand)
				{
					res = (PHPR)MBMultiParam(dlg->php,dlg->pszWmCommand,&mbe,"llll",uid,dc->param,wParam,lParam);
					if(res == PHPR_END){
						PostMessage(hDlg,WM_USER + 3,0,0);
					}
				}
				dlg->lid = uid;
				dlg->ltc = GetTickCount();
			}
			break;
		}
		break;
	case WM_NOTIFY:
		if(wParam >= 1000 && (wParam < (dlg->lNum + 1000)) && 
			(((LPNMHDR)lParam)->code == NM_RCLICK || ((LPNMHDR)lParam)->code == NM_CLICK))
		{
			LVITEM lvi = {0};
			sDlgControl* dc = dlg->table[wParam - 1000];

			if(!dc || dc->type != 6 || !(*dc->pszCallback))break;

			mb_event mbe={MBT_DIALOG,wParam,lParam,0};
			mbe.php = dlg->php;
			mbe.t3 = MBE_DIALOGCB;
			mbe.p3 = (void*)dlg;
			mbe.lFlags = MBOT_FLAG_NOOUTPUT;

			res = (PHPR)MBMultiParam(dlg->php,dc->pszCallback,&mbe,"llll",wParam,dc->param,
				((LPNMLISTVIEW)lParam)->iItem,
				((LPNMHDR)lParam)->code == NM_RCLICK);

			if(res == PHPR_END){
				PostMessage(hDlg,WM_USER + 3,0,0);
			}
		}
		else if(*dlg->pszWmNotify && ((LPNMHDR)lParam)->idFrom >= 1000)
		{
			mb_event mbe={MBT_DIALOG,wParam,lParam,0};
			mbe.php = dlg->php;
			mbe.t3 = MBE_DIALOGCB;
			mbe.p3 = (void*)dlg;
			mbe.lFlags = MBOT_FLAG_NOOUTPUT;

			res = (PHPR)MBMultiParam(dlg->php,dlg->pszWmNotify,&mbe,"llll",((LPNMHDR)lParam)->idFrom,
				dlg->param,wParam,lParam);

			if(res == PHPR_END){
				PostMessage(hDlg,WM_USER + 3,0,0);
			}
		}
		break;
	case WM_TIMER:
		{
			if(*dlg->pszWmTimer){
				mb_event mbe={MBT_DIALOG,wParam,lParam,0};
				mbe.php = dlg->php;
				mbe.t3 = MBE_DIALOGCB;
				mbe.p3 = (void*)dlg;
				mbe.lFlags = MBOT_FLAG_NOOUTPUT;

				res = (PHPR)MBMultiParam(dlg->php,dlg->pszWmTimer,&mbe,"llll",wParam,
				dlg->param,wParam,lParam);

				if(res == PHPR_END){
					PostMessage(hDlg,WM_USER + 3,0,0);
				}
			}
		}break;
	case WM_DESTROY:
		sman_dec(dlg->php);
		DlgFree(dlg);
		SetWindowLong(hDlg,GWL_USERDATA,0);
		return (TRUE);
	default:
		dlg->lDepth--;
		return (FALSE);
	};
	dlg->lDepth--;
	return (TRUE);
}