// this program is heaviliy inspired by the readline example
// project fileman.c under section 2.6.4 of the manual:
// https://tiswww.case.edu/php/chet/readline/readline.html#A-Short-Completion-Example-1
// License: GNU General Public License (https://www.gnu.org/licenses/gpl-3.0.html)

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <uuid/uuid.h>
#include <readline/readline.h>
#include <readline/history.h>

/* **************************************************************** */
/*                                                                  */
/*                           Definitions                            */
/*                                                                  */
/* **************************************************************** */

#define UNUSED(x) ((void) (x))

typedef struct {
	char *name;
	rl_icpfunc_t *func;
	char *doc;
} Command;

typedef struct {
	char *name;
	char *address;
	char *mobile_phone;
	char *house_phone;
	char *description;
} Entry;

char *dupstr(char*);
char *stripwhite(char*);

Command *cli_find_command(char*);
int cli_execute(char*);
char **cli_completor(const char*, int, int);
char *cli_command_generator(const char*, int);

int cmd_help(char*);
int cmd_quit(char*);
int cmd_search(char*);
int cmd_create(char*);
int cmd_edit(char*);
int cmd_remove(char*);

static sqlite3 *db;

Command commands[] = {
	{ "h", cmd_help,   "Print help page"     },
	{ "q", cmd_quit,   "Quit"                },
	{ "s", cmd_search, "Search for records"  },
	{ "c", cmd_create, "Create a new record" },
	{ "e", cmd_edit,   "Edit a record"       },
	{ "r", cmd_remove, "Remove a record"     },
	{ (char*) NULL, (rl_icpfunc_t*) NULL, (char*) NULL }
};

/* **************************************************************** */
/*                                                                  */
/*                               Main                               */
/*                                                                  */
/* **************************************************************** */

// FIXME: This program is currently vulnerable to SQL-injection.
int main()
{
	rl_readline_name = "ppl";
	rl_attempted_completion_function = cli_completor;

	if (sqlite3_open("storage.db", &db) != SQLITE_OK) {
		printf("Error: %s", sqlite3_errmsg(db));
		return 1;
	}

	char *err;
	if (sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS `ppl` ("
					"id TEXT UNIQUE PRIMARY KEY,"
					"name VARCHAR(32),"
					"address VARCHAR(128),"
					"mobile_phone VARCHAR(32),"
					"house_phone VARCHAR(32),"
					"description TEXT"
				");", NULL, NULL, &err) != SQLITE_OK) {
		printf("Error: encountered sql error: %s\n", err);
		return 1;
	}

	printf("\n\tWelcome to PPL.\n\t(enter `h` to show all available commands)\n\n");

	char *line, *s;
	do {
		line = readline("(main) > ");

		if (!line)
			continue;

		s = stripwhite(line);

		if (*s) {
			int res = cli_execute(s);
			if (res == -1) {
				free(line);
				break;
			}

			if (res == 0)
				add_history(s);
		}

		free(line);
	} while (true);

	sqlite3_close(db);
	return 0;
}

/* **************************************************************** */
/*                                                                  */
/*                   Command Line Interface (CLI)                   */
/*                                                                  */
/* **************************************************************** */

Command *cli_find_command(char *name)
{
	register int i;

	for (i = 0; commands[i].name; i++)
		if (strcmp(name, commands[i].name) == 0)
			return &commands[i];

	return NULL;
}

int cli_execute(char *line)
{
	register int i;
	Command *command;
	char *word;

	i = 0;
	while (line[i] && whitespace(line[i]))
		i++;
	word = line + i;

	while (line[i] && !whitespace(line[i]))
		i++;

	if (line[i])
		line[i++] = '\0';

	command = cli_find_command(word);

	if (!command)
		return 1;

	while (whitespace(line[i]))
		i++;
	word = line + i;

	return (*(command->func)) (word);
}

char **cli_completor(const char *text, int start, int end)
{
	(void) (end);

	char **matches = NULL;

	if (start == 0)
		matches = rl_completion_matches(text, cli_command_generator);

	return matches;
}

char *cli_command_generator(const char *text, int state)
{
	static int list_index, len;
	char *name;

	if (!state) {
		list_index = 0;
		len = strlen(text);
	}

	while ((name = commands[list_index].name) != NULL) {
		list_index++;

		if (strncmp(name, text, len) == 0)
			return dupstr(name);
	}

	return NULL;
}

/* **************************************************************** */
/*                                                                  */
/*                         Uility Functions                         */
/*                                                                  */
/* **************************************************************** */

char *dupstr(char *text)
{
	int len = strlen(text) + 1;
	char *r;
	r = malloc(len);
	strncpy(r, text, len);
	return r;
}

char *stripwhite(char *text)
{
	register char *s, *t;

	for (s = text; whitespace(*s); s++)
		;

	if (*s == 0)
		return s;

	t = s + strlen(s) - 1;
	while (t > s && whitespace(*t))
		t--;
	*++t = '\0';

	return s;
}

char *random_uuid()
{
	uuid_t uuid_bin;
	char *uuid = malloc(37);

	uuid_generate_random(uuid_bin);
	uuid_unparse_lower(uuid_bin, uuid);

	return uuid;
}

/* **************************************************************** */
/*                                                                  */
/*                             Commands                             */
/*                                                                  */
/* **************************************************************** */

int cmd_help(char *arg)
{
	printf(
		"\n"
		"\th - Print help page with all commands\n"
		"\tq - Quit\n"
		"\n"
		"\ts - Search for records in the database\n"
		"\tc - Create a new record in the database\n"
		"\te - Edit a record in the database\n"
		"\tr - Remove a record from the database\n"
		"\n"
	);
	return 0;
}

int cmd_quit(char *arg)
{
	return -1;
}

int cmd_search(char *arg)
{
	register int i = 0;
	char *field = arg;

	while (arg[i] && !whitespace(arg[i]))
		i++;

	if (arg[i])
		arg[i++] = '\0';

	if (strlen(field) < 1)
		field = "name";

	if (strcmp(field, "name") != 0 && strcmp(field, "address") != 0 &&
	    strcmp(field, "mobile") != 0 && strcmp(field, "home") != 0) {
		printf("Error: unknown field specified. Available are: name, address, mobile, home\n");
		return 1;
	}

	char search[32];
	snprintf(search, sizeof(search), "(search:%s) > ", field);
	char *answer = readline(search);

	if (strcmp(field, "mobile") == 0 || strcmp(field, "home") == 0) {
		// grow string if it does not fit suffix
		if (sizeof(field) < strlen(field) + strlen("_phone")) {
			field = (char*) malloc(strlen(field) + strlen("_phone"));
		}

		strcat(field, "_phone");
	}

	char sql[128];
	if (!snprintf(
				sql,
				sizeof(sql),
				"SELECT * FROM `ppl` WHERE %s LIKE ?;",
				field)) {
		printf("Error: could not write sql statement.\n");
		return 1;
	}

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)) {
		printf("Error: could not create sqlite statement.\n");
		return 1;
	}

	char to_bind[sizeof(answer) + 2];
	snprintf(to_bind, sizeof(to_bind), "%%%s%%", answer);

	sqlite3_bind_text(stmt, 1, to_bind, -1, NULL);

	printf("\n");

	int j = 0;
	while (sqlite3_step(stmt) != SQLITE_DONE) {
		const unsigned char *real_id     = sqlite3_column_text(stmt, 0);
		const unsigned char *name        = sqlite3_column_text(stmt, 1);
		const unsigned char *address     = sqlite3_column_text(stmt, 2);
		const unsigned char *mobile      = sqlite3_column_text(stmt, 3);
		const unsigned char *home        = sqlite3_column_text(stmt, 4);
		const unsigned char *description = sqlite3_column_text(stmt, 5);

		char *id = dupstr((char *)(real_id));
		id[5] = '\0';

		// TODO: edit description such that it word wraps
		if (j > 0) {
			printf("\n");
		}
		printf(
			"\t==== Result: #%d (%s)\n"
			"\t       Name: %s\n"
			"\t    Address: %s\n"
			"\t     Mobile: %s\n"
			"\tHouse Phone: %s\n"
			"\tDescription: %s\n",
			j + 1, id, name, address, mobile, home, description
		);
		j++;
	}

	// If j remains Zero, we encountered no matching columns
	if (j == 0) {
		printf("\tCould not find any matching Entries.\n");
	}

	printf("\n");

	sqlite3_finalize(stmt);
	return 0;
}

int cmd_create(char *arg)
{
	UNUSED(arg);

	printf("\n");
	char *name = readline("\tEnter Name: ");
	char *address = readline("\tEnter Address: ");
	char *mobile = readline("\tEnter Mobile Number: ");
	char *home = readline("\tEnter House Phone Number: ");
	char *description = readline("\tEnter Description: ");

	printf("\tInserting...\n");

	char sql[2048] = "INSERT INTO `ppl` (id, name, address, mobile_phone,"
		"house_phone, description) VALUES (?, ?, ?, ?, ?, ?);";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)) {
		printf("\nError: could not create sqlite statement.");
		return 1;
	}

	sqlite3_bind_text(stmt, 1, random_uuid(), -1, NULL);
	sqlite3_bind_text(stmt, 2, name, -1, NULL);
	sqlite3_bind_text(stmt, 3, address, -1, NULL);
	sqlite3_bind_text(stmt, 4, mobile, -1, NULL);
	sqlite3_bind_text(stmt, 5, home, -1, NULL);
	sqlite3_bind_text(stmt, 6, description, -1, NULL);

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		printf("\nError: encountered sql error: %s\n", sqlite3_errmsg(db));
		return 1;
	}

	printf("\tInserted record into database.\n\n");

	sqlite3_finalize(stmt);
	return 0;
}

int cmd_remove(char *arg)
{
	register int i = 0;
	char *id = arg;

	while (arg[i] && !whitespace(arg[i]))
		i++;

	if (arg[i])
		arg[i++] = '\0';

	if (strlen(id) != 5) {
		printf("\n\tUsage: r <id>\n\n");
		return 1;
	}

	char sql[128];
	if (!snprintf(
				sql,
				sizeof(sql),
				"DELETE FROM `ppl` WHERE id LIKE ?;")) {
		printf("Error: could not write sql statement.\n");
		return 1;
	}

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)) {
		printf("Error: could not create sqlite statement.\n");
		return 1;
	}

	char *to_bind = malloc(7);
	snprintf(to_bind, sizeof(to_bind), "%s%%", id);

	sqlite3_bind_text(stmt, 1, to_bind, -1, NULL);

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	printf("\n\tRecord deleted.\n\n");

	return 0;
}

// TODO:
int cmd_edit  (char *arg) { UNUSED(arg); return 0; }
