#ifndef GLOBAL_H
#define GLOBAL_H

#define gen_attrs_colors() \
	X(LINENUM, 0, COLOR_YELLOW, COLOR_BLACK)

//ENUM for attron(COLOR_PAIR(x)), x being ENUM value of each thiung
enum COLORS {
	COLOR_NULL, //to startt, from 1
	#define X(name, attrs, fg, bg) COLOR_##name,
	gen_attrs_colors()
	#undef X
	COLOR_COUNT
};

//ENUM for attributtes
enum ATTRS {
	#define X(name, attrs, fg, bg) ATTRS_##name = attrs,
	gen_attrs_colors()
	#undef X
	ATTR_COUNT
};

#define wattr_colors(win, name)     wattron(win, ATTRS_##name | COLOR_PAIR(COLOR_##name))
#define wattr_colors_off(win, name) wattroff(win, ATTRS_##name | COLOR_PAIR(COLOR_##name))

#endif
