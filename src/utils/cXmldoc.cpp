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
#include "includes.h"
#include "cUtils.h"
#include "cXmlDoc.h"

/*utils*/
char* xml_strstrx (const char * str1,const char * str2)
{
    char *cp = (char*)str1;
    char *s1, *s2;
	long in = 0;
	char term = '\"';

    if (!*str2)return((char*)str1);

    while(*cp)
    {
		if(in && *cp == term)
		{
			in = 0;
		}
		else
		{
			if(*cp == '\'' || *cp == '"')
			{
				term = *cp;
				in = 1;
			}
			else
			{
				s1 = cp;
				s2 = (char*) str2;

				while (*s1 && *s2 && !(*s1-*s2))
				{
					s1++, s2++;
				}

				if (!*s2)
				{
					return(cp);
				}
			}
		}
        cp++;
    }
    return(NULL);
}

char* xml_strnws(const char* str)
{
	while(*str)
	{
		if(*str != ' ' && *str != '\r' && *str != '\n' && *str != '\t')
		{
			return (char*)str;
		}
		else
		{
			str++;
		}
	}
	return (NULL);
}

char* xml_strrnws(const char* s_str)
{
	const char* str = s_str + strlen(s_str) - 1;

	if(*str != ' ' && *str != '\r' && *str != '\n' && *str != '\t')
	{
		return (char*)str;
	}

	while(str != s_str)
	{
		if(*str != ' ' && *str != '\r' && *str != '\n' && *str != '\t')
		{
			break;
		}
		else
		{
			str--;
		}
	}
	return (char*)str;
}

char* xml_strfnalnum(const char* str)
{
	if(!str)return NULL;

	if(!isalpha(*str) && *str!='_')return (char*)str;

	str++;

	while(*str)
	{
		if(!isalnum(*str) && *str != '_' && *str != '.' && *str != '-' && *str != ':')return (char*)str;
		str ++;
	}
	return (NULL);
}
char* xml_strndup(const char* str,unsigned int n)
{
	char* tmp = NULL;
	n = xmlmin(strlen(str),n);

	tmp = (char*)malloc(n + 1);
	if(tmp)
	{
		memcpy(tmp,str,n);
		tmp[n] = '\0';
	}
	return tmp;
}

char* xml_parsendup(char* str,unsigned int n,long fast)
{
	long true_length = 0;
	char* out = NULL;
	char* o = NULL;
	const char* x = 0;
	char ctmp = str[n];
	str[n] = '\0';

	if(!fast)//calc the real length
	{
		for(const char* c=str;*c;)
		{
			const char* x = c + 1;

			if(*c=='&')
			{
				for(x=c+1;*x && *x!=';';x++);
				if(!(*x) || (x - c) < 3){
					true_length++;
					c++;
					continue;
				}
				//&amp;&gt;&lt;&nbsp;
				if(*((DWORD*)c) == 'pma&'){
					true_length++;
					c+=5;
				}else if(*((DWORD*)c) == ';tg&' || *((DWORD*)c) == ';tl&'){
					true_length++;
					c+=4;
				}else if(strcmp(c,"&nbsp;")==0){
					true_length++;
					c+=6;
				}else if(*(c+1)=='#' && (x-c)==4){//&#XX;
					true_length++;
					c+=5;
				}else{
					true_length++;
					c++;
					continue;
				}
			}
			else
			{
				c++;
				true_length++;
			}
		}//for
		true_length++;
	}else{
		true_length = strlen(str) + 1;
	}
	//now do the encoding
	o = out = (char*)malloc(true_length);
	if(!out){
		str[n] = ctmp;
		return NULL;
	}

	for(const char* c=str;*c;)
	{
		if(*c=='&')
		{
			for(x=c+1;*x && *x!=';';x++);
			if(!(*x) || (x - c) < 3){
				*o++ = *c++;
				continue;
			}
			//&amp;&gt;&lt;&nbsp;
			if(*((DWORD*)c) == 'pma&'){
				*o++ = '&';
				c+=5;
			}else if(*((DWORD*)c) == ';tg&'){
				*o++ = '>';
				c+=4;
			}else if(*((DWORD*)c) == ';tl&'){
				*o++ = '<';
				c+=4;
			}else if(strcmp(c,"&nbsp;")==0){
				*o++ = ' ';
				c+=6;
			}else if(*(c+1)=='#' && (x-c)==4){
				*o = (char)(strtoul((c+2),NULL,16) & 0xFF);
				if(!*o)*o='?';
				o++;
				c+=5;
			}else{
				*o++ = *c++;
			}
		}
		else{
			*o++ = *c++;
		}
	}
	*o++ = '\0';
	str[n] = ctmp;
	return out;
}

void xml_put_tabs(FILE* out,long level)
{
	while(level > 0){
		fputc('\t',out);
		level--;
	}
}

void xml_write_value(FILE* out,const char* val,long length)
{
	if(!length){
		length = strlen(val);
	}

	for(int i=0;i<length;i++,val++)
	{
		if(*val < 0x20 || *val == '#' || *val=='@' || *val=='%' || *val=='>' || *val=='<' || *val=='&'){
			fprintf(out,"&#%.2x;",*val);
		}else{
			fputc(*val,out);
		}
	}
}

cXmlDoc::cXmlDoc()
{
	memset(&m_root,0,sizeof(m_root));
	m_parse_flags = m_flags = m_level = 0;
}

cXmlDoc::~cXmlDoc()
{
	m_root.Free();
	m_parse_flags = m_flags = m_level = 0;
}

long cXmlDoc::ParseProperties(char* start,sXmlNode* node)
{
	char* s = start;
	char* ids = NULL;
	char* ide = NULL;
	char* vs = NULL;
	char* ve = NULL;
	char  et = '\0';
	sXmlProperty prop;

	while((s = xml_strnws(s)) && *s!='\0' && *s!='/') // id = [',"] .... [',"]
	{
		ids = s;
		if(!isalpha(*s))
		{
			return (m_parse_flags & cXmlDoc::PARSE_IGNORE_BAD_PROPERTIES)!=0;
		}
		s++;
		while(isalnum(*s) || *s=='_' || *s=='.' || *s=='-' || *s == ':')
		{
			s++;
		}
		if(*s == '\0')
		{
			return (m_parse_flags & cXmlDoc::PARSE_IGNORE_BAD_PROPERTIES)!=0;
		}
		ide = s;

		s = xml_strnws(s);
		if(!s || *s != '=')
		{
			return (m_parse_flags & cXmlDoc::PARSE_IGNORE_BAD_PROPERTIES)!=0;
		}
		s++;

		s = xml_strnws(s);
		if(!s || (*s!='\'' && *s != '\"'))
		{
			return (m_parse_flags & cXmlDoc::PARSE_IGNORE_BAD_PROPERTIES)!=0;
		}
		et = *s;
		vs = ++s;
		ve = strchr(vs,et);
		if(!ve)
		{
			return (m_parse_flags & cXmlDoc::PARSE_IGNORE_BAD_PROPERTIES)!=0;
		}

		prop.name = xml_strndup(ids,ide - ids);
		prop.value = xml_parsendup(vs,ve - vs,0);//xml_strndup(vs,ve - vs);
		if(!AddProperty(&prop,node))return 0;
		memset(&prop,0,sizeof(prop));

		s = ve + 1;
	}
	return 1;
}

long cXmlDoc::AddProperty(sXmlProperty* prop,sXmlNode* node)
{
	sXmlProperty* tmp = (sXmlProperty*)malloc(sizeof(sXmlProperty));
	
	if(!tmp){
		return 0;
	}
	memcpy(tmp,prop,sizeof(sXmlProperty));

	if(node->f_prop == NULL)
	{
		node->f_prop = node->l_prop = tmp;
	}
	else
	{
		node->l_prop->next = tmp;
		node->l_prop = tmp;
	}
	return 1;
}

long cXmlDoc::AddBinProperty(sXmlBinProperty* prop,sXmlNode* node)
{
	sXmlBinProperty* tmp = (sXmlBinProperty*)malloc(sizeof(sXmlBinProperty));
	if(!tmp)return 0;
	memcpy(tmp,prop,sizeof(sXmlBinProperty));

	if(node->f_prop == NULL)
	{
		node->f_prop = node->l_prop = tmp;
	}
	else
	{
		node->l_prop->next = tmp;
		node->l_prop = tmp;
	}
	return 1;
}

long  cXmlDoc::AddNewBinProperty(sXmlNode* node,const char* name,void* value)
{
	sXmlBinProperty* tmp = (sXmlBinProperty*)malloc(sizeof(sXmlBinProperty));
	if(!tmp)return 0;

	tmp->name = _strdup(name);
	tmp->value = (const char*)value;

	if(node->f_prop == NULL)
	{
		node->f_prop = node->l_prop = tmp;
	}
	else
	{
		node->l_prop->next = tmp;
		node->l_prop = tmp;
	}
	return 1;
}

long cXmlDoc::SetBinPropertyValue(sXmlNode* node,const char* name,void* value)
{
	sXmlProperty* p = node->f_prop;
	while(p)
	{
		if(p->type() == 1 && strcmp(p->name,name)==0){
			p->value = (const char*)value;
			return 1;
		}
		p = p->next;
	}
	return 0;
}

void* cXmlDoc::GetBinPropertyValue(sXmlNode* node,const char* name,long* success)
{
	sXmlProperty* p = node->f_prop;
	while(p)
	{
		if(p->type() == 1 && strcmp(p->name,name)==0){
			*success = TRUE;
			return (void*)p->value;
		}
		p = p->next;
	}
	*success = FALSE;
	return NULL;
}

long cXmlDoc::JoinParent(sXmlNode* parent,sXmlNode* tmp)
{
	tmp->parent = parent;
	parent->num_children ++;

	if(parent->value){
		free((void*)parent->value);
		parent->value = NULL;
	}
	parent->type = NODE_PARENT;

	if(parent->f_child == NULL)
	{
		parent->f_child = parent->l_child = tmp;
		return 1;
	}
	else
	{
		parent->l_child->next = tmp;
		parent->l_child = tmp;
		return 1;
	}
}
char* cXmlDoc::ParseNode(char* start,sXmlNode* parent)
{
	char* c = xml_strnws(start);
	char* t = NULL;
	char* e = NULL;
	char* x = NULL;
	sXmlNode* node = (sXmlNode*)malloc(sizeof(sXmlNode));
	if(!node)return NULL;
	memset(node,0,sizeof(sXmlNode));

	//<!-- !--> comment
	//<? ?>
	//< ... > v />
	if(!c || *c != '<')
	{
		goto Error;
	}
	c++;
	//node type..
	if(*c == '!' && strncmp(c,"!--",3)==0)/*comment*/
	{
		c += 3;
		e = strstr(c,"-->");
		if(!e)
		{
			goto Error;
		}
		else
		{
			if(m_parse_flags & cXmlDoc::PARSE_COMMENTS)
			{
				node->type = cXmlDoc::NODE_COMMENT;
				node->value = xml_strndup(c,e - c);
				JoinParent(parent,node);
			}
			else
			{
				free(node);
			}
			return e + 3;
		}
	}
	else if(*c == '!')/*special command <! blabla bla >*/
	{
		c ++;
		e = xml_strstrx(c,">");
		if(!e)
		{
			goto Error;
		}

		if(m_parse_flags & cXmlDoc::PARSE_SPECIAL)
		{
			node->type = cXmlDoc::NODE_SPECIAL;
			node->value = xml_strndup(c,e - c);
			JoinParent(parent,node);
		}
		else
		{
			free(node);
		}
		return e + 1;
	}
	else if(*c == '?')/*<?command ?>*/
	{
		c++; 
		e = xml_strstrx(c,"?>");
		if(!e)
		{
			goto Error;
		}
		if(m_parse_flags & cXmlDoc::PARSE_COMMANDS)
		{
			node->type = cXmlDoc::NODE_COMMAND;
			node->value = xml_strndup(c,e - c);
			JoinParent(parent,node);
		}
		else
		{
			free(node);
		}
		return c + 2;
	}
	else/*normal node*/
	{
		if(!isalnum(*c))
		{
			goto Error;//improper.. too bad :-)
		}
		t = xml_strfnalnum(c);
		if(!t || (*t != ' ' && *t != '>' && *t != '/' && !isspace(*t)))
		{
			goto Error;
		}

		node->name = xml_strndup(c,t - c);

		e = xml_strstrx(c,">");
		if(!e)
		{
			goto Error;
		}
		*e = '\0';//terminate ;-) cool, isn't it?
		if(!ParseProperties(t,node))
		{
			goto Error;
		}
		*e = '>';//
		x = e + 1;

		if(*(e - 1) == '/')
		{
			node->type = cXmlDoc::NODE_DATA;
			JoinParent(parent,node);
			return e + 1;/*<item />*/
		}
		c = e + 1;
Next:
		e = strchr(c,'<');
		if(!e)
		{
			goto Error;//error?
		}
		else
		{
			//end tag, or child
			if(*(e + 1) == '/')//end tag.. possibly
			{
				if(strncmp(e + 2,node->name,strlen(node->name))!=0)
				{
					goto Error; // too bad :-)
				}
				c = xml_strnws(e + 2 + strlen(node->name));
				if(!c || *c != '>')
				{
					goto Error;
				}
				t = strchr(t,'>');
				if(!t)
				{
					goto Error;
				}
				/*append as a child, etc...*/

				if(node->f_child == NULL)//no children.. then data is interesting ;-)
				{
					t = xml_strnws(x);
					if(!t)
					{
						goto Error;
					}
					*e = '\0'; // '<';
					x = xml_strrnws(t);
					if(!x)
					{
						goto Error;
					}
					x++;
					node->type = cXmlDoc::NODE_DATA;
					node->value = xml_parsendup(t,x - t,0);//xml_strndup(t,x - t);
				}
				else
				{
					node->type = node->type = cXmlDoc::NODE_PARENT;
				}
				JoinParent(parent,node);
				return c + 1;
			}
			else
			{
				c = ParseNode(e,node);
				if(!c)
				{
					goto Error;
				}
				else
				{
					goto Next;
				}
			}
		}
	}
Error:
	if(node)
	{
		node->Free();
		free(node);
	}
	return NULL;
}

long cXmlDoc::SaveNode(FILE* f,sXmlNode* node)
{
	while(node)
	{
		switch(node->type)
		{
		case cXmlDoc::NODE_COMMAND:
			xml_put_tabs(f,m_level);
			fprintf(f,"<?%s?>\r\n",node->value);
			break;
		case cXmlDoc::NODE_SPECIAL:
			xml_put_tabs(f,m_level);
			fprintf(f,"<!%s >\r\n",node->value);
			break;
		case cXmlDoc::NODE_COMMENT:
			xml_put_tabs(f,m_level);
			fprintf(f,"<!--%s-->\r\n",node->value);
			break;
		case cXmlDoc::NODE_PARENT:
			xml_put_tabs(f,m_level);

			if(node->f_prop)
			{
				fprintf(f,"<%s",node->name);
				sXmlProperty* p = node->f_prop;
				while(p)
				{
					fprintf(f," %s=\"%s\"",p->name,p->value);
					p = p->next;
				}
				fprintf(f,">\r\n");
			}
			else
			{
				fprintf(f,"<%s>\r\n",node->name);
			}

			m_level ++;
			SaveNode(f,node->f_child);
			m_level --;
			xml_put_tabs(f,m_level);
			fprintf(f,"</%s>\r\n",node->name);
			break;
		case cXmlDoc::NODE_DATA:
			xml_put_tabs(f,m_level);

			if(node->f_prop)
			{
				fprintf(f,"<%s",node->name);
				sXmlProperty* p = node->f_prop;
				while(p)
				{
					fprintf(f," %s=\"",p->name);
					xml_write_value(f,p->value,0);
					fputc('\"',f);
					p = p->next;
				}
			}
			else
			{
				fprintf(f,"<%s",node->name);
			}

			if(node->value)
			{
				fputc('>',f);
				xml_write_value(f,node->value,0);
				fprintf(f,"</%s>\r\n",node->name);
			}
			else
			{
				fprintf(f,"/>\r\n");
			}
			break;
		}
		node = node->next;
	}
	return 1;
}

long cXmlDoc::SaveToFile(const char* file)
{
	FILE* f = fopen(file,"wb");
	m_level = 0;
	if(f)
	{
		fprintf(f,"<?xml version=\"1.0\" ?>\r\n");
		SaveNode(f,m_root.f_child);
		fclose(f);
		return 1;
	}
	else
	{
		return 0;
	}
}
void cXmlDoc::Free()
{
	m_root.Free();
}

const char* cXmlDoc::GetProperty(sXmlNode* node,const char* name)
{
	sXmlProperty* prop;
	if(!node || !name)return NULL;

	prop = node->f_prop;
	while(prop){
		if(strcmp(prop->name,name)==0){
			return (const char*)prop->value;
		}
		prop = prop->next;
	}
	return NULL;
}

long cXmlDoc::SetValue(sXmlNode* node,const char* value)
{
	if(!node || !node->value || !value)return 0;

	char* tmp = _strdup(value);
	if(!tmp)return 0;
	free((void*)node->value);
	node->value = tmp;
	return 1;
}

sXmlNode* cXmlDoc::GetNode(const char* path,sXmlNode* parent)
{
	sXmlNode* n = NULL;
	char* c = NULL;
	char* sc = (char*)path;

	if(!path || !strlen(path))
	{
		return NULL;
	}
	if(!parent)
	{
		parent = &m_root;
	}
Next:
	c = strchr(sc,'/');
	if(!c)
	{
		c = sc + strlen(sc);
	}
	if(c == sc)
	{
		return NULL;
	}

	n = parent->f_child;
	while(n)
	{
		if(n->name && strncmp(n->name,sc,c - sc) == 0)
		{
			if(*c == '\0')
			{
				return n;
			}
			else
			{
				sc = c + 1;
				parent = n;
				goto Next;
			}
		}
		n = n->next;
	}
	return NULL;
}

sXmlNode* cXmlDoc::AddNewNode(const char* name,const char* value,sXmlNode* parent)
{
	sXmlNode* tmp;

	if(!parent){parent = &m_root;}

	if(!parent || !name || !value){
		return NULL;
	}

	if((tmp = GetNode(name,parent)) && tmp->value && strcmp(tmp->value,value)==0){
		return tmp;
	}else{
		tmp = NULL;
	}

	tmp = (sXmlNode*)malloc(sizeof(sXmlNode));
	if(!tmp){
		return NULL;
	}
	memset(tmp,0,sizeof(sXmlNode));

	tmp->name = _strdup(name);
	tmp->value = _strdup(value);
	tmp->parent = parent;
	tmp->type = NODE_DATA;
	JoinParent(parent,tmp);
	return tmp;
}

long cXmlDoc::ParseString(char* buffer, long flags)
{
	char*	c = NULL;
	char*	s = NULL;
	long	level = 0;

	m_root.Free();

	/*initializing processor*/
	s = c = buffer;
	/*processing*/
	c = xml_strnws(c);
	if(!c){
		goto End;//empty?
		}
	
	if(strncmp(c,"<?xml ",6)!=0 && !(flags & cXmlDoc::PARSE_NOT_REQUIRE_DEF))
	{
		goto Error;
	}
	else
	{
		c = xml_strstrx(c,"?>");
		if(!c)
		{
			goto Error;
		}
		c += 2;
	}
	m_parse_flags = flags;
GoOn:
	c = ParseNode(c,&m_root);
	if(!c)
	{
		goto Error;
	}
	else if(*c != '\0' && ((m_parse_flags & cXmlDoc::PARSE_MULTIPLE_ROOTS) || m_root.l_child == NULL 
		|| (m_root.l_child->type != cXmlDoc::NODE_PARENT && m_root.l_child->type != cXmlDoc::NODE_DATA)))
	{
		goto GoOn;
	}

End:
	return 1;
Error:
	m_root.Free();
	return 0;
}

long cXmlDoc::ParseFile(const char* file,long flags)
{
	cutMemf mf;

	if(!mf.load(mb2u(file), 0)){
		return 0;
	}

	if(ParseString((char*)mf.getdata(),flags)){
		mf.close();
		return 1;
	}else{
		mf.close();
		return 0;
	}
}