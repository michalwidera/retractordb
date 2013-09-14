/**
 * File: test.cpp
 *
 * About: Description
 * This is a C++ example / test program for rlutil that can be
 * used for validating or showcasing the functionality.
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

#include "rlutil.h"
#include <cstdio>


#define FSM_TEST_OFF
//-----fsm test start - testowanie zaleznosci
#ifndef FSM_TEST_OFF

#include <fsmtemplate.h>
using namespace fsm ;
#include <ctime>




struct play {};
struct open_close {};
struct cd_detected {
    cd_detected(char const*, std::vector<std::clock_t> const&) {}
};
struct pauze {};
struct stop {};

// konkretna implementacja automatu
class player : public state_machine<player>
{
    // lista stanow
    enum states {
        Empty, Open, Stopped, Playing, Paused, initial_state = Empty
    };

    void start_playback(play const&);
    void open_drawer(open_close const&);
    void close_drawer(open_close const&);
    void store_cd_info(cd_detected const&);
    void stop_playback(stop const&);
    void pause_playback(pauze const&);
    void resume_playback(play const&);
    void stop_and_open(open_close const&);

    friend class state_machine<player>;

    typedef player p; // upraszcza zapis tabeli przejsc

    // tabela przejsc
    struct transition_table : mpl::vector11<

    //    Aktualny   Zdarzenie    Nastepny      Akcja
    //  +---------+-------------+---------+---------------------+
    row < Stopped , play        , Playing , &p::start_playback  >,
    row < Stopped , open_close  , Open    , &p::open_drawer     >,
    //  +---------+-------------+---------+---------------------+
    row < Open    , open_close  , Empty   , &p::close_drawer    >,
    //  +---------+-------------+---------+---------------------+
    row < Empty   , open_close  , Open    , &p::open_drawer     >,
    row < Empty   , cd_detected , Stopped , &p::store_cd_info   >,
    //  +---------+-------------+---------+---------------------+
    row < Playing , stop        , Stopped , &p::stop_playback   >,
    row < Playing , pauze       , Paused  , &p::pause_playback  >,
    row < Playing , open_close  , Open    , &p::stop_and_open   >,
    //  +---------+-------------+---------+---------------------+
    row < Paused  , play        , Playing , &p::resume_playback >,
    row < Paused  , stop        , Stopped , &p::stop_playback   >,
    row < Paused  , open_close  , Open    , &p::stop_and_open   >
    //  +---------+-------------+---------+---------------------+

    > {};
};

void player::start_playback(play const&){ std::cerr << "start "; }
void player::open_drawer(open_close const&){ std::cerr << "open "; }
void player::close_drawer(open_close const&){ std::cerr << "close "; }
void player::store_cd_info(cd_detected const&){ std::cerr << "store "; }
void player::stop_playback(stop const&){ std::cerr << "stop ";}
void player::pause_playback(const pauze &){ std::cerr << "pause ";}
void player::resume_playback(play const&){ std::cerr << "resume "; }
void player::stop_and_open(open_close const&){ std::cerr << "stop-open ";}

int fsmTest2(){
    std::cerr << "*Program main body - start*" << std::endl ;

    player pl;                      // egzemplarz automatu

    pl.process_event(open_close()); // uzytkownik otwiera odtwarzacz CD
    pl.process_event(open_close()); // wklada CD i zamyka tacke
    pl.process_event(               // nastepuje wykrywanie plyty CD
        cd_detected(
             "louie, louie"
           , std::vector<std::clock_t>( /* dlugosci sciezek */ )
        )
    );
    pl.process_event(play());       // itd.
    pl.process_event(pauze());
    pl.process_event(play());
    pl.process_event(stop());

    std::cerr << std::endl ;
    std::cerr << "*Program main body - stop*" << std::endl ;

    return 0;
}
#endif
//-----fst test stop

#define waitkey std::cout << "Press any key to continue..." << std::endl; rlutil::anykey()

int main_rlutil() {
	std::cout << "Welcome to rlutil test program." << std::endl;
	waitkey;

	std::cout << "Test 1: Colors" << std::endl;
	for (int i = 0; i < 16; i++) {
		rlutil::setColor(i);
		std::cout << i << " ";
	}
	std::cout << std::endl << "You should see numbers 0-15 in different colors." << std::endl;
	waitkey;

	rlutil::cls();
	std::cout << "Test 2: Clear screen" << std::endl;
	std::cout << "The colors should now be gone." << std::endl;
	waitkey;

	std::cout << "Test 3: Cursor hiding" << std::endl;
	rlutil::hidecursor();
	std::cout << "The cursor should now be invisible." << std::endl;
	waitkey;

	std::cout << "Test 4: Cursor showing" << std::endl;
	rlutil::showcursor();
	std::cout << "The cursor should now be visible again." << std::endl;
	waitkey;

	rlutil::cls();
	std::cout << "Test 5: Cursor positioning" << std::endl;
	rlutil::locate(16,6); std::cout << "(16,6)";
	rlutil::locate(4,3); std::cout << "(4,3)";
	rlutil::locate(8,8); std::cout << "(8,8)";
	std::cout << std::endl << "You should see three coordinates in their specified locations." << std::endl;
	waitkey;

	// Test 6
	{	int x = 7; int y = 7; unsigned int cnt = 0;
		while (true) {
			rlutil::cls();
			std::cout << "Test 6: Blocking keyboard input" << std::endl;
			std::cout << "You should be able to move the '@' character with WASD keys." << std::endl;
			std::cout << "Hit Space to continue to the next test." << std::endl;
			std::cout << "Turn count: " << cnt << std::endl;
			rlutil::locate(x,y); std::cout << "@"; // Output player
			char k = getch(); // Get character
			if (k == 'a') --x;
			else if (k == 'd') ++x;
			else if (k == 'w') --y;
			else if (k == 's') ++y;
			else if (k == ' ') break;
			cnt++;
		}
	}

	// Test 7
	{	int x = 7; int y = 7; unsigned int cnt = 0;
		rlutil::hidecursor();
		rlutil::cls();
		std::cout << "Test 7: Non-blocking keyboard input" << std::endl;
		std::cout << "You should be able to move the '@' character with WASD keys." << std::endl;
		std::cout << "Hit Space to continue to the next test." << std::endl;
		std::cout << "Turn count: " << cnt << std::endl;
		rlutil::locate(x,y); std::cout << "@" << std::endl; // Output player
		while (true) {
			rlutil::locate(1,4); std::cout << "Turn count: " << cnt;
			if (kbhit()) {
				char k = getch(); // Get character
				rlutil::locate(x,y); std::cout << " "; // Erase player
				if (k == 'a') --x;
				else if (k == 'd') ++x;
				else if (k == 'w') --y;
				else if (k == 's') ++y;
				else if (k == ' ') break;
				rlutil::locate(x,y); std::cout << "@"; // Output player
			}
			cnt++;
			fflush(stdout);
		}
		rlutil::showcursor();
	}

	// Test 8
	{	int x = 7; int y = 7;
		rlutil::cls();
		rlutil::CursorHider curs;
		std::cout << "Test 8: Arrow keys" << std::endl;
		std::cout << "You should be able to move the '@' character with arrow keys." << std::endl;
		std::cout << "Hit Escape to continue to the next test." << std::endl;
		gotoxy(x,y); std::cout << "@" << std::endl; // Output player
		while (true) {
			if (kbhit()) {
				int k = rlutil::getkey(); // Get character
				gotoxy(x,y); std::cout << " "; // Erase player
				if (k == rlutil::KEY_LEFT) --x;
				else if (k == rlutil::KEY_RIGHT) ++x;
				else if (k == rlutil::KEY_UP) --y;
				else if (k == rlutil::KEY_DOWN) ++y;
				else if (k == rlutil::KEY_ESCAPE) break;
				gotoxy(x,y); std::cout << "@"; // Output player
				fflush(stdout);
			}
		}
	}

	rlutil::cls();
	std::cout << "Test 9: Delay" << std::endl;
	std::cout << "Next numbers should appear rapidly after each other (but not instantly)." << std::endl;
	rlutil::msleep(500);
	for (float t = 0; t <= 4; t += 0.33333333f) {
		std::cout << t << "s" << std::endl;
		rlutil::msleep(333);
	}
	waitkey;

	rlutil::cls();
	std::cout << "Test 10: Terminal Dimensions" << std::endl;
	std::cout << "You should see the size in character rows and columns of your terminal window." << std::endl;
	std::cout << rlutil::trows() << " Rows" << std::endl;
	std::cout << rlutil::tcols() << " Columns" << std::endl;
	waitkey;
	
	std::cout << "All tests done. Bye!" << std::endl;
	return 0;
}


