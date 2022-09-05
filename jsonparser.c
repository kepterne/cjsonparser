#define	jsonparser_c

#include	"jsonparser.h"

int	PushName(JSONParser *j) {
	if (j->name_level >= MAX_NAME_LEVEL - 1)
		return 1;
	strncpy(j->names[j->name_level], j->vname, sizeof(j->vname));
	j->indexes[j->name_level] = j->index;
	j->name_level++;
	j->vname[j->namepos = 0] = 0;
	j->index = -1;
	return 0;
}

int	PopName(JSONParser *j) {
	if (!j->name_level)
		return 1;
	j->name_level--;
	j->index = j->indexes[j->name_level];  
	return 0;	
}

int	PrintName(JSONParser *j, char *n) {
	int	i = 0, l = 0;
	for ( ; i < j->name_level; i++) {
		if (j->indexes[i] >= 0)
			l += sprintf(n + l, "[%d]", j->indexes[i]);
		else
			l += sprintf(n + l, "%s.", j->names[i]);
	}
	if (j->index >= 0)
		l += sprintf(n + l, "[%d]", j->index);
	else
		l += sprintf(n + l, "%s", j->vname);
	return 0;
}

uint32_t	pushState(JSONParser *j, uint32_t st) {
	if (j->lex_sp >= MAX_LEX_STACK - 1)
		return 0;
	j->lex_stack[j->lex_sp++] = st;
	return st;
}

uint32_t	popState(JSONParser *j) {
	if (!j->lex_sp)
		return 0;
	return (j->lexstate = j->lex_stack[--j->lex_sp]);
}

#define	LEXSTATE_STRING		'"'
#define	LEXSTATE_COLON		':'
#define	LEXSTATE_ARRAYEND	']'
#define	LEXSTATE_OBJEND		'}'
#define	LEXSTATE_SPACE		0x1000
#define	LEXSTATE_NOTSPACE	0x1001
#define	LEXSTATE_COMMENT	0x1002
#define	LEXSTATE_START		0x2000
#define	LEXSTATE_NAME		0x3000
#define	LEXSTATE_INNAME		0x3001
#define	LEXSTATE_VALUE		0x4000
#define	LEXSTATE_SEPERATOR	0x4001

#define	LEXSTATE_ARRAY		0x5000
#define	LEXSTATE_OBJECT		0x6000

#define	JSON_ENDED			1
#define	JSON_CONTINUE		0

JSONParser	*startJSON(char *name) {
	JSONParser	*jsp = calloc(sizeof(JSONParser), 1);
	if (!jsp)
		goto l0;
	strncpy(jsp->name, name, sizeof(jsp->name));
	jsp->lexstate = LEXSTATE_SPACE;
	pushState(jsp, LEXSTATE_START);
	
	
	strcpy(jsp->vname, name);
	PushName(jsp);
	for (int i = 0; i < MAX_NAME_LEVEL; i++)
		jsp->indexes[i] = -1;
	jsp->index = -1;
l0:
	return jsp;
}

int	processJSON(JSONParser *j, char *d, int l) {
	for (int i = 0; *d && (i < l); i++, d++) {
		char	c = *d;

		if (c == 13) {
			j->colpos = 0;
			j->rowpos++;
		} else
			j->colpos++;
l0:		
		switch (j->lexstate) {
		case LEXSTATE_COMMENT:
			if (c == 13) {
				if (!popState(j))
					return 1;
			}
			continue;
		case LEXSTATE_SPACE:
			if (c == '#') {
				pushState(j, j->lexstate);
				j->lexstate = LEXSTATE_COMMENT;
				continue;
			}
			if (c == 13 || c == 10 || c == 9 || c == 32)
				continue;
			if (!popState(j))
				return 1;
			goto l0;
		case LEXSTATE_START:
			if (c == '[') {
				pushState(j, LEXSTATE_ARRAY);
				pushState(j, LEXSTATE_VALUE);
				j->lexstate = LEXSTATE_SPACE;
				j->index = 0;
				continue;
			}
			if (c == '{') {
				pushState(j, LEXSTATE_OBJECT);
				pushState(j, LEXSTATE_NAME);
				j->lexstate = LEXSTATE_SPACE;
				continue;
			}
		return 1;
		case LEXSTATE_ARRAY:
			if (c == ',') {
				j->index++;
				pushState(j, LEXSTATE_ARRAY);
				pushState(j, LEXSTATE_VALUE);
				j->lexstate = LEXSTATE_SPACE;
				continue;
			}
			if (c == ']') {
				if (j->cb) {
					int		r = (*j->cb)(j, JP_ARREND, (char *) j->index);
					if (r)
						return r;
				}
				PopName(j);
				if (!popState(j))
					return 1;
				continue;
			}
		return 1;
		case LEXSTATE_OBJECT:
			if (c == ',') {
				pushState(j, LEXSTATE_SPACE);
				pushState(j, LEXSTATE_OBJECT);
				pushState(j, LEXSTATE_NAME);
				j->lexstate = LEXSTATE_SPACE;
				continue;
			}
			if (c == '}') {

				if (j->cb) {
					int		r = (*j->cb)(j, JP_OBJEND, 0);
					if (r)
						return r;
				}
				if (!popState(j))
					return 1;
				PopName(j);
				continue;
			}
		return 1;
		case LEXSTATE_VALUE:
			j->value[j->valuepos = 0] = 0;
			switch (c) {
			case '{':
				PushName(j);
				pushState(j, LEXSTATE_SPACE);
				pushState(j, LEXSTATE_OBJECT);
				pushState(j, LEXSTATE_NAME);
				j->lexstate = LEXSTATE_SPACE;
				continue;
			case '[':
				PushName(j);
				j->index = 0;
				pushState(j, LEXSTATE_SPACE);
				pushState(j, LEXSTATE_ARRAY);
				pushState(j, LEXSTATE_VALUE);
				j->lexstate = LEXSTATE_SPACE;
				continue;
			case '"':
				pushState(j, LEXSTATE_SPACE);
				j->lexstate = LEXSTATE_STRING;
				j->value[j->valuepos = 0] = 0;
				continue;
			case ']': 
				if (j->cb) {
					int		r = (*j->cb)(j, JP_ARREND, (char *) j->index);
					if (r)
						return r;
				}
				if (!popState(j))
					return 1;
				PopName(j);
				continue;
			default:
				pushState(j, LEXSTATE_SPACE);
				j->lexstate = LEXSTATE_NOTSPACE;
				goto l0;
			}
		continue;
		case LEXSTATE_STRING:
			if (c == '"') {
				j->value[j->valuepos] = 0;
				char	nm[256];
			//	PrintName(j, nm);
			//	LogPrint("{%s : %s}\r\n", nm, j->value);
				if (j->cb) {
					int		r = (*j->cb)(j, JP_NAME_VALUE, j->value);
					if (r)
						return r;
				}
				j->value[j->valuepos = 0] = 0;
				if (!popState(j))
					return 1;
				continue;
			} else if (c == 13 || c == 10 || c == 9) {
				return 1;
			}
			j->value[j->valuepos] = c;
			if (j->valuepos < sizeof(j->value) - 1)
				j->valuepos++;
		continue;
		case LEXSTATE_NOTSPACE:
			if (c == 13 || c == 10 || c == 9 || c == 32 || c == '}' || c == ']' || c == ',' || c == '.') {
				j->value[j->valuepos] = 0;
				char	nm[256];
			//	PrintName(j, nm);
			//	LogPrint("{%s : %s}\r\n", nm, j->value);
				if (j->cb) {
					int		r = (*j->cb)(j, JP_NAME_VALUE, j->value);
					if (r)
						return r;
				}
				if (!popState(j))
					return 1;
				goto l0;
			}
			j->value[j->valuepos] = c;
			if (j->valuepos < sizeof(j->value) - 1)
				j->valuepos++;
		continue;
		case LEXSTATE_NAME:

			if (c =='}') {				
				if (j->cb) {
					int		r = (*j->cb)(j, JP_OBJEND, 0);
					if (r)
						return r;
				} 
				if (!popState(j))
					return 1;
				goto l0;
			}
			if (c != '"')
				return 1;
			j->vname[j->namepos = 0] = 0;
			pushState(j, LEXSTATE_VALUE);
			pushState(j, LEXSTATE_SPACE);
			pushState(j, LEXSTATE_COLON);
			pushState(j, LEXSTATE_SPACE);
			j->lexstate = LEXSTATE_INNAME;
		continue;
		case LEXSTATE_INNAME:
			if (c == 13 || c == 10)
				return 1;
			if (c == '"') {
				j->vname[j->namepos] = 0;
			//	LogPrint("{N : %s}\r\n", j->vname);
				
				if (!popState(j))
					return 1;
				continue;
			}
			j->vname[j->namepos] = c;
			if (j->namepos < sizeof(j->vname) - 1)
				j->namepos++;
		continue;
		case LEXSTATE_COLON:
			if (c != ':')
				return 1;
			if (!popState(j))
				return 1;
		continue;
		}
	}
	return JSON_CONTINUE;
}
