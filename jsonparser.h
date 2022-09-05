#ifndef	jsonparser_h
#define	jsonparser_h

#include	<stdint.h>
#include	<stdlib.h>
#include	<string.h>
#include	<stdio.h>

#define	MAX_LEX_STACK	64
#define	MAX_NAME_LEVEL	64

typedef	struct JSONParser_st {
	char	name[32];
	char	names[MAX_NAME_LEVEL][128];
	int		indexes[MAX_NAME_LEVEL];
	int		(*cb)(struct JSONParser_st *j, int cmd, char *value);

#define		JP_NAME_VALUE		01
#define		JP_OBJEND			02
#define		JP_ARREND			03
#define		JP_JSONEND			04

	int		index;
	int		name_level;
	uint32_t	lexstate;
	uint32_t	lex_sp;
	uint32_t	lex_stack[MAX_LEX_STACK];
	uint32_t	colpos, rowpos;
	char	vname[128];
	char	value[512];
	int		valuepos;
	int		namepos;
} JSONParser;

int	PrintName(JSONParser *j, char *n);
int	processJSON(JSONParser *j, char *d, int l);
#define	JSON_ENDED			1
#define	JSON_CONTINUE		0
JSONParser	*startJSON(char *name);

#endif