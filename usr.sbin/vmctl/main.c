/*	$OpenBSD: main.c,v 1.14 2016/01/26 07:55:47 reyk Exp $	*/

/*
 * Copyright (c) 2015 Reyk Floeter <reyk@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/un.h>

#include <machine/vmmvar.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <imsg.h>

#include "vmd.h"
#include "proc.h"
#include "vmctl.h"

static const char	*socket_name = SOCKET_NAME;
static int		 ctl_sock = -1;
static int		 tty_autoconnect = 0;

__dead void	 usage(void);
__dead void	 ctl_usage(struct ctl_command *);

int		 vmm_action(struct parse_result *);

int		 ctl_console(struct parse_result *, int, char *[]);
int		 ctl_create(struct parse_result *, int, char *[]);
int		 ctl_load(struct parse_result *, int, char *[]);
int		 ctl_start(struct parse_result *, int, char *[]);
int		 ctl_status(struct parse_result *, int, char *[]);
int		 ctl_stop(struct parse_result *, int, char *[]);

struct ctl_command ctl_commands[] = {
	{ "console",	CMD_CONSOLE,	ctl_console,	"id" },
	{ "create",	CMD_CREATE,	ctl_create,	"\"name\" -s size", 1 },
	{ "load",	CMD_LOAD,	ctl_load,	"[path]" },
	{ "reload",	CMD_RELOAD,	ctl_load,	"[path]" },
	{ "start",	CMD_START,	ctl_start,	"\"name\""
	    " [-c] -k kernel -m size [-i count] [-d disk]*" },
	{ "status",	CMD_STATUS,	ctl_status,	"[id]" },
	{ "stop",	CMD_STOP,	ctl_stop,	"id" },
	{ NULL }
};

__dead void
usage(void)
{
	extern char	*__progname;
	int		 i;

	fprintf(stderr, "usage:\t%s command [arg ...]\n",
	    __progname);
	for (i = 0; ctl_commands[i].name != NULL; i++) {
		fprintf(stderr, "\t%s %s %s\n", __progname,
		    ctl_commands[i].name, ctl_commands[i].usage);
	}
	exit(1);
}

__dead void
ctl_usage(struct ctl_command *ctl)
{
	extern char	*__progname;

	fprintf(stderr, "usage:\t%s %s %s\n", __progname,
	    ctl->name, ctl->usage);
	exit(1);
}

int
main(int argc, char *argv[])
{
	int	 ch;

	while ((ch = getopt(argc, argv, "")) != -1) {
		switch (ch) {
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;
	optreset = 1;

	if (argc < 1)
		usage();

	return (parse(argc, argv));
}

int
parse(int argc, char *argv[])
{
	struct ctl_command	*ctl = NULL;
	struct parse_result	 res;
	int			 i;

	memset(&res, 0, sizeof(res));
	res.nifs = -1;

	for (i = 0; ctl_commands[i].name != NULL; i++) {
		if (strncmp(ctl_commands[i].name,
		    argv[0], strlen(argv[0])) == 0) {
			if (ctl != NULL) {
				fprintf(stderr,
				    "ambiguous argument: %s\n", argv[0]);
				usage();
			}
			ctl = &ctl_commands[i];
		}
	}

	if (ctl == NULL) {
		fprintf(stderr, "unknown argument: %s\n", argv[0]);
		usage();
	}

	res.action = ctl->action;
	res.ctl = ctl;

	if (!ctl->has_pledge) {
		/* pledge(2) default if command doesn't have its own pledge */
		if (pledge("stdio rpath exec unix", NULL) == -1)
			err(1, "pledge");
	}
	if (ctl->main(&res, argc, argv) != 0)
		err(1, "failed");

	if (ctl_sock != -1) {
		close(ibuf->fd);
		free(ibuf);
	}

	return (0);
}

int
vmmaction(struct parse_result *res)
{
	struct sockaddr_un	 sun;
	struct imsg		 imsg;
	int			 done = 0;
	int			 n;
	int			 ret, action;

	if (ctl_sock == -1) {
		if ((ctl_sock = socket(AF_UNIX,
		    SOCK_STREAM|SOCK_CLOEXEC, 0)) == -1)
			err(1, "socket");

		bzero(&sun, sizeof(sun));
		sun.sun_family = AF_UNIX;
		strlcpy(sun.sun_path, socket_name, sizeof(sun.sun_path));

		if (connect(ctl_sock,
		    (struct sockaddr *)&sun, sizeof(sun)) == -1)
			err(1, "connect: %s", socket_name);

		if ((ibuf = malloc(sizeof(struct imsgbuf))) == NULL)
			err(1, "malloc");
		imsg_init(ibuf, ctl_sock);
	}

	switch (res->action) {
	case CMD_START:
		/* XXX validation should be done in start_vm() */
		if (res->size < 1)
			errx(1, "specified memory size too small");
		if (res->path == NULL)
			errx(1, "no kernel specified");
		if (res->ndisks > VMM_MAX_DISKS_PER_VM)
			errx(1, "too many disks");
		else if (res->ndisks == 0)
			warnx("starting without disks");
		if (res->nifs == -1)
			res->nifs = 0;
		if (res->nifs == 0)
			warnx("starting without network interfaces");

		ret = start_vm(res->name, res->size, res->nifs,
		    res->ndisks, res->disks, res->path);
		if (ret) {
			errno = ret;
			err(1, "start VM operation failed");
		}
		break;
	case CMD_STOP:
		terminate_vm(res->id, res->name);
		break;
	case CMD_STATUS:
		get_info_vm(res->id, res->name, 0);
		break;
	case CMD_CONSOLE:
		get_info_vm(res->id, res->name, 1);
		break;
	case CMD_RELOAD:
		imsg_compose(ibuf, IMSG_VMDOP_RELOAD, 0, 0, -1,
		    res->path, res->path == NULL ? 0 : strlen(res->path) + 1);
		done = 1;
		break;
	case CMD_LOAD:
		imsg_compose(ibuf, IMSG_VMDOP_LOAD, 0, 0, -1,
		    res->path, res->path == NULL ? 0 : strlen(res->path) + 1);
		done = 1;
		break;
	case CMD_CREATE:
	case NONE:
		break;
	}

	action = res->action;
	parse_free(res);

	while (ibuf->w.queued)
		if (msgbuf_write(&ibuf->w) <= 0 && errno != EAGAIN)
			err(1, "write error");

	while (!done) {
		if ((n = imsg_read(ibuf)) == -1 && errno != EAGAIN)
			errx(1, "imsg_read error");
		if (n == 0)
			errx(1, "pipe closed");

		while (!done) {
			if ((n = imsg_get(ibuf, &imsg)) == -1)
				errx(1, "imsg_get error");
			if (n == 0)
				break;

			if (imsg.hdr.type == IMSG_CTL_FAIL) {
				if (IMSG_DATA_SIZE(&imsg) == sizeof(ret)) {
					memcpy(&ret, imsg.data, sizeof(ret));
					errno = ret;
					warn("command failed");
				} else {
					warnx("command failed");
				}
				done = 1;
				break;
			}

			ret = 0;
			switch (action) {
			case CMD_START:
				done = start_vm_complete(&imsg, &ret,
				    tty_autoconnect);
				break;
			case CMD_STOP:
				done = terminate_vm_complete(&imsg, &ret);
				break;
			case CMD_CONSOLE:
			case CMD_STATUS:
				done = add_info(&imsg, &ret);
				break;
			default:
				done = 1;
				break;
			}

			imsg_free(&imsg);
		}
	}

	return (0);
}

void
parse_free(struct parse_result *res)
{
	size_t	 i;

	free(res->name);
	free(res->path);
	for (i = 0; i < res->ndisks; i++)
		free(res->disks[i]);
	free(res->disks);
	memset(res, 0, sizeof(*res));
}

int
parse_ifs(struct parse_result *res, char *word, int val)
{
	const char	*error;

	if (word != NULL) {
		val = strtonum(word, 0, INT_MAX, &error);
		if (error != NULL)  {
			warnx("invalid count \"%s\": %s", word, error);
			return (-1);
		}
	}
	res->nifs = val;
	return (0);
}

int
parse_size(struct parse_result *res, char *word, long long val)
{
	if (word != NULL) {
		if (scan_scaled(word, &val) != 0) {
			warn("invalid size: %s", word);
			return (-1);
		}
	}

	if (val < (1024 * 1024)) {
		warnx("size must be at least one megabyte");
		return (-1);
	} else
		res->size = val / 1024 / 1024;

	if ((res->size * 1024 * 1024) != val)
		warnx("size rounded to %lld megabytes", res->size);

	return (0);
}

int
parse_disk(struct parse_result *res, char *word)
{
	char		**disks;
	char		*s;

	if ((disks = reallocarray(res->disks, res->ndisks + 1,
	    sizeof(char *))) == NULL) {
		warn("reallocarray");
		return (-1);
	}
	if ((s = strdup(word)) == NULL) {
		warn("strdup");
		return (-1);
	}
	disks[res->ndisks] = s;
	res->disks = disks;
	res->ndisks++;

	return (0);
}

int
parse_vmid(struct parse_result *res, char *word)
{
	const char	*error;
	uint32_t	 id;

	if (word == NULL) {
		warnx("missing vmid argument");
		return (-1);
	}
	id = strtonum(word, 0, UINT32_MAX, &error);
	if (error == NULL) {
		res->id = id;
		res->name = NULL;
	} else {
		if (strlen(word) >= VMM_MAX_NAME_LEN) {
			warnx("name too long");
			return (-1);
		}
		res->id = 0;
		if ((res->name = strdup(word)) == NULL)
			errx(1, "strdup");
	}

	return (0);
}

int
ctl_create(struct parse_result *res, int argc, char *argv[])
{
	int		 ch, ret;
	const char	*paths[2];

	if (argc < 2)
		ctl_usage(res->ctl);

	paths[0] = argv[1];
	paths[1] = NULL;
	if (pledge("stdio rpath wpath cpath", NULL) == -1)
		err(1, "pledge");
	argc--;
	argv++;

	while ((ch = getopt(argc, argv, "s:")) != -1) {
		switch (ch) {
		case 's':
			if (parse_size(res, optarg, 0) != 0)
				errx(1, "invalid size: %s", optarg);
			break;
		default:
			ctl_usage(res->ctl);
			/* NOTREACHED */
		}
	}

	if (res->size == 0) {
		fprintf(stderr, "missing size argument\n");
		ctl_usage(res->ctl);
	}
	ret = create_imagefile(paths[0], res->size);
	if (ret != 0) {
		errno = ret;
		err(1, "create imagefile operation failed");
	} else
		warnx("imagefile created");
	return (0);
}

int
ctl_status(struct parse_result *res, int argc, char *argv[])
{
	if (argc == 2) {
		if (parse_vmid(res, argv[1]) == -1)
			errx(1, "invalid id: %s", argv[1]);
	} else if (argc > 2)
		ctl_usage(res->ctl);

	return (vmmaction(res));
}

int
ctl_load(struct parse_result *res, int argc, char *argv[])
{
	char	*config_file = NULL;

	if (argc == 2)
		config_file = argv[1];
	else if (argc > 2)
		ctl_usage(res->ctl);

	if (config_file != NULL &&
	    (res->path = strdup(config_file)) == NULL)
		err(1, "strdup");

	return (vmmaction(res));
}

int
ctl_start(struct parse_result *res, int argc, char *argv[])
{
	int		 ch;
	char		 path[PATH_MAX];

	if (argc < 2)
		ctl_usage(res->ctl);

	if ((res->name = strdup(argv[1])) == NULL)
		errx(1, "strdup");
	argc--;
	argv++;

	while ((ch = getopt(argc, argv, "ck:m:d:i:")) != -1) {
		switch (ch) {
		case 'c':
			tty_autoconnect = 1;
			break;
		case 'k':
			if (res->path)
				errx(1, "kernel specified multiple times");
			if (realpath(optarg, path) == NULL)
				err(1, "invalid kernel path");
			if ((res->path = strdup(path)) == NULL)
				errx(1, "strdup");
			break;
		case 'm':
			if (res->size)
				errx(1, "memory specified multiple times");
			if (parse_size(res, optarg, 0) != 0)
				errx(1, "invalid memory size: %s", optarg);
			break;
		case 'd':
			if (realpath(optarg, path) == NULL)
				err(1, "invalid disk path");
			if (parse_disk(res, path) != 0)
				errx(1, "invalid disk: %s", optarg);
			break;
		case 'i':
			if (res->nifs != -1)
				errx(1, "interfaces specified multiple times");
			if (parse_ifs(res, optarg, 0) != 0)
				errx(1, "invalid interface count: %s", optarg);
			break;
		default:
			ctl_usage(res->ctl);
			/* NOTREACHED */
		}
	}

	return (vmmaction(res));
}

int
ctl_stop(struct parse_result *res, int argc, char *argv[])
{
	if (argc == 2) {
		if (parse_vmid(res, argv[1]) == -1)
			errx(1, "invalid id: %s", argv[1]);
	} else if (argc != 2)
		ctl_usage(res->ctl);

	return (vmmaction(res));
}

int
ctl_console(struct parse_result *res, int argc, char *argv[])
{
	if (argc == 2) {
		if (parse_vmid(res, argv[1]) == -1)
			errx(1, "invalid id: %s", argv[1]);
	} else if (argc != 2)
		ctl_usage(res->ctl);

	return (vmmaction(res));
}

__dead void
ctl_openconsole(const char *name)
{
	closefrom(STDERR_FILENO + 1);
	execl(VMCTL_CU, VMCTL_CU, "-l", name, "-s", "9600", NULL);
	err(1, "failed to open the console");
}
