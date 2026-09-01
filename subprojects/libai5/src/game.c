/* Copyright (C) 2023 Nunuhara Cabbage <nunuhara@haniwa.technology>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#include <string.h>

#include "ai5/anim.h"
#include "ai5/game.h"
#include "ai5/mes.h"

enum ai5_game_id ai5_target_game = -1;

struct ai5_game ai5_games[] = {
	{ "aishimai",       GAME_AI_SHIMAI,      "愛姉妹 ～二人の果実～" },
	{ "allstars",       GAME_ALLSTARS,       "エルフオールスターズ脱衣雀" },
	{ "beyond",         GAME_BEYOND,         "ビ・ ヨンド ～黒大将に見られてる～" },
	{ "doukyuusei",     GAME_DOUKYUUSEI,     "同級生 Windows版" },
	{ "doukyuusei2",    GAME_DOUKYUUSEI2,    "同級生２" },
	{ "doukyuusei2-dl", GAME_DOUKYUUSEI2_DL, "同級生２ ＤＬ版" },
	{ "isaku",          GAME_ISAKU,          "遺作 リニューアル" },
	{ "kakyuusei",      GAME_KAKYUUSEI,      "下級生" },
	{ "kawarazakike",   GAME_KAWARAZAKIKE,   "河原崎家の一族" },
	{ "kisaku",         GAME_KISAKU,         "鬼作" },
	{ "kisaku-anim",    GAME_KISAKU_ANIM,    "鬼作 アニメーション追加完全版" },
	{ "koihime",        GAME_KOIHIME,        "恋姫" },
	{ "nonomura",       GAME_NONOMURA,       "野々村病院の人々" },
	{ "shangrlia",      GAME_SHANGRLIA,      "SHANGRLIA" },
	{ "shangrlia2",     GAME_SHANGRLIA2,     "SHANGRLIA2" },
	{ "shuusaku",       GAME_SHUUSAKU,       "臭作" },
	{ "yukinojou",      GAME_YUKINOJOU,      "あしたの雪之丞" },
	{ "yuno",           GAME_YUNO,           "この世の果てで恋を唄う少女YU-NO (エルフclassics)" },
};

enum ai5_game_id ai5_parse_game_id(const char *str)
{
	for (unsigned i = 0; i < ARRAY_SIZE(ai5_games); i++) {
		if (!strcmp(str, ai5_games[i].name))
			return ai5_games[i].id;
	}
	sys_warning("Unrecognized game name: %s\n", str);
	sys_warning("Valid names are:\n");
	for (unsigned i = 0; i < ARRAY_SIZE(ai5_games); i++) {
		sys_warning("    %-14s - %s\n", ai5_games[i].name, ai5_games[i].description);
	}
	sys_exit(EXIT_FAILURE);
}

void ai5_set_game(const char *name)
{
	ai5_target_game = ai5_parse_game_id(name);
	mes_set_game(ai5_target_game);
	anim_set_game(ai5_target_game);
}
