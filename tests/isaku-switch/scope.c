#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "isaku_switch.h"
#include "ai5.h"
#include "game.h"
#include "gfx_private.h"
#include "cursor.h"
#include "vm_private.h"
extern bool char_is_opener(const char*);
extern bool char_is_closer(const char*);
int main(void)
{
    /* Every non-Isaku game must retain its shared behavior even when the
     * opt-in binary is explicitly launched with another game. */
    for(unsigned i=0;i<AI5_NR_GAME_IDS;i++) {
        ai5_target_game=i;
#ifdef ISAKU_SWITCH_PORT
        assert(isaku_switch_active()==(i==GAME_ISAKU));
#else
        assert(!isaku_switch_active());
#endif
        if(isaku_switch_active())continue;
        ai5_set_text_encoding(AI5_TEXT_ENCODING_GBK);
        assert(char_is_opener("\x81\x69"));
        assert(!char_is_opener("\xa3\xa8"));
    }
    ai5_set_game("isaku");
    struct game g=game_isaku;game=&g;g.update=NULL;g.handle_event=NULL;
    g.mem_init();vm_init();gfx_init("scope");
#ifdef ISAKU_SWITCH_PORT
    /* Run shared functions as another game while leaving a compatible
     * fixture surface layout. No Isaku file initialization or batching. */
    ai5_target_game=GAME_YUNO;g.id=GAME_YUNO;
#endif
    assert(!isaku_switch_active());
    cursor_init(NULL); /* Must not require isaku-cursors.bin. */
    for(int x=0;x<3;x++) {
        gfx_fill(x,0,1,1,0,0x7fff);gfx_update_pending();
        assert(((uint8_t*)gfx.display->pixels)[x*3]==248);
    }
    char dir[]="/tmp/isaku-scope-XXXXXX";assert(mkdtemp(dir));assert(!chdir(dir));
    struct param_list p={.nr_params=2};p.params[0].type=p.params[1].type=MES_PARAM_EXPRESSION;
    p.params[0].val=2;p.params[1].val=8;g.sys[4](&p);
    uint8_t data[4096];FILE*f=fopen("FLAG08","rb");assert(f);assert(fread(data,1,4096,f)==4096);fclose(f);
    for(unsigned i=0;i<4096;i++)assert(data[i]==0);
    unlink("FLAG08");assert(!chdir("/tmp"));assert(!rmdir(dir));
    puts("PASS: game activation matrix, unchanged non-target GBK punctuation, native cursor fallback, immediate present, original empty-save behavior");
}
