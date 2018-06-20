/******************************************************************************
Copyright (c) 2004, Chris Johnson <cej@nightwolf.org.uk>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer. 

	* Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution. 

	* Neither the name of Chris Johnson nor the names of any contributors
	may be used to endorse or promote products derived from this software
	without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sablot.h>

/******************************************************************************
* Tuning defines
******************************************************************************/
#define PARAM_ARRAY_SIZE	32
#define FILEPATHLEN	256

/******************************************************************************
* Globals - these hold the QS and config file settings
******************************************************************************/
char *params[PARAM_ARRAY_SIZE];
char xmlfile[FILEPATHLEN];
char xslfile[FILEPATHLEN];
int debugFlag = 0;

/******************************************************************************
* This is where it all starts!
******************************************************************************/
int hexval (char c)
{
	char *hex = "0123456789ABCDEF";
	int i, retval = 0;

	for (i = 0; i < 16; i++) {
		if (c == hex[i])
			retval = i;
	}
	
	return retval;
}

char *sanitise_set_string (char *s)
{
	char *e, *n, *x;

	/* Translate CGI-encoded string with '%' and '+' */
	for (n = s; *n != 0; n++) {
		if (*n == '%') {
			*n = (char)((16 * hexval(toupper(*(n+1)))) + hexval(toupper(*(n+2))));
			for (x = n + 1; *(x-1) != 0; x++) {
				*x = *(x+2);
			}
			*x = 0;
		}
		if (*n == '+') {
			*n = ' ';
		}
	}

	/* Look for non-printable chars; terminate string if/when found */
	for (n = s; (*n >= 0x20) && (*n < 0x7F); n++) {
		/* Do nothing */
	}
	*n = 0;

	/* Find the equals sign in the setting, and point to the value */
	for (e = s; (*e != '=') && (*e != 0) ; e++) {
		/* Do nothing */
	}
	if (*e == '=')
		e++;

	return e;
}

char *sanitise_filename (char *s) {
	char *n;

	/* Look for anything other than [A-Za-z0-9._-]; terminate string if found */
	for (n = s; ((*n >= 'A') && (*n <= 'Z')) || \
			((*n >= 'a') && (*n <= 'z')) || \
			((*n >= '0') && (*n <= '9')) || 
			*n == '.' || *n == '_' || *n == '-'; n++)
	{
		/* Do nothing */
	}
	*n = 0;

	return s;
}

/******************************************************************************
* Given a pathstring, returns the string less the last componenet
******************************************************************************/
char *dirname (char *s)
{
	int i, sp;

	sp = -1;
	for (i = 0; s[i] != 0; i++) {
		if (s[i] == '/')
			sp = i;
	}
	if (sp >= 0)
		s[sp] = 0;

	return s;
}

/******************************************************************************
* Given a pathstring, returns the string less the path componenet
******************************************************************************/
char *basename (char *s)
{
	char *p, *bn;

	bn = NULL;
	for (p = s; *p != 0; p++) {
		if (*p == '/')
			bn = p;
	}

	if (bn)
		bn++;

	return bn;
}

/******************************************************************************
* Process the querystring; extract stylesheet and parameters
******************************************************************************/
int parse_querystring ()
{
	char qs[1024];
	char tempf[FILEPATHLEN];
	char *tok;
	char *p, *e;
	int i, x = 0;
	int haveXSL = 0;
	struct stat statbuf;

	strncpy (qs, getenv ("QUERY_STRING"), sizeof(qs) - 1);
	qs[sizeof(qs) - 1] = 0;

	if (debugFlag) {
		printf ("QUERY_STRING = %s\n", qs);
	}

	tok = strtok (qs, "&");
	while (tok && (x < PARAM_ARRAY_SIZE)) {
		if (debugFlag) {
			printf ("TOKEN: %s\n", tok);
		}
		if ((*tok == 'X') && *(tok+1) == '=') {
			strncat (xslfile, sanitise_filename(tok + 2), sizeof(xslfile) - 1);
			strncat (xslfile, ".xslt", sizeof(xslfile) - 1);
			haveXSL = 1;
		} else {
			p = (char *)malloc(strlen(tok) + 1);
			memset (p, 0, strlen(tok) + 1);
			strncpy (p, tok, strlen(tok));
			e = sanitise_set_string(p);
			*(e-1) = 0;
			params[x++] = p;
			params[x++] = e;
		}
		tok = strtok (NULL, "&");
	}

	if (!haveXSL) {
		/* If nowt specified on querystring, use filename + .xslt */
		memset (tempf, 0, sizeof(tempf));
		strncpy (tempf, xslfile, sizeof (tempf) - 1);

		strncat (xslfile, basename(getenv("PATH_TRANSLATED")), sizeof(xslfile) - 1);
		strncat (xslfile, ".xslt", sizeof(xslfile) - 1);

		/* If that doesn't exist, try some different extensions */
		if (stat (xslfile, &statbuf) != 0) {

	  		/*	Still nothing? Fall back to using _DEFAULT.xslt */
			strncpy (xslfile, tempf, sizeof(xslfile) - 1);
			strncat (xslfile, "_DEFAULT.xslt", sizeof(xslfile) - 1);
		}
	}

	if (debugFlag) {
		printf ("Stylesheet = %s\n", xslfile);
		printf ("Parameter count = %d strings (%d settings)\n", x, x / 2);
		for (i = 0; i < x; i += 2)
				printf ("    %s = %s\n", params[i], params[i + 1]);
	}

	return x;
}

/******************************************************************************
* The main bit of code...
******************************************************************************/
int init()
{
	int param_count, i;
	char temps[FILEPATHLEN];

  	puts ("Content-type: text/html");
	puts ("");
 
	memset (temps,   0, sizeof(xmlfile));
	memset (xmlfile, 0, sizeof(xmlfile));
	memset (xslfile, 0, sizeof(xslfile));
	for (i = 0; i < PARAM_ARRAY_SIZE; i++)
		params[i] = NULL;

	strncpy (temps, getenv("PATH_TRANSLATED"), sizeof(temps) - 1);
	strncpy (xmlfile, temps, sizeof(xmlfile) - 1);
	strncpy (xslfile, dirname(temps), sizeof(xmlfile) - 1);
	strncat (xslfile, "/", sizeof(xslfile) - 1);

	if (debugFlag) {
		printf ("XML file in = %s, \n", xmlfile);
		printf ("XSL dir in  = %s\n", xslfile);
	}

	param_count = parse_querystring();

	return param_count;
}

int run()
{
	void *xsl_proc;
	int err;
	void *situation = NULL;

	if (debugFlag) {
		puts ("Calling Sablotron...");
	}
	err = SablotCreateSituation(&situation);
	if (err == 0) {
		err = SablotCreateProcessorForSituation(situation, &xsl_proc);
		if (err == 0) {
			err = SablotRunProcessor(xsl_proc, xslfile, xmlfile, "file:///__stdout", (const char **)params, NULL);
			SablotDestroyProcessor(xsl_proc);
		}
		SablotDestroySituation(situation);
	}
	if (debugFlag) {
		puts ("...and all done.");
	}

	return err;
}

void done(int param_count)
{
	int i;

	for (i = 0; i < param_count; i += 2)
		if (params[i] != NULL)
			free (params[i]);
}

int main(int argc, char **argv)
{
	int err, param_count;

	param_count = init();
	err = run();
	if (err) {
		printf ("Error %d during run()\n", err);
	}
	done(param_count);

	return err;
}
