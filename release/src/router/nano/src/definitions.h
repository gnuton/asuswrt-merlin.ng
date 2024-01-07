/**************************************************************************
 *   definitions.h  --  This file is part of GNU nano.                    *
 *                                                                        *
 *   Copyright (C) 1999-2011, 2013-2023 Free Software Foundation, Inc.    *
 *   Copyright (C) 2014-2017 Benno Schulenberg                            *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef NEED_XOPEN_SOURCE_EXTENDED
#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED  1
#endif
#endif

#if defined(__HAIKU__) && !defined(_DEFAULT_SOURCE)
#define _DEFAULT_SOURCE  1
#endif

#ifdef __TANDEM
/* Tandem NonStop Kernel support. */
#include <floss.h>
#define ROOT_UID  65535
#else
#define ROOT_UID  0
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

/* Set a default value for PATH_MAX if there isn't one. */
#ifndef PATH_MAX
#define PATH_MAX  4096
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <dirent.h>
#include <regex.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>

/* Prefer wide ncurses over normal ncurses over curses. */
#if defined(HAVE_NCURSESW_NCURSES_H)
#include <ncursesw/ncurses.h>
#elif defined(HAVE_NCURSES_H)
#include <ncurses.h>
#else
#include <curses.h>
#endif

/* Native language support. */
#ifdef ENABLE_NLS
#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#endif
#define _(string)  gettext(string)
#define P_(singular, plural, number)  ngettext(singular, plural, number)
#else
#define _(string)  (string)
#define P_(singular, plural, number)  (number == 1 ? singular : plural)
#endif
/* For marking a string on which gettext() will be called later. */
#define gettext_noop(string)  (string)
#define N_(string)  gettext_noop(string)

/* If we aren't using an ncurses with mouse support, then
 * exclude the mouse routines, as they are useless then. */
#ifndef NCURSES_MOUSE_VERSION
#undef ENABLE_MOUSE
#endif

#if defined(ENABLE_WRAPPING) || defined(ENABLE_JUSTIFY)
#define ENABLED_WRAPORJUSTIFY  1
#endif

/* Suppress warnings for __attribute__((warn_unused_result)). */
#define IGNORE_CALL_RESULT(call)  do { if (call) {} } while(0)

/* Macros for flags, indexing each bit in a small array. */
#define FLAGS(flag)  flags[((flag) / (sizeof(unsigned) * 8))]
#define FLAGMASK(flag)  ((unsigned)1 << ((flag) % (sizeof(unsigned) * 8)))
#define SET(flag)  FLAGS(flag) |= FLAGMASK(flag)
#define UNSET(flag)  FLAGS(flag) &= ~FLAGMASK(flag)
#define ISSET(flag)  ((FLAGS(flag) & FLAGMASK(flag)) != 0)
#define TOGGLE(flag)  FLAGS(flag) ^= FLAGMASK(flag)

#define BACKWARD  FALSE
#define FORWARD  TRUE

#define YESORNO  FALSE
#define YESORALLORNO  TRUE

#define YES      1
#define ALL      2
#define NO       0
#define CANCEL  -1

#define BLIND  FALSE
#define VISIBLE  TRUE

#define JUSTFIND   0
#define REPLACING  1
#define INREGION   2

#define NORMAL  TRUE
#define SPECIAL  FALSE
#define TEMPORARY  FALSE

#define ANNOTATE  TRUE
#define NONOTES  FALSE

#define PRUNE_DUPLICATE  TRUE
#define IGNORE_DUPLICATES  FALSE

#ifdef ENABLE_UTF8
/* In UTF-8 a valid character is at most four bytes long. */
#define MAXCHARLEN  4
#else
#define MAXCHARLEN  1
#endif

/* The default width of a tab in spaces. */
#define WIDTH_OF_TAB  8

/* The default number of columns from end of line where wrapping occurs. */
#define COLUMNS_FROM_EOL  8

/* The default comment character when a syntax does not specify any. */
#define GENERAL_COMMENT_CHARACTER  "#"

/* The maximum number of search/replace history strings saved. */
#define MAX_SEARCH_HISTORY  100

/* The largest size_t number that doesn't have the high bit set. */
#define HIGHEST_POSITIVE  ((~(size_t)0) >> 1)

#ifdef ENABLE_COLOR
#define THE_DEFAULT  -1
#define BAD_COLOR  -2

/* Flags for indicating how a multiline regex pair apply to a line. */
#define NOTHING      (1<<1)
		/* The start/end regexes don't cover this line at all. */
#define STARTSHERE   (1<<2)
		/* The start regex matches on this line, the end regex on a later one. */
#define WHOLELINE    (1<<3)
		/* The start regex matches on an earlier line, the end regex on a later one. */
#define ENDSHERE     (1<<4)
		/* The start regex matches on an earlier line, the end regex on this one. */
#define JUSTONTHIS   (1<<5)
		/* Both the start and end regexes match within this line. */
#endif

/* Basic control codes. */
#define ESC_CODE  0x1B
#define DEL_CODE  0x7F

/* Codes for "modified" Arrow keys, beyond KEY_MAX of ncurses. */
#define CONTROL_LEFT    0x401
#define CONTROL_RIGHT   0x402
#define CONTROL_UP      0x403
#define CONTROL_DOWN    0x404
#define CONTROL_HOME    0x405
#define CONTROL_END     0x406
#define CONTROL_DELETE  0x40D
#define SHIFT_CONTROL_LEFT    0x411
#define SHIFT_CONTROL_RIGHT   0x412
#define SHIFT_CONTROL_UP      0x413
#define SHIFT_CONTROL_DOWN    0x414
#define SHIFT_CONTROL_HOME    0x415
#define SHIFT_CONTROL_END     0x416
#define CONTROL_SHIFT_DELETE  0x41D
#define ALT_LEFT      0x421
#define ALT_RIGHT     0x422
#define ALT_UP        0x423
#define ALT_DOWN      0x424
#define ALT_PAGEUP    0x427
#define ALT_PAGEDOWN  0x428
#define ALT_INSERT    0x42C
#define ALT_DELETE    0x42D
#define SHIFT_ALT_LEFT   0x431
#define SHIFT_ALT_RIGHT  0x432
#define SHIFT_ALT_UP     0x433
#define SHIFT_ALT_DOWN   0x434
//#define SHIFT_LEFT 0x451
//#define SHIFT_RIGHT 0x452
#define SHIFT_UP        0x453
#define SHIFT_DOWN      0x454
#define SHIFT_HOME      0x455
#define SHIFT_END       0x456
#define SHIFT_PAGEUP    0x457
#define SHIFT_PAGEDOWN  0x458
#define SHIFT_DELETE    0x45D
#define SHIFT_TAB       0x45F

/* Special keycodes for when a string bind has been partially implanted
 * or has an unpaired opening brace, or when a function in a string bind
 * needs execution or a specified function name is invalid. */
#define MORE_PLANTS       0x4EA
#define MISSING_BRACE     0x4EB
#define PLANTED_COMMAND   0x4EC
#define NO_SUCH_FUNCTION  0x4EF

/* A special keycode for when <Tab> is pressed while the mark is on. */
#define INDENT_KEY  0x4F1

/* A special keycode to signal the beginning and end of a bracketed paste. */
#define BRACKETED_PASTE_MARKER  0x4FB

/* A special keycode for when a key produces an unknown escape sequence. */
#define FOREIGN_SEQUENCE  0x4FC

/* A special keycode for plugging into the input stream after a suspension. */
#define KEY_FRESH  0x4FE

#ifndef NANO_TINY
/* A special keycode for when we get a SIGWINCH (a window resize). */
#define KEY_WINCH  -2

/* Some extra flags for the undo function. */
#define WAS_BACKSPACE_AT_EOF  (1<<1)
#define WAS_WHOLE_LINE        (1<<2)
#define INCLUDED_LAST_LINE    (1<<3)
#define MARK_WAS_SET          (1<<4)
#define CURSOR_WAS_AT_HEAD    (1<<5)
#define HAD_ANCHOR_AT_START   (1<<6)
#endif /* !NANO_TINY */

/* Identifiers for the different menus. */
#define MMAIN          (1<<0)
#define MWHEREIS       (1<<1)
#define MREPLACE       (1<<2)
#define MREPLACEWITH   (1<<3)
#define MGOTOLINE      (1<<4)
#define MWRITEFILE     (1<<5)
#define MINSERTFILE    (1<<6)
#define MEXECUTE       (1<<7)
#define MHELP          (1<<8)
#define MSPELL         (1<<9)
#define MBROWSER      (1<<10)
#define MWHEREISFILE  (1<<11)
#define MGOTODIR      (1<<12)
#define MYESNO        (1<<13)
#define MLINTER       (1<<14)
#define MFINDINHELP   (1<<15)
/* This is an abbreviation for all menus except Help and Browser and YesNo. */
#define MMOST  (MMAIN|MWHEREIS|MREPLACE|MREPLACEWITH|MGOTOLINE|MWRITEFILE|MINSERTFILE|\
                MEXECUTE|MWHEREISFILE|MGOTODIR|MFINDINHELP|MSPELL|MLINTER)
#ifndef NANO_TINY
#define MSOME  MMOST|MBROWSER
#else
#define MSOME  MMAIN|MBROWSER
#endif

/* Enumeration types. */
typedef enum {
	UNSPECIFIED, NIX_FILE, DOS_FILE, MAC_FILE
} format_type;

typedef enum {
	VACUUM, HUSH, REMARK, INFO, NOTICE, AHEM, MILD, ALERT
} message_type;

typedef enum {
	OVERWRITE, APPEND, PREPEND
} kind_of_writing_type;

typedef enum {
	CENTERING, FLOWING, STATIONARY
} update_type;

/* The kinds of undo actions.  ADD...REPLACE must come first. */
typedef enum {
	ADD, ENTER, BACK, DEL, JOIN, REPLACE,
#ifdef ENABLE_WRAPPING
	SPLIT_BEGIN, SPLIT_END,
#endif
	INDENT, UNINDENT,
#ifdef ENABLE_COMMENT
	COMMENT, UNCOMMENT, PREFLIGHT,
#endif
	ZAP, CUT, CUT_TO_EOF, COPY, PASTE, INSERT,
	COUPLE_BEGIN, COUPLE_END, OTHER
} undo_type;

/* The elements of the interface that can be colored differently. */
enum {
	TITLE_BAR = 0,
	LINE_NUMBER,
	GUIDE_STRIPE,
	SCROLL_BAR,
	SELECTED_TEXT,
	SPOTLIGHTED,
	MINI_INFOBAR,
	PROMPT_BAR,
	STATUS_BAR,
	ERROR_MESSAGE,
	KEY_COMBO,
	FUNCTION_TAG,
	NUMBER_OF_ELEMENTS
};

/* Enumeration used in the flags array.  See the definition of FLAGMASK. */
enum {
	DONTUSE = 0,
	CASE_SENSITIVE,
	CONSTANT_SHOW,
	NO_HELP,
	NO_WRAP,
	AUTOINDENT,
	VIEW_MODE,
	USE_MOUSE,
	USE_REGEXP,
	SAVE_ON_EXIT,
	CUT_FROM_CURSOR,
	BACKWARDS_SEARCH,
	MULTIBUFFER,
	REBIND_DELETE,
	RAW_SEQUENCES,
	NO_CONVERT,
	MAKE_BACKUP,
	INSECURE_BACKUP,
	NO_SYNTAX,
	PRESERVE,
	HISTORYLOG,
	RESTRICTED,
	SMART_HOME,
	WHITESPACE_DISPLAY,
	TABS_TO_SPACES,
	QUICK_BLANK,
	WORD_BOUNDS,
	NO_NEWLINES,
	BOLD_TEXT,
	SOFTWRAP,
	POSITIONLOG,
	LOCKING,
	NOREAD_MODE,
	MAKE_IT_UNIX,
	TRIM_BLANKS,
	SHOW_CURSOR,
	LINE_NUMBERS,
	AT_BLANKS,
	AFTER_ENDS,
	LET_THEM_ZAP,
	BREAK_LONG_LINES,
	JUMPY_SCROLLING,
	EMPTY_LINE,
	INDICATOR,
	BOOKSTYLE,
	STATEFLAGS,
	USE_MAGIC,
	MINIBAR,
	ZERO
};

/* Structure types. */
#ifdef ENABLE_COLOR
typedef struct colortype {
	short id;
		/* An ordinal number (if this color combo is for a multiline regex). */
	short fg;
		/* This combo's foreground color. */
	short bg;
		/* This combo's background color. */
	short pairnum;
		/* The pair number for this foreground/background color combination. */
	int attributes;
		/* Pair number and brightness composed into ready-to-use attributes. */
	regex_t *start;
		/* The compiled regular expression for 'start=', or the only one. */
	regex_t *end;
		/* The compiled regular expression for 'end=', if any. */
	struct colortype *next;
		/* Next color combination. */
} colortype;

typedef struct regexlisttype {
	regex_t *one_rgx;
		/* A regex to match things that imply a certain syntax. */
	struct regexlisttype *next;
		/* The next regex. */
} regexlisttype;

typedef struct augmentstruct {
	char *filename;
		/* The file where the syntax is extended. */
	ssize_t lineno;
		/* The number of the line of the extendsyntax command. */
	char *data;
		/* The text of the line. */
	struct augmentstruct *next;
		/* Next node. */
} augmentstruct;

typedef struct syntaxtype {
	char *name;
		/* The name of this syntax. */
	char *filename;
		/* File where the syntax is defined, or NULL if not an included file. */
	size_t lineno;
		/* The line number where the 'syntax' command was found. */
	augmentstruct *augmentations;
		/* List of extendsyntax commands to apply when loaded. */
	regexlisttype *extensions;
		/* The list of extensions that this syntax applies to. */
	regexlisttype *headers;
		/* The list of headerlines that this syntax applies to. */
	regexlisttype *magics;
		/* The list of libmagic results that this syntax applies to. */
	char *linter;
		/* The command with which to lint this type of file. */
	char *formatter;
		/* The command with which to format/modify/arrange this type of file. */
	char *tab;
		/* What the Tab key should produce; NULL for default behavior. */
#ifdef ENABLE_COMMENT
	char *comment;
		/* The line comment prefix (and postfix) for this type of file. */
#endif
	colortype *color;
		/* The colors and their regexes used in this syntax. */
	short nmultis;
		/* How many multiline regex strings this syntax has. */
	struct syntaxtype *next;
		/* Next syntax. */
} syntaxtype;

typedef struct lintstruct {
	ssize_t lineno;
		/* Line number of the error. */
	ssize_t colno;
		/* Column # of the error. */
	char *msg;
		/* Error message text. */
	char *filename;
		/* Filename. */
	struct lintstruct *next;
		/* Next error. */
	struct lintstruct *prev;
		/* Previous error. */
} lintstruct;
#endif /* ENABLE_COLOR */

/* More structure types. */
typedef struct linestruct {
	char *data;
		/* The text of this line. */
	ssize_t lineno;
		/* The number of this line. */
	struct linestruct *next;
		/* Next node. */
	struct linestruct *prev;
		/* Previous node. */
#ifdef ENABLE_COLOR
	short *multidata;
		/* Array of which multi-line regexes apply to this line. */
#endif
#ifndef NANO_TINY
	bool has_anchor;
		/* Whether the user has placed an anchor at this line. */
#endif
} linestruct;

#ifndef NANO_TINY
typedef struct groupstruct {
	ssize_t top_line;
		/* First line of group. */
	ssize_t bottom_line;
		/* Last line of group. */
	char **indentations;
		/* String data used to restore the affected lines; one per line. */
	struct groupstruct *next;
		/* The next group, if any. */
} groupstruct;

typedef struct undostruct {
	undo_type type;
		/* The operation type that this undo item is for. */
	int xflags;
		/* Some flag data to mark certain corner cases. */
	ssize_t head_lineno;
		/* The line number where the operation began or ended. */
	size_t head_x;
		/* The x position where the operation began or ended. */
	char *strdata;
		/* String data to help restore the affected line. */
	size_t wassize;
		/* The file size before the action. */
	size_t newsize;
		/* The file size after the action. */
	groupstruct *grouping;
		/* Undo info specific to groups of lines. */
	linestruct *cutbuffer;
		/* A copy of the cutbuffer. */
	ssize_t tail_lineno;
		/* Mostly the line number of the current line; sometimes something else. */
	size_t tail_x;
		/* The x position corresponding to the above line number. */
	struct undostruct *next;
		/* A pointer to the undo item of the preceding action. */
} undostruct;
#endif /* !NANO_TINY */

#ifdef ENABLE_HISTORIES
typedef struct poshiststruct {
	char *filename;
		/* The full path plus name of the file. */
	ssize_t linenumber;
		/* The line where the cursor was when we closed the file. */
	ssize_t columnnumber;
		/* The column where the cursor was. */
	struct poshiststruct *next;
		/* The next item of position history. */
} poshiststruct;
#endif

typedef struct openfilestruct {
	char *filename;
		/* The file's name. */
	linestruct *filetop;
		/* The file's first line. */
	linestruct *filebot;
		/* The file's last line. */
	linestruct *edittop;
		/* The current top of the edit window for this file. */
	linestruct *current;
		/* The current line for this file. */
	size_t totsize;
		/* The file's total number of characters. */
	size_t firstcolumn;
		/* The starting column of the top line of the edit window.
		 * When not in softwrap mode, it's always zero. */
	size_t current_x;
		/* The file's x-coordinate position. */
	size_t placewewant;
		/* The file's x position we would like. */
	ssize_t current_y;
		/* The file's y-coordinate position. */
	struct stat *statinfo;
		/* The file's stat information from when it was opened or last saved. */
#ifdef ENABLE_WRAPPING
	linestruct *spillage_line;
		/* The line for prepending stuff to during automatic hard-wrapping. */
#endif
#ifndef NANO_TINY
	linestruct *mark;
		/* The line in the file where the mark is set; NULL if not set. */
	size_t mark_x;
		/* The mark's x position in the above line. */
	bool softmark;
		/* Whether a marked region was made by holding Shift. */
	format_type fmt;
		/* The file's format -- Unix or DOS or Mac. */
	char *lock_filename;
		/* The path of the lockfile, if we created one. */
	undostruct *undotop;
		/* The top of the undo list. */
	undostruct *current_undo;
		/* The current (i.e. next) level of undo. */
	undostruct *last_saved;
		/* The undo item at which the file was last saved. */
	undo_type last_action;
		/* The type of the last action the user performed. */
#endif
	bool modified;
		/* Whether the file has been modified. */
#ifdef ENABLE_COLOR
	syntaxtype *syntax;
		/* The syntax that applies to this file, if any. */
#endif
#ifdef ENABLE_MULTIBUFFER
	char *errormessage;
		/* The ALERT message (if any) that occurred when opening the file. */
	struct openfilestruct *next;
		/* The next open file, if any. */
	struct openfilestruct *prev;
		/* The preceding open file, if any. */
#endif
} openfilestruct;

#ifdef ENABLE_NANORC
typedef struct rcoption {
	const char *name;
		/* The name of the rcfile option. */
	long flag;
		/* The flag associated with it, if any. */
} rcoption;
#endif

typedef struct keystruct {
	const char *keystr;
		/* The string that describes the keystroke, like "^C" or "M-R". */
	int keycode;
		/* The integer that, together with meta, identifies the keystroke. */
	int menus;
		/* The menus in which this keystroke is bound. */
	void (*func)(void);
		/* The function to which this keystroke is bound. */
#ifndef NANO_TINY
	int toggle;
		/* If a toggle, what we're toggling. */
	int ordinal;
		/* The how-manieth toggle this is, in order to be able to
		 * keep them in sequence. */
#endif
#ifdef ENABLE_NANORC
	char *expansion;
		/* The string of keycodes to which this shortcut is expanded. */
#endif
	struct keystruct *next;
		/* Next in the list. */
} keystruct;

typedef struct funcstruct {
	void (*func)(void);
		/* The actual function to call. */
	const char *tag;
		/* The function's help-line label, for example "Where Is". */
#ifdef ENABLE_HELP
	const char *phrase;
		/* The function's description for in the help viewer. */
	bool blank_after;
		/* Whether to distance this function from the next in the help viewer. */
#endif
	int menus;
		/* In what menus this function applies. */
	struct funcstruct *next;
		/* Next item in the list. */
} funcstruct;

#ifdef ENABLE_WORDCOMPLETION
typedef struct completionstruct {
	char *word;
	struct completionstruct *next;
} completionstruct;
#endif
