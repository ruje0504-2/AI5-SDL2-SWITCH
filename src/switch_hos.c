/*
 * Switch HOS integration for the ai5-sdl2 Nintendo Switch port.
 *
 * Two runtime layouts are supported:
 *
 *  1. Full NSP (application title): read-only game data lives in the title's
 *     RomFS and save files live in HOS SaveData mounted at "save:/".
 *
 *  2. Homebrew NRO launched from hbmenu: game data lives on the SD card under
 *     /switch/syuusaku (or friends) and save files are written next to the
 *     data (legacy behaviour).  No RomFS/SaveData is available in that
 *     context, so everything falls back to plain file I/O in the cwd.
 *
 * The engine reads game data through relative paths after chdir(), so for the
 * NSP layout we chdir("romfs:/") and let the RomFS devoptab serve stdio.
 *
 * This file is compiled only when __SWITCH__ is defined; it has no effect on
 * any other platform or game.
 */

#ifdef __SWITCH__

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <switch.h>

#include "switch_hos.h"

int ai5_switch_romfs_active = 0;
int ai5_switch_save_active = 0;

/* ---- HOS SaveData ------------------------------------------------------ */

/*
 * Mount the current title's SaveData as "save:/".
 *
 * Homebrew NSPs running as real application titles get their own per-user
 * SaveData from HOS, exactly like commercial games.  The account is the
 * preselected user; when no account is available we fall back to the common
 * (all-zero uid) space.  If mounting fails we return an error and the engine
 * runs with saves disabled (never writing to the read-only RomFS).
 */
static Result mount_savedata_device(void)
{
	FsFileSystem fs;
	Result rc;

	/* Determine the user to save under (all-zero uid = common save data). */
	AccountUid uid = {0};
	rc = accountInitialize(AccountServiceType_Application);
	if (R_SUCCEEDED(rc)) {
		rc = accountGetPreselectedUser(&uid);
		accountExit();
	}
	if (R_FAILED(rc))
		uid = (AccountUid){0};

	rc = fsOpen_SaveData(&fs, FS_SAVEDATA_CURRENT_APPLICATIONID, uid);
	if (R_FAILED(rc))
		return rc;

	if (fsdevMountDevice("save", fs) == -1)
		return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
	return 0;
}

/* ---- Data directory ---------------------------------------------------- */

static bool dir_exists(const char *path)
{
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/*
 * Point the cwd at the game data.  Full NSP: the title's RomFS.  Otherwise
 * fall back to the legacy SD card search used by the homebrew NRO build.
 */
static void select_data_dir(void)
{
	Result rc = romfsMountSelf("romfs");
	if (R_SUCCEEDED(rc) && chdir("romfs:/") == 0) {
		ai5_switch_romfs_active = 1;
		return;
	}

	static const char *sd_dirs[] = {
		"/switch/syuusaku",
		"/switch/ai5/syuusaku",
		"/switch/ai5-sdl2/syuusaku",
		NULL
	};
	for (int i = 0; sd_dirs[i]; i++) {
		if (dir_exists(sd_dirs[i]) && chdir(sd_dirs[i]) == 0) {
			ai5_switch_romfs_active = 0;
			return;
		}
	}
}

void ai5_switch_storage_init(void)
{
	ai5_switch_romfs_active = 0;
	ai5_switch_save_active = 0;

	/* SD card access (needed by the legacy layout, harmless otherwise). */
	fsdevMountSdmc();

	select_data_dir();

	if (R_SUCCEEDED(mount_savedata_device()))
		ai5_switch_save_active = 1;
}

#endif /* __SWITCH__ */
