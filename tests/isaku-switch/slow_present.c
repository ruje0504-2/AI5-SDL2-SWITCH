#include <stdlib.h>
/* Model a blocking 60Hz display on macOS, independently of CPU/SD speed. */
#include <SDL.h>
#include <stdio.h>
static unsigned presents;
static void slow_present(SDL_Renderer*r){presents++;SDL_Delay(16);SDL_RenderPresent(r);}
__attribute__((used)) static struct {const void *replacement,*replacee;} interpose
__attribute__((section("__DATA,__interpose")))={(const void*)slow_present,(const void*)SDL_RenderPresent};
__attribute__((destructor)) static void report(void){fprintf(stderr,"Blocking present calls: %u\n",presents);}
