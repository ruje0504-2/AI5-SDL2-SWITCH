/* Switch HOS integration for the ai5-sdl2 Nintendo Switch port.
 *
 * Full-NSP builds embed the read-only game data in the title's RomFS and
 * keep save files in HOS-managed SaveData.  This module mounts both and
 * tells the rest of the engine where to look.
 *
 * Everything in here is compiled only when __SWITCH__ is defined, so it has
 * no effect on any other platform or game.
 */
#ifndef AI5_SWITCH_HOS_H
#define AI5_SWITCH_HOS_H

/* Non-zero when the title's RomFS was mounted successfully and the current
 * working directory points into it (full-NSP runtime). */
extern int ai5_switch_romfs_active;

/* Non-zero when HOS SaveData was mounted as "save:/" (full-NSP runtime).
 * When zero, savedata.c keeps its original behaviour: on non-Switch builds
 * and homebrew NROs saves go to the cwd; on a full NSP (read-only RomFS)
 * saves are silently disabled rather than touching the RomFS. */
extern int ai5_switch_save_active;

/* Mount the title RomFS (if any) and the HOS SaveData, then chdir into the
 * data directory (RomFS when available, otherwise the first existing
 * /switch/... directory, matching the old SD-card behaviour).  Call this
 * before any game-data or save file is opened. */
void ai5_switch_storage_init(void);

#endif /* AI5_SWITCH_HOS_H */
