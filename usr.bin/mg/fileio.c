/*
 *	POSIX fileio.c
 */
#include	"def.h"

static FILE    *ffp;

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>

/*
 * Open a file for reading.
 */
int
ffropen(fn, bp)
	char           *fn;
	BUFFER         *bp;
{
	struct stat     statbuf;

	if ((ffp = fopen(fn, "r")) == NULL)
		return (FIOFNF);
	if (bp && fstat(fileno(ffp), &statbuf) == 0) {
		/* set highorder bit to make sure this isn't all zero */
		bp->b_fi.fi_mode = statbuf.st_mode | 0x8000;
		bp->b_fi.fi_uid = statbuf.st_uid;
		bp->b_fi.fi_gid = statbuf.st_gid;
	}
	return (FIOSUC);
}

/*
 * Open a file for writing.
 * Return TRUE if all is well, and
 * FALSE on error (cannot create).
 */
int
ffwopen(fn, bp)
	char   *fn;
	BUFFER *bp;
{

	if ((ffp = fopen(fn, "w")) == NULL) {
		ewprintf("Cannot open file for writing");
		return (FIOERR);
	}

	/*
	 * If we have file information, use it.  We don't bother to check for
	 * errors, because there's no a lot we can do about it.  Certainly
	 * trying to change ownership will fail if we aren' root.  That's
	 * probably OK.  If we don't have info, no need to get it, since any
	 * future writes will do the same thing.
	 */
	if (bp && bp->b_fi.fi_mode) {
		chmod(fn, bp->b_fi.fi_mode & 07777);
		chown(fn, bp->b_fi.fi_uid, bp->b_fi.fi_gid);
	}
	return (FIOSUC);
}

/*
 * Close a file.
 * XXX - Should look at the status.
 */
/* ARGSUSED */
int
ffclose(bp)
	BUFFER *bp;
{

	(VOID) fclose(ffp);
	return (FIOSUC);
}

/*
 * Write a buffer to the already
 * opened file. bp points to the
 * buffer. Return the status.
 * Check only at the newline and
 * end of buffer.
 */
int
ffputbuf(bp)
	BUFFER *bp;
{
	char   *cp;
	char   *cpend;
	LINE   *lp;
	LINE   *lpend;

	lpend = bp->b_linep;
	lp = lforw(lpend);
	do {
		cp = &ltext(lp)[0];		/* begining of line	 */
		cpend = &cp[llength(lp)];	/* end of line		 */
		while (cp != cpend) {
			putc(*cp, ffp);
			cp++;			/* putc may evaluate arguments
						   more than once */
		}
		lp = lforw(lp);
		if (lp == lpend)
			break;			/* no implied \n on last line */
		putc('\n', ffp);
	} while (!ferror(ffp));
	if (ferror(ffp)) {
		ewprintf("Write I/O error");
		return FIOERR;
	}
	return (FIOSUC);
}

/*
 * Read a line from a file, and store the bytes
 * in the supplied buffer. Stop on end of file or end of
 * line.  When FIOEOF is returned, there is a valid line
 * of data without the normally implied \n.
 */
int
ffgetline(buf, nbuf, nbytes)
	char  *buf;
	int    nbuf;
	int   *nbytes;
{
	int    c;
	int    i;

	i = 0;
	while ((c = getc(ffp)) != EOF && c != '\n') {
		buf[i++] = c;
		if (i >= nbuf)
			return FIOLONG;
	}
	if (c == EOF && ferror(ffp) != FALSE) {
		ewprintf("File read error");
		return FIOERR;
	}
	*nbytes = i;
	return c == EOF ? FIOEOF : FIOSUC;
}

#ifndef NO_BACKUP
/*
 * Rename the file "fname" into a backup
 * copy. On Unix the backup has the same name as the
 * original file, with a "~" on the end; this seems to
 * be newest of the new-speak. The error handling is
 * all in "file.c". The "unlink" is perhaps not the
 * right thing here; I don't care that much as
 * I don't enable backups myself.
 */
int
fbackupfile(fn)
	char  *fn;
{
	char  *nname;

	if ((nname = malloc((unsigned) (strlen(fn) + 1 + 1))) == NULL) {
		ewprintf("Can't get %d bytes", strlen(fn) + 1);
		return (ABORT);
	}
	(void) strcpy(nname, fn);
	(void) strcat(nname, "~");
	if (rename(fn, nname) < 0) {
		free(nname);
		return (FALSE);
	}
	free(nname);
	return (TRUE);
}
#endif

/*
 * The string "fn" is a file name.
 * Perform any required appending of directory name or case adjustments.
 * If NO_DIR is not defined, the same file should be refered to even if the
 * working directory changes.
 */
#ifdef SYMBLINK
#include <sys/types.h>
#include <sys/stat.h>
#ifndef MAXLINK
#define MAXLINK 8		/* maximum symbolic links to follow */
#endif
#endif
#include <pwd.h>
#ifndef NO_DIR
extern char    *wdir;
#endif

char *
adjustname(fn)
	char           *fn;
{
	char           *cp;
	static char     fnb[NFILEN];
	struct passwd  *pwent;
#ifdef	SYMBLINK
	struct stat     statbuf;
	int             i, j;
	char            linkbuf[NFILEN];
#endif

	switch (*fn) {
	case '/':
		cp = fnb;
		*cp++ = *fn++;
		break;
	case '~':
		fn++;
		cp = getenv("HOME");
		if (cp != NULL && *cp != '\0' && (*fn == '/' || *fn == '\0')) {
			(VOID) strcpy(fnb, cp);
			cp = fnb + strlen(fnb);
			if (*fn)
				fn++;
			break;
		} else {
			cp = fnb;
			while (*fn && *fn != '/')
				*cp++ = *fn++;
			*cp = '\0';
			if ((pwent = getpwnam(fnb)) != NULL) {
				(VOID) strcpy(fnb, pwent->pw_dir);
				cp = fnb + strlen(fnb);
				break;
			} else {
				fn -= strlen(fnb) + 1;
				/* can't find ~user, continue to default case */
			}
		}
	default:
#ifndef	NODIR
		strcpy(fnb, wdir);
		cp = fnb + strlen(fnb);
		break;
#else
		return fn;	/* punt */
#endif
	}
	if (cp != fnb && cp[-1] != '/')
		*cp++ = '/';
	while (*fn) {
		switch (*fn) {
		case '.':
			switch (fn[1]) {
			case '\0':
				*--cp = '\0';
				return fnb;
			case '/':
				fn += 2;
				continue;
			case '.':
				if (fn[2] != '/' && fn[2] != '\0')
					break;
#ifdef SYMBLINK
				cp[-1] = '\0';
				for (j = MAXLINK; j-- &&
				     lstat(fnb, &statbuf) != -1 &&
				     (statbuf.st_mode & S_IFMT) == S_IFLNK &&
				     (i = readlink(fnb, linkbuf, sizeof linkbuf))
				     != -1;) {
					if (linkbuf[0] != '/') {
						--cp;
						while (cp > fnb && *--cp != '/') {
						}
						++cp;
						(VOID) strncpy(cp, linkbuf, i);
						cp += i;
					} else {
						(VOID) strncpy(fnb, linkbuf, i);
						cp = fnb + i;
					}
					if (cp[-1] != '/')
						*cp++ = '\0';
					else
						cp[-1] = '\0';
				}
				cp[-1] = '/';
#endif
				--cp;
				while (cp > fnb && *--cp != '/') {
				}
				++cp;
				if (fn[2] == '\0') {
					*--cp = '\0';
					return fnb;
				}
				fn += 3;
				continue;
			default:
				break;
			}
			break;
		case '/':
			fn++;
			continue;
		default:
			break;
		}
		while (*fn && (*cp++ = *fn++) != '/') {
		}
	}
	if (cp[-1] == '/')
		--cp;
	*cp = '\0';
	return fnb;
}

#ifndef NO_STARTUP
#include <sys/file.h>

/*
 * Find a startup file for the user and return its name. As a service
 * to other pieces of code that may want to find a startup file (like
 * the terminal driver in particular), accepts a suffix to be appended
 * to the startup file name.
 */
char *
startupfile(suffix)
	char           *suffix;
{
	char           *file;
	static char     home[NFILEN];

	if ((file = getenv("HOME")) == NULL || *file == '\0')
		goto notfound;
	if (strlen(file) + 7 >= NFILEN - 1)
		goto notfound;
	(VOID) strcpy(home, file);
	(VOID) strcat(home, "/.mg");
	if (suffix != NULL) {
		(VOID) strcat(home, "-");
		(VOID) strcat(home, suffix);
	}
	if (access(home, F_OK) == 0)
		return home;

notfound:
#ifdef	STARTUPFILE
	file = STARTUPFILE;
	if (suffix != NULL) {
		(VOID) strcpy(home, file);
		(VOID) strcat(home, "-");
		(VOID) strcat(home, suffix);
		file = home;
	}
	if (access(file, F_OK) == 0)
		return file;
#endif

	return NULL;
}
#endif

#ifndef NO_DIRED
#include <sys/wait.h>
#include "kbd.h"

int
copy(frname, toname)
	char   *frname;
	char   *toname;
{
	pid_t   pid;
	int     status;

	if ((pid = vfork())) {
		if (pid == -1)
			return -1;
		execl("/bin/cp", "cp", frname, toname, (char *) NULL);
		_exit(1);	/* shouldn't happen */
	}
	while (wait(&status) != pid);
	return status == 0;
}

BUFFER *
dired_(dirname)
	char   *dirname;
{
	BUFFER *bp;
	FILE   *dirpipe;
	char    line[256];

	if ((dirname = adjustname(dirname)) == NULL) {
		ewprintf("Bad directory name");
		return NULL;
	}
	if (dirname[strlen(dirname) - 1] != '/')
		(VOID) strcat(dirname, "/");
	if ((bp = findbuffer(dirname)) == NULL) {
		ewprintf("Could not create buffer");
		return NULL;
	}
	if (bclear(bp) != TRUE)
		return FALSE;
	(VOID) strcpy(line, "ls -al ");
	(VOID) strcpy(&line[7], dirname);
	if ((dirpipe = popen(line, "r")) == NULL) {
		ewprintf("Problem opening pipe to ls");
		return NULL;
	}
	line[0] = line[1] = ' ';
	while (fgets(&line[2], 254, dirpipe) != NULL) {
		line[strlen(line) - 1] = '\0';	/* remove ^J	 */
		(VOID) addline(bp, line);
	}
	if (pclose(dirpipe) == -1) {
		ewprintf("Problem closing pipe to ls");
		return NULL;
	}
	bp->b_dotp = lforw(bp->b_linep);	/* go to first line */
	(VOID) strncpy(bp->b_fname, dirname, NFILEN);
	if ((bp->b_modes[0] = name_mode("dired")) == NULL) {
		bp->b_modes[0] = &map_table[0];
		ewprintf("Could not find mode dired");
		return NULL;
	}
	bp->b_nmodes = 0;
	return bp;
}

int
d_makename(lp, fn)
	LINE  *lp;
	char  *fn;
{
	char  *cp;

	if (llength(lp) <= 56)
		return ABORT;
	(VOID) strcpy(fn, curbp->b_fname);
	cp = fn + strlen(fn);
	bcopy(&lp->l_text[56], cp, llength(lp) - 56);
	cp[llength(lp) - 56] = '\0';
	return lgetc(lp, 2) == 'd';
}
#endif				/* NO_DIRED */

struct filelist {
	LIST            fl_l;
	char            fl_name[NFILEN + 2];
};

/*
 * these things had better be contiguous, because we're going to refer to the
 * end of dirbuf + 1 byte
 */
struct dirent   dirbuf;
char            dirdummy;

/*
 * return list of file names that match the name in buf.
 * System V version.  listing is a flag indicating whether the
 * list is being used for printing a listing rather than
 * completion.  In that case, trailing * and / are put on
 * for executables and directories.  The list is not sorted.
 */

LIST *
make_file_list(buf, listing)
	char           *buf;
	int             listing;
{
	char           *dir, *file, *cp;
	int             len, i, preflen;
	int             fp;
	LIST           *last;
	struct filelist *current;
	char            prefixx[NFILEN + 1];
	struct stat     statbuf;
	char            statname[NFILEN + 2];

	/*
	 * We need three different strings: dir - the name of the directory
	 * containing what the user typed. Must be a real unix file name,
	 * e.g. no ~user, etc..  Must not end in /. prefix - the portion of
	 * what the user typed that is before the names we are going to find
	 * in the directory.  Must have a trailing / if the user typed it.
	 * names from the directory. we open dir, and return prefix
	 * concatenated with names.
	 */

	/* first we get a directory name we can look up */
	/*
	 * Names ending in . are potentially odd, because adjustname will
	 * treat foo/.. as a reference to another directory, whereas we are
	 * interested in names starting with ..
	 */
	len = strlen(buf);
	if (buf[len - 1] == '.') {
		buf[len - 1] = 'x';
		dir = adjustname(buf);
		buf[len - 1] = '.';
	} else
		dir = adjustname(buf);
	/*
	 * If the user typed a trailing / or the empty string
	 * he wants us to use his file spec as a directory name.
	 */
	if (buf[0] && buf[strlen(buf) - 1] != '/') {
		file = strrchr(dir, '/');
		if (file) {
			*file = 0;
			if (*dir == 0)
				dir = "/";
		} else {
			return (NULL);
		}
	}
	/* Now we get the prefix of the name the user typed. */
	strcpy(prefixx, buf);
	cp = strrchr(prefixx, '/');
	if (cp == NULL)
		prefixx[0] = 0;
	else
		cp[1] = 0;

	preflen = strlen(prefixx);
	/* cp is the tail of buf that really needs to be compared */
	cp = buf + preflen;
	len = strlen(cp);

	/*
	 * Now make sure that file names will fit in the buffers allocated.
	 * SV files are fairly short.  For BSD, something more general would
	 * be required.
	 */
	if ((preflen + MAXNAMLEN) > NFILEN)
		return (NULL);
	if ((strlen(dir) + MAXNAMLEN) > NFILEN)
		listing = 0;

	/* loop over the specified directory, making up the list of files */

	/*
	 * Note that it is worth our time to filter out names that don't
	 * match, even though our caller is going to do so again, and to
	 * avoid doing the stat if completion is being done, because stat'ing
	 * every file in the directory is relatively expensive.
	 */

	fp = open(dir, 0);
	if (fp < 0) {
		return (NULL);
	}
	last = NULL;
	/* clear entry after last so we can treat d_name as ASCIZ */
	dirbuf.d_name[MAXNAMLEN] = 0;
	while (1) {
		if (read(fp, &dirbuf, sizeof(struct dirent)) <= 0) {
			break;
		}
		if (dirbuf.d_ino == 0)	/* entry not allocated */
			continue;
		for (i = 0; i < len; ++i) {
			if (cp[i] != dirbuf.d_name[i])
				break;
		}
		if (i < len)
			continue;
		current = (struct filelist *) malloc(sizeof(struct filelist));
		current->fl_l.l_next = last;
		current->fl_l.l_name = current->fl_name;
		last = (LIST *) current;
		strcpy(current->fl_name, prefixx);
		strcat(current->fl_name, dirbuf.d_name);
		if (listing) {
			statbuf.st_mode = 0;
			strcpy(statname, dir);
			strcat(statname, "/");
			strcat(statname, dirbuf.d_name);
			stat(statname, &statbuf);
			if (statbuf.st_mode & 040000)
				strcat(current->fl_name, "/");
			else if (statbuf.st_mode & 0100)
				strcat(current->fl_name, "*");
		}
	}
	close(fp);

	return (last);
}
