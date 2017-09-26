#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <math.h>

#include "buffer.h"
#include "global.h"

//OK......for some-reason,. it sefgfaults WHEN trying to realloc NORMALLY...
//so ijust create my own,
//SHOULNdt add to much over-head
//TODO: fix realloc( )
void* my_realloc(void* ptr, size_t orig_size, size_t dest_size) {
	if (dest_size <= orig_size) return ptr;

	void* newptr = malloc(dest_size);
	memcpy(newptr, ptr, orig_size);
	
	free(ptr);

	return newptr;
}

struct line_s* lines_add(struct line_s* where, char* data, int size) {
	struct line_s* new;
	new = malloc(sizeof(struct line_s));
	new->data = data;
	new->capacity = size;
	new->prev = new->next = NULL;
	
	if (where) { //if the NEW line is not the head (passed NULL)
		//arragnge pointers
		if (where->next) {
			//is not tail
			//where->next->prev == where
			where->next->prev = new;
		}
		new->next = where->next;
		where->next = new;
		new->prev = where;
	}
	
	return new;
}

struct line_s* lines_del(struct line_s* line) {
	if (!line) return NULL;

	if (line->next) { //make next node,. point to node before.
		line->next->prev = line->prev;
	}
	if (line->prev) { //MAKE prev node.. point to node AFTTER
		line->prev->next = line->next;
	}
	
	struct line_s* next = line->next; //for return:

	//free..
	free(line->data);
	free(line);

	return next;
}

#define MIN_LINE_SIZE 40

int line_num_offset = 4;

struct buffer_s* buffer_new(FILE* f, char* filename) {
	struct buffer_s* buf = malloc(sizeof(struct buffer_s));
	buf->file = f;
	if (!(buf->file)) {
		return 0;
	}

	buf->filename = malloc(strlen(filename) + 1); //alloc space for file name
	strcpy(buf->filename, filename); //PUT filename in buffefr
	
	buf->cx = buf->cy = 0;
	buf->line_cur = 1;
	buf->lines_head = NULL;

	struct line_s* line = NULL;

	int linecount = 0; //nesencary for line num of set

	while (1) {
		char* data = malloc(MIN_LINE_SIZE); //INITIAL (mninum) line size
		size_t n = 0;

		if (getline(&data, &n, buf->file) == -1) {
			if (linecount != 0) break;

			//EMPTY FILE.. add empty, line
			*data = '\0';
		}
		int last = strlen(data)-1;
		if (data[last] == '\n') data[last] = 0;

		/*char* data2 = malloc(strlen(data)+1);
		memcpy(data2, data, strlen(data)+1);
		free(data);*/
		
		linecount++;
		line = lines_add(line, data, n);

		if (linecount == 1) {
			//firts interation.- add head
			buf->lines_head = line;
		}
	}

	line_num_offset = linecount > 999 ? log10(linecount)+2 : line_num_offset;

	return buf;
}

void buffer_destroy(struct buffer_s* buf) {
	if (buf->filename) free(buf->filename);
	if (buf->file) fclose(buf->file);
	struct line_s* line = buf->lines_head;
	while ((line = lines_del(line)));

	free(buf);
}

#define TAB_SIZE 4 //tab size,. in spaces

inline int abs(int x) { return x < 0 ? -x : x;}

inline int strcountc(char* str, char c) { //count  chars c  IN str
	int n = 0;
	while (*str) n += (*(str++) == c) ? 1 : 0;
	return n;
}

inline int strdisplen(char* str) { //string DISPLAY length (for tabs..)
	int len = strlen(str);
	len -= strcountc(str, '\t');
	len += strcountc(str, '\t')*TAB_SIZE;
	return len;
}

inline int strcuridx(int cx, char* str) { //return, index for  string RELATIVE to cursor peosition
	int str_idx = 0; //the valuem, to return
	int disp_idx = 0; //the index of display, to see IF cx concide
	while (disp_idx != cx) {
		if (!str[str_idx]) return -1;
		if (str[str_idx++] == '\t') disp_idx += TAB_SIZE;
		else disp_idx++;
	}

	return str_idx;
}

void buffer_cursor_move(struct buffer_s* buf, WINDOW* win, int dy, int dx) {
	int cx = buf->cx, cy = buf->cy;
	int h,w;
	getmaxyx(win, h, w);

	//get line of cursor
	struct line_s* line = buf->lines_head;
	// THAT 2-buf->line_cur.. i DONT get whyit work but its magic:
	for (int i = 2-buf->line_cur; i <= cy && line; i++) line = line ? line->next : NULL;

	if (!line) return;
	
	int sign_y = dy/abs(dy);

	for (int i=0;i<abs(dy);i++) { //STEP one at a a time..
		//MOVE Y
		if (line && (sign_y > 0 ? line->next : line->prev)) cy += sign_y;
		else break;

		if (cy < 0) {
			cy = 0;
			if (buf->line_cur > 1) buf->line_cur--;
		} else if (cy >= h) {
			cy = h-1;
			if (line && line->next) buf->line_cur++;
		}
	}

	int sign_x = dx/abs(dx);

	for (int i=0;i<abs(dx);i++) {
		if (cx < line_num_offset) cx = line_num_offset;

		int i = strcuridx(cx-line_num_offset, line->data);

		if (line && i != -1 && sign_x > 0 ? line->data[i] : 1) {
			//move by tab size WHEN its tab
			if ((sign_x < 0 ? line->data[i-1] : line->data[i]) == '\t') cx += TAB_SIZE*sign_x;
			else cx += sign_x;
		}
		else break;
	}	

	buf->cx = cx, buf->cy = cy;
}

//check bounds andALIGN cursor to line, SO it doent go out of it...	
void buffer_cursor_fix(struct buffer_s* buf, WINDOW* win) {
	//bounds checks
	if (win) {
		int w,h;
		getmaxyx(win, h, w);

		if (buf->cx > w) {
			buf->cx = line_num_offset; buf->cy++;
		}

		if (buf->cy >= h) {
			buf->cy = h-1;
			buf->line_cur++;
		}
	}
	
	if (buf->cx < line_num_offset) {
		buf->cx = line_num_offset;
	}

	if (buf->cy < 0) {
		buf->cy = 0;
		if (buf->line_cur > 1) buf->line_cur--;
	}
	
	if (buf->line_cur < 1) buf->line_cur = 1;
	
	//get line of cursor
	struct line_s* line = buf->lines_head;
	// THAT 2-buf->line_cur.. i DONT get whyit work but its magic:
	for (int i = 2-buf->line_cur; i <= buf->cy && line; i++) line = line ? line->next : NULL;

	if (line) {
		int disp_len = strdisplen(line->data);
		if (buf->cx-line_num_offset > disp_len) buf->cx = disp_len+line_num_offset;
	}

	/*snap, cursor when tabs to tab. begingng
	char* s = strchr(line->data, '\t');
	int offset; //where the tab, is in the string ((index)
	while (s) {
		offset = s - line->data;
		for (int i = 0; i < TAB_SIZE; i++) {
			if (offset + i == buf->cx - line_num_offset) {
				buf->cx = offset+line_num_offset;
				goto pft;
			}
		}
		s = strchr(s+1, '\t');
	}
	pft:;*/
}

void buffer_print(struct buffer_s* buf, WINDOW* win) { //prints bufferrfe
	buffer_cursor_fix(buf, win);
	int w,h;
	getmaxyx(win,h,w);

	struct line_s* line = buf->lines_head;
	for (int i = 1; i < buf->line_cur && line; i++) line = line->next; //topmost line
	
	werase(win);

	int ln;
	for (ln = 0; line && ln < h; ln++) {
		#ifndef NLINE_NUM
		wattr_colors(win, LINENUM);
		char fmt[10];
		snprintf(fmt, sizeof(fmt), "%%-%di|", line_num_offset-1);
		mvwprintw(win, ln, 0, fmt, ln+buf->line_cur);
		wattr_colors_off(win, LINENUM);
		#endif
	   	char* s = line->data;
		while (*s) {
			if (*s == '\t') for (int i = 0; i<TAB_SIZE; i++) waddch(win, ' ');
			else waddch(win, *s);
			s++;
		}
		line = line->next;
	}
	//if (ln == 0) buf->line_cur--;
	while (ln < h) { //fill empty, lines with '~'
		wmove(win, ln, 0);
		wclrtoeol(win);
		wmove(win, ln++, 0);
		waddch(win, '~');
	}
	
	wmove(win, buf->cy, buf->cx);
}

char* last_search = NULL;

void buffer_search(struct buffer_s* buf, WINDOW* win) {
	int w,h;
	getmaxyx(win,h,w);
	char str[64] = "";
	
	wmove(win, h-1,0);
	wclrtoeol(win);
	wprintw(win, "SEARCH: ");
	
	echo();
	wgetnstr(win, str, sizeof(str));
	noecho();

	if (!*str && last_search) {
		strcpy(str, last_search);
		free(last_search);
	}
	last_search = malloc(strlen(str)+1);
	strcpy(last_search, str);

	set_status("/%s", str);
	
	//SEARCH !!
	struct line_s* line = buf->lines_head;
	for (int i = 1; line; (i++, line = line->next)) {
		if (strstr(line->data, str)) {
			buf->line_cur = i;
			break;
		}
	}
}

void buffer_type(struct buffer_s* buf, char c) {
	buffer_cursor_fix(buf, NULL);

	//get cursor line
	struct line_s* line = buf->lines_head;
	for (int i = 2-buf->line_cur; i <= buf->cy && line; i++) line = line->next;

	//CHECK if NEW string would, be over capcity

	if (!line) return;
		
	if (strlen(line->data)+2 > line->capacity) { //2 for: nul charcter+c
		#define MIN_REALLOC 10
		line->data = my_realloc(line->data, line->capacity, line->capacity + MIN_REALLOC);
		line->capacity += MIN_REALLOC;
	}

	int idx = strcuridx(buf->cx-line_num_offset, line->data);
	if (idx == -1) return; //what....... (cursour out of bound)
	
	//shif t string to right, to add
	//memmove(line->data+i+2, line->data+i+1, strlen(line->data)-i+2);
	for (int i = strlen(line->data)+2; i > idx ;i--) line->data[i] = line->data[i-1];

	line->data[idx] = c;
	buf->cx += c == '\t' ? TAB_SIZE : 1;
}

void buffer_enter(struct buffer_s* buf) {
	buffer_cursor_fix(buf, NULL);
	
	//get cursor line
	struct line_s* line = buf->lines_head;
	for (int i = 2-buf->line_cur; i <= buf->cy && line; i++) line = line->next;

	if (!line) return;

	int idx = strcuridx(buf->cx-line_num_offset, line->data);

	if (idx == -1) {
		char* s = malloc(MIN_LINE_SIZE);
		//*s = '\0';
		lines_add(line, s, MIN_LINE_SIZE);
		buf->cy--;
		buf->cx = line_num_offset;
		return;
	}
	
	//STRING after enter.. (to split LINE)
	int size = strlen(line->data)-idx+1;
	char* after = malloc(size);
	
	//FOR auto tab's
	int tabs = strcountc(line->data, '\t');
	int i;
	for (i=0;i<tabs;i++) after[i] = '\t';
	after[i] = 0;

	strncpy(after+tabs, line->data+idx, size);
	line->data[idx] = '\0';
	lines_add(line, after, size);
	buf->cx = line_num_offset+tabs*TAB_SIZE;
	buf->cy++;
}

void buffer_erase(struct buffer_s* buf, int is_del) {
	buffer_cursor_fix(buf, NULL);

	//get cursor line
	struct line_s* line = buf->lines_head;
	for (int i = 2-buf->line_cur; i <= buf->cy && line; i++) line = line->next;

	if (!line) return;

	int idx = strcuridx(buf->cx-line_num_offset, line->data);
	if (idx == -1) return;
	
	idx += is_del ? 0 : -1; //IF del , delete charactrecer after. if not . chracrater of cursro

	if (idx == -1) { //merge, with LINE before
		if (!(line->prev)) return;
		
		buf->cy--;
		buf->cx = strdisplen(line->prev->data)+line_num_offset;int size = strlen(line->data)+1 + strlen(line->prev->data)+1;
		
		if (line->prev->capacity < size) {
			//NOT ENOUGH  capcity
			line->prev->data = my_realloc(line->prev->data, line->prev->capacity, size);
			line->prev->capacity = size;
		}
		strcat(line->prev->data, line->data);
		lines_del(line);
	
		return;
	}
	
	if (line->data[idx] == '\0') { //merge,with line after
		if (!(line->next)) return;

		int size = strlen(line->data)+1 + strlen(line->next->data)+1;
		if (line->capacity < size) {
			//NOT ENOUGH  capcity
			line->data = my_realloc(line->data, line->capacity, size);
			line->capacity = size;
		}
		strcat(line->data, line->next->data);
		lines_del(line->next);

		return;
	}

	//normal charcrater remove
	int len = strlen(line->data)+1;
	buf->cx += (is_del ? 0 : -1) * (line->data[idx] == '\t' ? TAB_SIZE : 1);
	for (int i = idx; i < len ;i++) line->data[i] = line->data[i+1];
}

void buffer_write(struct buffer_s* buf) {
	set_status("'%s'", buf->filename);

	struct line_s* line = buf->lines_head;
	rewind(buf->file);
	int lines = 0;
	int bytes = 0;
	int err = 0;
	while (line) {
		lines++;
		bytes += strlen(line->data)+1;
		err = (fputs(line->data, buf->file) == EOF);
		fputc('\n', buf->file);
		line = line->next;
	}
	if (fflush(buf->file) == 0 && !err)
		set_status("'%s' %iL, %iC written", buf->filename, lines, bytes);
	else
		set_status("'%s' is open in readonly", buf->filename);
}

void buffer_dosyntax(struct buffer_s* buf) {
	/*struct line_s* line = buf->lines_head;
	//TODO decide, if VISIBLE lines or all. lines
	//get cursor line
	for (int i = 2-buf->line_cur; i <= buf->cy && line; i++) line = line->next;
	
	int c = '\0';
	char str[20] = "";
	int stridx = 0;
	int i = 0;
	while (1) {
		c = line->data[i];
		if (!c) {i = 0; stridx = 0; ; continue}
		if (isspace(c)) {
			
		}
		i++;
	}
*/
}
