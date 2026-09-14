#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ai5.h"
#include "ai5/game.h"
#include "game.h"
#include "gfx_private.h"
#include "input.h"
#include "vm_private.h"
extern bool char_is_opener(const char *);
extern bool char_is_closer(const char *);
static unsigned calls;
static void capture(const char*s){assert(!strcmp(s,"\xd6\xd0" "A1" "\xce\xc4"));calls++;}
int main(void){
 ai5_set_game("isaku");ai5_set_text_encoding(AI5_TEXT_ENCODING_GBK);
 struct game g=game_isaku;game=&g;g.update=NULL;g.handle_event=NULL;g.draw_text_zen=capture;
 g.mem_init();vm_init();config.controller.enabled=false;gfx_init("CHS text");input_init();
 gfx_text_init(getenv("ISAKU_CHS_FONT"),0);
 uint8_t text[]={0xd6,0xd0,'A','1',0xce,0xc4,0,0x0f};
 vm.ip.code=text;vm.ip.ptr=0;vm_stmt_txt_no_log();assert(calls==1&&vm.ip.ptr==7);
 text[6]=0x0f;vm.ip.ptr=0;vm_stmt_txt_no_log();assert(calls==2&&vm.ip.ptr==6);
 assert(char_is_opener("\xa3\xa8")&&char_is_closer("\xa3\xa9"));
 assert(char_is_closer("\xa1\xa3"));assert(!char_is_opener("\x81\x69"));
 ai5_set_text_encoding(AI5_TEXT_ENCODING_SJIS);
 assert(char_is_opener("\x81\x69")&&char_is_closer("\x81\x6a"));
 puts("PASS: mixed GBK/ASCII VM text, null/control boundaries, GBK punctuation, preserved SJIS punctuation");
}
