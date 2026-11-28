#pragma once
#include <stddef.h>
#define SHELL_LINE_MAX 128
#define SHELL_HISTORY_MAX 16
#define KEY_UP 0x100
#define KEY_DOWN 0x101
struct lineedit {char line[SHELL_LINE_MAX],draft[SHELL_LINE_MAX],history[SHELL_HISTORY_MAX][SHELL_LINE_MAX];unsigned len,count,next,cursor;};
enum line_event { LINE_NONE, LINE_ECHO, LINE_ERASE, LINE_SUBMIT, LINE_REPAINT };
void line_init(struct lineedit*);
enum line_event line_key(struct lineedit*,int);
void line_finish(struct lineedit*);
