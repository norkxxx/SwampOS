#include "lineedit.h"
static unsigned copy(char*d,const char*s){unsigned i=0;while(i<SHELL_LINE_MAX-1&&s[i]){d[i]=s[i];i++;}d[i]=0;return i;}
static int equal(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static unsigned history_index(const struct lineedit*s,unsigned cursor){return (s->next+SHELL_HISTORY_MAX-s->count+cursor)%SHELL_HISTORY_MAX;}
void line_init(struct lineedit*s){*s=(struct lineedit){0};}
static void detach(struct lineedit*s){if(s->cursor!=s->count){s->cursor=s->count;s->draft[0]=0;}}
enum line_event line_key(struct lineedit*s,int k){
 if(k==KEY_UP){if(!s->cursor)return LINE_NONE;if(s->cursor==s->count)copy(s->draft,s->line);s->cursor--;s->len=copy(s->line,s->history[history_index(s,s->cursor)]);return LINE_REPAINT;}
 if(k==KEY_DOWN){if(s->cursor>=s->count)return LINE_NONE;s->cursor++;s->len=copy(s->line,s->cursor==s->count?s->draft:s->history[history_index(s,s->cursor)]);return LINE_REPAINT;}
 if(k=='\r'||k=='\n')return LINE_SUBMIT;
 if(k==8||k==127){if(!s->len)return LINE_NONE;detach(s);s->line[--s->len]=0;return LINE_ERASE;}
 if(k>=32&&k<=126&&s->len<SHELL_LINE_MAX-1){detach(s);s->line[s->len++]=(char)k;s->line[s->len]=0;return LINE_ECHO;}
 return LINE_NONE;
}
void line_finish(struct lineedit*s){if(s->len&&(!s->count||!equal(s->line,s->history[(s->next+SHELL_HISTORY_MAX-1)%SHELL_HISTORY_MAX]))){copy(s->history[s->next],s->line);s->next=(s->next+1)%SHELL_HISTORY_MAX;if(s->count<SHELL_HISTORY_MAX)s->count++;}s->cursor=s->count;s->len=0;s->line[0]=s->draft[0]=0;}
