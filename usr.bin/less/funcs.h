/*
 * Copyright 2014 Garrett D'Amore <garrett@damore.org>
 *
 * This file is made available under the terms of the Less License.
 */

#include <regex.h>

struct mlist;
struct loption;

extern void *ecalloc(int, unsigned int);
/*PRINTFLIKE1*/
extern char *easprintf(const char *, ...);
extern char *estrdup(const char *);
extern char *skipsp(char *);
extern int sprefix(char *, char *, int);
extern void quit(int);
extern void raw_mode(int);
extern	char *special_key_str(int);
extern	void get_term(void);
extern	void init(void);
extern	void deinit(void);
extern	void home(void);
extern	void add_line(void);
extern	void lower_left(void);
extern	void line_left(void);
extern	void goto_line(int);
extern	void vbell(void);
extern	void ring_bell(void);
extern	void do_clear(void);
extern	void clear_eol(void);
extern	void clear_bot(void);
extern	void at_enter(int);
extern	void at_exit(void);
extern	void at_switch(int);
extern	int is_at_equiv(int, int);
extern	int apply_at_specials(int);
extern	void putbs(void);
extern	void match_brac(int, int, int, int);
extern	int ch_get(void);
extern	void ch_ungetchar(int);
extern	void end_logfile(void);
extern	void sync_logfile(void);
extern	int ch_seek(off_t);
extern	int ch_end_seek(void);
extern	int ch_beg_seek(void);
extern	off_t ch_length(void);
extern	off_t ch_tell(void);
extern	int ch_forw_get(void);
extern	int ch_back_get(void);
extern	void ch_setbufspace(int);
extern	void ch_flush(void);
extern	int seekable(int);
extern	void ch_set_eof(void);
extern	void ch_init(int, int);
extern	void ch_close(void);
extern	int ch_getflags(void);
extern	void init_charset(void);
extern	int binary_char(LWCHAR);
extern	int control_char(LWCHAR);
extern	char *prchar(LWCHAR);
extern	char *prutfchar(LWCHAR);
extern	int utf_len(char);
extern	int is_utf8_well_formed(const char *);
extern	LWCHAR get_wchar(const char *);
extern	void put_wchar(char **, LWCHAR);
extern	LWCHAR step_char(char **, int, char *);
extern	int is_composing_char(LWCHAR);
extern	int is_ubin_char(LWCHAR);
extern	int is_wide_char(LWCHAR);
extern	int is_combining_char(LWCHAR, LWCHAR);
extern	void cmd_reset(void);
extern	void clear_cmd(void);
extern	void cmd_putstr(char *);
extern	int len_cmdbuf(void);
extern	void set_mlist(void *, int);
extern	void cmd_addhist(struct mlist *, const char *);
extern	void cmd_accept(void);
extern	int cmd_char(int);
extern	LINENUM cmd_int(long *);
extern	char *get_cmdbuf(void);
extern	char *cmd_lastpattern(void);
extern	void init_cmdhist(void);
extern	void save_cmdhist(void);
extern	int in_mca(void);
extern	void dispversion(void);
extern	int getcc(void);
extern	void ungetcc(int);
extern	void ungetsc(char *);
extern	void commands(void);
extern	int cvt_length(int);
extern	int *cvt_alloc_chpos(int);
extern	void cvt_text(char *, char *, int *, int *, int);
extern	void init_cmds(void);
extern	void add_fcmd_table(char *, int);
extern	void add_ecmd_table(char *, int);
extern	int fcmd_decode(const char *, char **);
extern	int ecmd_decode(const char *, char **);
extern	char *lgetenv(char *);
extern	int lesskey(char *, int);
extern	void add_hometable(char *, char *, int);
extern	int editchar(int, int);
extern	void init_textlist(struct textlist *, char *);
extern	char *forw_textlist(struct textlist *, char *);
extern	char *back_textlist(struct textlist *, char *);
extern	int edit(char *);
extern	int edit_ifile(IFILE);
extern	int edit_list(char *);
extern	int edit_first(void);
extern	int edit_last(void);
extern	int edit_next(int);
extern	int edit_prev(int);
extern	int edit_index(int);
extern	IFILE save_curr_ifile(void);
extern	void unsave_ifile(IFILE);
extern	void reedit_ifile(IFILE);
extern	void reopen_curr_ifile(void);
extern	int edit_stdin(void);
extern	void cat_file(void);
extern	void use_logfile(char *);
extern	char *shell_unquote(char *);
extern	char *get_meta_escape(void);
extern	char *shell_quote(const char *);
extern	char *homefile(char *);
extern	char *fexpand(char *);
extern	char *fcomplete(char *);
extern	int bin_file(int f);
extern	char *lglob(char *);
extern	char *open_altfile(char *, int *, void **);
extern	void close_altfile(char *, char *, void *);
extern	int is_dir(char *);
extern	char *bad_file(char *);
extern	off_t filesize(int);
extern	char *last_component(char *);
extern	int eof_displayed(void);
extern	int entire_file_displayed(void);
extern	void squish_check(void);
extern	void forw(int, off_t, int, int, int);
extern	void back(int, off_t, int, int);
extern	void forward(int, int, int);
extern	void backward(int, int, int);
extern	int get_back_scroll(void);
extern	void del_ifile(IFILE);
extern	IFILE next_ifile(IFILE);
extern	IFILE prev_ifile(IFILE);
extern	IFILE getoff_ifile(IFILE);
extern	int nifile(void);
extern	IFILE get_ifile(char *, IFILE);
extern	char *get_filename(IFILE);
extern	int get_index(IFILE);
extern	void store_pos(IFILE, struct scrpos *);
extern	void get_pos(IFILE, struct scrpos *);
extern	int opened(IFILE);
extern	void hold_ifile(IFILE, int);
extern	int held_ifile(IFILE);
extern	void set_open(IFILE);
extern	void *get_filestate(IFILE);
extern	void set_filestate(IFILE, void *);
extern	off_t forw_line(off_t);
extern	off_t back_line(off_t);
extern	void set_attnpos(off_t);
extern	void jump_forw(void);
extern	void jump_back(LINENUM);
extern	void repaint(void);
extern	void jump_percent(int, long);
extern	void jump_line_loc(off_t, int);
extern	void jump_loc(off_t, int);
extern	void init_line(void);
extern	int is_ascii_char(LWCHAR);
extern	void prewind(void);
extern	void plinenum(LINENUM);
extern	void pshift_all(void);
extern	int is_ansi_end(LWCHAR);
extern	int is_ansi_middle(LWCHAR);
extern	int pappend(char, off_t);
extern	int pflushmbc(void);
extern	void pdone(int, int);
extern	void set_status_col(char);
extern	int gline(int, int *);
extern	void null_line(void);
extern	off_t forw_raw_line(off_t, char **, int *);
extern	off_t back_raw_line(off_t, char **, int *);
extern	void clr_linenum(void);
extern	void add_lnum(LINENUM, off_t);
extern	LINENUM find_linenum(off_t);
extern	off_t find_pos(LINENUM);
extern	LINENUM currline(int);
extern	void lsystem(const char *, const char *);
extern	int pipe_mark(int, char *);
extern	void init_mark(void);
extern	int badmark(int);
extern	void setmark(int);
extern	void lastmark(void);
extern	void gomark(int);
extern	off_t markpos(int);
extern	void unmark(IFILE);
extern	void opt_o(int, char *);
extern	void opt__O(int, char *);
extern	void opt_j(int, char *);
extern	void calc_jump_sline(void);
extern	void opt_shift(int, char *);
extern	void calc_shift_count(void);
extern	void opt_k(int, char *);
extern	void opt_t(int, char *);
extern	void opt__T(int, char *);
extern	void opt_p(int, char *);
extern	void opt__P(int, char *);
extern	void opt_b(int, char *);
extern	void opt_i(int, char *);
extern	void opt__V(int, char *);
extern	void opt_x(int, char *);
extern	void opt_quote(int, char *);
extern	void opt_query(int, char *);
extern	int get_swindow(void);
extern	char *propt(int);
extern	void scan_option(char *);
extern	void toggle_option(struct loption *, int, char *, int);
extern	int opt_has_param(struct loption *);
extern	char *opt_prompt(struct loption *);
extern	int isoptpending(void);
extern	void nopendopt(void);
extern	int getnum(char **, char *, int *);
extern	long getfraction(char **, char *, int *);
extern	int get_quit_at_eof(void);
extern	void init_option(void);
extern	struct loption *findopt(int);
extern	struct loption *findopt_name(char **, char **, int *);
extern	int iread(int, unsigned char *, unsigned int);
extern	char *errno_message(char *);
extern	int percentage(off_t, off_t);
extern	off_t percent_pos(off_t, int, long);
extern	void put_line(void);
extern	void flush(int);
extern	int putchr(int);
extern	void putstr(const char *);
extern	void get_return(void);
extern	void error(const char *, PARG *);
extern	void ierror(const char *, PARG *);
extern	int query(const char *, PARG *);
extern	int compile_pattern(char *, int, regex_t **);
extern	void uncompile_pattern(regex_t **);
extern	int match_pattern(void *, char *, char *, int, char **, char **,
    int, int);
extern	off_t position(int);
extern	void add_forw_pos(off_t);
extern	void add_back_pos(off_t);
extern	void pos_clear(void);
extern	void pos_init(void);
extern	int onscreen(off_t);
extern	int empty_screen(void);
extern	int empty_lines(int, int);
extern	void get_scrpos(struct scrpos *);
extern	int adjsline(int);
extern	void init_prompt(void);
extern	char *pr_expand(const char *, int);
extern	char *eq_message(void);
extern	char *prompt_string(void);
extern	char *wait_message(void);
extern	void init_search(void);
extern	void repaint_hilite(int);
extern	void clear_attn(void);
extern	void undo_search(void);
extern	void clr_hilite(void);
extern	int is_filtered(off_t);
extern	int is_hilited(off_t, off_t, int, int *);
extern	void chg_caseless(void);
extern	void chg_hilite(void);
extern	int search(int, char *, int);
extern	void prep_hilite(off_t, off_t, int);
extern	void set_filter_pattern(char *, int);
extern	int is_filtering(void);
extern	void sigwinch(int);
extern	void init_signals(int);
extern	void psignals(void);
extern	void cleantags(void);
extern	void findtag(char *);
extern	off_t tagsearch(void);
extern	char *nexttag(int);
extern	char *prevtag(int);
extern	int ntags(void);
extern	int curr_tag(void);
extern	int edit_tagfile(void);
extern	void open_getchr(void);
extern	int getchr(void);
extern	void *lsignal(int, void (*)(int));
extern	char *helpfile(void);
