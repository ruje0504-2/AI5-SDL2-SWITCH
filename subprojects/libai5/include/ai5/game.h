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

#ifndef AI5_GAME_H
#define AI5_GAME_H

#include <stdbool.h>

enum ai5_game_id {
	GAME_KISAKU_ANIM,    // 2011-04-28 release (animated complete edition)
	GAME_DOUKYUUSEI2_DL, // 2007-07-06 release
	GAME_KAWARAZAKIKE,   // 2003-10-24 release (AIWIN engine)
	GAME_NONOMURA,       // 2003-10-24 release
	GAME_KISAKU,         // 2001-09-28 release (DVD-ROM edition)
	GAME_YUKINOJOU,      // 2001-08-31 release
	GAME_YUNO,           // 2000-12-22 release (elf classics)
	GAME_SHANGRLIA,      // 2000-12-22 release (elf classics)
	GAME_SHANGRLIA2,     // 2000-12-22 release (elf classics)
	GAME_BEYOND,         // 2000-07-19 release
	GAME_AI_SHIMAI,      // 2000-05-26 release (windows 98/2k version)
	GAME_ALLSTARS,       // 2000-03-30 release
	GAME_KOIHIME,        // 1999-12-24 release
	GAME_DOUKYUUSEI,     // 1999-08-27 release (windows edition)
	GAME_ISAKU,          // 1999-02-26 release (renewal version)
	GAME_KAKYUUSEI,      // 1998-06-26 release
	GAME_SHUUSAKU,       // 1998-03-27/2001-10-26 release (AIWIN engine)
	GAME_DOUKYUUSEI2,    // 1997-08-29 release (windows CD version)
};
#define AI5_NR_GAME_IDS (GAME_DOUKYUUSEI2+1)

extern enum ai5_game_id ai5_target_game;

struct ai5_game {
	const char *name;
	enum ai5_game_id id;
	const char *description;
};

static inline bool game_is_aiwin(void)
{
	switch (ai5_target_game) {
	case GAME_KAWARAZAKIKE:
	case GAME_SHUUSAKU:
	case GAME_KISAKU:
		return true;
	default:
		return false;
	}
}

extern struct ai5_game ai5_games[AI5_NR_GAME_IDS];

enum ai5_game_id ai5_parse_game_id(const char *str);
void ai5_set_game(const char *name);

#endif // AI5_GAME_H
