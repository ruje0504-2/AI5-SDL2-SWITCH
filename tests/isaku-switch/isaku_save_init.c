#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ai5.h"
#include "ai5/game.h"
#include "game.h"
#include "vm_private.h"
static void load8(void){
 struct param_list p={.nr_params=2};
 p.params[0].type=p.params[1].type=MES_PARAM_EXPRESSION;
 p.params[0].val=2;p.params[1].val=8;game->sys[4](&p);
}
static void writefile(const uint8_t*b){FILE*f=fopen("FLAG08","wb");assert(f);assert(fwrite(b,1,4096,f)==4096);fclose(f);}
int main(void){
 ai5_set_game("isaku");game=&game_isaku;game->mem_init();
 char dir[]="/tmp/isaku-save-test-XXXXXX";assert(mkdtemp(dir));assert(!chdir(dir));
 load8();
 assert(vm_flag_is_on(FLAG_VOICE_ENABLE)&&vm_flag_is_on(FLAG_AUDIO_ENABLE));
 assert(mem_get_sysvar16(mes_sysvar16_font_width)==16);
 uint8_t clean[4096],old[4096]={0},after[4096];
 FILE*f=fopen("FLAG08","rb");assert(f);assert(fread(clean,1,4096,f)==4096);fclose(f);
 assert(!memcmp(clean,"FLAGINIT.MES",12));assert(clean[0x8bc]==0x0f&&clean[0x8bd]==3);
 /* Old global file may already hold progress and persistent heap data. */
 old[128+45]=1;old[3132]=0x5a;writefile(old);load8();
 assert(vm_flag_is_on(FLAG_VOICE_ENABLE)&&vm_flag_is_on(FLAG_AUDIO_ENABLE));
 assert(memory_raw[128+45]==1&&memory_raw[3132]==0x5a);
 f=fopen("FLAG08","rb");assert(fread(after,1,4096,f)==4096);fclose(f);
 assert(!memcmp(old,after,4096));
 /* A valid saved mute setting must not be switched on by migration. */
 clean[0x8bd]=0;clean[128+45]=2;writefile(clean);load8();
 assert(!vm_flag_is_on(FLAG_VOICE_ENABLE)&&!vm_flag_is_on(FLAG_AUDIO_ENABLE));
 assert(memory_raw[128+45]==2);
 unlink("FLAG08");assert(!chdir("/tmp"));assert(!rmdir(dir));
 puts("PASS: actual SaveData.load(8), first-run audio/font initialization, legacy empty-file repair, progress preservation, no file rewrite, valid mute preserved");
}
