#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "ai5.h"
#include "ai5/game.h"
#include "game.h"
#include "gfx_private.h"
#include "input.h"
#include "vm_private.h"
static void scanlines(unsigned masked, unsigned width, unsigned height) {
    SDL_Surface *src=gfx_get_surface(1),*dst=gfx_get_surface(0);
    SDL_FillRect(src,NULL,SDL_MapRGB(src->format,248,128,64));
    SDL_FillRect(dst,NULL,0);gfx_screen_dirty();gfx_update();
    struct param_list p={.nr_params=9};
    unsigned vals[]={masked,0,0,width-1,0,1,0,0,0};
    for(unsigned i=0;i<9;i++){p.params[i].type=MES_PARAM_EXPRESSION;p.params[i].val=vals[i];}
    uint32_t start=SDL_GetTicks();
    for(unsigned y=0;y<height;y++) {
        p.params[2].val=p.params[4].val=p.params[7].val=y;
        game->sys[10](&p);vm_peek();
    }
    gfx_update();
    printf("%s %ux%u scanline refresh: %ums\n",masked?"portrait":"scene",width,height,SDL_GetTicks()-start);
    for(unsigned y=0;y<height;y++)for(unsigned x=0;x<width;x++) {
        const uint8_t *p=(uint8_t*)dst->pixels+y*dst->pitch+x*3;
        assert(p[0]==248&&p[1]==128&&p[2]==64);
        const uint8_t *shown=(uint8_t*)gfx.display->pixels+y*gfx.display->pitch+x*3;
        assert(shown[0]==248&&shown[1]==128&&shown[2]==64);
    }
    /* Freeze must snapshot the final pending change, not an older texture. */
    gfx_fill(0,0,1,1,0,0x7fff);gfx_display_freeze();
    assert(((uint8_t*)gfx.display->pixels)[0]==248);
    assert(((uint8_t*)gfx.display->pixels)[1]==248);
    assert(((uint8_t*)gfx.display->pixels)[2]==248);
    gfx_display_unfreeze();gfx_update();
}
int main(void) {
    ai5_set_game("isaku");struct game g=game_isaku;game=&g;g.update=NULL;g.handle_event=NULL;
    config.controller.enabled=false;g.mem_init();vm_init();vm_flag_off(FLAG_ANIM_ENABLE);
    gfx_init("refresh benchmark");input_init();
    scanlines(0,640,480);scanlines(1,240,360);
    puts("PASS: all scene/portrait pixels, including final row");
}
