#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

int input(char *str, size_t size);
void trim(char *str);

int command_create(sqlite3 *db);
int command_search(sqlite3 *db, char *raw_line, size_t raw_line_size);

typedef struct {
  char *name;
	char *address;
	char *mobile_phone;
	char *house_phone;
	char *description;
} Entry;

// FIXME:
// NOTE: This program is currently vulnerable to SQL-injection.
int main()
{
	sqlite3 *db;
	if (sqlite3_open("storage.db", &db) != SQLITE_OK) {
		printf("Error: %s", sqlite3_errmsg(db));
		return 1;
	}

	char *err;
	if (sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS `ppl` ("
					"name VARCHAR(32) UNIQUE PRIMARY KEY,"
					"address VARCHAR(128),"
					"mobile_phone VARCHAR(32),"
					"house_phone VARCHAR(32),"
					"description TEXT"
				");", NULL, NULL, &err) != SQLITE_OK) {
		printf("Error: encountered sql error: %s\n", err);
		return 1;
	}

	printf("\n\tWelcome to PPL.\n\t(enter `h` to show all available commands)\n\n");

	do {
		printf("(main) > ");
		char raw_line[64];
		char *command;

		if (input(raw_line, sizeof(raw_line)) != 0) continue;
		if (strlen(raw_line) < 1) continue;

		command = strtok(raw_line, " ");

		if (strcmp(command, "q") == 0) {
			break;
		} else if (strcmp(command, "h") == 0) {
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
		} else if (strcmp(command, "s") == 0) {
			if (command_search(db, raw_line, sizeof(raw_line)) != 0) continue;
		} else if (strcmp(command, "c") == 0) {
			if (command_create(db) != 0) continue;
		}
		// TODO: commands: e, r
	} while (true);

	sqlite3_close(db);
	return 0;
}

int command_create(sqlite3 *db)
{
	printf("\n\tEnter Name: ");
	char name[32];
	if (input(name, sizeof(name)) != 0) return 1;

	printf("\tEnter Address: ");
	char address[128];
	if (input(address, sizeof(address)) != 0) return 1;

	printf("\tEnter Mobile Number: ");
	char mobile[32];
	if (input(mobile, sizeof(mobile)) != 0) return 1;

	printf("\tEnter House Phone Number: ");
	char home[32];
	if (input(home, sizeof(home)) != 0) return 1;

	printf("\tEnter Description: ");
	char description[1024];
	if (input(description, sizeof(description)) != 0) return 1;

	printf("\tInserting...\n");

	char sql[2048] = "INSERT INTO `ppl` (name, address, mobile_phone, house_phone, description) VALUES (?, ?, ?, ?, ?);";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)) {
		printf("\nError: could not create sqlite statement.");
		return 1;
	}

	sqlite3_bind_text(stmt, 1, name, -1, NULL);
	sqlite3_bind_text(stmt, 2, address, -1, NULL);
	sqlite3_bind_text(stmt, 3, mobile, -1, NULL);
	sqlite3_bind_text(stmt, 4, home, -1, NULL);
	sqlite3_bind_text(stmt, 5, description, -1, NULL);

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		printf("\nError: encountered sql error: %s\n", sqlite3_errmsg(db));
		return 1;
	}

	printf("\tInserted record into database.\n\n");

	sqlite3_finalize(stmt);
	return 0;
}

int command_search(sqlite3 *db, char *raw_line, size_t raw_line_size)
{
	char *field = strtok(NULL, " ");
	if (field == NULL) {
		field = "name";
	}

	if (strcmp(field, "name") != 0 && strcmp(field, "address") != 0 &&
			strcmp(field, "mobile") != 0 && strcmp(field, "home") != 0) {
		printf("Error: unknown field specified. Available are: name, address, mobile, home\n");
		return 1;
	}

	printf("(search:%s) > ", field);
	if (input(raw_line, raw_line_size) != 0) return 1;

	if (strcmp(field, "mobile") == 0 || strcmp(field, "home") == 0) {
		// grow string if it does not fit suffix
		if (sizeof(field) < strlen(field) + strlen("_phone")) {
			field = (char*) malloc(strlen(field) + strlen("_phone"));
		}

		strcat(field, "_phone");
	}

	// TODO: currently only searching exact matches, at including statements
	char sql[128];
	if (!snprintf(sql, sizeof(sql), "SELECT * FROM `ppl` WHERE %s LIKE ?;", field)) {
		printf("Error: could not write sql statement.\n");
		return 1;
	}

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)) {
		printf("Error: could not create sqlite statement.\n");
		return 1;
	}

	char to_bind[raw_line_size];
	snprintf(to_bind, raw_line_size, "%%%s%%", raw_line);

	sqlite3_bind_text(stmt, 1, to_bind, -1, NULL);

	printf("\n");

	int i = 0;
	while (sqlite3_step(stmt) != SQLITE_DONE) {
		const unsigned char *name = sqlite3_column_text(stmt, 0);
		const unsigned char *address = sqlite3_column_text(stmt, 1);
		const unsigned char *mobile = sqlite3_column_text(stmt, 2);
		const unsigned char *home = sqlite3_column_text(stmt, 3);
		const unsigned char *description = sqlite3_column_text(stmt, 4);

		// TODO: edit description such that it word wraps
		if (i > 0) {
			printf("\n");
		}
		printf(
			"\t==== Result: #%d\n"
			"\t       Name: %s\n"
			"\t    Address: %s\n"
			"\t     Mobile: %s\n"
			"\tHouse Phone: %s\n"
			"\tDescription: %s\n",
			i + 1, name, address, mobile, home, description
		);
		i++;
	}

	// If i remains Zero, we encountered no matching columns
	if (i == 0) {
		printf("\tCould not find any matching Entries.\n");
	}

	printf("\n");

	sqlite3_finalize(stmt);
	return 0;
}

int input(char *str, size_t size)
{
	if (!fgets(str, size, stdin)) {
		printf("Error: could not read line.\n");
		return 1;
	}

	trim(str);
	return 0;
}

void trim(char *str)
{
	int i = strlen(str) - 1;
	if (str[i] == '\n') {
		str[i] = '\0';
	}
}
