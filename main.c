#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

#include "buffer.h"
#include "global.h"

#define HELPSTRING \
	"usage: %s [OPTION] [FILE]\n\n"\
	" -h, show this helpstring\n"\
	" -v, show version\n"\
	" -r, open read'only"

#define VERSIONSTRING "cjed 0.1 compiled " __DATE__ " " __TIME__

#define MAX_BUFS 10 //TODO: dynalloc bufs
struct buffer_s* bufs[MAX_BUFS];
int buf_count = 0;

int current_buf = 0;

char* status_msg = NULL;

int main(int argc, char** argv) {
	if (argc < 2) {
		printf(HELPSTRING"\n", argv[0]);
		return 1;
	}

	char* mode = "r+";

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			for (char* c = &argv[i][1]; *c; c++) switch (*c) {
				case 'h':
					printf(HELPSTRING"\n", argv[0]);
					return 0;
				case 'v':
					printf(VERSIONSTRING"\n");
					return 0;
				case 'r':
					mode = "r";
					break;
			}
		} else {
			bufs[buf_count] = buffer_new(fopen(argv[i], mode), argv[i]);
			if (!bufs[buf_count]) {
				printf("CANN'OT open '%s': %s\n", argv[i], strerror(errno));
				return 1;
			} else buf_count++;
		}
	}
	
	//INIT curses
	initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();
	set_escdelay(0);
	start_color();

	//init color-pairs	
	#define X(name, attrs, fg, bg) init_pair(COLOR_##name, fg, bg);
	gen_attrs_colors();

	//init MOUSE
	mouseinterval(0);
	mmask_t oldmask;
	mousemask(ALL_MOUSE_EVENTS, &oldmask);

	mmask_t MOUSE_SCROLL_UP = 0;
	mmask_t MOUSE_SCROLL_DOWN = 0;
	FILE* mfile;
	mfile = fopen(".cjed_mcfg", "r");
	if (mfile) {
		fread(&MOUSE_SCROLL_UP, sizeof(mmask_t), 1, mfile);
		fread(&MOUSE_SCROLL_DOWN, sizeof(mmask_t), 1, mfile);
		fclose(mfile);
	} else {
		MEVENT ev;
		
		mvprintw(0,0, "SCROLL MOUSE UP");
		refresh();
		getch();
		if (getmouse(&ev) == OK) MOUSE_SCROLL_UP = ev.bstate;
		
		mvprintw(1,0,"OK");
		refresh();
		sleep(1);

		mvprintw(2,0, "SCROLL MOUSE DOWN");
		refresh();
		getch();
		if (getmouse(&ev) == OK) MOUSE_SCROLL_DOWN = ev.bstate;

		mfile = fopen(".cjed_mcfg", "w");
		if (mfile) {
			fwrite(&MOUSE_SCROLL_UP, sizeof(mmask_t), 1, mfile);
			fwrite(&MOUSE_SCROLL_DOWN, sizeof(mmask_t), 1, mfile);
			fclose(mfile);
		}
	}

	WINDOW* win = subwin(stdscr, LINES-1,COLS,0,0);

	//LOOP
	int running = 1;
	while (running) {	
		if (status_msg) {
			move(LINES-1, 0);
			clrtoeol();
			wattr_colors(stdscr, STATUSMSG);
			printw(status_msg);
			wattr_colors_off(stdscr, STATUSMSG);
		}

		buffer_print(bufs[0], win);
		touchwin(stdscr);
		wrefresh(stdscr);

		int cx,cy;
		getyx(win, cy,cx);
		wmove(stdscr, cy,cx);		
		int c = getch();

		//CTRL + ABCDEFGHIJKLMNOPQRSTUVWXYZ
		#define CTRL(c) 1+c-'A'

		switch (c) {
			case CTRL('Q'):
				running = 0;
				break;

			case CTRL('S'):
				buffer_write(bufs[current_buf]);
				break;

			case CTRL('F'):
				buffer_search(bufs[current_buf], win);
				break;

			#define ARROW_KEY(n, y, x) case KEY_##n: buffer_cursor_move(bufs[current_buf], win, y, x); break;
			ARROW_KEY(UP,   -1, 0);
			ARROW_KEY(DOWN,  1, 0);
			ARROW_KEY(LEFT,  0,-1);
			ARROW_KEY(RIGHT, 0, 1);
			ARROW_KEY(END, 0, 1000);
			ARROW_KEY(HOME,0,-1000);

			#define SCROLL_SPEED 40

			case KEY_PPAGE:
				bufs[current_buf]->line_cur -= SCROLL_SPEED;
				break;
			case KEY_NPAGE:
				bufs[current_buf]->line_cur += SCROLL_SPEED;
				break;

			case KEY_MOUSE: {
				//mouse  action
				MEVENT ev;
				if (getmouse(&ev) != OK) break;
				
				if (ev.bstate & BUTTON1_PRESSED) {
					//lef-t button click
					bufs[current_buf]->cx = ev.x;
					bufs[current_buf]->cy = ev.y;
				}
				#define MOUSE_SCROLL_SPEED 3
				if (ev.bstate & MOUSE_SCROLL_UP) {
					bufs[current_buf]->line_cur -= MOUSE_SCROLL_SPEED;
				} else if (ev.bstate & MOUSE_SCROLL_DOWN) {
					bufs[current_buf]->line_cur += MOUSE_SCROLL_SPEED;
				}

			} break;

			case KEY_BACKSPACE:
			case KEY_DC: //DELETE key
				buffer_erase(bufs[current_buf], c == KEY_DC);
				break;

			case '\n':
				buffer_enter(bufs[current_buf]);
				break;

			default:
				if (isprint(c)) {
					case '\t':
					buffer_type(bufs[current_buf], c);
				}
		}
	}

	//cleanup
	delwin(win);
	endwin();
	
	for (int i = 0; i < buf_count; i++) {
		buffer_destroy(bufs[i]);
	}

	return 0;
}
