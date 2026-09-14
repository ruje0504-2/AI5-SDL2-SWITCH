#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "ai5.h"
#include "ai5/game.h"
#include "game.h"
#include "gfx_private.h"
#include "input.h"
#include "vm_private.h"
static unsigned calls, mode, previous;
static unsigned pixel(unsigned n,int x,int y) {
    SDL_Surface *s=gfx_get_surface(n);
    return ((uint8_t*)s->pixels)[y*s->pitch+x*3];
}
static void fill(unsigned n,unsigned c) {
    SDL_Surface*s=gfx_get_surface(n);
    SDL_FillRect(s,NULL,SDL_MapRGB(s->format,c,c,c));
}
static void observe(void) {
    if(mode) {
        /* Independent values from the PC integer instructions, with 5-bit
         * sources 31 and 8. mode 2 tests source B aliasing the destination. */
        static const unsigned weights[]={0,1,4,9,16,25};
        static const unsigned expected[]={64,64,80,112,152,200};
        assert(calls<6);
        unsigned value=mode==1?expected[calls]:
            ((31*weights[calls]+(previous/8)*(32-weights[calls]))/32)*8;
        assert(pixel(0,9,11)==value && pixel(0,11,12)==value);
        assert(pixel(0,8,11)==24 && pixel(0,12,12)==24);
        previous=value;
    }
    calls++;
}
static struct param_list params(const unsigned*v,unsigned n) {
    struct param_list p={.nr_params=n};
    for(unsigned i=0;i<n;i++){p.params[i].type=MES_PARAM_EXPRESSION;p.params[i].val=v[i];}
    return p;
}
int main(void) {
    ai5_set_game("isaku");struct game g=game_isaku;game=&g;
    g.update=observe;g.handle_event=NULL;config.controller.enabled=false;
    g.mem_init();vm_init();vm_flag_off(FLAG_ANIM_ENABLE);gfx_init("fidelity");input_init();
    unsigned wait[]={0};struct param_list p=params(wait,1);
    uint32_t start=SDL_GetTicks();
    for(unsigned i=0;i<100;i++)g.sys[11](&p);
    assert(calls==100 && SDL_GetTicks()-start<800);
    for(unsigned i=1;i<4;i++){p.params[0].val=i;g.sys[11](&p);}
    assert(calls==103);
    input_key_event(INPUT_SHIFT,true);g.sys[11](&p);assert(calls==103);
    input_key_event(INPUT_SHIFT,false);SDL_Delay(31); /* input latch */
    /* Use non-origin rectangles to catch hardcoded full-screen copies. */
    unsigned fade[]={7,2,3,4,4,3,5,6,4,9,11,0};p=params(fade,12);
    fill(3,248);fill(4,64);fill(0,24);
    calls=0;mode=1;g.sys[10](&p);assert(calls==6);
    assert(pixel(0,9,11)==248&&pixel(0,11,12)==248&&pixel(0,0,0)==24);
    /* Ctrl must not skip; Shift must copy the final image immediately. */
    calls=0;fill(0,24);input_key_event(INPUT_CTRL,true);
    g.sys[10](&p);assert(calls==6);input_key_event(INPUT_CTRL,false);
    calls=0;fill(0,24);input_key_event(INPUT_SHIFT,true);
    g.sys[10](&p);assert(calls==0&&pixel(0,9,11)==248);
    input_key_event(INPUT_SHIFT,false);SDL_Delay(31); /* input latch */
    /* Matching source-B/destination is used by the original scripts. */
    p.params[6].val=9;p.params[7].val=11;p.params[8].val=0;
    fill(0,24);SDL_Surface *s=gfx_get_surface(0);SDL_Rect r={9,11,3,2};
    SDL_FillRect(s,&r,SDL_MapRGB(s->format,64,64,64));
    calls=0;mode=2;previous=64;g.sys[10](&p);assert(calls==6);
    assert(pixel(0,9,11)==248&&pixel(0,8,11)==24);
    mode=0;calls=0;fill(0,64);
    unsigned ending[]={7,0,0,639,479,3,0,0,0,0,0};
    p=params(ending,11);g.sys[10](&p);
    assert(calls==6&&pixel(0,639,479)==248);
    puts("PASS: Wait(0..3) message pump, Shift bypass, six PC crossfade frames, rectangles, aliases, Ctrl/Shift semantics");
}
