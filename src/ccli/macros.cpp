/* macros.cpp expansion of sql command to some macros */
#ifdef WINS
#include  <windows.h>
#else 
#include "winrepl.h"
#endif
#include <stdio.h>
#include <string.h>  // memset on Linux in wbkernel.h
// general client headers
#include "cdp.h"
#include "wbkernel.h"
// 602ccli headers
#include "options.h"
#include "wbcl.h"
#include "commands.h"
#pragma hdrstop
#include <stdlib.h>
#include <assert.h>

struct macro_descr{
	const char *name;
	int params;
	const char *expansion;
};

struct macro_descr *macros=NULL;
/*{
	{"list tables", 0, "select tab_name from tabtab"},
	{"table", 1, "select defin from tabtab where tab_name=\"%s\""},
	{NULL, -1, NULL}
};
*/

struct macro_descr* alloc_macro()
{
	static int macros_allocated=0;
	static int free_macro_slot=0;

	if (++free_macro_slot>=macros_allocated){
		macros_allocated*=2;
		macros_allocated+=10;
		macros=(macro_descr *)realloc(macros,
		       	macros_allocated*sizeof(struct macro_descr));
		assert(macros!=NULL);
	}
	macros[free_macro_slot].name=NULL;
	return macros+free_macro_slot-1;
}


		
char *expand_command(char *to_expand)
{
	static char buffer[256];
	struct macro_descr *current_macro=macros;

	if (current_macro==NULL) return NULL;
	while (current_macro->name!=NULL){
		size_t len=strlen(current_macro->name);
		if (strncmp(current_macro->name, to_expand, len)){
			current_macro++;
			continue;
		}
		const char *pars[3];
		to_expand+=len;
		for (int i=0; i<current_macro->params; i++){
			pars[i]=strtok(to_expand, " \t");
			to_expand=NULL;
		}
		snprintf(buffer, sizeof(buffer),
		       	current_macro->expansion, pars[0], pars[1], pars[2]);
		return buffer;
	}
	return NULL;
}

void load_macros(const char *configfile, const char *section)
{
	char listbuffer[4096];
	char expansionbuffer[4096];
	
	/* seznam maker */
	int size=GetPrivateProfileString(section, NULL, "",
		listbuffer, sizeof(listbuffer), configfile);

	char *macro_name=listbuffer;
	while (*macro_name){
		int expsize=GetPrivateProfileString(section, macro_name, "",
			expansionbuffer, sizeof(expansionbuffer), configfile);
		char *name=(char *)malloc(expsize+strlen(macro_name)+3);
		if (NULL==name){
		       fprintf(stderr, "Not enough memory!\n");
	       	       return;
		}
		char *expansion=name+strlen(macro_name)+2;
		strcpy(name, macro_name);
		strncpy(expansion, expansionbuffer, expsize);
		expansion[expsize]=0;
		macro_descr* macro=alloc_macro();
		macro->name=name;
		macro->expansion=expansion;
		macro->params=3;
		if (verbose) printf("Macro %s\n", macro_name);
		macro_name+=strlen(macro_name)+1;
	}
}
