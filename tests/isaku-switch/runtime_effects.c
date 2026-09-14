#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include "cursor.h"
#include "popup_menu.h"
#include "ai5.h"
#include "ai5/game.h"
#include "game.h"
#include "gfx_private.h"
#include "input.h"
#include "vm_private.h"
#include "isaku_effects.h"
static struct param_list params(const unsigned *v, unsigned n) {
    struct param_list p={.nr_params=n};
    for(unsigned i=0;i<n;i++){p.params[i].type=MES_PARAM_EXPRESSION;p.params[i].val=v[i];}
    return p;
}
static void fill(unsigned n,unsigned c){SDL_Surface*s=gfx_get_surface(n);SDL_FillRect(s,NULL,SDL_MapRGB(s->format,c,c,c));}
static unsigned pixel(unsigned n,int x,int y){SDL_Surface*s=gfx_get_surface(n);return ((uint8_t*)s->pixels)[y*s->pitch+x*3];}
static int popup_clicked;
static void clicked(void *unused) { (void)unused;popup_clicked=1; }
static uint32_t confirm_popup(uint32_t interval,void *unused) {
    (void)interval;(void)unused;SDL_Event e={0};e.type=SDL_KEYDOWN;
    static int phase;
    e.key.windowID=gfx.window_id;e.key.keysym.sym=SDLK_RETURN;e.key.state=phase?SDL_RELEASED:SDL_PRESSED;
    e.type=phase?SDL_KEYUP:SDL_KEYDOWN;SDL_PushEvent(&e);return phase++?0:100;
}
int main(void) {
    ai5_set_game("isaku");
    struct game g=game_isaku;game=&g;g.update=NULL;g.handle_event=NULL;
    config.controller.enabled=false;g.mem_init();vm_init();vm_flag_off(FLAG_ANIM_ENABLE);
    gfx_init("Isaku effects test");input_init();
    for(unsigned i=0;i<INPUT_NR_INPUTS;i++)assert(!input_down(i));
    gfx_text_init(getenv("ISAKU_TEST_FONT"),-1);input_init();
    fill(1,0);SDL_Surface*s=gfx_get_surface(1);
    SDL_Rect column={254,0,1,480};SDL_FillRect(s,&column,SDL_MapRGB(s->format,248,248,248));
    unsigned scroll[]={6,0,0,640,480,1,0,0,0,254};struct param_list p=params(scroll,10);g.util[6](&p);
    assert(pixel(0,0,0)==248); /* catches missing exact endpoint (last step is 252) */
    fill(3,248);fill(4,0);
    unsigned fade[]={8,0,0,640,480,3,0,0,4,0,0,0,16,17};p=params(fade,14);g.util[8](&p);
    assert(pixel(0,0,0)==128&&pixel(0,639,479)==128);
    fill(0,24);fill(1,248);fill(3,64);
    unsigned credits[]={13,32};p=params(credits,2);
    input_key_event(INPUT_SHIFT,true);g.util[13](&p);input_key_event(INPUT_SHIFT,false);
    assert(pixel(0,140,288)==isaku_blend_channel(248,64,30));
    assert(pixel(0,140,319)==64);
    assert(pixel(0,139,288)==24&&pixel(0,500,288)==24&&pixel(0,140,159)==24);
    /* Exercise all native textures on the real software renderer. */
    assert(chdir(getenv("ISAKU_TEST_DATA"))==0);
    cursor_init("AI5WIN.EXE");cursor_show();cursor_set_pos(320,240);
    unsigned px,py;cursor_get_pos(&px,&py);
    SDL_Surface *bg=gfx.display;SDL_FillRect(bg,NULL,SDL_MapRGB(bg->format,64,64,64));
    FILE*f=fopen("isaku-cursors.bin","rb");assert(f);assert(!fseek(f,12,SEEK_SET));
    uint8_t record[4104];SDL_Surface *out=SDL_CreateRGBSurfaceWithFormat(0,640,480,32,SDL_PIXELFORMAT_RGBA32);assert(out);
    for(unsigned id=0;id<54;id++) {
        assert(fread(record,1,sizeof(record),f)==sizeof(record));
        unsigned hx=record[2]|record[3]<<8,hy=record[4]|record[5]<<8;
        cursor_load(id,1,NULL);SDL_SetRenderDrawColor(gfx.renderer,64,64,64,255);SDL_RenderClear(gfx.renderer);cursor_draw();
        assert(!SDL_RenderReadPixels(gfx.renderer,NULL,SDL_PIXELFORMAT_RGBA32,out->pixels,out->pitch));
        for(int y=0;y<32;y++)for(int x=0;x<32;x++) {
            int sx=(int)px-(int)hx+x,sy=(int)py-(int)hy+y;
            if(sx<0||sy<0||sx>=640||sy>=480)continue;
            uint8_t*m=record+8+(y*32+x)*4,*d=(uint8_t*)out->pixels+sy*out->pitch+sx*4;
            for(int c=0;c<3;c++)assert(d[c]==((64&m[3])^m[2-c]));
        }
    }
    fclose(f);SDL_FreeSurface(out);
    struct menu *m=popup_menu_new();popup_menu_append_entry(m,-1,"Confirm",NULL,clicked,NULL);
    SDL_AddTimer(100,confirm_popup,NULL);popup_menu_run(m,0,0);popup_menu_free(m);
    assert(popup_clicked);input_key_event(INPUT_ACTIVATE,false);
    puts("PASS: single-window Switch popup selection and callback");
    puts("PASS: 54 native cursor textures, hotspots, clipping and AND/XOR compositing");
    puts("PASS: actual VM utilities: scroll endpoint, fade weights, credits geometry and clipping");
}
