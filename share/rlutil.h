#pragma once
/** Adaptacja zewnetrzych bibliotek - plik zawiera rozne kawalki kodu z sieci
 */

/**
 * File: rlutil.h
 *
 * About: Description
 * This file provides some useful utilities for console mode
 * roguelike game development with C and C++. It is aimed to
 * be cross-platform (at least Windows and Linux).
 *
 * About: Copyright
 * (C) 2010 Tapio Vierros (http://tapio.github.com/rlutil/)
 * (C) 2012 Michal Widera (http://widera.com.pl)
 *
 *>        DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *>                Version 2, December 2004
 *>
 *>Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
 *>
 *>Everyone is permitted to copy and distribute verbatim or modified
 *>copies of this license document, and changing it is allowed as long
 *>as the name is changed.
 *>
 *>        DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *>TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *>
 *>0. You just DO WHAT THE FUCK YOU WANT TO.
 */


/// Define: RLUTIL_USE_ANSI
/// Define this to use ANSI escape sequences also on Windows
/// (defaults to using WinAPI instead).
#if 0
#define RLUTIL_USE_ANSI
#endif

/// Define: RLUTIL_STRING_T
/// Define/typedef this to your preference to override rlutil's string type.
///
/// Defaults to std::string with C++ and char* with C.
#if 0
#define RLUTIL_STRING_T char*
#endif

#ifdef __cplusplus
	/// Common C++ headers
	#include <iostream>
	#include <string>
	#include <sstream>
	/// Namespace forward declarations
	namespace rlutil {
		void locate(int x, int y);
	}
#else
	void locate(int x, int y); // Forward declare for C to avoid warnings
#endif // __cplusplus

#ifdef WIN32
	#include <windows.h>  // for WinAPI and Sleep()
	#include <conio.h>    // for getch() and kbhit()
#else
	#ifdef __cplusplus
		#include <cstdio> // for getch()
	#else // __cplusplus
		#include <stdio.h> // for getch()
	#endif // __cplusplus
	#include <termios.h> // for getch() and kbhit()
	#include <unistd.h> // for getch(), kbhit() and (u)sleep()
	#include <sys/ioctl.h> // for getkey()
	#include <sys/types.h> // for kbhit()
	#include <sys/time.h> // for kbhit()

/// Function: getch
/// Get character without waiting for Return to be pressed.
/// Windows has this in conio.h
int getch() {
	// Here be magic.
	struct termios oldt, newt;
	int ch;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}

/// Function: kbhit
/// Determines if keyboard has been hit.
/// Windows has this in conio.h
int kbhit() {
	// Here be dragons.
	static struct termios oldt, newt;
	int cnt = 0;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag    &= ~(ICANON | ECHO);
	newt.c_iflag     = 0; // input mode
	newt.c_oflag     = 0; // output mode
	newt.c_cc[VMIN]  = 1; // minimum time to wait
	newt.c_cc[VTIME] = 1; // minimum characters to wait for
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ioctl(0, FIONREAD, &cnt); // Read count
	struct timeval tv;
	tv.tv_sec  = 0;
	tv.tv_usec = 100;
	select(STDIN_FILENO+1, NULL, NULL, NULL, &tv); // A small time delay
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return cnt; // Return number of characters
}
#endif // WIN32

#ifndef gotoxy
/// Function: gotoxy
/// Same as <rlutil.locate>.
void inline gotoxy(int x, int y) {
	#ifdef __cplusplus
	rlutil::
	#endif
	locate(x,y);
}
#endif // gotoxy

#ifdef __cplusplus
/// Namespace: rlutil
/// In C++ all functions except <getch>, <kbhit> and <gotoxy> are arranged
/// under namespace rlutil. That is because some platforms have them defined
/// outside of rlutil.
namespace rlutil {
#endif

/**
 * Defs: Internal typedefs and macros
 * RLUTIL_STRING_T - String type depending on which one of C or C++ is used
 * RLUTIL_PRINT(str) - Printing macro independent of C/C++
 */

#ifdef __cplusplus
	#ifndef RLUTIL_STRING_T
		typedef std::string RLUTIL_STRING_T;
	#endif // RLUTIL_STRING_T

	void inline RLUTIL_PRINT(RLUTIL_STRING_T st) { std::cout << st; }

#else // __cplusplus
	#ifndef RLUTIL_STRING_T
		typedef char* RLUTIL_STRING_T;
	#endif // RLUTIL_STRING_T

	#define RLUTIL_PRINT(st) printf("%s", st)
#endif // __cplusplus

/**
 * Enums: Color codes
 *
 * BLACK - Black
 * RED - Red
 * GREEN - Green
 * BROWN - Brown / dark yellow
 * BLUE - Blue
 * MAGENTA - Magenta / purple
 * CYAN - Cyan
 * GREY - Grey / dark white
 * DARKGREY - Dark grey / light black
 * LIGHTRED - Light red
 * LIGHTGREEN - Light green
 * YELLOW - Yellow (bright)
 * LIGHTBLUE - Light blue
 * LIGHTMAGENTA - Light magenta / light purple
 * LIGHTCYAN - Light cyan
 * WHITE - White (bright)
 */
enum {
	BLACK,
	RED,
	GREEN,
	BROWN,
	BLUE,
	MAGENTA,
	CYAN,
	GREY,
	DARKGREY,
	LIGHTRED,
	LIGHTGREEN,
	YELLOW,
	LIGHTBLUE,
	LIGHTMAGENTA,
	LIGHTCYAN,
	WHITE
};

/**
 * Consts: ANSI color strings
 *
 * ANSI_CLS - Clears screen
 * ANSI_BLACK - Black
 * ANSI_RED - Red
 * ANSI_GREEN - Green
 * ANSI_BROWN - Brown / dark yellow
 * ANSI_BLUE - Blue
 * ANSI_MAGENTA - Magenta / purple
 * ANSI_CYAN - Cyan
 * ANSI_GREY - Grey / dark white
 * ANSI_DARKGREY - Dark grey / light black
 * ANSI_LIGHTRED - Light red
 * ANSI_LIGHTGREEN - Light green
 * ANSI_YELLOW - Yellow (bright)
 * ANSI_LIGHTBLUE - Light blue
 * ANSI_LIGHTMAGENTA - Light magenta / light purple
 * ANSI_LIGHTCYAN - Light cyan
 * ANSI_WHITE - White (bright)
 */
const RLUTIL_STRING_T ANSI_CLS = "\033[2J";
const RLUTIL_STRING_T ANSI_BLACK = "\033[22;30m";
const RLUTIL_STRING_T ANSI_RED = "\033[22;31m";
const RLUTIL_STRING_T ANSI_GREEN = "\033[22;32m";
const RLUTIL_STRING_T ANSI_BROWN = "\033[22;33m";
const RLUTIL_STRING_T ANSI_BLUE = "\033[22;34m";
const RLUTIL_STRING_T ANSI_MAGENTA = "\033[22;35m";
const RLUTIL_STRING_T ANSI_CYAN = "\033[22;36m";
const RLUTIL_STRING_T ANSI_GREY = "\033[22;37m";
const RLUTIL_STRING_T ANSI_DARKGREY = "\033[01;30m";
const RLUTIL_STRING_T ANSI_LIGHTRED = "\033[01;31m";
const RLUTIL_STRING_T ANSI_LIGHTGREEN = "\033[01;32m";
const RLUTIL_STRING_T ANSI_YELLOW = "\033[01;33m";
const RLUTIL_STRING_T ANSI_LIGHTBLUE = "\033[01;34m";
const RLUTIL_STRING_T ANSI_LIGHTMAGENTA = "\033[01;35m";
const RLUTIL_STRING_T ANSI_LIGHTCYAN = "\033[01;36m";
const RLUTIL_STRING_T ANSI_WHITE = "\033[01;37m";

/**
 * Consts: Key codes for keyhit()
 *
 * KEY_ESCAPE  - Escape
 * KEY_ENTER   - Enter
 * KEY_SPACE   - Space
 * KEY_INSERT  - Insert
 * KEY_HOME    - Home
 * KEY_END     - End
 * KEY_DELETE  - Delete
 * KEY_PGUP    - PageUp
 * KEY_PGDOWN  - PageDown
 * KEY_UP      - Up arrow
 * KEY_DOWN    - Down arrow
 * KEY_LEFT    - Left arrow
 * KEY_RIGHT   - Right arrow
 * KEY_F1      - F1
 * KEY_F2      - F2
 * KEY_F3      - F3
 * KEY_F4      - F4
 * KEY_F5      - F5
 * KEY_F6      - F6
 * KEY_F7      - F7
 * KEY_F8      - F8
 * KEY_F9      - F9
 * KEY_F10     - F10
 * KEY_F11     - F11
 * KEY_F12     - F12
 * KEY_NUMDEL  - Numpad del
 * KEY_NUMPAD0 - Numpad 0
 * KEY_NUMPAD1 - Numpad 1
 * KEY_NUMPAD2 - Numpad 2
 * KEY_NUMPAD3 - Numpad 3
 * KEY_NUMPAD4 - Numpad 4
 * KEY_NUMPAD5 - Numpad 5
 * KEY_NUMPAD6 - Numpad 6
 * KEY_NUMPAD7 - Numpad 7
 * KEY_NUMPAD8 - Numpad 8
 * KEY_NUMPAD9 - Numpad 9
 */
const int KEY_ESCAPE  = 0;
const int KEY_ENTER   = 1;
const int KEY_SPACE   = 32;

const int KEY_INSERT  = 2;
const int KEY_HOME    = 3;
const int KEY_PGUP    = 4;
const int KEY_DELETE  = 5;
const int KEY_END     = 6;
const int KEY_PGDOWN  = 7;

const int KEY_UP      = 14;
const int KEY_DOWN    = 15;
const int KEY_LEFT    = 16;
const int KEY_RIGHT   = 17;

const int KEY_F1      = 18;
const int KEY_F2      = 19;
const int KEY_F3      = 20;
const int KEY_F4      = 21;
const int KEY_F5      = 22;
const int KEY_F6      = 23;
const int KEY_F7      = 24;
const int KEY_F8      = 25;
const int KEY_F9      = 26;
const int KEY_F10     = 27;
const int KEY_F11     = 28;
const int KEY_F12     = 29;

const int KEY_NUMDEL  = 30;
const int KEY_NUMPAD0 = 31;
const int KEY_NUMPAD1 = 127;
const int KEY_NUMPAD2 = 128;
const int KEY_NUMPAD3 = 129;
const int KEY_NUMPAD4 = 130;
const int KEY_NUMPAD5 = 131;
const int KEY_NUMPAD6 = 132;
const int KEY_NUMPAD7 = 133;
const int KEY_NUMPAD8 = 134;
const int KEY_NUMPAD9 = 135;

/// Function: getkey
/// Reads a key press (blocking) and returns a key code.
///
/// See <Key codes for keyhit()>
///
/// Note:
/// Only Arrows, Esc, Enter and Space are currently working properly.
int getkey(void) {
	#ifndef WIN32
	int cnt = kbhit(); // for ANSI escapes processing
	#endif
	int k = getch();
	switch(k) {
		case 0: {
			int kk;
			switch (kk = getch()) {
				case 71: return KEY_NUMPAD7;
				case 72: return KEY_NUMPAD8;
				case 73: return KEY_NUMPAD9;
				case 75: return KEY_NUMPAD4;
				case 77: return KEY_NUMPAD6;
				case 79: return KEY_NUMPAD1;
				case 80: return KEY_NUMPAD4;
				case 81: return KEY_NUMPAD3;
				case 82: return KEY_NUMPAD0;
				case 83: return KEY_NUMDEL;
				default: return kk-59+KEY_F1; // Function keys
			}}
		case 224: {
			int kk;
			switch (kk = getch()) {
				case 71: return KEY_HOME;
				case 72: return KEY_UP;
				case 73: return KEY_PGUP;
				case 75: return KEY_LEFT;
				case 77: return KEY_RIGHT;
				case 79: return KEY_END;
				case 80: return KEY_DOWN;
				case 81: return KEY_PGDOWN;
				case 82: return KEY_INSERT;
				case 83: return KEY_DELETE;
				default: return kk-123+KEY_F1; // Function keys
			}}
		case 13: return KEY_ENTER;
#ifdef WIN32
		case 27: return KEY_ESCAPE;
#else // WIN32
		case 155: // single-character CSI
		case 27: {
			// Process ANSI escape sequences
			if (cnt >= 3 && getch() == '[') {
				switch (k = getch()) {
					case 'A': return KEY_UP;
					case 'B': return KEY_DOWN;
					case 'C': return KEY_RIGHT;
					case 'D': return KEY_LEFT;
				}
			} else return KEY_ESCAPE;
		}
#endif // WIN32
		default: return k;
	}
}

/// Function: nb_getch
/// Non-blocking getch(). Returns 0 if no key was pressed.
int inline nb_getch() {
	if (kbhit()) return getch();
	else return 0;
}

/// Function: getANSIColor
/// Return ANSI color escape sequence for specified number 0-15.
///
/// See <Color Codes>
RLUTIL_STRING_T getANSIColor(const int c) {
	switch (c) {
		case 0 : return ANSI_BLACK;
		case 1 : return ANSI_BLUE; // non-ANSI
		case 2 : return ANSI_GREEN;
		case 3 : return ANSI_CYAN; // non-ANSI
		case 4 : return ANSI_RED; // non-ANSI
		case 5 : return ANSI_MAGENTA;
		case 6 : return ANSI_BROWN;
		case 7 : return ANSI_GREY;
		case 8 : return ANSI_DARKGREY;
		case 9 : return ANSI_LIGHTBLUE; // non-ANSI
		case 10: return ANSI_LIGHTGREEN;
		case 11: return ANSI_LIGHTCYAN; // non-ANSI;
		case 12: return ANSI_LIGHTRED; // non-ANSI;
		case 13: return ANSI_LIGHTMAGENTA;
		case 14: return ANSI_YELLOW; // non-ANSI
		case 15: return ANSI_WHITE;
		default: return "";
	}
}

/// Function: setColor
/// Change color specified by number (Windows / QBasic colors).
///
/// See <Color Codes>
void inline setColor(int c) {
#if defined(WIN32) && !defined(RLUTIL_USE_ANSI)
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, c);
#else
	RLUTIL_PRINT(getANSIColor(c));
#endif
}

/// Function: cls
/// Clears screen and moves cursor home.
void inline cls() {
#if defined(WIN32) && !defined(RLUTIL_USE_ANSI)
	// TODO: This is cheating...
	system("cls");
#else
	RLUTIL_PRINT("\033[2J\033[H");
#endif
}

/// Function: locate
/// Sets the cursor position to 1-based x,y.
void locate(int x, int y) {
#if defined(WIN32) && !defined(RLUTIL_USE_ANSI)
	COORD coord = {x-1, y-1}; // Windows uses 0-based coordinates
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
#else // WIN32 || USE_ANSI
	#ifdef __cplusplus
		std::ostringstream oss;
		oss << "\033[" << y << ";" << x << "H";
		RLUTIL_PRINT(oss.str());
	#else // __cplusplus
		char buf[32];
		sprintf(buf, "\033[%d;%df", y, x);
		RLUTIL_PRINT(buf);
	#endif // __cplusplus
#endif // WIN32 || USE_ANSI
}

/// Function: hidecursor
/// Hides the cursor.
void inline hidecursor() {
#if defined(WIN32) && !defined(RLUTIL_USE_ANSI)
	HANDLE hConsoleOutput;
	CONSOLE_CURSOR_INFO structCursorInfo;
	hConsoleOutput = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleCursorInfo( hConsoleOutput, &structCursorInfo ); // Get current cursor size
	structCursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo( hConsoleOutput, &structCursorInfo );
#else // WIN32 || USE_ANSI
	RLUTIL_PRINT("\033[?25l");
#endif // WIN32 || USE_ANSI
}

/// Function: showcursor
/// Shows the cursor.
void inline showcursor() {
#if defined(WIN32) && !defined(RLUTIL_USE_ANSI)
	HANDLE hConsoleOutput;
	CONSOLE_CURSOR_INFO structCursorInfo;
	hConsoleOutput = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleCursorInfo( hConsoleOutput, &structCursorInfo ); // Get current cursor size
	structCursorInfo.bVisible = TRUE;
	SetConsoleCursorInfo( hConsoleOutput, &structCursorInfo );
#else // WIN32 || USE_ANSI
	RLUTIL_PRINT("\033[?25h");
#endif // WIN32 || USE_ANSI
}

/// Function: msleep
/// Waits given number of milliseconds before continuing.
void inline msleep(unsigned int ms) {
#ifdef WIN32
	Sleep(ms);
#else
	// usleep argument must be under 1 000 000
	if (ms > 1000) sleep(ms/1000000);
	usleep((ms % 1000000) * 1000);
#endif
}

/// Function: trows
/// Get the number of rows in the terminal window or -1 on error.
int trows() {
#ifdef WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
		return -1;
	else
		return csbi.srWindow.Bottom - csbi.srWindow.Top + 1; // Window height
		// return csbi.dwSize.Y; // Buffer height
#else
#ifdef TIOCGSIZE
	struct ttysize ts;
	ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
	return ts.ts_lines;
#elif defined(TIOCGWINSZ)
	struct winsize ts;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
	return ts.ws_row;
#else // TIOCGSIZE
	return -1;
#endif // TIOCGSIZE
#endif // WIN32
}

/// Function: tcols
/// Get the number of columns in the terminal window or -1 on error.
int tcols() {
#ifdef WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
		return -1;
	else
		return csbi.srWindow.Right - csbi.srWindow.Left + 1; // Window width
		// return csbi.dwSize.X; // Buffer width
#else
#ifdef TIOCGSIZE
	struct ttysize ts;
	ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
	return ts.ts_cols;
#elif defined(TIOCGWINSZ)
	struct winsize ts;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
	return ts.ws_col;
#else // TIOCGSIZE
	return -1;
#endif // TIOCGSIZE
#endif // WIN32
}	
	
// TODO: Allow optional message for anykey()?

/// Function: anykey
/// Waits until a key is pressed.
void inline anykey() {
	getch();
}

#ifndef min
/// Function: min
/// Returns the lesser of the two arguments.
#ifdef __cplusplus
template <class T> const T& min ( const T& a, const T& b ) { return (a<b)?a:b; }
#else
#define min(a,b) (((a)<(b))?(a):(b))
#endif // __cplusplus
#endif // min

#ifndef max
/// Function: max
/// Returns the greater of the two arguments.
#ifdef __cplusplus
template <class T> const T& max ( const T& a, const T& b ) { return (b<a)?a:b; }
#else
#define max(a,b) (((b)<(a))?(a):(b))
#endif // __cplusplus
#endif // max

// Classes are here at the end so that documentation is pretty.

#ifdef __cplusplus
/// Class: CursorHider
/// RAII OOP wrapper for <rlutil.hidecursor>.
/// Hides the cursor and shows it again
/// when the object goes out of scope.
struct CursorHider {
	CursorHider() { hidecursor(); }
	~CursorHider() { showcursor(); }
};
#endif

//orginalne zrodlo pochodzi/zosta³o pobrane z tej strony

/* ***************************************************************************
 *
 *          Copyright 1992-2005 by Pete Wilson All Rights Reserved
 *           50 Staples Street : Lowell Massachusetts 01851 : USA
 *        http://www.pwilson.net/   pete at pwilson dot net   +1 978-454-4547
 *
 * This item is free software: you can redistribute it and/or modify it as
 * long as you preserve this copyright notice. Pete Wilson prepared this item
 * hoping it might be useful, but it has NO WARRANTY WHATEVER, not even any
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *************************************************************************** */

/* ***************************************************************************
 *
 *                          KBHIT.C
 *
 * Based on the work of W. Richard Stevens in "Advanced Programming in
 *   the Unix Environment," Addison-Wesley; and of Floyd Davidson.
 *
 * Contains these functions:
 *
 *  To set the TTY mode:
 *     tty_set_raw() Unix setup to read a character at a time.
 *     tty_set_cooked() Unix setup to reverse tty_set_raw()
 *
 *  To read keyboard input:
 *     kb_getc()      keyboard get character, NON-BLOCKING. If a char
 *                      has been typed, return it. Else return 0.
 *     kb_getc_w()    kb get char with wait: BLOCKING. Wait for a char
 *                      to be typed and return it.
 *
 *  How to use:
 *     tty_set_raw()  set the TTY mode to read one char at a time.
 *     kb_getc()      read chars one by one.
 *     tty_set_cooked() VERY IMPORTANT: restore cooked mode when done.
 *
 * Revision History:
 *
 *     DATE                  DESCRIPTION
 * -----------    --------------------------------------------
 * 12-jan-2002     new
 * 20-aug-2002     cleanup
 * 24-nov-2003     Fixed kb_getc() so that it really is non blocking(JH)
 * 10-sep-2006     Let kb_getc() work right under certain Unix/Linux flavors
 *
 *************************************************************************** */

#ifndef WIN32

#ifndef STDIN_FILENO
  #define STDIN_FILENO 0
#endif

extern int errno;

static struct termios termattr, save_termattr;
static int ttysavefd = -1;
static enum
{
  RESET, RAW, CBREAK
} ttystate = RESET;

/* ***************************************************************************
 *
 * set_tty_raw(), put the user's TTY in one-character-at-a-time mode.
 * returns 0 on success, -1 on failure.
 *
 *************************************************************************** */
int
set_tty_raw(void)
{
  int i;

  i = tcgetattr (STDIN_FILENO, &termattr);
  if (i < 0)
  {
    printf("tcgetattr() returned %d for fildes=%d\n",i,STDIN_FILENO);
    perror ("");
    return -1;
  }
  save_termattr = termattr;

  termattr.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  termattr.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  termattr.c_cflag &= ~(CSIZE | PARENB);
  termattr.c_cflag |= CS8;
  termattr.c_oflag &= ~(OPOST);

  termattr.c_cc[VMIN] = 1;  /* or 0 for some Unices;  see note 1 */
  termattr.c_cc[VTIME] = 0;

  i = tcsetattr (STDIN_FILENO, TCSANOW, &termattr);
  if (i < 0)
  {
    printf("tcsetattr() returned %d for fildes=%d\n",i,STDIN_FILENO);
    perror("");
    return -1;
  }

  ttystate = RAW;
  ttysavefd = STDIN_FILENO;

  return 0;
}

/* ***************************************************************************
 *
 * set_tty_cbreak(), put the user's TTY in cbreak mode.
 * returns 0 on success, -1 on failure.
 *
 *************************************************************************** */
int
set_tty_cbreak()
{
  int i;

  i = tcgetattr (STDIN_FILENO, &termattr);
  if (i < 0)
  {
    printf("tcgetattr() zwraca %d dla pliku=%d\n",i,STDIN_FILENO);
    perror ("");
    return -1;
  }

  save_termattr = termattr;

  termattr.c_lflag &= ~(ECHO | ICANON);
  termattr.c_cc[VMIN] = 1;
  termattr.c_cc[VTIME] = 0;

  i = tcsetattr (STDIN_FILENO, TCSANOW, &termattr);
  if (i < 0)
  {
    printf("tcsetattr() zwraca %d dla pliku=%d\n",i,STDIN_FILENO);
    perror ("");
    return -1;
  }
  ttystate = CBREAK;
  ttysavefd = STDIN_FILENO;

  return 0;
}

/* ***************************************************************************
 *
 * set_tty_cooked(), restore normal TTY mode. Very important to call
 *   the function before exiting else the TTY won't be too usable.
 * returns 0 on success, -1 on failure.
 *
 *************************************************************************** */
int
set_tty_cooked()
{
  int i;
  if (ttystate != CBREAK && ttystate != RAW)
  {
    return 0;
  }
  i = tcsetattr (STDIN_FILENO, TCSAFLUSH, &save_termattr);
  if (i < 0)
  {
    return -1;
  }
  ttystate = RESET;
  return 0;
}

/* ***************************************************************************
 *
 * kb_getc(), if there's a typed character waiting to be read,
 *   return it; else return 0.
 * 10-sep-2006: kb_getc() fails (it hangs on the read() and never returns
 * until a char is typed) under some Unix/Linux versions: ubuntu, suse, and
 * maybe others. To make it work, please uncomment two source lines below.
 *
 *************************************************************************** */
unsigned char
kb_getc(void)
{
  int i;
  unsigned char ch;
  ssize_t size;
  usleep(20000);
/*  termattr.c_cc[VMIN] = 0; */ /* uncomment if needed */
  i = tcsetattr (STDIN_FILENO, TCSANOW, &termattr);
  size = read (STDIN_FILENO, &ch, 1);
/*  termattr.c_cc[VMIN] = 1; */ /* uncomment if needed */
  i = tcsetattr (STDIN_FILENO, TCSANOW, &termattr);
  if (size == 0)
  {
    return 0;
  }
  else
  {
    return ch;
  }
}

/* ***************************************************************************
 *
 * kb_getc_w(), wait for a character to be typed and return it.
 *
 *************************************************************************** */
unsigned char
kb_getc_w(void)
{
  unsigned char ch;
  size_t size;
  usleep(20000);
  while (1)
  {

    usleep(20000);        /* 1/50th second: thanks, Floyd! */

    size = read (STDIN_FILENO, &ch, 1);
    if (size > 0)
    {
      break;
    }
  }
  return ch;
}
#else
void usleep(int waitTime) {
    __int64 time1 = 0, time2 = 0, freq = 0;

    QueryPerformanceCounter((LARGE_INTEGER *) &time1);
    QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

    do {
        QueryPerformanceCounter((LARGE_INTEGER *) &time2);
    } while((time2-time1) < waitTime);
}


unsigned char
kb_getc_w(void)
{
    return getch();
} ;

unsigned char
kb_getc(void)
{
    usleep(20000);
    if ( !kbhit() )
        return getch();
    else
        return 0 ;
} ;

int
set_tty_cooked() {};

int
set_tty_cbreak() {};

int
set_tty_raw(void) {};
#endif /* linux */

#ifdef __cplusplus
/// Class: ttyRawer
/// RAII OOP wrapper for kbhit tty raw>
struct ttyRawer {
    ttyRawer() { set_tty_raw(); }
    ~ttyRawer() { set_tty_cooked(); }
};
#endif

} // namespace rlutil

