#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <math.h>

#define COLOR_RED   31
#define COLOR_GREEN 32
#define COLOR_BLUE  34

lua_State *L_c;
int clie_file_completion;

struct str_array
{
	char             *str;
	struct str_array *next;
};

void   sa_print(struct str_array *str_array_head);
void   sa_append(struct str_array **str_array_head, const char* str);
void   sa_free(struct str_array *str_array_head);
int    sa_getn(struct str_array *str_array_head);
void   bail(lua_State *L, char *msg);
struct str_array *tableminer_c(const char *table_path);
char   *get_table_path(const char *text, char **table_key);
char   *get_last_key(char* text);
char   *completion_gen(const char* text, int state);
void   completion_display_hook(char **matches, int num_matches, int max_length);
void   add_color(int color, char** str);
void   clie_completion_display(char **matches, int num_matches, int max_length);
int    help(int a, int b);


////////////////////////////////////////////////////////////////////////////////
// String chained array
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
void sa_print(struct str_array *str_array_head)
{
	if(str_array_head != NULL)
	{
		printf("STR: %s\n", str_array_head->str);
		sa_print(str_array_head->next);
	}
}

//------------------------------------------------------------------------------
void sa_append(struct str_array **str_array_head, const char* str)
{
	struct str_array *str_array_tail;

	if(*str_array_head == NULL)
	{
		*str_array_head         = (struct str_array*) malloc(sizeof(struct str_array));
		(*str_array_head)->next = NULL;
		str_array_tail          = *str_array_head;
	}
	else
	{
		str_array_tail = *str_array_head;
		while(str_array_tail->next != NULL)
			str_array_tail = str_array_tail->next;

		str_array_tail->next = (struct str_array*) malloc(sizeof(struct str_array));
		str_array_tail       = str_array_tail->next;
		str_array_tail->next = NULL;
	}

	str_array_tail->str = (char*) malloc(sizeof(char)*(strlen(str) + 1));
	strcpy(str_array_tail->str, str);
}

//------------------------------------------------------------------------------
void sa_free(struct str_array *str_array_head)
{
	if(str_array_head != NULL)
	{
		sa_free(str_array_head->next);
		free(str_array_head->str);
		str_array_head->str = NULL;
		free(str_array_head);
		str_array_head = NULL;
	}
}

//------------------------------------------------------------------------------
int sa_getn(struct str_array *str_array_head)
{
	int len = 0;
	struct str_array *str_array_traveler;

	str_array_traveler = str_array_head;
	while(str_array_traveler != NULL)
	{
		len++;
		str_array_traveler = str_array_traveler->next;
	}

	return len;
}


////////////////////////////////////////////////////////////////////////////////
// Lua interface
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
void bail(lua_State *L, char *msg)
{
	fprintf(stderr, "\nFATAL ERROR:\n  %s: %s\n\n",
		msg, lua_tostring(L, -1));
// 	exit(1);
}

//------------------------------------------------------------------------------
struct str_array *tableminer_c(const char *table_path)//lua_State *L)
{
	lua_State *L = L_c;
	const char *table_name;
	struct str_array *table_names = NULL;

// 	L = luaL_newstate();
// 	luaL_openlibs(L);

// 	if(luaL_loadfile(L, "tableminer.lua"))
// 		bail(L, "luaL_loadfile() failed");

// 	// Prime run
// 	if (lua_pcall(L, 0, 0, 0)) 
// 		bail(L, "lua_pcall() failed");

	// Executes luaclie.tableminer(tablepath)
	lua_getglobal(L, "luaclie");
	lua_pushstring(L, "tableminer");
	lua_gettable(L, -2);

	lua_pushstring(L, table_path);

	if(lua_pcall(L, 1, 1, 0))
		bail(L, "lua_pcall() failed");

	lua_pushnil(L);
	while(lua_next(L, -2))
	{
		table_name = lua_tostring(L, -1);
		sa_append(&table_names, table_name);
		lua_pop(L,1);
	}

	lua_pop(L,2); // 	lua_close(L);

	return table_names;
}


////////////////////////////////////////////////////////////////////////////////
// String management
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
char* get_table_path(const char *text, char **table_key)
{
	int i, path_len = 0;
	char* table_path;

	for(i=0; i<strlen(text); i++)
		if(text[i] == '.' || text[i] == ':')
			path_len = i;

	if(path_len > 0)
	{
		table_path = (char*) malloc(sizeof(char)*(path_len + 2));
		*table_key = (char*) malloc(sizeof(char)*(strlen(text) - path_len + 1));

		for(i=0; i < strlen(text); i++)
			if(i <= path_len)
				table_path[i] = text[i];
			else
				(*table_key)[i - path_len - 1] = text[i];

		table_path[path_len + 1]                  = '\0';
		(*table_key)[strlen(text) - path_len - 1] = '\0';
	}
	else
	{
		*table_key = (char*) malloc(sizeof(char)*(strlen(text) + 1));
		strcpy(*table_key, text);
		table_path    = (char*) malloc(sizeof(char));
		table_path[0] = '\0';
	}

	return table_path;
}

//------------------------------------------------------------------------------
// Get last key after '.' or ':'
char* get_last_key(char* text)
{
	int  i, last_dot = -1;
	char *last_key;

	int length = (int)strlen(text);

	// Find last dot
	for(i=0; i < length - 1; i++)
		if(text[i] == '.' || text[i] == ':')
			last_dot = i;

	last_key = (char*) malloc(sizeof(char)*(length - last_dot + 1));

	for(i = last_dot + 1; i <= length; i++)
		last_key[i - (last_dot + 1)] = text[i];

	return last_key;
}

//------------------------------------------------------------------------------
// Add linux style color to string
void add_color(int color, char** str)
{
	char* str_color = malloc(sizeof(char) * ((int)strlen(*str) + 11 + 1));

	sprintf(str_color, "\e[%d;1m", color);
	strcat(str_color, *str);
	strcat(str_color, "\e[0m");

	free(*str);
	*str = str_color;
}


////////////////////////////////////////////////////////////////////////////////
// Readline hooks and callbacks
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// Completion generator to rl_completion_entry_function
char* completion_gen(const char* text, int state)
{
	char                    *table_key, *str;
	static char             *table_path;
	static struct str_array *table_names_head, *table_names;
	static struct str_array *possible_completions_head, *possible_completions;
	int                     is_possible_completion, i;

	// If quoted, use file completion.
	clie_file_completion = 0;
	if(rl_completion_quote_character == '\"')
	{
		clie_file_completion = 1;
		return rl_filename_completion_function(text, state);
	}

	// Prevents insersion of space at the end of the completed word.
	rl_completion_suppress_append = 1;

	// First call
	if(state == 0)
	{
		table_path                = NULL;
		table_key                 = NULL;
		possible_completions_head = NULL;
		table_path                = get_table_path(text, &table_key);
		table_names_head          = tableminer_c(table_path);
		table_names               = table_names_head;

		while(table_names != NULL)
		{
			is_possible_completion = 1;
			for(i=0; i < strlen(table_key); i++)
				if(table_key[i] != table_names->str[i] && table_names->str[i] != '\0')
				{
					is_possible_completion = 0;
					break;
				}

			if(is_possible_completion)
				sa_append(&possible_completions_head, table_names->str);

			table_names = table_names->next;
		}
		possible_completions = possible_completions_head;
		free(table_key);
		table_key = NULL;
	}

	// Return a possible completion at each call
	if(possible_completions != NULL)
	{
		str = (char*) malloc(sizeof(char)*(strlen(possible_completions->str) + strlen(table_path) + 1));
		strcpy(str, table_path);
		strcat(str, possible_completions->str);

		possible_completions = possible_completions->next;

		if(sa_getn(possible_completions_head) == 1 && !strcmp(text, str))
		{
			sa_free(table_names_head);
			sa_free(possible_completions_head);
			free(table_path);
			return NULL;
		}

		return str;
	}
	else
	{
		sa_free(table_names_head);
		sa_free(possible_completions_head);
		free(table_path);
		return NULL;
	}
}

//------------------------------------------------------------------------------
// Matches display hook for readline completion
void completion_display_hook(char **matches, int num_matches, int max_length)
{
	int  i, length, new_max_length = 0;
	char **parsed_matches;

	if(clie_file_completion == 1)
	{
		rl_display_match_list(matches, num_matches, max_length);
		rl_forced_update_display();
		return;
	}

	parsed_matches = (char**) malloc(sizeof(char*)*num_matches + 1);

	for(i=0; i<=num_matches; i++)
	{
		parsed_matches[i] = get_last_key(matches[i]);
		length = (int)strlen(parsed_matches[i]);
		if(length > new_max_length)
			new_max_length = length;
	}

	clie_completion_display(parsed_matches, num_matches, new_max_length);
	free(parsed_matches);

	rl_forced_update_display();
}

//------------------------------------------------------------------------------
// Emulates the rl_display_match_list function
void clie_completion_display(char **matches, int num_matches, int max_length)
{
	int   terminal_width, col_num, prints_column, i, j, k, match_id, length;
	char *terminal_width_env;

	printf("\n");

	// Length of each printed column
	max_length = max_length + 2;

	// Get column number
	terminal_width_env = getenv("COLUMNS");
	if (terminal_width_env != NULL)
		terminal_width = atoi(terminal_width_env);
	else
		terminal_width = 80;
	col_num = terminal_width/max_length;

	// Get number of prints per column
	prints_column = ceil((float)num_matches/(float)col_num);

	// Print routine
	for(i=0; i<prints_column; i++)
	{
		for(j=0; j<col_num; j++)
		{
			match_id = i+j*prints_column + 1;

			if(match_id <= num_matches)
			{
				length = (int)strlen(matches[match_id]);

				// Add color if needed
				if(matches[match_id][length - 1] == '.' || matches[match_id][length - 1] == ':')
					add_color(COLOR_BLUE, &matches[match_id]);
				else if (matches[match_id][length - 1] == '(')
					add_color(COLOR_GREEN, &matches[match_id]);

				// Do the printing stuff
				printf(matches[match_id]);
				for(k=length; k<max_length; k++)
					printf(" ");
			}
		}
		printf("\n");
	}
}


////////////////////////////////////////////////////////////////////////////////
// Other
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
int help(int a, int b)
{
	lua_State *L = L_c;

	// Executes luaclie.tableminer(tablepath)
	lua_getglobal(L, "luaclie");
	lua_pushstring(L, "funcref");
	lua_gettable(L, -2);

// 	lua_pushnil(L);
	lua_pushstring(L, rl_copy_text(0, rl_point));

	if(lua_pcall(L, 1, 1, 0))
		bail(L, "lua_pcall() failed");

	lua_pop(L, 2); // 	lua_close(L);

	rl_forced_update_display();

	return 0;
}

//------------------------------------------------------------------------------
int luaopen_luaclie_c(lua_State *L)
{
	L_c = L;

// 	lua_register(L, "tableminer_c", tableminer_c);
	rl_completion_entry_function       = completion_gen;
	rl_completer_quote_characters      = "\"";
	rl_completion_display_matches_hook = completion_display_hook;
	rl_bind_keyseq("\\eh", help);
	return 0;
}
