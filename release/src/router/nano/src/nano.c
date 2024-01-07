/**************************************************************************
 *   nano.c  --  This file is part of GNU nano.                           *
 *                                                                        *
 *   Copyright (C) 1999-2011, 2013-2023 Free Software Foundation, Inc.    *
 *   Copyright (C) 2014-2022 Benno Schulenberg                            *
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

#include "prototypes.h"
#include "revision.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#ifdef __linux__
#include <sys/ioctl.h>
#endif
#ifdef ENABLE_UTF8
#include <langinfo.h>
#endif
#include <locale.h>
#include <string.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#include <unistd.h>
#ifdef __linux__
#include <sys/vt.h>
#endif

#ifdef ENABLE_MULTIBUFFER
#define read_them_all  TRUE
#else
#define read_them_all  FALSE
#endif

#ifdef ENABLE_MOUSE
static int oldinterval = -1;
		/* Used to store the user's original mouse click interval. */
#endif
#ifdef HAVE_TERMIOS_H
static struct termios original_state;
		/* The original settings of the user's terminal. */
#else
# define tcsetattr(...)
# define tcgetattr(...)
#endif

static struct sigaction oldaction, newaction;
		/* Containers for the original and the temporary handler for SIGINT. */

/* Create a new linestruct node.  Note that we do not set prevnode->next. */
linestruct *make_new_node(linestruct *prevnode)
{
	linestruct *newnode = nmalloc(sizeof(linestruct));

	newnode->prev = prevnode;
	newnode->next = NULL;
	newnode->data = NULL;
#ifdef ENABLE_COLOR
	newnode->multidata = NULL;
#endif
	newnode->lineno = (prevnode) ? prevnode->lineno + 1 : 1;
#ifndef NANO_TINY
	newnode->has_anchor = FALSE;
#endif

	return newnode;
}

/* Splice a new node into an existing linked list of linestructs. */
void splice_node(linestruct *afterthis, linestruct *newnode)
{
	newnode->next = afterthis->next;
	newnode->prev = afterthis;
	if (afterthis->next != NULL)
		afterthis->next->prev = newnode;
	afterthis->next = newnode;

	/* Update filebot when inserting a node at the end of file. */
	if (openfile && openfile->filebot == afterthis)
		openfile->filebot = newnode;
}

/* Free the data structures in the given node. */
void delete_node(linestruct *line)
{
	/* If the first line on the screen gets deleted, step one back. */
	if (line == openfile->edittop)
		openfile->edittop = line->prev;
#ifdef ENABLE_WRAPPING
	/* If the spill-over line for hard-wrapping is deleted... */
	if (line == openfile->spillage_line)
		openfile->spillage_line = NULL;
#endif
	free(line->data);
#ifdef ENABLE_COLOR
	free(line->multidata);
#endif
	free(line);
}

/* Disconnect a node from a linked list of linestructs and delete it. */
void unlink_node(linestruct *line)
{
	if (line->prev != NULL)
		line->prev->next = line->next;
	if (line->next != NULL)
		line->next->prev = line->prev;

	/* Update filebot when removing a node at the end of file. */
	if (openfile && openfile->filebot == line)
		openfile->filebot = line->prev;

	delete_node(line);
}

/* Free an entire linked list of linestructs. */
void free_lines(linestruct *src)
{
	if (src == NULL)
		return;

	while (src->next != NULL) {
		src = src->next;
		delete_node(src->prev);
	}

	delete_node(src);
}

/* Make a copy of a linestruct node. */
linestruct *copy_node(const linestruct *src)
{
	linestruct *dst = nmalloc(sizeof(linestruct));

	dst->data = copy_of(src->data);
#ifdef ENABLE_COLOR
	dst->multidata = NULL;
#endif
	dst->lineno = src->lineno;
#ifndef NANO_TINY
	dst->has_anchor = src->has_anchor;
#endif

	return dst;
}

/* Duplicate an entire linked list of linestructs. */
linestruct *copy_buffer(const linestruct *src)
{
	linestruct *head, *item;

	head = copy_node(src);
	head->prev = NULL;

	item = head;
	src = src->next;

	while (src != NULL) {
		item->next = copy_node(src);
		item->next->prev = item;

		item = item->next;
		src = src->next;
	}

	item->next = NULL;

	return head;
}

/* Renumber the lines in a buffer, from the given line onwards. */
void renumber_from(linestruct *line)
{
	ssize_t number = (line->prev == NULL) ? 0 : line->prev->lineno;

	while (line != NULL) {
		line->lineno = ++number;
		line = line->next;
	}
}

/* Display a warning about a key disabled in view mode. */
void print_view_warning(void)
{
	statusline(AHEM, _("Key is invalid in view mode"));
}

/* When in restricted mode, show a warning and return TRUE. */
bool in_restricted_mode(void)
{
	if (ISSET(RESTRICTED)) {
		statusline(AHEM, _("This function is disabled in restricted mode"));
		beep();
		return TRUE;
	} else
		return FALSE;
}

#ifndef NANO_TINY
/* Say how the user can achieve suspension (when they typed ^Z). */
void suggest_ctrlT_ctrlZ(void)
{
#ifdef ENABLE_NANORC
	if (first_sc_for(MMAIN, do_execute) && first_sc_for(MMAIN, do_execute)->keycode == 0x14 &&
			first_sc_for(MEXECUTE, do_suspend) && first_sc_for(MEXECUTE, do_suspend)->keycode == 0x1A)
#endif
		statusline(AHEM, _("To suspend, type ^T^Z"));
}
#endif

/* Make sure the cursor is visible, then exit from curses mode, disable
 * bracketed-paste mode, and restore the original terminal settings. */
void restore_terminal(void)
{
	curs_set(1);
	endwin();
#ifndef NANO_TINY
	printf("\x1B[?2004l");
	fflush(stdout);
#endif
	tcsetattr(STDIN_FILENO, TCSANOW, &original_state);
}

/* Exit normally: restore terminal state and report any startup errors. */
void finish(void)
{
	/* Blank the status bar and (if applicable) the shortcut list. */
	blank_statusbar();
	blank_bottombars();
	wrefresh(footwin);

#ifndef NANO_TINY
	/* Deallocate the two or three subwindows. */
	if (topwin != NULL)
		delwin(topwin);
	delwin(midwin);
	delwin(footwin);
#endif
	/* Switch the cursor on, exit from curses, and restore terminal settings. */
	restore_terminal();

#if defined(ENABLE_NANORC) || defined(ENABLE_HISTORIES)
	display_rcfile_errors();
#endif

	/* Get out. */
	exit(0);
}

/* Close the current buffer, and terminate nano if it is the only buffer. */
void close_and_go(void)
{
#ifndef NANO_TINY
	if (openfile->lock_filename)
		delete_lockfile(openfile->lock_filename);
#endif
#ifdef ENABLE_HISTORIES
	if (ISSET(POSITIONLOG))
		update_poshistory();
#endif
#ifdef ENABLE_MULTIBUFFER
	/* If there is another buffer, close this one; otherwise just terminate. */
	if (openfile != openfile->next) {
		switch_to_next_buffer();
		openfile = openfile->prev;
		close_buffer();
		openfile = openfile->next;
		/* Adjust the count in the top bar. */
		titlebar(NULL);
	} else
#endif
	{
#ifdef ENABLE_HISTORIES
		if (ISSET(HISTORYLOG))
			save_history();
#endif
		finish();
	}
}

/* Close the current buffer if it is unmodified; otherwise (when not doing
 * automatic saving), ask the user whether to save it, then close it and
 * exit, or return when the user cancelled. */
void do_exit(void)
{
	int choice;

	/* When unmodified, simply close.  Else, when doing automatic saving
	 * and the file has a name, simply save.  Otherwise, ask the user. */
	if (!openfile->modified)
		choice = NO;
	else if (ISSET(SAVE_ON_EXIT) && openfile->filename[0] != '\0')
		choice = YES;
	else {
		if (ISSET(SAVE_ON_EXIT))
			warn_and_briefly_pause(_("No file name"));

		choice = ask_user(YESORNO, _("Save modified buffer? "));
	}

	/* When not saving, or the save succeeds, close the buffer. */
	if (choice == NO || (choice == YES && write_it_out(TRUE, TRUE) > 0))
		close_and_go();
	else if (choice != YES)
		statusbar(_("Cancelled"));
}

/* Save the current buffer under the given name (or "nano.<pid>" when nameless)
 * with suffix ".save".  If needed, the name is further suffixed to be unique. */
void emergency_save(const char *filename)
{
	char *plainname, *targetname;

	if (*filename == '\0') {
		plainname = nmalloc(28);
		sprintf(plainname, "nano.%u", getpid());
	} else
		plainname = copy_of(filename);

	targetname = get_next_filename(plainname, ".save");

	if (*targetname == '\0')
		fprintf(stderr, _("\nToo many .save files\n"));
	else if (write_file(targetname, NULL, SPECIAL, OVERWRITE, NONOTES)) {
		fprintf(stderr, _("\nBuffer written to %s\n"), targetname);
#if !defined(NANO_TINY) && defined(HAVE_CHMOD) && defined(HAVE_CHOWN)
		/* Try to chmod/chown the saved file to the values of the original file,
		 * but ignore any failure as we are in a hurry to get out. */
		if (openfile->statinfo) {
			IGNORE_CALL_RESULT(chmod(targetname, openfile->statinfo->st_mode));
			IGNORE_CALL_RESULT(chown(targetname, openfile->statinfo->st_uid,
													openfile->statinfo->st_gid));
		}
#endif
	}

	free(targetname);
	free(plainname);
}

/* Die gracefully -- by restoring the terminal state and saving any buffers
 * that were modified. */
void die(const char *msg, ...)
{
	openfilestruct *firstone = openfile;
	static int stabs = 0;
	va_list ap;

	/* When dying for a second time, just give up. */
	if (++stabs > 1)
		exit(11);

	restore_terminal();

#ifdef ENABLE_NANORC
	display_rcfile_errors();
#endif

	/* Display the dying message. */
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);

	while (openfile) {
#ifndef NANO_TINY
		/* If the current buffer has a lock file, remove it. */
		if (openfile->lock_filename)
			delete_lockfile(openfile->lock_filename);
#endif
		/* When modified, save the current buffer.  But not when in restricted
		 * mode, as it would write a file not mentioned on the command line. */
		if (openfile->modified && !ISSET(RESTRICTED))
			emergency_save(openfile->filename);

#ifdef ENABLE_MULTIBUFFER
		openfile = openfile->next;
#endif
		if (openfile == firstone)
			break;
	}

	/* Abandon the building. */
	exit(1);
}

/* Initialize the three window portions nano uses. */
void window_init(void)
{
	/* When resizing, first delete the existing windows. */
	if (midwin != NULL) {
		if (topwin != NULL)
			delwin(topwin);
		delwin(midwin);
		delwin(footwin);
	}

	topwin = NULL;

	/* If the terminal is very flat, don't set up a title bar. */
	if (LINES < 3) {
		editwinrows = (ISSET(ZERO) ? LINES : 1);
		/* Set up two subwindows.  If the terminal is just one line,
		 * edit window and status-bar window will cover each other. */
		midwin = newwin(editwinrows, COLS, 0, 0);
		footwin = newwin(1, COLS, LINES - 1, 0);
	} else {
		int toprows = ((ISSET(EMPTY_LINE) && LINES > 6) ? 2 : 1);
		int bottomrows = ((ISSET(NO_HELP) || LINES < 6) ? 1 : 3);

		if (ISSET(MINIBAR) || ISSET(ZERO))
			toprows = 0;

		editwinrows = LINES - toprows - bottomrows + (ISSET(ZERO) ? 1 : 0);

		/* Set up the normal three subwindows. */
		if (toprows > 0)
			topwin = newwin(toprows, COLS, 0, 0);
		midwin = newwin(editwinrows, COLS, toprows, 0);
		footwin = newwin(bottomrows, COLS, LINES - bottomrows, 0);
	}

	/* In case the terminal shrunk, make sure the status line is clear. */
	wnoutrefresh(footwin);

	/* When not disabled, turn escape-sequence translation on. */
	if (!ISSET(RAW_SEQUENCES)) {
		keypad(midwin, TRUE);
		keypad(footwin, TRUE);
	}

#ifdef ENABLED_WRAPORJUSTIFY
	/* Set up the wrapping point, accounting for screen width when negative. */
	if (COLS + fill < 0)
		wrap_at = 0;
	else if (fill <= 0)
		wrap_at = COLS + fill;
	else
		wrap_at = fill;
#endif
}

#ifdef ENABLE_MOUSE
void disable_mouse_support(void)
{
	mousemask(0, NULL);
	mouseinterval(oldinterval);
}

void enable_mouse_support(void)
{
	mousemask(ALL_MOUSE_EVENTS, NULL);
	oldinterval = mouseinterval(50);
}

/* Switch mouse support on or off, as needed. */
void mouse_init(void)
{
	if (ISSET(USE_MOUSE))
		enable_mouse_support();
	else
		disable_mouse_support();
}
#endif /* ENABLE_MOUSE */

/* Print the usage line for the given option to the screen. */
void print_opt(const char *shortflag, const char *longflag, const char *description)
{
	int firstwidth = breadth(shortflag);
	int secondwidth = breadth(longflag);

	printf(" %s", shortflag);
	if (firstwidth < 14)
		printf("%*s", 14 - firstwidth, " ");

	printf(" %s", longflag);
	if (secondwidth < 24)
		printf("%*s", 24 - secondwidth, " ");

	printf("%s\n", _(description));
}

/* Explain how to properly use nano and its command-line options. */
void usage(void)
{
	printf(_("Usage: nano [OPTIONS] [[+LINE[,COLUMN]] FILE]...\n\n"));
#ifndef NANO_TINY
	/* TRANSLATORS: The next two strings are part of the --help output.
	 * It's best to keep its lines within 80 characters. */
	printf(_("To place the cursor on a specific line of a file, put the line number with\n"
				"a '+' before the filename.  The column number can be added after a comma.\n"));
	printf(_("When a filename is '-', nano reads data from standard input.\n\n"));
	/* TRANSLATORS: The next three are column headers of the --help output. */
	print_opt(_("Option"), _("Long option"), N_("Meaning"));
	/* TRANSLATORS: The next forty or so strings are option descriptions
	 * for the --help output.  Try to keep them at most 40 characters. */
	print_opt("-A", "--smarthome", N_("Enable smart home key"));
	if (!ISSET(RESTRICTED)) {
		print_opt("-B", "--backup", N_("Save backups of existing files"));
		print_opt(_("-C <dir>"), _("--backupdir=<dir>"),
					N_("Directory for saving unique backup files"));
	}
#endif
	print_opt("-D", "--boldtext", N_("Use bold instead of reverse video text"));
#ifndef NANO_TINY
	print_opt("-E", "--tabstospaces", N_("Convert typed tabs to spaces"));
#endif
#ifdef ENABLE_MULTIBUFFER
	if (!ISSET(RESTRICTED))
		print_opt("-F", "--multibuffer",
					N_("Read a file into a new buffer by default"));
#endif
#ifndef NANO_TINY
	print_opt("-G", "--locking", N_("Use (vim-style) lock files"));
#endif
#ifdef ENABLE_HISTORIES
	if (!ISSET(RESTRICTED))
		print_opt("-H", "--historylog",
					N_("Save & reload old search/replace strings"));
#endif
#ifdef ENABLE_NANORC
	print_opt("-I", "--ignorercfiles", N_("Don't look at nanorc files"));
#endif
#ifndef NANO_TINY
	print_opt(_("-J <number>"), _("--guidestripe=<number>"),
					N_("Show a guiding bar at this column"));
#endif
	print_opt("-K", "--rawsequences",
					N_("Fix numeric keypad key confusion problem"));
#ifndef NANO_TINY
	print_opt("-L", "--nonewlines",
					N_("Don't add an automatic newline"));
#endif
#ifdef ENABLED_WRAPORJUSTIFY
	print_opt("-M", "--trimblanks",
					N_("Trim tail spaces when hard-wrapping"));
#endif
#ifndef NANO_TINY
	print_opt("-N", "--noconvert",
					N_("Don't convert files from DOS/Mac format"));
	print_opt("-O", "--bookstyle",
					N_("Leading whitespace means new paragraph"));
#endif
#ifdef ENABLE_HISTORIES
	if (!ISSET(RESTRICTED))
		print_opt("-P", "--positionlog",
					N_("Save & restore position of the cursor"));
#endif
#ifdef ENABLE_JUSTIFY
	print_opt(_("-Q <regex>"), _("--quotestr=<regex>"),
					/* TRANSLATORS: This refers to email quoting,
					 * like the > in: > quoted text. */
					N_("Regular expression to match quoting"));
#endif
	if (!ISSET(RESTRICTED))
		print_opt("-R", "--restricted", N_("Restrict access to the filesystem"));
#ifndef NANO_TINY
	print_opt("-S", "--softwrap", N_("Display overlong lines on multiple rows"));
	print_opt(_("-T <number>"), _("--tabsize=<number>"),
					N_("Make a tab this number of columns wide"));
#endif
	print_opt("-U", "--quickblank", N_("Wipe status bar upon next keystroke"));
	print_opt("-V", "--version", N_("Print version information and exit"));
#ifndef NANO_TINY
	print_opt("-W", "--wordbounds",
					N_("Detect word boundaries more accurately"));
	print_opt(_("-X <string>"), _("--wordchars=<string>"),
					N_("Which other characters are word parts"));
#endif
#ifdef ENABLE_COLOR
	if (!ISSET(RESTRICTED))
		print_opt(_("-Y <name>"), _("--syntax=<name>"),
					N_("Syntax definition to use for coloring"));
#endif
#ifndef NANO_TINY
	print_opt("-Z", "--zap", N_("Let Bsp and Del erase a marked region"));
	print_opt("-a", "--atblanks", N_("When soft-wrapping, do it at whitespace"));
#endif
#ifdef ENABLE_WRAPPING
	print_opt("-b", "--breaklonglines", N_("Automatically hard-wrap overlong lines"));
#endif
	print_opt("-c", "--constantshow", N_("Constantly show cursor position"));
	print_opt("-d", "--rebinddelete",
					N_("Fix Backspace/Delete confusion problem"));
#ifndef NANO_TINY
	print_opt("-e", "--emptyline", N_("Keep the line below the title bar empty"));
#endif
#ifdef ENABLE_NANORC
	print_opt(_("-f <file>"), _("--rcfile=<file>"),
					N_("Use only this file for configuring nano"));
#endif
#if defined(ENABLE_BROWSER) || defined(ENABLE_HELP)
	print_opt("-g", "--showcursor", N_("Show cursor in file browser & help text"));
#endif
	print_opt("-h", "--help", N_("Show this help text and exit"));
#ifndef NANO_TINY
	print_opt("-i", "--autoindent", N_("Automatically indent new lines"));
	print_opt("-j", "--jumpyscrolling", N_("Scroll per half-screen, not per line"));
	print_opt("-k", "--cutfromcursor", N_("Cut from cursor to end of line"));
#endif
#ifdef ENABLE_LINENUMBERS
	print_opt("-l", "--linenumbers", N_("Show line numbers in front of the text"));
#endif
#ifdef ENABLE_MOUSE
	print_opt("-m", "--mouse", N_("Enable the use of the mouse"));
#endif
#ifndef NANO_TINY
	print_opt("-n", "--noread", N_("Do not read the file (only write it)"));
#endif
#ifdef ENABLE_OPERATINGDIR
	print_opt(_("-o <dir>"), _("--operatingdir=<dir>"),
					N_("Set operating directory"));
#endif
	print_opt("-p", "--preserve", N_("Preserve XON (^Q) and XOFF (^S) keys"));
#ifndef NANO_TINY
	print_opt("-q", "--indicator", N_("Show a position+portion indicator"));
#endif
#ifdef ENABLED_WRAPORJUSTIFY
	print_opt(_("-r <number>"), _("--fill=<number>"),
					N_("Set width for hard-wrap and justify"));
#endif
#ifdef ENABLE_SPELLER
	if (!ISSET(RESTRICTED))
		print_opt(_("-s <program>"), _("--speller=<program>"),
					N_("Use this alternative spell checker"));
#endif
	print_opt("-t", "--saveonexit", N_("Save changes on exit, don't prompt"));
#ifndef NANO_TINY
	print_opt("-u", "--unix", N_("Save a file by default in Unix format"));
#endif
	print_opt("-v", "--view", N_("View mode (read-only)"));
#ifdef ENABLE_WRAPPING
	print_opt("-w", "--nowrap", N_("Don't hard-wrap long lines [default]"));
#endif
	print_opt("-x", "--nohelp", N_("Don't show the two help lines"));
#ifndef NANO_TINY
	print_opt("-y", "--afterends", N_("Make Ctrl+Right stop at word ends"));
#endif
#ifdef HAVE_LIBMAGIC
	print_opt("-!", "--magic", N_("Also try magic to determine syntax"));
#endif
#ifndef NANO_TINY
	print_opt("-%", "--stateflags", N_("Show some states on the title bar"));
	print_opt("-_", "--minibar", N_("Show a feedback bar at the bottom"));
	print_opt("-0", "--zero", N_("Hide all bars, use whole terminal"));
#endif
}

/* Display the version number of this nano, a copyright notice, some contact
 * information, and the configuration options this nano was compiled with. */
void version(void)
{
#ifdef REVISION
	printf(" GNU nano from git, %s\n", REVISION);
#else
	printf(_(" GNU nano, version %s\n"), VERSION);
#endif
#ifndef NANO_TINY
	/* TRANSLATORS: The %s is the year of the latest release. */
	printf(_(" (C) %s the Free Software Foundation and various contributors\n"), "2023");
#endif
	printf(_(" Compiled options:"));

#ifdef NANO_TINY
	printf(" --enable-tiny");
#ifdef ENABLE_BROWSER
	printf(" --enable-browser");
#endif
#ifdef ENABLE_COLOR
	printf(" --enable-color");
#endif
#ifdef ENABLE_EXTRA
	printf(" --enable-extra");
#endif
#ifdef ENABLE_FORMATTER
	printf(" --enable-formatter");
#endif
#ifdef ENABLE_HELP
	printf(" --enable-help");
#endif
#ifdef ENABLE_HISTORIES
	printf(" --enable-histories");
#endif
#ifdef ENABLE_JUSTIFY
	printf(" --enable-justify");
#endif
#ifdef HAVE_LIBMAGIC
	printf(" --enable-libmagic");
#endif
#ifdef ENABLE_LINENUMBERS
	printf(" --enable-linenumbers");
#endif
#ifdef ENABLE_LINTER
	printf(" --enable-linter");
#endif
#ifdef ENABLE_MOUSE
	printf(" --enable-mouse");
#endif
#ifdef ENABLE_NANORC
	printf(" --enable-nanorc");
#endif
#ifdef ENABLE_MULTIBUFFER
	printf(" --enable-multibuffer");
#endif
#ifdef ENABLE_OPERATINGDIR
	printf(" --enable-operatingdir");
#endif
#ifdef ENABLE_SPELLER
	printf(" --enable-speller");
#endif
#ifdef ENABLE_TABCOMP
	printf(" --enable-tabcomp");
#endif
#ifdef ENABLE_WRAPPING
	printf(" --enable-wrapping");
#endif
#else /* !NANO_TINY */
#ifndef ENABLE_BROWSER
	printf(" --disable-browser");
#endif
#ifndef ENABLE_COLOR
	printf(" --disable-color");
#endif
#ifndef ENABLE_COMMENT
	printf(" --disable-comment");
#endif
#ifndef ENABLE_EXTRA
	printf(" --disable-extra");
#endif
#ifndef ENABLE_FORMATTER
	printf(" --disable-formatter");
#endif
#ifndef ENABLE_HELP
	printf(" --disable-help");
#endif
#ifndef ENABLE_HISTORIES
	printf(" --disable-histories");
#endif
#ifndef ENABLE_JUSTIFY
	printf(" --disable-justify");
#endif
#ifndef HAVE_LIBMAGIC
	printf(" --disable-libmagic");
#endif
#ifndef ENABLE_LINENUMBERS
	printf(" --disable-linenumbers");
#endif
#ifndef ENABLE_LINTER
	printf(" --disable-linter");
#endif
#ifndef ENABLE_MOUSE
	printf(" --disable-mouse");
#endif
#ifndef ENABLE_MULTIBUFFER
	printf(" --disable-multibuffer");
#endif
#ifndef ENABLE_NANORC
	printf(" --disable-nanorc");
#endif
#ifndef ENABLE_OPERATINGDIR
	printf(" --disable-operatingdir");
#endif
#ifndef ENABLE_SPELLER
	printf(" --disable-speller");
#endif
#ifndef ENABLE_TABCOMP
	printf(" --disable-tabcomp");
#endif
#ifndef ENABLE_WORDCOMPLETION
	printf(" --disable-wordcomp");
#endif
#ifndef ENABLE_WRAPPING
	printf(" --disable-wrapping");
#endif
#endif /* !NANO_TINY */

#ifdef DEBUG
	printf(" --enable-debug");
#endif
#ifndef ENABLE_NLS
	printf(" --disable-nls");
#endif
#ifdef ENABLE_UTF8
	printf(" --enable-utf8");
#else
	printf(" --disable-utf8");
#endif
	printf("\n");
}

/* Register that Ctrl+C was pressed during some system call. */
void make_a_note(int signal)
{
	control_C_was_pressed = TRUE;
}

/* Make ^C interrupt a system call and set a flag. */
void install_handler_for_Ctrl_C(void)
{
	/* Enable the generation of a SIGINT when ^C is pressed. */
	enable_kb_interrupt();

	/* Set up a signal handler so that pressing ^C will set a flag. */
	newaction.sa_handler = make_a_note;
	newaction.sa_flags = 0;
	sigaction(SIGINT, &newaction, &oldaction);
}

/* Go back to ignoring ^C. */
void restore_handler_for_Ctrl_C(void)
{
	sigaction(SIGINT, &oldaction, NULL);
	disable_kb_interrupt();
}

#ifndef NANO_TINY
/* Reconnect standard input to the tty, and store its state. */
void reconnect_and_store_state(void)
{
	int thetty = open("/dev/tty", O_RDONLY);

	if (thetty < 0 || dup2(thetty, STDIN_FILENO) < 0)
		die(_("Could not reconnect stdin to keyboard\n"));

	close(thetty);

	/* If input was not cut short, store the current state of the terminal. */
	if (!control_C_was_pressed)
		tcgetattr(STDIN_FILENO, &original_state);
}

/* Read whatever comes from standard input into a new buffer. */
bool scoop_stdin(void)
{
	FILE *stream;

	restore_terminal();

	/* When input comes from a terminal, show a helpful message. */
	if (isatty(STDIN_FILENO))
		fprintf(stderr, _("Reading data from keyboard; "
							"type ^D or ^D^D to finish.\n"));

	/* Open standard input. */
	stream = fopen("/dev/stdin", "rb");
	if (stream == NULL) {
		int errnumber = errno;

		terminal_init();
		doupdate();
		statusline(ALERT, _("Failed to open stdin: %s"), strerror(errnumber));
		return FALSE;
	}

	/* Set up a signal handler so that ^C will stop the reading. */
	install_handler_for_Ctrl_C();

	/* Read the input into a new buffer. */
	make_new_buffer();
	read_file(stream, 0, "stdin", TRUE);
#ifdef ENABLE_COLOR
	find_and_prime_applicable_syntax();
#endif

	/* Restore the original ^C handler. */
	restore_handler_for_Ctrl_C();

	if (!ISSET(VIEW_MODE) && openfile->totsize > 0)
		set_modified();

	return TRUE;
}
#endif

/* Register half a dozen signal handlers. */
void signal_init(void)
{
	struct sigaction deed = {{0}};

	/* Trap SIGINT and SIGQUIT because we want them to do useful things. */
	deed.sa_handler = SIG_IGN;
	sigaction(SIGINT, &deed, NULL);
#ifdef SIGQUIT
	sigaction(SIGQUIT, &deed, NULL);
#endif

	/* Trap SIGHUP and SIGTERM because we want to write the file out. */
	deed.sa_handler = handle_hupterm;
#ifdef SIGHUP
	sigaction(SIGHUP, &deed, NULL);
#endif
	sigaction(SIGTERM, &deed, NULL);

#ifndef NANO_TINY
#ifdef SIGWINCH
	/* Trap SIGWINCH because we want to handle window resizes. */
	deed.sa_handler = handle_sigwinch;
	sigaction(SIGWINCH, &deed, NULL);
#endif
#ifdef SIGTSTP
	/* Prevent the suspend handler from getting interrupted. */
	sigfillset(&deed.sa_mask);
	deed.sa_handler = suspend_nano;
	sigaction(SIGTSTP, &deed, NULL);
#endif
#endif /* !NANO_TINY */
#ifdef SIGCONT
	sigfillset(&deed.sa_mask);
	deed.sa_handler = continue_nano;
	sigaction(SIGCONT, &deed, NULL);
#endif

#if !defined(NANO_TINY) && !defined(DEBUG)
	if (getenv("NANO_NOCATCH") == NULL) {
		/* Trap SIGSEGV and SIGABRT to save any changed buffers and reset
		 * the terminal to a usable state.  Reset these handlers to their
		 * defaults as soon as their signal fires. */
		deed.sa_handler = handle_crash;
		deed.sa_flags |= SA_RESETHAND;
		sigaction(SIGSEGV, &deed, NULL);
		sigaction(SIGABRT, &deed, NULL);
	}
#endif
}

/* Handler for SIGHUP (hangup) and SIGTERM (terminate). */
void handle_hupterm(int signal)
{
	die(_("Received SIGHUP or SIGTERM\n"));
}

#if !defined(NANO_TINY) && !defined(DEBUG)
/* Handler for SIGSEGV (segfault) and SIGABRT (abort). */
void handle_crash(int signal)
{
	die(_("Sorry! Nano crashed!  Code: %d.  Please report a bug.\n"), signal);
}
#endif

#ifndef NANO_TINY
/* Handler for SIGTSTP (suspend). */
void suspend_nano(int signal)
{
#ifdef ENABLE_MOUSE
	disable_mouse_support();
#endif
	restore_terminal();

	printf("\n\n");

	/* Display our helpful message. */
	printf(_("Use \"fg\" to return to nano.\n"));
	fflush(stdout);

	/* The suspend keystroke must not elicit cursor-position display. */
	lastmessage = HUSH;

#ifdef SIGSTOP
	/* Do what mutt does: send ourselves a SIGSTOP. */
	kill(0, SIGSTOP);
#endif
}

/* When permitted, put nano to sleep. */
void do_suspend(void)
{
	if (in_restricted_mode())
		return;

	suspend_nano(0);

	ran_a_tool = TRUE;
}
#endif /* !NANO_TINY */

/* Handler for SIGCONT (continue after suspend). */
void continue_nano(int signal)
{
#ifdef ENABLE_MOUSE
	if (ISSET(USE_MOUSE))
		enable_mouse_support();
#endif

#ifndef NANO_TINY
	/* Perhaps the user resized the window while we slept. */
	the_window_resized = TRUE;
#else
	/* Put the terminal in the desired state again. */
	terminal_init();
#endif

	/* Insert a fake keystroke, to neutralize a key-eating issue. */
	ungetch(KEY_FRESH);
}

#if !defined(NANO_TINY) || defined(ENABLE_SPELLER) || defined(ENABLE_COLOR)
/* Block or unblock the SIGWINCH signal, depending on the blockit parameter. */
void block_sigwinch(bool blockit)
{
#ifdef SIGWINCH
	sigset_t winch;

	sigemptyset(&winch);
	sigaddset(&winch, SIGWINCH);
	sigprocmask(blockit ? SIG_BLOCK : SIG_UNBLOCK, &winch, NULL);
#endif

#ifndef NANO_TINY
	if (the_window_resized)
		regenerate_screen();
#endif
}
#endif

#ifndef NANO_TINY
/* Handler for SIGWINCH (window size change). */
void handle_sigwinch(int signal)
{
	/* Let the input routine know that a SIGWINCH has occurred. */
	the_window_resized = TRUE;
}

/* Reinitialize and redraw the screen completely. */
void regenerate_screen(void)
{
	/* Reset the trigger. */
	the_window_resized = FALSE;

	/* Leave and immediately reenter curses mode, so that ncurses notices
	 * the new screen dimensions and sets LINES and COLS accordingly. */
	endwin();
	refresh();

	thebar = (ISSET(INDICATOR) && LINES > 5 && COLS > 9) ? 1 : 0;
	bardata = nrealloc(bardata, LINES * sizeof(int));

	editwincols = COLS - margin - thebar;

	/* Put the terminal in the desired state again, and
	 * recreate the subwindows with their (new) sizes. */
	terminal_init();
	window_init();

	/* If we have an open buffer, redraw the contents of the subwindows. */
	if (openfile) {
		ensure_firstcolumn_is_aligned();
		draw_all_subwindows();
	}
}

/* Invert the given global flag and adjust things for its new value. */
void toggle_this(int flag)
{
	bool enabled = !ISSET(flag);

	TOGGLE(flag);
	focusing = FALSE;

	switch (flag) {
		case ZERO:
			window_init();
			draw_all_subwindows();
			return;
		case NO_HELP:
			if (LINES < 6) {
				statusline(AHEM, _("Too tiny"));
				TOGGLE(flag);
				return;
			}
			window_init();
			draw_all_subwindows();
			break;
		case CONSTANT_SHOW:
			if (ISSET(ZERO) || LINES == 1) {
				statusline(AHEM, _("Not possible"));
				TOGGLE(flag);
			} else if (!ISSET(MINIBAR))
				wipe_statusbar();
			return;
		case SOFTWRAP:
			if (!ISSET(SOFTWRAP))
				openfile->firstcolumn = 0;
			refresh_needed = TRUE;
			break;
		case WHITESPACE_DISPLAY:
			titlebar(NULL);
			refresh_needed = TRUE;
			break;
#ifdef ENABLE_COLOR
		case NO_SYNTAX:
			precalc_multicolorinfo();
			refresh_needed = TRUE;
			break;
		case TABS_TO_SPACES:
			if (openfile->syntax && openfile->syntax->tab) {
				statusline(AHEM, _("Current syntax determines Tab"));
				TOGGLE(flag);
				return;
			}
			break;
#endif
#ifdef ENABLE_MOUSE
		case USE_MOUSE:
			mouse_init();
			break;
#endif
	}

	if (flag == AUTOINDENT || flag == BREAK_LONG_LINES || flag == SOFTWRAP) {
		if (ISSET(MINIBAR) && !ISSET(ZERO) && ISSET(STATEFLAGS))
			return;
		if (ISSET(STATEFLAGS))
			titlebar(NULL);
	}

	if (flag == NO_HELP || flag == LINE_NUMBERS || flag == WHITESPACE_DISPLAY)
		if (ISSET(MINIBAR) || ISSET(ZERO) || LINES == 1)
			return;

	if (flag == NO_HELP || flag == NO_SYNTAX)
		enabled = !enabled;

	statusline(REMARK, "%s %s", _(epithet_of_flag(flag)),
									enabled ? _("enabled") : _("disabled"));
}
#endif /* !NANO_TINY */

/* Disable extended input and output processing in our terminal settings. */
void disable_extended_io(void)
{
#ifdef HAVE_TERMIOS_H
	struct termios settings = {0};

	tcgetattr(0, &settings);
	settings.c_lflag &= ~IEXTEN;
	settings.c_oflag &= ~OPOST;
	tcsetattr(0, TCSANOW, &settings);
#endif
}

/* Stop ^C from generating a SIGINT. */
void disable_kb_interrupt(void)
{
#ifdef HAVE_TERMIOS_H
	struct termios settings = {0};

	tcgetattr(0, &settings);
	settings.c_lflag &= ~ISIG;
	tcsetattr(0, TCSANOW, &settings);
#endif
}

/* Make ^C generate a SIGINT. */
void enable_kb_interrupt(void)
{
#ifdef HAVE_TERMIOS_H
	struct termios settings = {0};

	tcgetattr(0, &settings);
	settings.c_lflag |= ISIG;
	tcsetattr(0, TCSANOW, &settings);
#endif
}

/* Disable the terminal's XON/XOFF flow-control characters. */
void disable_flow_control(void)
{
#ifdef HAVE_TERMIOS_H
	struct termios settings;

	tcgetattr(0, &settings);
	settings.c_iflag &= ~IXON;
	tcsetattr(0, TCSANOW, &settings);
#endif
}

/* Enable the terminal's XON/XOFF flow-control characters. */
void enable_flow_control(void)
{
#ifdef HAVE_TERMIOS_H
	struct termios settings;

	tcgetattr(0, &settings);
	settings.c_iflag |= IXON;
	tcsetattr(0, TCSANOW, &settings);
#endif
}

/* Set up the terminal state.  Put the terminal in raw mode (read one
 * character at a time, disable the special control keys, and disable
 * the flow control characters), disable translation of carriage return
 * (^M) into newline (^J) so that we can tell the difference between the
 * Enter key and Ctrl-J, and disable echoing of characters as they're
 * typed.  Finally, disable extended input and output processing, and,
 * if we're not in preserve mode, reenable interpretation of the flow
 * control characters. */
void terminal_init(void)
{
	raw();
	nonl();
	noecho();

	disable_extended_io();

	if (ISSET(PRESERVE))
		enable_flow_control();

	disable_kb_interrupt();

#ifndef NANO_TINY
	/* Tell the terminal to enable bracketed pastes. */
	printf("\x1B[?2004h");
	fflush(stdout);
#endif
}

/* Ask ncurses for a keycode, or assign a default one. */
int get_keycode(const char *keyname, const int standard)
{
#ifdef HAVE_KEY_DEFINED
	const char *keyvalue = tigetstr(keyname);

	if (keyvalue != 0 && keyvalue != (char *)-1 && key_defined(keyvalue))
		return key_defined(keyvalue);
#endif
#ifdef DEBUG
	if (!ISSET(RAW_SEQUENCES))
		fprintf(stderr, "Using fallback keycode for %s\n", keyname);
#endif
	return standard;
}

#ifdef ENABLE_LINENUMBERS
/* Ensure that the margin can accommodate the buffer's highest line number. */
void confirm_margin(void)
{
	int needed_margin = digits(openfile->filebot->lineno) + 1;

	/* When not requested or space is too tight, suppress line numbers. */
	if (!ISSET(LINE_NUMBERS) || needed_margin > COLS - 4)
		needed_margin = 0;

	if (needed_margin != margin) {
		bool keep_focus = (margin > 0) && focusing;

		margin = needed_margin;
		editwincols = COLS - margin - thebar;

#ifndef NANO_TINY
		/* Ensure a proper starting column for the first screen row. */
		ensure_firstcolumn_is_aligned();
#endif
		focusing = keep_focus;

		/* The margin has changed -- schedule a full refresh. */
		refresh_needed = TRUE;
	}
}
#endif /* ENABLE_LINENUMBERS */

/* Say that an unbound key was struck, and if possible which one. */
void unbound_key(int code)
{
	if (code == FOREIGN_SEQUENCE)
		/* TRANSLATORS: This refers to a sequence of escape codes
		 * (from the keyboard) that nano does not recognize. */
		statusline(AHEM, _("Unknown sequence"));
#ifdef ENABLE_NANORC
	else if (code == NO_SUCH_FUNCTION)
		statusline(AHEM, _("Unknown function: %s"), commandname);
	else if (code == MISSING_BRACE)
		statusline(AHEM, _("Missing }"));
#endif
#ifndef NANO_TINY
	else if (code > KEY_F0 && code < KEY_F0 + 25)
		/* TRANSLATORS: This refers to an unbound function key. */
		statusline(AHEM, _("Unbound key: F%i"), code - KEY_F0);
#endif
	else if (code > 0x7F)
		statusline(AHEM, _("Unbound key"));
	else if (meta_key) {
#ifndef NANO_TINY
		if (code < 0x20)
			statusline(AHEM, _("Unbindable key: M-^%c"), code + 0x40);
		else
#endif
#ifdef ENABLE_NANORC
		if (shifted_metas && 'A' <= code && code <= 'Z')
			statusline(AHEM, _("Unbound key: %s%c"), "Sh-M-", code);
		else
#endif
			statusline(AHEM, _("Unbound key: %s%c"), "M-", toupper(code));
	} else if (code == ESC_CODE)
		statusline(AHEM, _("Unbindable key: ^["));
	else if (code < 0x20)
		statusline(AHEM, _("Unbound key: %s%c"), "^", code + 0x40);
#if defined(ENABLE_BROWSER) || defined (ENABLE_HELP)
	else
		statusline(AHEM, _("Unbound key: %s%c"), "", code);
#endif
	set_blankdelay_to_one();
}

#ifdef ENABLE_MOUSE
/* Handle a mouse click on the edit window or the shortcut list. */
int do_mouse(void)
{
	int click_row, click_col;
	int retval = get_mouseinput(&click_row, &click_col, TRUE);

	/* If the click is wrong or already handled, we're done. */
	if (retval != 0)
		return retval;

	/* If the click was in the edit window, put the cursor in that spot. */
	if (wmouse_trafo(midwin, &click_row, &click_col, FALSE)) {
		linestruct *current_save = openfile->current;
		ssize_t row_count = click_row - openfile->current_y;
		size_t leftedge;
#ifndef NANO_TINY
		size_t current_x_save = openfile->current_x;
		bool sameline = (click_row == openfile->current_y);
			/* Whether the click was on the row where the cursor is. */

		if (ISSET(SOFTWRAP))
			leftedge = leftedge_for(xplustabs(), openfile->current);
		else
#endif
			leftedge = get_page_start(xplustabs());

		/* Move current up or down to the row that was clicked on. */
		if (row_count < 0)
			go_back_chunks(-row_count, &openfile->current, &leftedge);
		else
			go_forward_chunks(row_count, &openfile->current, &leftedge);

		openfile->current_x = actual_x(openfile->current->data,
								actual_last_column(leftedge, click_col));

#ifndef NANO_TINY
		/* Clicking where the cursor is toggles the mark, as does clicking
		 * beyond the line length with the cursor at the end of the line. */
		if (sameline && openfile->current_x == current_x_save) {
			do_mark();
			if (ISSET(STATEFLAGS))
				titlebar(NULL);
		} else
#endif
			/* The cursor moved; clean the cutbuffer on the next cut. */
			keep_cutbuffer = FALSE;

		edit_redraw(current_save, CENTERING);
	}

	/* No more handling is needed. */
	return 2;
}
#endif /* ENABLE_MOUSE */

/* Return TRUE when the given function is a cursor-moving command. */
bool wanted_to_move(void (*func)(void))
{
	return (func == do_left || func == do_right ||
			func == do_up || func == do_down ||
			func == do_home || func == do_end ||
			func == to_prev_word || func == to_next_word ||
#ifdef ENABLE_JUSTIFY
			func == to_para_begin || func == to_para_end ||
#endif
			func == to_prev_block || func == to_next_block ||
			func == do_page_up || func == do_page_down ||
			func == to_first_line || func == to_last_line);
}

/* Return TRUE when the given function makes a change -- no good for view mode. */
bool changes_something(functionptrtype f)
{
	return (f == do_savefile || f == do_writeout || f == do_enter || f == do_tab ||
			f == do_delete || f == do_backspace || f == cut_text || f == paste_text ||
#ifndef NANO_TINY
			f == chop_previous_word || f == chop_next_word ||
			f == zap_text || f == cut_till_eof || f == do_execute ||
			f == do_indent || f == do_unindent ||
#endif
#ifdef ENABLE_JUSTIFY
			f == do_justify || f == do_full_justify ||
#endif
#ifdef ENABLE_COMMENT
			f == do_comment ||
#endif
#ifdef ENABLE_SPELLER
			f == do_spell ||
#endif
#ifdef ENABLE_FORMATTER
			f == do_formatter ||
#endif
#ifdef ENABLE_WORDCOMPLETION
			f == complete_a_word ||
#endif
			f == do_replace || f == do_verbatim_input);
}

#ifndef NANO_TINY
/* Read in all waiting input bytes and paste them into the buffer in one go. */
void suck_up_input_and_paste_it(void)
{
	linestruct *was_cutbuffer = cutbuffer;
	linestruct *line = make_new_node(NULL);
	size_t index = 0;

	line->data = copy_of("");
	cutbuffer = line;

	while (bracketed_paste) {
		int input = get_kbinput(midwin, BLIND);

		if (input == '\r' || input == '\n') {
			line->next = make_new_node(line);
			line = line->next;
			line->data = copy_of("");
			index = 0;
		} else if ((0x20 <= input && input <= 0xFF && input != DEL_CODE) ||
														input == '\t') {
			line->data = nrealloc(line->data, index + 2);
			line->data[index++] = (char)input;
			line->data[index] = '\0';
		} else if (input != BRACKETED_PASTE_MARKER)
			beep();
	}

	if (ISSET(VIEW_MODE))
		print_view_warning();
	else
		paste_text();

	free_lines(cutbuffer);
	cutbuffer = was_cutbuffer;
}
#endif

/* Insert the given short burst of bytes into the edit buffer. */
void inject(char *burst, size_t count)
{
	linestruct *thisline = openfile->current;
	size_t datalen = strlen(thisline->data);
#ifndef NANO_TINY
	size_t original_row = 0;
	size_t old_amount = 0;

	if (ISSET(SOFTWRAP)) {
		if (openfile->current_y == editwinrows - 1)
			original_row = chunk_for(xplustabs(), thisline);
		old_amount = extra_chunks_in(thisline);
	}
#endif

	/* Encode an embedded NUL byte as 0x0A. */
	for (size_t index = 0; index < count; index++)
		if (burst[index] == '\0')
			burst[index] = '\n';

#ifndef NANO_TINY
	/* Only add a new undo item when the current item is not an ADD or when
	 * the current typing is not contiguous with the previous typing. */
	if (openfile->last_action != ADD ||
				openfile->current_undo->tail_lineno != thisline->lineno ||
				openfile->current_undo->tail_x != openfile->current_x)
		add_undo(ADD, NULL);
#endif

	/* Make room for the new bytes and copy them into the line. */
	thisline->data = nrealloc(thisline->data, datalen + count + 1);
	memmove(thisline->data + openfile->current_x + count,
						thisline->data + openfile->current_x,
						datalen - openfile->current_x + 1);
	strncpy(thisline->data + openfile->current_x, burst, count);

#ifndef NANO_TINY
	/* When the mark is to the right of the cursor, compensate its position. */
	if (thisline == openfile->mark && openfile->current_x < openfile->mark_x)
		openfile->mark_x += count;

	/* When the cursor is on the top row and not on the first chunk
	 * of a line, adding text there might change the preceding chunk
	 * and thus require an adjustment of firstcolumn. */
	if (thisline == openfile->edittop && openfile->firstcolumn > 0) {
		ensure_firstcolumn_is_aligned();
		refresh_needed = TRUE;
	}
#endif
	/* If text was added to the magic line, create a new magic line. */
	if (thisline == openfile->filebot && !ISSET(NO_NEWLINES)) {
		new_magicline();
		if (margin > 0)
			refresh_needed = TRUE;
	}

	openfile->current_x += count;

	openfile->totsize += mbstrlen(burst);
	set_modified();

#ifndef NANO_TINY
	update_undo(ADD);
#endif

#ifdef ENABLE_WRAPPING
	if (ISSET(BREAK_LONG_LINES))
		do_wrap();
#endif

#ifndef NANO_TINY
	/* When softwrapping and the number of chunks in the current line changed,
	 * or we were on the last row of the edit window and moved to a new chunk,
	 * we need a full refresh. */
	if (ISSET(SOFTWRAP) && (extra_chunks_in(openfile->current) != old_amount ||
					(openfile->current_y == editwinrows - 1 &&
					chunk_for(xplustabs(), openfile->current) > original_row))) {
		refresh_needed = TRUE;
		focusing = FALSE;
	}
#endif

	openfile->placewewant = xplustabs();

#ifdef ENABLE_COLOR
	if (!refresh_needed)
		check_the_multis(openfile->current);
#endif
	if (!refresh_needed)
		update_line(openfile->current, openfile->current_x);
}

/* Read in a keystroke, and execute its command or insert it into the buffer. */
void process_a_keystroke(void)
{
	int input;
		/* The keystroke we read in: a character or a shortcut. */
	static char *puddle = NULL;
		/* The input buffer for actual characters. */
	static size_t capacity = 12;
		/* The size of the input buffer; gets doubled whenever needed. */
	static size_t depth = 0;
		/* The length of the input buffer. */
#ifndef NANO_TINY
	linestruct *was_mark = openfile->mark;
#endif
	static bool give_a_hint = TRUE;
	const keystruct *shortcut;
	functionptrtype function;

	/* Read in a keystroke, and show the cursor while waiting. */
	input = get_kbinput(midwin, VISIBLE);

	lastmessage = VACUUM;

#ifndef NANO_TINY
	if (input == KEY_WINCH)
		return;
#endif
#ifdef ENABLE_MOUSE
	if (input == KEY_MOUSE) {
		/* If the user clicked on a shortcut, read in the key code that it was
		 * converted into.  Else the click has been handled or was invalid. */
		if (do_mouse() == 1)
			input = get_kbinput(midwin, BLIND);
		else
			return;
	}
#endif

	/* Check for a shortcut in the main list. */
	shortcut = get_shortcut(input);
	function = (shortcut ? shortcut->func : NULL);

	/* If not a command, discard anything that is not a normal character byte. */
	if (!function) {
		if (input < 0x20 || input > 0xFF || meta_key)
			unbound_key(input);
		else if (ISSET(VIEW_MODE))
			print_view_warning();
		else {
#ifndef NANO_TINY
			if (openfile->mark && openfile->softmark) {
				openfile->mark = NULL;
				refresh_needed = TRUE;
			}
#endif
			/* When the input buffer (plus room for terminating NUL) is full,
			 * extend it; otherwise, if it does not exist yet, create it. */
			if (depth + 1 == capacity) {
				capacity = 2 * capacity;
				puddle = nrealloc(puddle, capacity);
			} else if (!puddle)
				puddle = nmalloc(capacity);

			puddle[depth++] = (char)input;
		}
	}

	/* If there are gathered bytes and we have a command or no other key codes
	 * are waiting, it's time to insert these bytes into the edit buffer. */
	if (depth > 0 && (function || waiting_keycodes() == 0)) {
		puddle[depth] = '\0';
		inject(puddle, depth);
		depth = 0;
	}

	if (!function) {
		pletion_line = NULL;
		keep_cutbuffer = FALSE;
		return;
	}

	if (ISSET(VIEW_MODE) && changes_something(function)) {
		print_view_warning();
		return;
	}

	if (input == '\b' && give_a_hint && openfile->current_x == 0 &&
				openfile->current == openfile->filetop && !ISSET(NO_HELP)) {
		statusbar(_("^W = Ctrl+W    M-W = Alt+W"));
		give_a_hint = FALSE;
	} else if (meta_key)
		give_a_hint = FALSE;

	/* When not cutting or copying text, drop the cutbuffer the next time. */
	if (function != cut_text) {
#ifndef NANO_TINY
		if (function != copy_text && function != zap_text)
#endif
			keep_cutbuffer = FALSE;
	}

#ifdef ENABLE_WORDCOMPLETION
	if (function != complete_a_word)
		pletion_line = NULL;
#endif
#ifdef ENABLE_NANORC
	if (function == (functionptrtype)implant) {
		implant(shortcut->expansion);
		return;
	}
#endif
#ifndef NANO_TINY
	if (function == do_toggle) {
		toggle_this(shortcut->toggle);
		if (shortcut->toggle == CUT_FROM_CURSOR)
			keep_cutbuffer = FALSE;
		return;
	}

	linestruct *was_current = openfile->current;
	size_t was_x = openfile->current_x;

	/* If Shifted movement occurs, set the mark. */
	if (shift_held && !openfile->mark) {
		openfile->mark = openfile->current;
		openfile->mark_x = openfile->current_x;
		openfile->softmark = TRUE;
	}
#endif

	/* Execute the function of the shortcut. */
	function();

#ifndef NANO_TINY
	/* When the marked region changes without Shift being held,
	 * discard a soft mark.  And when the set of lines changes,
	 * reset the "last line too" flag. */
	if (openfile->mark && openfile->softmark && !shift_held &&
						(openfile->current != was_current ||
						openfile->current_x != was_x ||
						wanted_to_move(function))) {
		openfile->mark = NULL;
		refresh_needed = TRUE;
	} else if (openfile->current != was_current)
		also_the_last = FALSE;

	if (bracketed_paste)
		suck_up_input_and_paste_it();

	if (ISSET(STATEFLAGS) && openfile->mark != was_mark)
		titlebar(NULL);
#endif
}

int main(int argc, char **argv)
{
	int stdin_flags, optchr;
#ifdef ENABLE_NANORC
	bool ignore_rcfiles = FALSE;
		/* Whether to ignore the nanorc files. */
#endif
#if defined(ENABLED_WRAPORJUSTIFY) && defined(ENABLE_NANORC)
	bool fill_used = FALSE;
		/* Was the fill option used on the command line? */
#endif
#ifdef ENABLE_WRAPPING
	int hardwrap = -2;
		/* Becomes 0 when --nowrap and 1 when --breaklonglines is used. */
#endif
#ifdef ENABLE_JUSTIFY
	int quoterc;
		/* Whether the quoting regex was compiled successfully. */
#endif
	const struct option long_options[] = {
		{"boldtext", 0, NULL, 'D'},
#ifdef ENABLE_MULTIBUFFER
		{"multibuffer", 0, NULL, 'F'},
#endif
#ifdef ENABLE_NANORC
		{"ignorercfiles", 0, NULL, 'I'},
#endif
		{"rawsequences", 0, NULL, 'K'},
#ifdef ENABLED_WRAPORJUSTIFY
		{"trimblanks", 0, NULL, 'M'},
#endif
#ifdef ENABLE_JUSTIFY
		{"quotestr", 1, NULL, 'Q'},
#endif
		{"restricted", 0, NULL, 'R'},
		{"quickblank", 0, NULL, 'U'},
		{"version", 0, NULL, 'V'},
#ifdef ENABLE_COLOR
		{"syntax", 1, NULL, 'Y'},
#endif
#ifdef ENABLE_WRAPPING
		{"breaklonglines", 0, NULL, 'b'},
#endif
		{"constantshow", 0, NULL, 'c'},
		{"rebinddelete", 0, NULL, 'd'},
#ifdef ENABLE_NANORC
		{"rcfile", 1, NULL, 'f'},
#endif
#if defined(ENABLE_BROWSER) || defined(ENABLE_HELP)
		{"showcursor", 0, NULL, 'g'},
#endif
		{"help", 0, NULL, 'h'},
#ifdef ENABLE_LINENUMBERS
		{"linenumbers", 0, NULL, 'l'},
#endif
#ifdef ENABLE_MOUSE
		{"mouse", 0, NULL, 'm'},
#endif
#ifdef ENABLE_OPERATINGDIR
		{"operatingdir", 1, NULL, 'o'},
#endif
		{"preserve", 0, NULL, 'p'},
#ifdef ENABLED_WRAPORJUSTIFY
		{"fill", 1, NULL, 'r'},
#endif
#ifdef ENABLE_SPELLER
		{"speller", 1, NULL, 's'},
#endif
		{"saveonexit", 0, NULL, 't'},
		{"view", 0, NULL, 'v'},
#ifdef ENABLE_WRAPPING
		{"nowrap", 0, NULL, 'w'},
#endif
		{"nohelp", 0, NULL, 'x'},
#ifndef NANO_TINY
		{"smarthome", 0, NULL, 'A'},
		{"backup", 0, NULL, 'B'},
		{"backupdir", 1, NULL, 'C'},
		{"tabstospaces", 0, NULL, 'E'},
		{"locking", 0, NULL, 'G'},
		{"historylog", 0, NULL, 'H'},
		{"guidestripe", 1, NULL, 'J'},
		{"nonewlines", 0, NULL, 'L'},
		{"noconvert", 0, NULL, 'N'},
		{"bookstyle", 0, NULL, 'O'},
		{"positionlog", 0, NULL, 'P'},
		{"softwrap", 0, NULL, 'S'},
		{"tabsize", 1, NULL, 'T'},
		{"wordbounds", 0, NULL, 'W'},
		{"wordchars", 1, NULL, 'X'},
		{"zap", 0, NULL, 'Z'},
		{"atblanks", 0, NULL, 'a'},
		{"emptyline", 0, NULL, 'e'},
		{"autoindent", 0, NULL, 'i'},
		{"jumpyscrolling", 0, NULL, 'j'},
		{"cutfromcursor", 0, NULL, 'k'},
		{"noread", 0, NULL, 'n'},
		{"indicator", 0, NULL, 'q'},
		{"unix", 0, NULL, 'u'},
		{"afterends", 0, NULL, 'y'},
		{"stateflags", 0, NULL, '%'},
		{"minibar", 0, NULL, '_'},
		{"zero", 0, NULL, '0'},
#endif
#ifdef HAVE_LIBMAGIC
		{"magic", 0, NULL, '!'},
#endif
		{NULL, 0, NULL, 0}
	};

#ifdef __linux__
	struct vt_stat dummy;

	/* Check whether we're running on a Linux console. */
	on_a_vt = (ioctl(STDOUT_FILENO, VT_GETSTATE, &dummy) == 0);
#endif

	/* Back up the terminal settings so that they can be restored. */
	tcgetattr(STDIN_FILENO, &original_state);

#if defined(F_GETFL) && defined(F_SETFL)
	/* Get the state of standard input and ensure it uses blocking mode. */
	stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	if (stdin_flags != -1)
		fcntl(STDIN_FILENO, F_SETFL, stdin_flags & ~O_NONBLOCK);
#endif

#ifdef ENABLE_UTF8
	/* If setting the locale is successful and it uses UTF-8, we will
	 * need to use the multibyte functions for text processing. */
	if (setlocale(LC_ALL, "") && strcmp(nl_langinfo(CODESET), "UTF-8") == 0)
		utf8_init();
#else
	setlocale(LC_ALL, "");
#endif

#ifdef ENABLE_NLS
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	/* Set a sensible default, different from what Pico does. */
	SET(NO_WRAP);

	/* If the executable's name starts with 'r', activate restricted mode. */
	if (*(tail(argv[0])) == 'r')
		SET(RESTRICTED);

	while ((optchr = getopt_long(argc, argv, "ABC:DEFGHIJ:KLMNOPQ:RS$T:UVWX:Y:Z"
				"abcdef:ghijklmno:pqr:s:tuvwxy!%_0", long_options, NULL)) != -1) {
		switch (optchr) {
#ifndef NANO_TINY
			case 'A':
				SET(SMART_HOME);
				break;
			case 'B':
				SET(MAKE_BACKUP);
				break;
			case 'C':
				backup_dir = mallocstrcpy(backup_dir, optarg);
				break;
#endif
			case 'D':
				SET(BOLD_TEXT);
				break;
#ifndef NANO_TINY
			case 'E':
				SET(TABS_TO_SPACES);
				break;
#endif
#ifdef ENABLE_MULTIBUFFER
			case 'F':
				SET(MULTIBUFFER);
				break;
#endif
#ifndef NANO_TINY
			case 'G':
				SET(LOCKING);
				break;
#endif
#ifdef ENABLE_HISTORIES
			case 'H':
				SET(HISTORYLOG);
				break;
#endif
#ifdef ENABLE_NANORC
			case 'I':
				ignore_rcfiles = TRUE;
				break;
#endif
#ifndef NANO_TINY
			case 'J':
				if (!parse_num(optarg, &stripe_column) || stripe_column <= 0) {
					fprintf(stderr, _("Guide column \"%s\" is invalid"), optarg);
					fprintf(stderr, "\n");
					exit(1);
				}
				break;
#endif
			case 'K':
				SET(RAW_SEQUENCES);
				break;
#ifndef NANO_TINY
			case 'L':
				SET(NO_NEWLINES);
				break;
#endif
#ifdef ENABLED_WRAPORJUSTIFY
			case 'M':
				SET(TRIM_BLANKS);
				break;
#endif
#ifndef NANO_TINY
			case 'N':
				SET(NO_CONVERT);
				break;
			case 'O':
				SET(BOOKSTYLE);
				break;
#endif
#ifdef ENABLE_HISTORIES
			case 'P':
				SET(POSITIONLOG);
				break;
#endif
#ifdef ENABLE_JUSTIFY
			case 'Q':
				quotestr = mallocstrcpy(quotestr, optarg);
				break;
#endif
			case 'R':
				SET(RESTRICTED);
				break;
#ifndef NANO_TINY
			case 'S':
			case '$':  /* Deprecated; remove in 2024. */
				SET(SOFTWRAP);
				break;
			case 'T':
				if (!parse_num(optarg, &tabsize) || tabsize <= 0) {
					fprintf(stderr, _("Requested tab size \"%s\" is invalid"), optarg);
					fprintf(stderr, "\n");
					exit(1);
				}
				break;
#endif
			case 'U':
				SET(QUICK_BLANK);
				break;
			case 'V':
				version();
				exit(0);
#ifndef NANO_TINY
			case 'W':
				SET(WORD_BOUNDS);
				break;
			case 'X':
				word_chars = mallocstrcpy(word_chars, optarg);
				break;
#endif
#ifdef ENABLE_COLOR
			case 'Y':
				syntaxstr = mallocstrcpy(syntaxstr, optarg);
				break;
#endif
#ifndef NANO_TINY
			case 'Z':
				SET(LET_THEM_ZAP);
				break;
			case 'a':
				SET(AT_BLANKS);
				break;
#endif
#ifdef ENABLE_WRAPPING
			case 'b':
				hardwrap = 1;
				break;
#endif
			case 'c':
				SET(CONSTANT_SHOW);
				break;
			case 'd':
				SET(REBIND_DELETE);
				break;
#ifndef NANO_TINY
			case 'e':
				SET(EMPTY_LINE);
				break;
#endif
#ifdef ENABLE_NANORC
			case 'f':
				custom_nanorc = mallocstrcpy(custom_nanorc, optarg);
				break;
#endif
#if defined(ENABLE_BROWSER) || defined(ENABLE_HELP)
			case 'g':
				SET(SHOW_CURSOR);
				break;
#endif
			case 'h':
				usage();
				exit(0);
#ifndef NANO_TINY
			case 'i':
				SET(AUTOINDENT);
				break;
			case 'j':
				SET(JUMPY_SCROLLING);
				break;
			case 'k':
				SET(CUT_FROM_CURSOR);
				break;
#endif
#ifdef ENABLE_LINENUMBERS
			case 'l':
				SET(LINE_NUMBERS);
				break;
#endif
#ifdef ENABLE_MOUSE
			case 'm':
				SET(USE_MOUSE);
				break;
#endif
#ifndef NANO_TINY
			case 'n':
				SET(NOREAD_MODE);
				break;
#endif
#ifdef ENABLE_OPERATINGDIR
			case 'o':
				operating_dir = mallocstrcpy(operating_dir, optarg);
				break;
#endif
			case 'p':
				SET(PRESERVE);
				break;
#ifndef NANO_TINY
			case 'q':
				SET(INDICATOR);
				break;
#endif
#ifdef ENABLED_WRAPORJUSTIFY
			case 'r':
				if (!parse_num(optarg, &fill)) {
					fprintf(stderr, _("Requested fill size \"%s\" is invalid"), optarg);
					fprintf(stderr, "\n");
					exit(1);
				}
#ifdef ENABLE_NANORC
				fill_used = TRUE;
#endif
				break;
#endif
#ifdef ENABLE_SPELLER
			case 's':
				alt_speller = mallocstrcpy(alt_speller, optarg);
				break;
#endif
			case 't':
				SET(SAVE_ON_EXIT);
				break;
#ifndef NANO_TINY
			case 'u':
				SET(MAKE_IT_UNIX);
				break;
#endif
			case 'v':
				SET(VIEW_MODE);
				break;
#ifdef ENABLE_WRAPPING
			case 'w':
				hardwrap = 0;
				break;
#endif
			case 'x':
				SET(NO_HELP);
				break;
#ifndef NANO_TINY
			case 'y':
				SET(AFTER_ENDS);
				break;
#endif
#ifdef HAVE_LIBMAGIC
			case '!':
				SET(USE_MAGIC);
				break;
#endif
#ifndef NANO_TINY
			case '%':
				SET(STATEFLAGS);
				break;
			case '_':
				SET(MINIBAR);
				break;
			case '0':
				SET(ZERO);
				break;
#endif
			default:
				printf(_("Type '%s -h' for a list of available options.\n"), argv[0]);
				exit(1);
		}
	}

	/* Curses needs TERM; if it is unset, try falling back to a VT220. */
	if (getenv("TERM") == NULL)
		putenv("TERM=vt220");

	/* Enter into curses mode.  Abort if this fails. */
	if (initscr() == NULL)
		exit(1);

#ifdef ENABLE_COLOR
	/* If the terminal can do colors, tell ncurses to switch them on. */
	if (has_colors())
		start_color();

	/* When requested, suppress the default spotlight and error colors. */
	rescind_colors = (getenv("NO_COLOR") != NULL);
#endif

	/* Set up the function and shortcut lists.  This needs to be done
	 * before reading the rcfile, to be able to rebind/unbind keys. */
	shortcut_init();

#ifdef ENABLE_NANORC
	if (!ignore_rcfiles) {
		/* Back up the command-line options that take an argument. */
#ifdef ENABLED_WRAPORJUSTIFY
		ssize_t fill_cmdline = fill;
#endif
#ifndef NANO_TINY
		char *backup_dir_cmdline = backup_dir;
		char *word_chars_cmdline = word_chars;
		size_t stripeclm_cmdline = stripe_column;
		ssize_t tabsize_cmdline = tabsize;
#endif
#ifdef ENABLE_OPERATINGDIR
		char *operating_dir_cmdline = operating_dir;
#endif
#ifdef ENABLE_JUSTIFY
		char *quotestr_cmdline = quotestr;
#endif
#ifdef ENABLE_SPELLER
		char *alt_speller_cmdline = alt_speller;
#endif

		/* Back up the command-line flags. */
		unsigned flags_cmdline[sizeof(flags) / sizeof(flags[0])];
		memcpy(flags_cmdline, flags, sizeof(flags_cmdline));

		/* Clear the string options, to not overwrite the specified ones. */
#ifndef NANO_TINY
		backup_dir = NULL;
		word_chars = NULL;
#endif
#ifdef ENABLE_OPERATINGDIR
		operating_dir = NULL;
#endif
#ifdef ENABLE_JUSTIFY
		quotestr = NULL;
#endif
#ifdef ENABLE_SPELLER
		alt_speller = NULL;
#endif
		/* Now process the system's and the user's nanorc file, if any. */
		do_rcfiles();

		/* If the backed-up command-line options have a value, restore them. */
#ifdef ENABLED_WRAPORJUSTIFY
		if (fill_used)
			fill = fill_cmdline;
#endif
#ifndef NANO_TINY
		if (backup_dir_cmdline != NULL) {
			free(backup_dir);
			backup_dir = backup_dir_cmdline;
		}
		if (word_chars_cmdline != NULL) {
			free(word_chars);
			word_chars = word_chars_cmdline;
		}
		if (stripeclm_cmdline > 0)
			stripe_column = stripeclm_cmdline;
		if (tabsize_cmdline != -1)
			tabsize = tabsize_cmdline;
#endif
#ifdef ENABLE_OPERATINGDIR
		if (operating_dir_cmdline != NULL || ISSET(RESTRICTED)) {
			free(operating_dir);
			operating_dir = operating_dir_cmdline;
		}
#endif
#ifdef ENABLE_JUSTIFY
		if (quotestr_cmdline != NULL) {
			free(quotestr);
			quotestr = quotestr_cmdline;
		}
#endif
#ifdef ENABLE_SPELLER
		if (alt_speller_cmdline != NULL) {
			free(alt_speller);
			alt_speller = alt_speller_cmdline;
		}
		/* Strip leading whitespace from the speller command, if any. */
		while (alt_speller && *alt_speller && isblank(*alt_speller))
			memmove(alt_speller, alt_speller + 1, strlen(alt_speller));
#endif

		/* If an rcfile undid the default setting, copy it to the new flag. */
		if (!ISSET(NO_WRAP))
			SET(BREAK_LONG_LINES);

		/* Simply OR the boolean flags from rcfile and command line. */
		for (size_t i = 0; i < sizeof(flags) / sizeof(flags[0]); i++)
			flags[i] |= flags_cmdline[i];
	}
#endif /* ENABLE_NANORC */

#ifdef ENABLE_WRAPPING
	if (hardwrap == 0)
		UNSET(BREAK_LONG_LINES);
	else if (hardwrap == 1)
		SET(BREAK_LONG_LINES);
#endif

	/* If the user wants bold instead of reverse video for hilited text... */
	if (ISSET(BOLD_TEXT))
		hilite_attribute = A_BOLD;

	/* When in restricted mode, disable backups and history files, since they
	 * would allow writing to files not specified on the command line. */
	if (ISSET(RESTRICTED)) {
		UNSET(MAKE_BACKUP);
#ifdef ENABLE_NANORC
		UNSET(HISTORYLOG);
		UNSET(POSITIONLOG);
#endif
	}

	/* When getting untranslated escape sequences, the mouse cannot be used. */
	if (ISSET(RAW_SEQUENCES))
		UNSET(USE_MOUSE);

	/* When suppressing title bar or minibar, suppress also the help lines. */
	if (ISSET(ZERO))
		SET(NO_HELP);

#ifdef ENABLE_HISTORIES
	/* Initialize the pointers for the Search/Replace/Execute histories. */
	history_init();

	/* If we need history files, verify that we have a directory for them,
	 * and when not, cancel the options. */
	if ((ISSET(HISTORYLOG) || ISSET(POSITIONLOG)) && !have_statedir()) {
		UNSET(HISTORYLOG);
		UNSET(POSITIONLOG);
	}

	/* If the user wants history persistence, read the relevant files. */
	if (ISSET(HISTORYLOG))
		load_history();
	if (ISSET(POSITIONLOG))
		load_poshistory();
#endif

#ifndef NANO_TINY
	/* If a backup directory was specified and we're not in restricted mode,
	 * verify it is an existing folder, so backup files can be saved there. */
	if (backup_dir != NULL && !ISSET(RESTRICTED))
		init_backup_dir();
#endif
#ifdef ENABLE_OPERATINGDIR
	/* Set up the operating directory.  This entails chdir()ing there,
	 * so that file reads and writes will be based there. */
	if (operating_dir != NULL)
		init_operating_dir();
#endif

#ifdef ENABLE_JUSTIFY
	/* Set the default value for things that weren't specified. */
	if (punct == NULL)
		punct = copy_of("!.?");
	if (brackets == NULL)
		brackets = copy_of("\"')>]}");
	if (quotestr == NULL)
		quotestr = copy_of("^([ \t]*([!#%:;>|}]|/{2}))+");

	/* Compile the quoting regex, and exit when it's invalid. */
	quoterc = regcomp(&quotereg, quotestr, NANO_REG_EXTENDED);
	if (quoterc != 0) {
		size_t size = regerror(quoterc, &quotereg, NULL, 0);
		char *message = nmalloc(size);

		regerror(quoterc, &quotereg, message, size);
		die(_("Bad quoting regex \"%s\": %s\n"), quotestr, message);
	} else
		free(quotestr);
#endif

#ifdef ENABLE_SPELLER
	/* If we don't have an alternative spell checker after reading the
	 * command line and/or rcfile(s), check $SPELL for one, as Pico
	 * does (unless we're using restricted mode, in which case spell
	 * checking is disabled, since it would allow reading from or
	 * writing to files not specified on the command line). */
	if (alt_speller == NULL && !ISSET(RESTRICTED)) {
		const char *spellenv = getenv("SPELL");

		if (spellenv != NULL)
			alt_speller = copy_of(spellenv);
	}
#endif

#ifndef NANO_TINY
	/* If matchbrackets wasn't specified, set its default value. */
	if (matchbrackets == NULL)
		matchbrackets = copy_of("(<[{)>]}");

	/* If the whitespace option wasn't specified, set its default value. */
	if (whitespace == NULL) {
#ifdef ENABLE_UTF8
		if (using_utf8()) {
			/* A tab is shown as a Right-Pointing Double Angle Quotation Mark
			 * (U+00BB), and a space as a Middle Dot (U+00B7). */
			whitespace = copy_of("\xC2\xBB\xC2\xB7");
			whitelen[0] = 2;
			whitelen[1] = 2;
		} else
#endif
		{
			whitespace = copy_of(">.");
			whitelen[0] = 1;
			whitelen[1] = 1;
		}
	}
#endif /* !NANO_TINY */

	/* Initialize the search string. */
	last_search = copy_of("");
	UNSET(BACKWARDS_SEARCH);

	/* If tabsize wasn't specified, set its default value. */
	if (tabsize == -1)
		tabsize = WIDTH_OF_TAB;

#ifdef ENABLE_COLOR
	/* On capable terminals, use colors, otherwise just reverse or bold.*/
	if (has_colors())
		set_interface_colorpairs();
	else
#endif
	{
		interface_color_pair[TITLE_BAR] = hilite_attribute;
		interface_color_pair[LINE_NUMBER] = hilite_attribute;
		interface_color_pair[GUIDE_STRIPE] = A_REVERSE;
		interface_color_pair[SCROLL_BAR] = A_NORMAL;
		interface_color_pair[SELECTED_TEXT] = hilite_attribute;
		interface_color_pair[SPOTLIGHTED] = A_REVERSE;
		interface_color_pair[MINI_INFOBAR] = hilite_attribute;
		interface_color_pair[PROMPT_BAR] = hilite_attribute;
		interface_color_pair[STATUS_BAR] = hilite_attribute;
		interface_color_pair[ERROR_MESSAGE] = hilite_attribute;
		interface_color_pair[KEY_COMBO] = hilite_attribute;
		interface_color_pair[FUNCTION_TAG] = A_NORMAL;
	}

	/* Set up the terminal state. */
	terminal_init();

	/* Create the three subwindows, based on the current screen dimensions. */
	window_init();
	curs_set(0);

#ifndef NANO_TINY
	thebar = (ISSET(INDICATOR) && LINES > 5 && COLS > 9) ? 1 : 0;
	bardata = nrealloc(bardata, LINES * sizeof(int));
#endif
	editwincols = COLS - thebar;

	/* Set up the signal handlers. */
	signal_init();

#ifdef ENABLE_MOUSE
	/* Initialize mouse support. */
	mouse_init();
#endif

	/* Ask ncurses for the key codes for most modified editing keys. */
	controlleft = get_keycode("kLFT5", CONTROL_LEFT);
	controlright = get_keycode("kRIT5", CONTROL_RIGHT);
	controlup = get_keycode("kUP5", CONTROL_UP);
	controldown = get_keycode("kDN5", CONTROL_DOWN);

	controlhome = get_keycode("kHOM5", CONTROL_HOME);
	controlend = get_keycode("kEND5", CONTROL_END);
#ifndef NANO_TINY
	controldelete = get_keycode("kDC5", CONTROL_DELETE);
	controlshiftdelete = get_keycode("kDC6", CONTROL_SHIFT_DELETE);

	shiftup = get_keycode("kUP", SHIFT_UP);
	shiftdown = get_keycode("kDN", SHIFT_DOWN);

	shiftcontrolleft = get_keycode("kLFT6", SHIFT_CONTROL_LEFT);
	shiftcontrolright = get_keycode("kRIT6", SHIFT_CONTROL_RIGHT);
	shiftcontrolup = get_keycode("kUP6", SHIFT_CONTROL_UP);
	shiftcontroldown = get_keycode("kDN6", SHIFT_CONTROL_DOWN);

	shiftcontrolhome = get_keycode("kHOM6", SHIFT_CONTROL_HOME);
	shiftcontrolend = get_keycode("kEND6", SHIFT_CONTROL_END);

	altleft = get_keycode("kLFT3", ALT_LEFT);
	altright = get_keycode("kRIT3", ALT_RIGHT);
	altup = get_keycode("kUP3", ALT_UP);
	altdown = get_keycode("kDN3", ALT_DOWN);

	altpageup = get_keycode("kPRV3", ALT_PAGEUP);
	altpagedown = get_keycode("kNXT3", ALT_PAGEDOWN);
	altinsert = get_keycode("kIC3", ALT_INSERT);
	altdelete = get_keycode("kDC3", ALT_DELETE);

	shiftaltleft = get_keycode("kLFT4", SHIFT_ALT_LEFT);
	shiftaltright = get_keycode("kRIT4", SHIFT_ALT_RIGHT);
	shiftaltup = get_keycode("kUP4", SHIFT_ALT_UP);
	shiftaltdown = get_keycode("kDN4", SHIFT_ALT_DOWN);
#endif

#ifdef HAVE_SET_ESCDELAY
	/* Tell ncurses to pass the Esc key quickly. */
	set_escdelay(50);
#endif

	/* Read the files mentioned on the command line into new buffers. */
	while (optind < argc && (!openfile || read_them_all)) {
		ssize_t givenline = 0, givencol = 0;
#ifndef NANO_TINY
		char *searchstring = NULL;
#endif
		/* If there's a +LINE[,COLUMN] argument here, eat it up. */
		if (optind < argc - 1 && argv[optind][0] == '+') {
#ifndef NANO_TINY
			int n = 1;

			while (isalpha(argv[optind][n])) {
				switch (argv[optind][n++]) {
					case 'c': SET(CASE_SENSITIVE); break;
					case 'C': UNSET(CASE_SENSITIVE); break;
					case 'r': SET(USE_REGEXP); break;
					case 'R': UNSET(USE_REGEXP); break;
					default:
						statusline(ALERT, _("Invalid search modifier '%c'"),
											argv[optind][n - 1]);
				}
			}

			if (argv[optind][n] == '/' || argv[optind][n] == '?') {
				if (argv[optind][n + 1]) {
					searchstring = copy_of(&argv[optind][n + 1]);
					if (argv[optind][n] == '?')
						SET(BACKWARDS_SEARCH);
				} else
					statusline(ALERT, _("Empty search string"));
				optind++;
			} else
#endif
			/* When there is nothing after the "+", understand it as go-to-EOF,
			 * otherwise parse and store the given number(s).*/
			if (argv[optind++][1] == '\0')
				givenline = -1;
			else if (!parse_line_column(&argv[optind - 1][1], &givenline, &givencol))
				statusline(ALERT, _("Invalid line or column number"));
		}

#ifndef NANO_TINY
		/* If the filename is a dash, read from standard input; otherwise,
		 * open the file; skip positioning the cursor if either failed. */
		if (strcmp(argv[optind], "-") == 0) {
			optind++;
			if (!scoop_stdin())
				continue;
		} else
#endif
		 if (!open_buffer(argv[optind++], TRUE))
			continue;

		/* If a position was given on the command line, go there. */
		if (givenline != 0 || givencol != 0)
			goto_line_and_column(givenline, givencol, FALSE, FALSE);
#ifndef NANO_TINY
		else if (searchstring != NULL) {
			if (ISSET(USE_REGEXP))
				regexp_init(searchstring);
			if (!findnextstr(searchstring, FALSE, JUSTFIND, NULL,
							ISSET(BACKWARDS_SEARCH), openfile->filetop, 0))
				not_found_msg(searchstring);
			else if (lastmessage <= REMARK)
				wipe_statusbar();
			openfile->placewewant = xplustabs();
			adjust_viewport(CENTERING);
			if (ISSET(USE_REGEXP))
				tidy_up_after_search();
			free(last_search);
			last_search = searchstring;
			searchstring = NULL;
		}
#endif
#ifdef ENABLE_HISTORIES
		else if (ISSET(POSITIONLOG) && openfile->filename[0] != '\0') {
			ssize_t savedline, savedcol;
			/* If edited before, restore the last cursor position. */
			if (has_old_position(argv[optind - 1], &savedline, &savedcol))
				goto_line_and_column(savedline, savedcol, FALSE, FALSE);
		}
#endif
	}

	/* After handling the files on the command line, allow inserting files. */
	UNSET(NOREAD_MODE);

	/* Nano is a hands-on editor -- it needs a keyboard. */
	if (!isatty(STDIN_FILENO))
		die(_("Standard input is not a terminal\n"));

	/* If no filenames were given, or all of them were invalid things like
	 * directories, then open a blank buffer and allow editing.  Otherwise,
	 * switch from the last opened file to the next, that is: the first. */
	if (openfile == NULL) {
		open_buffer("", TRUE);
		UNSET(VIEW_MODE);
	}
#ifdef ENABLE_MULTIBUFFER
	else {
		openfile = openfile->next;
		if (more_than_one)
			mention_name_and_linecount();
		if (ISSET(VIEW_MODE))
			SET(MULTIBUFFER);
	}
#else
	if (optind < argc)
		die(_("Can open just one file\n"));
#endif

	prepare_for_display();

#ifdef ENABLE_NANORC
	if (startup_problem != NULL)
		statusline(ALERT, startup_problem);

#define NOTREBOUND  first_sc_for(MMAIN, do_help) && \
						first_sc_for(MMAIN, do_help)->keycode == 0x07
#else
#define NOTREBOUND  TRUE
#endif

#ifdef ENABLE_HELP
	if (*openfile->filename == '\0' && openfile->totsize == 0 &&
				openfile->next == openfile && !ISSET(NO_HELP) && NOTREBOUND)
		statusbar(_("Welcome to nano.  For basic help, type Ctrl+G."));
#endif

#ifdef ENABLE_LINENUMBERS
	/* Set the margin to an impossible value to force re-evaluation. */
	margin = 12345;
#endif

	we_are_running = TRUE;

	while (TRUE) {
#ifdef ENABLE_LINENUMBERS
		confirm_margin();
#endif
#ifdef __linux__
		if (on_a_vt && waiting_keycodes() == 0)
			mute_modifiers = FALSE;
#endif
		if (currmenu != MMAIN)
			bottombars(MMAIN);

#ifndef NANO_TINY
		if (ISSET(MINIBAR) && !ISSET(ZERO) && LINES > 1 && lastmessage < REMARK)
			minibar();
		else
#endif
		/* Update the displayed current cursor position only when there
		 * is no message and no keys are waiting in the input buffer. */
		if (ISSET(CONSTANT_SHOW) && lastmessage == VACUUM && LINES > 1 &&
								!ISSET(ZERO) && waiting_keycodes() == 0)
			report_cursor_position();

		as_an_at = TRUE;

		if ((refresh_needed && LINES > 1) || (LINES == 1 && lastmessage <= HUSH))
			edit_refresh();
		else
			place_the_cursor();

#ifndef NANO_TINY
		/* In barless mode, either redraw a relevant status message,
		 * or overwrite a minor, redundant one. */
		if (ISSET(ZERO) && lastmessage > HUSH) {
			if (openfile->current_y == editwinrows - 1 && LINES > 1) {
				edit_scroll(FORWARD);
				wnoutrefresh(midwin);
			}
			wredrawln(footwin, 0, 1);
			wnoutrefresh(footwin);
			place_the_cursor();
		} else if (ISSET(ZERO) && lastmessage > VACUUM)
			wredrawln(midwin, editwinrows - 1, 1);
#endif

		errno = 0;
		focusing = TRUE;

		/* Forget any earlier cursor position at the prompt. */
		put_cursor_at_end_of_answer();

		/* Read in and interpret a single keystroke. */
		process_a_keystroke();
	}
}
