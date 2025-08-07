.PHONY: ppl

ppl: ppl.c
	cc -Wall -Wextra -l sqlite3 -o ppl ppl.c
