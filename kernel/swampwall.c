#include "swampwall.h"
static int same(const uint8_t*a,const uint8_t*b){for(unsigned i=0;i<4;i++)if(a[i]!=b[i])return 0;return 1;}
static int valid(const struct wall_rule*r){return r&&r->app&&r->ip[0]&&r->ip[0]<224&&r->ip[0]!=127&&(r->proto==6||r->proto==17)&&r->port;}
int wall_allow(const struct wall*w,uint32_t a,const uint8_t ip[4],uint8_t p,uint16_t port){if(!w||!ip||!a||w->count>WALL_MAX)return 0;for(size_t i=0;i<w->count;i++){const struct wall_rule*r=&w->rules[i];if(valid(r)&&r->app==a&&r->proto==p&&r->port==port&&same(r->ip,ip))return 1;}return 0;}
int wall_grant(struct wall*w,const struct wall_rule*r){if(!w||!valid(r)||w->count>WALL_MAX)return 0;if(wall_allow(w,r->app,r->ip,r->proto,r->port))return 1;if(w->count==WALL_MAX)return 0;w->rules[w->count++]=*r;return 1;}
int wall_revoke(struct wall*w,uint32_t a){if(!w||!a||w->count>WALL_MAX)return 0;size_t n=0;for(size_t i=0;i<w->count;i++)if(w->rules[i].app!=a)w->rules[n++]=w->rules[i];for(size_t i=n;i<w->count;i++)w->rules[i]=(struct wall_rule){0};w->count=n;return 1;}
int wall_selftest(void){struct wall w={0};struct wall_rule r={1,{192,0,2,2},17,53};return !wall_allow(&w,1,r.ip,17,53)&&wall_grant(&w,&r)&&wall_allow(&w,1,r.ip,17,53)&&!wall_allow(&w,2,r.ip,17,53)&&!wall_allow(&w,1,r.ip,6,53)&&wall_revoke(&w,1)&&!wall_allow(&w,1,r.ip,17,53);}
