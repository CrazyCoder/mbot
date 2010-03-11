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

/******************************************
 * sounds                                 *
 ******************************************/
ZEND_FUNCTION(mb_SoundPlay)
{
	char*	snd = NULL;
	long	sl = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&snd,&sl) == FAILURE){
		PHP_FALSE_AND_WARNS(PHP_WARN_INVALID_PARAMS);
	}
	RETURN_LONG(SkinPlaySound(snd)==0);
}

ZEND_FUNCTION(mb_SoundAdd)
{
	char	*s1=NULL,*s2=NULL,*s3=NULL;
	long	l1=0,l2=0,l3=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss",&s1,&l1,&s2,&l2,&s3,&l3) == FAILURE)return;
	RETURN_LONG(SkinAddNewSound(s1,(const char*)s2,(const char*)s3)==0);
}

ZEND_FUNCTION(mb_SoundAddEx)
{
	char	*s1=NULL,*s2=NULL,*s3=NULL;
	long	l1=0,l2=0,l3=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss",&s1,&l1,&s2,&l2,&s3,&l3) == FAILURE)return;
	RETURN_LONG(SkinAddNewSoundEx(s1,(const char*)s2,(const char*)s3)==0);
}

ZEND_FUNCTION(mb_SoundSet)
{
	char	*s1=NULL,*s2=NULL;
	long	l1=0,l2=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",&s1,&l1,&s2,&l2) == FAILURE)return;

	RETURN_LONG(DBWriteContactSettingString(NULL,"SkinSounds",s1,s2)==0);
}

ZEND_FUNCTION(mb_SoundDel)
{
	char	*s1=NULL;
	long	l1=0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&s1,&l1) == FAILURE)return;

	RETURN_LONG(DBDeleteContactSetting(NULL,"SkinSounds",s1)==0);
}