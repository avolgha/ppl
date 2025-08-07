#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

typedef struct {
  char *name;
	char *address;
	char *mobile_phone;
	char *house_phone;
	char *description;
} Entry;

typedef struct {
	Entry *items;
	int count;
	int capacity;
} EntryArray;

void ea_append(EntryArray *array, Entry item)
{
	if (array->count >= array->capacity) {
		if (array->capacity == 0) {
			array->capacity = 256;
		} else {
			array->capacity *= 2;
		}
		realloc(array, sizeof(Entry) * array->capacity);
	}

	*(array->items + array->count) = item;
	array->count++;
}

void ea_destroy(EntryArray *array)
{
	free(array->items);
	free(array);
}

int callback_search(void *entries, int numColumns, char **columns, char **names) {
	return 0;
}

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
		char raw_line[32];
		char *command;

		if (!fgets(raw_line, sizeof(raw_line), stdin)) {
			printf("Error: could not read line.");
			continue;
		}

		if (raw_line[strlen(raw_line) - 1] == '\n') {
			raw_line[strlen(raw_line) - 1] = '\0';
		}

		command = strtok(raw_line, " ");

		if (strcmp(command, "q") == 0) {
			break;
		} else if (strcmp(command, "h") == 0) {
			printf(
					"h - Print help page with all commands\n"
					"q - Quit\n"
					"\n"
					"s - Search for records in the database\n"
			);
		} else if (strcmp(command, "s") == 0) {
			char *field = strtok(NULL, " ");
			if (field == NULL) {
				field = "name";
			}

			if (strcmp(field, "name") != 0 && strcmp(field, "address") != 0 &&
					strcmp(field, "mobile") != 0 && strcmp(field, "home") != 0) {
				printf("Error: unknown field specified. Available are: name, address, mobile, home\n");
				continue;
			}

			printf("(search:%s) > ", field);
			if (!fgets(raw_line, sizeof(raw_line), stdin)) {
				printf("Error: could not read line.");
				continue;
			}

			if (raw_line[strlen(raw_line) - 1] == '\n') {
				raw_line[strlen(raw_line) - 1] = '\0';
			}

			if (strcmp(field, "mobile") == 0 || strcmp(field, "home") == 0) {
				sprintf(field, "_phone");
			}

			char sql[128];
			if (!snprintf(sql, sizeof(sql), "SELECT * FROM `ppl` WHERE %s = ?;", field)) {
				printf("Error: could not write sql statement.");
				continue;
			}

			sqlite3_stmt *stmt;
			if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)) {
				printf("Error: could not create sqlite statement.");
				continue;
			}

			sqlite3_bind_text(stmt, 1, raw_line, -1, NULL);

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
				printf("\t== Result %d\n", i + 1);
				printf("\tName: %s\nAddress: %s\nMobile: %s\nHouse Phone: %s\nDescription: %s\n",
						name, address, mobile, home, description);

				i++;
			}

			// If i remains Zero, we encountered no matching columns
			if (i == 0) {
				printf("\tCould not find any matching Entries.\n");
			}

			sqlite3_finalize(stmt);
		}
	} while (true);

	sqlite3_close(db);
	return 0;
}
