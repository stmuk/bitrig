/*
 * Copyright (c) 1995, 1996, 1997, 1998, 1999 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Kungliga Tekniska
 *      H�gskolan and its contributors.
 * 
 * 4. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
RCSID("$Id: main.c,v 1.3 2000/09/11 14:41:41 art Exp $");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <roken.h>
#include "sym.h"
#include "output.h"
#include <err.h>
#include <roken.h>

extern FILE *yyin;

int parse_errors;

/*
 * ydr - generate stub routines for encode/decoding and RX
 */

int yyparse(void);

int
main (int argc, char **argv)
{
    int ret;
    FILE *foo;
    char tmp_filename[64];
    char *cpp = CPP;
    int arglen;
    int i;
    char *arg;
    char *filename;

    if (argc < 2)
	errx (1, "Usage: %s [cpp-arguments] filename", argv[0]);

    snprintf (tmp_filename, sizeof(tmp_filename),
	      "ydr_tmp_%u.c", (unsigned)getpid());
    foo = efopen (tmp_filename, "w");
    filename = copy_basename (argv[argc - 1]);
    fprintf (foo, "#include \"%s\"\n", argv[argc - 1]);
    efclose (foo);

    initsym ();
    init_generate (filename);

    arglen = strlen(cpp) + 1;
    for (i = 1; i < argc - 1; ++i) {
	arglen += strlen (argv[i]) + 1;
    }
    arglen += strlen(tmp_filename) + 1;

    arg = malloc (arglen);
    if (arg == NULL) {
	unlink (tmp_filename);
	errx (1, "malloc: out of memory");
    }
    strcpy (arg, cpp);
    strcat (arg, " ");
    for (i = 1; i < argc - 1; ++i) {
	strcat (arg, argv[i]);
	strcat (arg, " ");
    }
    strcat (arg, tmp_filename);

    yyin = popen (arg, "r");
    if (yyin == NULL) {
	unlink (tmp_filename);
	err (1, "popen `%s'", arg);
    }
    free (arg);
    ret = yyparse ();
    generate_server_switch (serverfile.stream, serverhdrfile.stream);
    generate_tcpdump_patches (td_file.stream, filename);
    pclose (yyin);
    close_generator (filename);
    unlink (tmp_filename);

    return ret + parse_errors;
}

