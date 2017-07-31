#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "io.h"

/* Read a line from input file and return as a string object */

#define MAXLINE 1024

char *freadln(FILE *ifile) {
  // allocate line buffer on the stack
  char line[MAXLINE];
  // ptr to return string object 	
  char *r = NULL;
  int len;
  char *s = fgets((char *)line, MAXLINE, ifile);
  if (!s) return s;
  len = strlen(s)+1;
  r = malloc(len);
  strncpy(r,s,len);
  return r;
}

void freeln(char *ln) {
  free(ln);
}
