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
#ifndef __cBASE64_H__
#define __cBASE64_H__

#include "cUtils.h"

typedef struct tagQuad
{
	union{
		unsigned char bData[4];
		unsigned long bDword;
	}d;
	char null;
	char size;
}QUAD;

class cBase64
{

public:
	cBase64();
	~cBase64();

public:
	int Encode(const char* pszIn);
	int EncodeRAW(unsigned char* pszIn, int length);
	int Decode(const char* pszIn);
	int DecodedMessageSize();
	int GetString(char* pszOut);
	const char* GetBuffer();
protected:
	static void EncodeQuad(QUAD &q);
	static void DecodeQuad(QUAD &q);
	static bool IsValidB64(register char c);
public:
	cutMemf m_data;
};
#endif //__BASE64_CODER_PIOPAWLU_H__