/* Miscellaneous support functions.

	Copyright (C) 1993-1998 Sebastiano Vigna 
	Copyright (C) 1999-2009 Todd M. Lewis and Sebastiano Vigna

	This file is part of ne, the nice editor.

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation; either version 2, or (at your option) any
	later version.
	
	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.
	
	You should have received a copy of the GNU General Public License along
	with this program; see the file COPYING.  If not, write to the Free
	Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.  */


#include "ne.h"
#include "cm.h"
#include <signal.h>

#ifdef _AMIGA
#include <proto/dos.h>
#else
#include <pwd.h>
#endif

/* Some systems do not define _POSIX_VDISABLE. We try to establish a reasonable value. */

#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE 0
#endif


/* This is the assert function. It appears that its number of arguments is
different on different ANSI C compilers. Better to redefine it. */

#ifndef NODEBUG

void __assert(int x, char *expr, char *file, int line) {
	if (!x) fprintf(stderr, "%s can't be false at line %d of %s!\n", expr, line, file);
}

#endif


/* Returns a pointer to the global ne directory if the environment variable
	NE_GLOBAL_DIR is set.  If it isn't set, then GLOBALDIR is returned. */

const char *get_global_dir(void) {
	char *ne_global_dir;
	ne_global_dir = getenv("NE_GLOBAL_DIR");
	if (!ne_global_dir) ne_global_dir = GLOBALDIR;
	return tilde_expand(ne_global_dir);
}



/* Some UNIXes allow "getcwd(NULL, size)" and will allocate the buffer for
	you when your first parm is NULL.  This is not really legal, so we've put a
	front end onto getcwd() called ne_getcwd() that allocates the buffer for you
	first.
*/

char *ne_getcwd(const int bufsize) {
	char *result = malloc(bufsize);
	if (result) result = getcwd(result, bufsize);
	return result;
}


/* is_migrated() tells whether the specified file is currently migrated.  A
  migrated file is one which is not actually on-line but is out on tape or
  other media. In general we don't want to try to load a migrated file in an
  interactive program because the delay (waiting for a tape mount, etc.)  is
  most annoying and frustrating.  On systems which don't support hierarchical
  storage, non-zero length files always take up at least one disk block, so
  this should never be an issue for them. */

#if defined _CONVEX_SOURCE
/* The Convex system uses CDVM, and can call cvxstat to see if a file is
  migrated. */
#include <sys/dmon.h>
int is_migrated(const char * const name) {
	struct cvxstat st;
	if (cvxstat(name, &st, sizeof(struct cvxstat)) == 0)
		if (st.st_dmonflags & IMIGRATED) return 1;

	return 0;
}
#elif defined ZERO_STAT_MIG_TEST
/* Some systems which support hierarchical storage will report a non-zero file
  size but zero blocks used.  (Since the file is on tape rather than disc, it's
  using no disc blocks.)  If this describes the behaviour of your system,
  define ZERO_STAT_MIG_TEST when building ne. */
int is_migrated(const char * const name) {
	struct stat statbuf;
	
	if ((stat(tilde_expand(name), &statbuf) == 0) &&
		  (statbuf.st_size > 0) &&
		  (statbuf.st_blocks == 0))
		return 1;
	else return 0;
  }
#else

/* Most systems have no hierarchical storage facility and need never concern
  themselves with this problem. For these systems, is_migrated() will always be
  false. */
int is_migrated(const char * const name) {
	 return 0;
}
#endif  


int is_directory(const char * const name) {
	struct stat statbuf;
	
	return stat(tilde_expand(name), &statbuf) == 0 && S_ISDIR(statbuf.st_mode);
}


/* Returns a pointer to a tilde-expanded version of the string pointed to by
	filename. The string should not be free()ed, since it is tracked
	locally. Note that this function can return the same pointer which is
	passed, in case no tilde expansion has to be performed. */

#ifndef _AMIGA

const char *tilde_expand(const char * filename) {

	static char *expanded_filename;
	
	char *home_dir, *p;
	struct passwd *passwd = NULL;
	
	if (!filename) return NULL;

	if (filename[0] != '~') return filename;
	
	if (filename[1] == '/') {
		home_dir = getenv("HOME");
		if (!home_dir) return filename;
		filename++;
	}
	else {
		
		const char *s;
		char *t;
		
		s = filename + 1;

		while(*s && *s != '/') s++;
		
		if (t = malloc(s - filename)) {
			
			memcpy(t, filename + 1, s - filename - 1);
			t[s - filename - 1] = 0;
			
			passwd = getpwnam(t);
			
			free(t);
		}
		
		if (!passwd) return filename;
		
		filename = s;
		home_dir = passwd->pw_dir;
	}

	if (p = realloc(expanded_filename, strlen(filename) + strlen(home_dir) + 1)) {
		strcat(strcpy(expanded_filename = p, home_dir), filename);
		return expanded_filename;
	}
	
	return filename;
}

#endif



/* Given a pathname, returns a pointer to the real file name (i.e., the pointer
	points inside the string passed). */

const char *file_part(const char * const pathname) {

	const char *p;

	if (!pathname) return NULL;

#ifdef _AMIGA

	return (const char *)FilePart((char *)pathname);

#else

	p = pathname + strlen(pathname);

	while(p > pathname && *(p - 1) != '/') p--;

	return p;

#endif

}


/* Duplicates a string. */

char *str_dup(const char * const s) {

	char *dup;

	if (!s) return NULL;

	dup = malloc(strlen(s) + 1);

	if (dup) strcpy(dup, s);
	return dup;
}

/* Tries to compute the length as a string of the given pointer,
	but stops after n characters (returning n). */

int strnlen_ne(const char *s, int n) {
	const char * const p = s;
	while(n-- != 0) if (!*(s++)) return s - p - 1;
	return s - p;
}


/* Computes the length of the maximal common prefix of s and	t. */

int max_prefix(const char * const s, const char * const t) {
	int i;

	for(i = 0; s[i] && t[i] && s[i] == t[i]; i++);

	return i;
}


/* Returns TRUE is the first string is a prefix of the second one. */

int is_prefix(const char * const p, const char * const s) {
	int i;

	for(i = 0; p[i] && s[i] && p[i] == s[i]; i++);

	return !p[i];
}


/* A string pointer comparison function for qsort(). */

int strcmpp(const void *a, const void *b) {

	return strcmp(*(const char **)a, *(const char **)b);
}

/* Another comparison for qsort, this one does dictionary order. */

int strdictcmp(const void *a, const void *b)	{
	int ci;
	
	if ( ci = strcasecmp(*(char * const *)a, *(char * const *)b) ) return ci;
	return strcmp(*(char * const *) a, * (char * const *) b);
}
                                                                       

/* A filename comparison function for qsort(). It makes "../" the first string, "./" the second string
   and then orders lexicographically. */

int filenamecmpp(const void *a, const void *b) {
	const char * const s = *(const char **)a, * const t = *(const char **)b; 
	if (strcmp(s, "../")==0) return strcmp(t, "../") == 0 ? 0 : -1;
	if (strcmp(t, "../")==0) return 1;
	if (strcmp(s, "./")==0) return strcmp(t, "./") == 0 ? 0 : -1;
	if (strcmp(t, "./")==0) return 1;
	/* return strcmp(s, t); */
	return strdictcmp(a, b);
}



#ifdef _AMIGA

/* Supplies a getenv() simulation on the Amiga. Unfortunately, the SAS/C
	version of getenv() does not look at the local variables, which are the only
	ones we are really interested in... */

char *getenv(const char * const name) {

	static char buffer[2048];

	if (GetVar((char *)name, buffer, sizeof buffer, 0) > -1) return buffer;
	else return NULL;
}

#endif



/* Sets the "interactive I/O mode" of the terminal. It suitably sets the mode
	bits of the termios structure, and then transmits various capability strings
	by calling set_terminal_modes(). This function assumes that the terminfo
	database has been properly initialized. The old_termios structure records
	the original state of the terminal interface. */

#ifndef _AMIGA
static struct termios termios, old_termios;
#endif

void set_interactive_mode(void) {

#ifndef _AMIGA

	tcgetattr(0, &termios);

	old_termios = termios;

	termios.c_iflag &= ~(IXON | IXOFF | ICRNL | INLCR | ISTRIP);
	termios.c_iflag |= IGNBRK;

	termios.c_oflag &= ~OPOST;

	termios.c_lflag &= ~(ISIG | ICANON | ECHO | ECHONL | IEXTEN);

	/* Cygwin's signal must be disabled, or CTRL-C won't work. There is no way
		to really change the sequences associated to signals. */

#ifndef __CYGWIN__
	termios.c_lflag |= ISIG;
#endif

	termios.c_cflag &= ~(CSIZE | PARENB);
	termios.c_cflag |= CS8;

	termios.c_cc[VTIME] = 0;
	termios.c_cc[VMIN] = 1;

	/* Now we keep the kernel from intercepting any keyboard input in order
	to turn it into a signal. Note that some signals, such as dsusp on BSD,
	are not trackable	here. They have to be disabled through suitable
	commands (for instance, `stty dsusp ^-'). */

	termios.c_cc[VSUSP] = _POSIX_VDISABLE;
	termios.c_cc[VQUIT] = _POSIX_VDISABLE;
	termios.c_cc[VKILL] = _POSIX_VDISABLE;

	/* Control-\ is the stop control sequence for ne. */

	termios.c_cc[VINTR] = '\\'-'@';

	tcsetattr(0, TCSADRAIN, &termios);

	/* SIGINT is used for the interrupt character. */

	signal(SIGINT, set_stop);

	/* We do not want to be stopped if we did not generate the signal */

	signal(SIGTSTP, SIG_IGN);

#ifdef SIGWINCH
	signal(SIGWINCH, handle_winch);
#endif

#else

	SetMode(Input(), 1);

#endif

	/* This ensures that a physical read will be performed at each getchar(). */

	setbuf(stdin, NULL);

	/* We enable the keypad, cursor addressing, etc. */

	set_terminal_modes();

}



/* Undoes the work of the previous function, in reverse order. It assumes the
	old_termios has been filled with the old termios structure. */

void unset_interactive_mode(void) {

	/* We move the cursor on the last line, clear it, and output a CR, so that
		the kernel can track the cursor position. Note that clear_to_eol() can
		move the cursor. */

	losecursor();
	move_cursor(ne_lines - 1, 0);
	clear_to_eol();
	move_cursor(ne_lines - 1, 0);

	/* Now we disable the keypad, cursor addressing, etc. fflush() guarantees
		that tcsetattr() won't clip part of the capability strings output by
		reset_terminal_modes(). */

	reset_terminal_modes();
	putchar('\r');
	fflush(stdout);

	/* Now we restore all the flags in the termios structure to the state they
		were before us. */

#ifndef _AMIGA
	tcsetattr(0, TCSADRAIN, &old_termios);
#else
	SetMode(Input(), 0);
#endif

#ifdef SIGWINCH
	signal(SIGWINCH, SIG_IGN);
#endif
	signal(SIGINT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
}



/* Computes the TAB-expanded width of a line descriptor up to a certain
	position. The position can be greater than the line length, the usual
	convention of infinite expansion via spaces being in place. */

int calc_width(const line_desc * const ld, const int n, const int tab_size, const encoding_type encoding) {

	int pos, len;

	for(pos = len = 0; pos < n; pos = next_pos(ld->line, pos, encoding)) {
		if (pos >= ld->line_len) len++;
		else if (ld->line[pos] != '\t') len += get_char_width(&ld->line[pos], encoding);
		else len += tab_size - len % tab_size;
	}

	return len;
}

/* Computes character length of a line descriptor. */

int calc_char_len(const line_desc * const ld, const encoding_type encoding) {
	int pos, len;
	for(pos = len = 0; pos < ld->line_len; pos = next_pos(ld->line, pos, encoding), len++);
	return len;
}


/* Given a column, the index of the character "containing" that position is
	given, that is, calc_width(index) > n, and index is minimum with this
	property. If the width of the line is smaller than the given column, the
	line length is returned. */

int calc_pos(const line_desc * const ld, const int col, const int tab_size, const encoding_type encoding) {

	int pos, width, c_width;

	for(pos = width = 0; pos < ld->line_len && width + (c_width = get_char_width(&ld->line[pos], encoding)) <= col; pos = next_pos(ld->line, pos, encoding)) {
		if (ld->line[pos] != '\t') width += c_width;
		else width += tab_size - width % tab_size;
	}

	return pos;
}

/* Returns true if the specified character is an US-ASCII whitespace character. */

int isasciispace(const int c) {
	return c < 0x80 && isspace(c);
}

/* Returns true if the specified character is an US-ASCII alphabetic character. */

int isasciialpha(const int c) {
	return c < 0x80 && isalpha(c);
}

/* Returns true if the specified block of text is US-ASCII. */

int is_ascii(const unsigned char * const s, int len) {
	while(len-- != 0) if (s[len] >= 0x80) return FALSE;
	return TRUE;
}


/* Returns toupper() of the given character, if it is US-ASCII, the character itself, otherwise. */

int asciitoupper(const int c) {
	return c < 0x80 ? toupper(c) : c;
}

/* Returns tolower() of the given character, if it is US-ASCII, the character itself, otherwise. */

int asciitolower(const int c) {
	return c < 0x80 ? tolower(c) : c;
}

/* Detects (heuristically) the encoding of a piece of text. */

encoding_type detect_encoding(const unsigned char *s, const int len) {
	int i, is_ascii = TRUE;
	const unsigned char * const t = s + len;

	if (len == 0) return ENC_ASCII;

	do {
		if (*s >= 0x80) { /* On US-ASCII text, we never enter here. */
			is_ascii = FALSE; /* Once we get here, we are either 8-bit or UTF-8. */
			i = utf8len(*s);
			if (i == -1) return ENC_8_BIT;
			else if (i > 1) {
				if (s + i > t) return ENC_8_BIT;
				else {
					/* We check for redundant representations. */
					if (i == 2) {
						if (!(*s & 0x1E)) return ENC_8_BIT;
					}
					else if (!(*s & (1 << 7 - i) - 1) && !(*(s + 1) & ((1 << i - 2) - 1) << 8 - i)) return ENC_8_BIT;
					while(--i != 0) if ((*(++s) & 0xC0) != 0x80) return ENC_8_BIT;
				}
			}
		}
	} while(++s < t);

	return is_ascii ? ENC_ASCII : ENC_UTF8;
}

/* Returns the position of the character after the one pointed by pos in s. If
	s is NULL, just returns pos + 1. If encoding is UTF8 it uses utf8len() to
	move forward. */

int next_pos(const unsigned char * const s, const int pos, const encoding_type encoding) {
	assert(encoding != ENC_UTF8 || s == NULL || utf8len(s[pos]) > 0);
	if (s == NULL) return pos + 1;
	if (encoding == ENC_UTF8) return pos + utf8len(s[pos]);
	else return pos + 1;
}

/* Returns the position of the character before the one pointed by pos in s.
	If s is NULL, just returns pos + 1. If encoding is UTF-8 uses utf8len() to
	move backward. If pos is 0, this function returns -1. */

int prev_pos(const unsigned char * const s, int pos, const encoding_type encoding) {
	assert(pos >= 0);
	if (pos == 0) return -1;
	if (s == NULL) return pos - 1;
	if (encoding == ENC_UTF8) {
		while((s[--pos] & 0xC0) == 0x80 && pos > 0);
		return pos;
	}
	else return pos - 1;
}


/* Returns the ISO 10646 character represented by the sequence of bytes
	starting at s, using the provided encoding. */

int get_char(const unsigned char * const s, const encoding_type encoding) {
	if (encoding == ENC_UTF8) return utf8char(s);
	else return *s;
}

/* Returns the width of the ISO 10646 character represented by the sequence of bytes
	starting at s, using the provided encoding. */

int get_char_width(const unsigned char * const s, const encoding_type encoding) {
	assert(s != NULL);
	return encoding == ENC_UTF8 ? output_width(utf8char(s)) : output_width(*s);
}

/* Returns the width of the first len characters of s, using the provided
	encoding. If s is NULL, returns len. */

int get_string_width(const unsigned char * const s, const int len, const encoding_type encoding) {
	int pos, width = 0;
	if (s == NULL) return len;
	for(pos = 0; pos < len; pos = next_pos(s, pos, encoding)) width += get_char_width(s + pos, encoding);
	return width;
}

/* Returns whether the given character is a punctuation character. This function is 
	compiled differently depending on whether wide-character function support is inhibited. */

int ne_ispunct(const int c, const int encoding) {
#ifdef NOWCHAR
	return encoding != ENC_UTF8 ? ispunct(c) : c < 0x80 ? ispunct(c) : 0;
#else
	return encoding != ENC_UTF8 ? ispunct(c) : iswpunct(c);
#endif
}

/* Returns whether the given character is whitespace. This function is 
	compiled differently depending on whether wide-character function support is inhibited. */

int ne_isspace(const int c, const int encoding) {
#ifdef NOWCHAR
	return encoding != ENC_UTF8 ? isspace(c) : c < 0x80 ? isspace(c) : 0;
#else
	return encoding != ENC_UTF8 ? isspace(c) : iswspace(c);
#endif
}

/* Returns whether the given character is a "word" character.
   Word characters are '_' plus any non-punctuation or space.
   
   TODO: implement a way for users to specify their own word characters.
   For now, hardcode '_'.  */

int ne_isword( const int c, const int encoding) {
   return c == '_' || !(ne_isspace(c, encoding) || ne_ispunct(c, encoding));
}