/**************************************************************************
 *   prototypes.h  --  This file is part of GNU nano.                     *
 *                                                                        *
 *   Copyright (C) 1999-2011, 2013-2023 Free Software Foundation, Inc.    *
 *                                                                        *
 *   GNU nano is free software: you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published    *
 *   by the Free Software Foundation, either version 3 of the License,    *
 *   or (at your option) any later version.                               *
 *                                                                        *
 *   GNU nano is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#include "definitions.h"

/* All external variables.  See global.c for their descriptions. */

#ifndef NANO_TINY
extern volatile sig_atomic_t the_window_resized;
#endif

extern bool on_a_vt;
extern bool shifted_metas;

extern bool meta_key;
extern bool shift_held;
extern bool mute_modifiers;
extern bool bracketed_paste;

extern bool we_are_running;
extern bool more_than_one;
extern bool report_size;
extern bool ran_a_tool;

extern bool inhelp;
extern char *title;

extern bool focusing;

extern bool as_an_at;

extern bool control_C_was_pressed;

extern message_type lastmessage;

extern linestruct *pletion_line;

extern bool also_the_last;

extern char *answer;

extern char *last_search;
extern int didfind;

extern char *present_path;

extern unsigned flags[4];

extern int controlleft, controlright;
extern int controlup, controldown;
extern int controlhome, controlend;
#ifndef NANO_TINY
extern int controldelete, controlshiftdelete;
extern int shiftleft, shiftright;
extern int shiftup, shiftdown;
extern int shiftcontrolleft, shiftcontrolright;
extern int shiftcontrolup, shiftcontroldown;
extern int shiftcontrolhome, shiftcontrolend;
extern int altleft, altright;
extern int altup, altdown;
extern int altpageup, altpagedown;
extern int altinsert, altdelete;
extern int shiftaltleft, shiftaltright;
extern int shiftaltup, shiftaltdown;
#endif

#ifdef ENABLED_WRAPORJUSTIFY
extern ssize_t fill;
extern size_t wrap_at;
#endif

extern WINDOW *topwin;
extern WINDOW *midwin;
extern WINDOW *footwin;
extern int editwinrows;
extern int editwincols;
extern int margin;
extern int thebar;
#ifndef NANO_TINY
extern int *bardata;
extern ssize_t stripe_column;
#endif

extern linestruct *cutbuffer;
extern linestruct *cutbottom;
extern bool keep_cutbuffer;

extern openfilestruct *openfile;
#ifdef ENABLE_MULTIBUFFER
extern openfilestruct *startfile;
#endif

#ifndef NANO_TINY
extern char *matchbrackets;
extern char *whitespace;
extern int whitelen[2];
#endif

extern const char *exit_tag;
extern const char *close_tag;
#ifdef ENABLE_JUSTIFY
extern char *punct;
extern char *brackets;
extern char *quotestr;
extern regex_t quotereg;
#endif

extern char *word_chars;

extern ssize_t tabsize;

#ifndef NANO_TINY
extern char *backup_dir;
#endif
#ifdef ENABLE_OPERATINGDIR
extern char *operating_dir;
#endif

#ifdef ENABLE_SPELLER
extern char *alt_speller;
#endif

#ifdef ENABLE_COLOR
extern syntaxtype *syntaxes;
extern char *syntaxstr;
extern bool have_palette;
extern bool rescind_colors;
extern bool perturbed;
extern bool recook;
#endif

extern bool refresh_needed;

extern int currmenu;
extern keystruct *sclist;
extern funcstruct *allfuncs;
extern funcstruct *exitfunc;

extern linestruct *search_history;
extern linestruct *replace_history;
extern linestruct *execute_history;
#ifdef ENABLE_HISTORIES
extern linestruct *searchtop;
extern linestruct *searchbot;
extern linestruct *replacetop;
extern linestruct *replacebot;
extern linestruct *executetop;
extern linestruct *executebot;
#endif

extern regex_t search_regexp;
extern regmatch_t regmatches[10];

extern int hilite_attribute;
#ifdef ENABLE_COLOR
extern colortype *color_combo[NUMBER_OF_ELEMENTS];
#endif
extern int interface_color_pair[NUMBER_OF_ELEMENTS];

extern char *homedir;
extern char *statedir;
#if defined(ENABLE_NANORC) || defined(ENABLE_HISTORIES)
extern char *startup_problem;
#endif
#ifdef ENABLE_NANORC
extern char *custom_nanorc;

extern char *commandname;
extern keystruct *planted_shortcut;
#endif

extern bool spotlighted;
extern size_t light_from_col;
extern size_t light_to_col;

typedef void (*functionptrtype)(void);

/* The two needed functions from browser.c. */
#ifdef ENABLE_BROWSER
void browser_refresh(void);
char *browse_in(const char *inpath);
void to_first_file(void);
void to_last_file(void);
#endif

/* Most functions in chars.c. */
#ifdef ENABLE_UTF8
void utf8_init(void);
bool using_utf8(void);
#endif
bool is_alpha_char(const char *c);
bool is_blank_char(const char *c);
bool is_cntrl_char(const char *c);
bool is_word_char(const char *c, bool allow_punct);
char control_mbrep(const char *c, bool isdata);
#ifdef ENABLE_UTF8
int mbtowide(wchar_t *wc, const char *c);
bool is_doublewidth(const char *ch);
bool is_zerowidth(const char *ch);
#endif
int char_length(const char *pointer);
size_t mbstrlen(const char *pointer);
int collect_char(const char *string, char *thechar);
int advance_over(const char *string, size_t *column);
size_t step_left(const char *buf, size_t pos);
size_t step_right(const char *buf, size_t pos);
int mbstrcasecmp(const char *s1, const char *s2);
int mbstrncasecmp(const char *s1, const char *s2, size_t n);
char *mbstrcasestr(const char *haystack, const char *needle);
char *revstrstr(const char *haystack, const char *needle, const char *pointer);
char *mbrevstrcasestr(const char *haystack, const char *needle, const char *pointer);
#if !defined(NANO_TINY) || defined(ENABLE_JUSTIFY)
char *mbstrchr(const char *string, const char *chr);
#endif
#ifndef NANO_TINY
char *mbstrpbrk(const char *string, const char *accept);
char *mbrevstrpbrk(const char *head, const char *accept, const char *pointer);
#endif
#if defined(ENABLE_NANORC) && (!defined(NANO_TINY) || defined(ENABLE_JUSTIFY))
bool has_blank_char(const char *string);
#endif
bool white_string(const char *string);

/* Most functions in color.c. */
#ifdef ENABLE_COLOR
void set_interface_colorpairs(void);
void prepare_palette(void);
void find_and_prime_applicable_syntax(void);
void check_the_multis(linestruct *line);
void precalc_multicolorinfo(void);
#endif

/* Most functions in cut.c. */
void do_delete(void);
void do_backspace(void);
#ifndef NANO_TINY
void chop_previous_word(void);
void chop_next_word(void);
#endif
void extract_segment(linestruct *top, size_t top_x, linestruct *bot, size_t bot_x);
void ingraft_buffer(linestruct *topline);
void copy_from_buffer(linestruct *somebuffer);
#ifndef NANO_TINY
void cut_marked_region(void);
#endif
void do_snip(bool marked, bool until_eof, bool append);
void cut_text(void);
#ifndef NANO_TINY
void cut_till_eof(void);
void zap_text(void);
void copy_marked_region(void);
void copy_text(void);
#endif
void paste_text(void);

/* Most functions in files.c. */
void make_new_buffer(void);
#ifndef NANO_TINY
bool delete_lockfile(const char *lockfilename);
#endif
bool open_buffer(const char *filename, bool new_one);
void set_modified(void);
void prepare_for_display(void);
#ifdef ENABLE_MULTIBUFFER
void mention_name_and_linecount(void);
void switch_to_prev_buffer(void);
void switch_to_next_buffer(void);
void close_buffer(void);
#endif
void read_file(FILE *f, int fd, const char *filename, bool undoable);
int open_file(const char *filename, bool new_one, FILE **f);
char *get_next_filename(const char *name, const char *suffix);
void do_insertfile(void);
#ifndef NANO_TINY
void do_execute(void);
#endif
char *get_full_path(const char *origpath);
char *safe_tempfile(FILE **stream);
#ifdef ENABLE_OPERATINGDIR
void init_operating_dir(void);
bool outside_of_confinement(const char *currpath, bool allow_tabcomp);
#endif
#ifndef NANO_TINY
void init_backup_dir(void);
#endif
int copy_file(FILE *inn, FILE *out, bool close_out);
bool write_file(const char *name, FILE *thefile, bool normal,
				kind_of_writing_type method, bool annotate);
#ifndef NANO_TINY
bool write_region_to_file(const char *name, FILE *stream, bool normal,
				kind_of_writing_type method);
#endif
int write_it_out(bool exiting, bool withprompt);
void do_writeout(void);
void do_savefile(void);
char *real_dir_from_tilde(const char *path);
#if defined(ENABLE_TABCOMP) || defined(ENABLE_BROWSER)
int diralphasort(const void *va, const void *vb);
#endif
#ifdef ENABLE_TABCOMP
char *input_tab(char *buf, size_t *place, void (*refresh_func)(void), bool *listed);
#endif

/* Some functions in global.c. */
const keystruct *first_sc_for(int menu, void (*function)(void));
size_t shown_entries_for(int menu);
const keystruct *get_shortcut(const int keycode);
functionptrtype func_from_key(const int keycode);
#if defined(ENABLE_BROWSER) || defined(ENABLE_HELP)
functionptrtype interpret(const int keycode);
#endif
int keycode_from_string(const char *keystring);
void shortcut_init(void);
const char *epithet_of_flag(int flag);

/* Some functions in help.c. */
#ifdef ENABLE_HELP
void wrap_help_text_into_buffer(void);
#endif
void do_help(void);

/* Most functions in history.c. */
#ifdef ENABLE_HISTORIES
void history_init(void);
void reset_history_pointer_for(const linestruct *list);
void update_history(linestruct **item, const char *text, bool avoid_duplicates);
#ifdef ENABLE_TABCOMP
char *get_history_completion(linestruct **h, char *s, size_t len);
#endif
bool have_statedir(void);
void load_history(void);
void save_history(void);
void load_poshistory(void);
void update_poshistory(void);
bool has_old_position(const char *file, ssize_t *line, ssize_t *column);
#endif

/* Most functions in move.c. */
void to_first_line(void);
void to_last_line(void);
void do_page_up(void);
void do_page_down(void);
#ifdef ENABLE_JUSTIFY
void do_para_begin(linestruct **line);
void do_para_end(linestruct **line);
void to_para_begin(void);
void to_para_end(void);
#endif
void to_prev_block(void);
void to_next_block(void);
void do_prev_word(void);
bool do_next_word(bool after_ends);
void to_prev_word(void);
void to_next_word(void);
void do_home(void);
void do_end(void);
void do_up(void);
void do_down(void);
#if !defined(NANO_TINY) || defined(ENABLE_HELP)
void do_scroll_up(void);
void do_scroll_down(void);
void do_center(void);
#endif
void do_left(void);
void do_right(void);

/* Most functions in nano.c. */
linestruct *make_new_node(linestruct *prevnode);
void splice_node(linestruct *afterthis, linestruct *newnode);
void unlink_node(linestruct *line);
void delete_node(linestruct *line);
linestruct *copy_buffer(const linestruct *src);
void free_lines(linestruct *src);
void renumber_from(linestruct *line);
void print_view_warning(void);
bool in_restricted_mode(void);
#ifndef NANO_TINY
void suggest_ctrlT_ctrlZ(void);
#endif
void finish(void);
void close_and_go(void);
void do_exit(void);
void die(const char *msg, ...);
void window_init(void);
void install_handler_for_Ctrl_C(void);
void restore_handler_for_Ctrl_C(void);
#ifndef NANO_TINY
void reconnect_and_store_state(void);
#endif
void handle_hupterm(int signal);
#ifndef DEBUG
void handle_crash(int signal);
#endif
void suspend_nano(int signal);
void do_suspend(void);
void continue_nano(int signal);
#if !defined(NANO_TINY) || defined(ENABLE_SPELLER) || defined(ENABLE_COLOR)
void block_sigwinch(bool blockit);
#endif
#ifndef NANO_TINY
void handle_sigwinch(int signal);
void regenerate_screen(void);
#endif
void disable_kb_interrupt(void);
void enable_kb_interrupt(void);
void disable_flow_control(void);
void enable_flow_control(void);
void terminal_init(void);
#ifdef ENABLE_LINENUMBERS
void confirm_margin(void);
#endif
void unbound_key(int code);
bool changes_something(functionptrtype f);
void inject(char *burst, size_t count);

/* Most functions in prompt.c. */
size_t get_statusbar_page_start(size_t base, size_t column);
void put_cursor_at_end_of_answer(void);
void add_or_remove_pipe_symbol_from_answer(void);
int do_prompt(int menu, const char *provided, linestruct **history_list,
		void (*refresh_func)(void), const char *msg, ...);
int ask_user(bool withall, const char *question);

/* Most functions in rcfile.c. */
#if defined(ENABLE_NANORC) || defined(ENABLE_HISTORIES)
void display_rcfile_errors(void);
void jot_error(const char *msg, ...);
#endif
#ifdef ENABLE_NANORC
keystruct *strtosc(const char *input);
#ifdef ENABLE_COLOR
void parse_one_include(char *file, syntaxtype *syntax);
void grab_and_store(const char *kind, char *ptr, regexlisttype **storage);
bool parse_syntax_commands(char *keyword, char *ptr);
#endif
void parse_rcfile(FILE *rcstream, bool just_syntax, bool intros_only);
void do_rcfiles(void);
#endif /* ENABLE_NANORC */

/* Most functions in search.c. */
bool regexp_init(const char *regexp);
void tidy_up_after_search(void);
int findnextstr(const char *needle, bool whole_word_only, int modus,
		size_t *match_len, bool skipone, const linestruct *begin, size_t begin_x);
void do_search_forward(void);
void do_search_backward(void);
void do_findprevious(void);
void do_findnext(void);
void not_found_msg(const char *str);
void go_looking(void);
ssize_t do_replace_loop(const char *needle, bool whole_word_only,
		const linestruct *real_current, size_t *real_current_x);
void do_replace(void);
void ask_for_and_do_replacements(void);
#if !defined(NANO_TINY) || defined(ENABLE_SPELLER) || defined (ENABLE_LINTER) || defined (ENABLE_FORMATTER)
void goto_line_posx(ssize_t line, size_t pos_x);
#endif
void goto_line_and_column(ssize_t line, ssize_t column, bool retain_answer,
		bool interactive);
void do_gotolinecolumn(void);
#ifndef NANO_TINY
void do_find_bracket(void);
void put_or_lift_anchor(void);
void to_prev_anchor(void);
void to_next_anchor(void);
#endif

/* Most functions in text.c. */
#ifndef NANO_TINY
void do_mark(void);
#endif
void do_tab(void);
#ifndef NANO_TINY
void do_indent(void);
void do_unindent(void);
#endif
#ifdef ENABLE_COMMENT
void do_comment(void);
#endif
void do_undo(void);
void do_redo(void);
void do_enter(void);
#ifndef NANO_TINY
void discard_until(const undostruct *thisitem);
void add_undo(undo_type action, const char *message);
void update_multiline_undo(ssize_t lineno, char *indentation);
void update_undo(undo_type action);
#endif /* !NANO_TINY */
#ifdef ENABLE_WRAPPING
void do_wrap(void);
#endif
#if defined(ENABLE_HELP) || defined(ENABLED_WRAPORJUSTIFY)
ssize_t break_line(const char *textstart, ssize_t goal, bool snap_at_nl);
#endif
#if !defined(NANO_TINY) || defined(ENABLED_WRAPORJUSTIFY)
size_t indent_length(const char *line);
#endif
#ifdef ENABLE_JUSTIFY
size_t quote_length(const char *line);
bool begpar(const linestruct *const line, int depth);
bool inpar(const linestruct *const line);
void do_justify(void);
void do_full_justify(void);
#endif
#ifdef ENABLE_SPELLER
void do_spell(void);
#endif
#ifdef ENABLE_LINTER
void do_linter(void);
#endif
#ifdef ENABLE_FORMATTER
void do_formatter(void);
#endif
#ifndef NANO_TINY
void count_lines_words_and_characters(void);
#endif
void do_verbatim_input(void);
#ifdef ENABLE_WORDCOMPLETION
void complete_a_word(void);
#endif

/* All functions in utils.c. */
void get_homedir(void);
const char *tail(const char *path);
char *concatenate(const char *path, const char *name);
int digits(ssize_t n);
bool parse_num(const char *str, ssize_t *result);
bool parse_line_column(const char *str, ssize_t *line, ssize_t *column);
void recode_NUL_to_LF(char *string, size_t length);
size_t recode_LF_to_NUL(char *string);
#if !defined(ENABLE_TINY) || defined(ENABLE_TABCOMP) || defined(ENABLE_BROWSER)
void free_chararray(char **array, size_t len);
#endif
#ifdef ENABLE_SPELLER
bool is_separate_word(size_t position, size_t length, const char *buf);
#endif
const char *strstrwrapper(const char *haystack, const char *needle,
		const char *start);
void *nmalloc(size_t howmuch);
void *nrealloc(void *ptr, size_t howmuch);
char *measured_copy(const char *string, size_t count);
char *mallocstrcpy(char *dest, const char *src);
char *copy_of(const char *string);
char *free_and_assign(char *dest, char *src);
size_t get_page_start(size_t column);
size_t xplustabs(void);
size_t actual_x(const char *text, size_t column);
size_t wideness(const char *text, size_t maxlen);
size_t breadth(const char *text);
void new_magicline(void);
#if !defined(NANO_TINY) || defined(ENABLE_HELP)
void remove_magicline(void);
#endif
#ifndef NANO_TINY
bool mark_is_before_cursor(void);
void get_region(linestruct **top, size_t *top_x, linestruct **bot, size_t *bot_x);
void get_range(linestruct **top, linestruct **bot);
#endif
size_t number_of_characters_in(const linestruct *begin, const linestruct *end);
#if !defined(NANO_TINY) || defined(ENABLE_SPELLER) || defined (ENABLE_LINTER) || defined (ENABLE_FORMATTER)
linestruct *line_from_number(ssize_t number);
#endif

/* Most functions in winio.c. */
#ifndef NANO_TINY
void record_macro(void);
void run_macro(void);
#endif
void reserve_space_for(size_t newsize);
size_t waiting_keycodes(void);
#ifdef ENABLE_NANORC
void implant(const char *string);
#endif
int get_input(WINDOW *win);
int get_kbinput(WINDOW *win, bool showcursor);
char *get_verbatim_kbinput(WINDOW *win, size_t *count);
#ifdef ENABLE_MOUSE
int get_mouseinput(int *mouse_y, int *mouse_x, bool allow_shortcuts);
#endif
void blank_edit(void);
void blank_statusbar(void);
void wipe_statusbar(void);
void blank_bottombars(void);
void check_statusblank(void);
void set_blankdelay_to_one(void);
char *display_string(const char *buf, size_t column, size_t span,
						bool isdata, bool isprompt);
void titlebar(const char *path);
void minibar(void);
void statusline(message_type importance, const char *msg, ...);
void statusbar(const char *msg);
void warn_and_briefly_pause(const char *msg);
void bottombars(int menu);
void post_one_key(const char *keystroke, const char *tag, int width);
void place_the_cursor(void);
int update_line(linestruct *line, size_t index);
#ifndef NANO_TINY
int update_softwrapped_line(linestruct *line);
#endif
bool line_needs_update(const size_t old_column, const size_t new_column);
int go_back_chunks(int nrows, linestruct **line, size_t *leftedge);
int go_forward_chunks(int nrows, linestruct **line, size_t *leftedge);
bool less_than_a_screenful(size_t was_lineno, size_t was_leftedge);
void edit_scroll(bool direction);
#ifndef NANO_TINY
size_t get_softwrap_breakpoint(const char *linedata, size_t leftedge,
								bool *kickoff, bool *end_of_line);
size_t get_chunk_and_edge(size_t column, linestruct *line, size_t *leftedge);
size_t chunk_for(size_t column, linestruct *line);
size_t leftedge_for(size_t column, linestruct *line);
size_t extra_chunks_in(linestruct *line);
void ensure_firstcolumn_is_aligned(void);
#endif
size_t actual_last_column(size_t leftedge, size_t column);
void edit_redraw(linestruct *old_current, update_type manner);
void edit_refresh(void);
void adjust_viewport(update_type manner);
void full_refresh(void);
void draw_all_subwindows(void);
void report_cursor_position(void);
void spotlight(size_t from_col, size_t to_col);
#ifndef NANO_TINY
void spotlight_softwrapped(size_t from_col, size_t to_col);
#endif
#ifdef ENABLE_EXTRA
void do_credits(void);
#endif

/* These are just name definitions. */
void case_sens_void(void);
void regexp_void(void);
void backwards_void(void);
#ifdef ENABLE_HISTORIES
void get_older_item(void);
void get_newer_item(void);
#endif
void flip_replace(void);
void flip_goto(void);
#ifdef ENABLE_BROWSER
void to_files(void);
void goto_dir(void);
#endif
#ifndef NANO_TINY
void do_nothing(void);
void do_toggle(void);
void dos_format(void);
void mac_format(void);
void append_it(void);
void prepend_it(void);
void back_it_up(void);
void flip_execute(void);
void flip_pipe(void);
void flip_convert(void);
#endif
#ifdef ENABLE_MULTIBUFFER
void flip_newbuffer(void);
#endif
void discard_buffer(void);
void do_cancel(void);
