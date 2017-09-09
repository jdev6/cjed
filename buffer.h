#ifndef BUFFER_H
#define BUFFER_H

//line (linked list)
struct line_s {
	char* data;
	int capacity;

	struct line_s* next; //next node
	struct line_s* prev; //prev node
};

//STRUCTURE represinting a file
struct buffer_s {
	int cx,cy; //cursor
	struct line_s* lines_head; //pointer to, linked list HEAD
	int line_cur; //current line
	char* filename;
	FILE* file;
};

struct line_s* lines_add(struct line_s* where, char* data, int size); //appendsline after line qwhere,retrutrns new
struct line_s* lines_del(struct line_s* line); //delete line, retruns LINE AFTER it before delettion

struct buffer_s* buffer_new(FILE* f, char* filename); //create, new buffer
void buffer_destroy(struct buffer_s* buf); //free buffer AND lines
void buffer_cursor_move(struct buffer_s* buf, WINDOW* win, int dy, int dx); //move cursor
void buffer_print(struct buffer_s* buf, WINDOW* win); //print buffre
void buffer_type(struct buffer_s* buf, char c); //type charactrer
void buffer_enter(struct buffer_s* buf); //enter (insrt, line)
void buffer_erase(struct buffer_s* buf, int is_del); //erase(BACKSPACE or DELETE)
void buffer_write(struct buffer_s* buf); //wrte buffer

#endif
