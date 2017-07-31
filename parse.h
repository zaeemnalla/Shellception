#ifndef _parse_H_
#define _parse_H_

#define MAXTOKS 100

typedef char *tok_t;

tok_t *getToks(char *line);
void freeToks(tok_t *toks);
int isDirectTok(tok_t *t, char *R);
int arrayCount(char** array);

#endif
