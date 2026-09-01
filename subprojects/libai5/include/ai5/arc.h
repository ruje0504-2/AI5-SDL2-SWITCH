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

#ifndef AI5_ARC_H
#define AI5_ARC_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nulib.h"
#include "nulib/hashtable.h"
#include "nulib/queue.h"
#include "nulib/string.h"
#include "nulib/vector.h"

declare_hashtable_string_type(arcindex, int);

enum {
	ARCHIVE_MMAP = 1,   // map archive in memory
	ARCHIVE_RAW  = 2,   // skip decompression when loading files
	ARCHIVE_CACHE = 4,  // cache loaded files
	ARCHIVE_STEREO = 8, // raw PCM is stereo (AWD/AWF archives)
};

enum archive_scheme {
	// archive index is encrypted with XOR cipher
	ARCHIVE_SCHEME_TYPICAL,
	// archive index is encrypted with game-specific cipher
	ARCHIVE_SCHEME_GAME_SPECIFIC,
};

enum archive_type {
	ARCHIVE_TYPE_ARC,
	ARCHIVE_TYPE_DAT,
	ARCHIVE_TYPE_AWD,
	ARCHIVE_TYPE_AWF,
};

struct arc_metadata {
	off_t arc_size;
	uint32_t nr_files;
	unsigned index_off;
	unsigned entry_size;
	unsigned name_length;

	uint32_t offset_key;
	uint32_t size_key;
	uint8_t name_key;

	unsigned offset_off;
	unsigned size_off;
	unsigned name_off;

	unsigned awd_type_off;
	unsigned loop_start_off;
	unsigned loop_end_off;

	enum archive_scheme scheme;
	enum archive_type type;
};

struct archive {
	hashtable_t(arcindex) index;
	TAILQ_HEAD(cache_head, archive_data) cache;
	unsigned nr_cached;
	unsigned cache_size;
	vector_t(struct archive_data) files;
	struct arc_metadata meta;
	unsigned flags;
	bool mapped;
	union {
		FILE *fp;
		struct {
			uint8_t *data;
			size_t size;
		} map;
	};
};

enum awd_file_type {
	AWD_PCM = 1,
	AWD_MP3 = 85,
};

struct awd_file_metadata {
	uint16_t type;
	uint32_t loop_start;
	uint32_t loop_end;
};

struct archive_data {
	TAILQ_ENTRY(archive_data) entry;
	uint32_t offset;
	uint32_t raw_size; // size of file in archive
	uint32_t size;     // size of data in `data` (uncompressed)
	string name;
	uint8_t *data;
	struct awd_file_metadata meta;
	unsigned int ref : 16;      // reference count
	unsigned int mapped : 1;    // true if `data` is a pointer into mmapped region
	unsigned int allocated : 1; // true if archive_data object needs to be freed
	unsigned int cached : 1;
	unsigned int reserved : 13; // reserved for future flags
	struct archive *archive;
};

/*
 * Close an archive.
 */
void archive_close(struct archive *arc)
	attr_nonnull;

/*
 * Open an archive.
 */
struct archive *archive_open(const char *path, unsigned flags)
	attr_dealloc(archive_close, 1)
	attr_nonnull;

/*
 * Release a reference to an entry. If the reference count becomes zero, the
 * loaded data is free'd.
 */
void archive_data_release(struct archive_data *data)
	attr_nonnull;

/*
 * Get an entry by name. The entry data is loaded and the caller owns a
 * reference to the entry when this function returns.
 */
struct archive_data *archive_get(struct archive *arc, const char *name)
	attr_nonnull;


struct archive_data *archive_get_by_index(struct archive *arc, unsigned i)
	attr_nonnull;

/*
 * Get the index of an entry by name.
 */
int archive_get_index(struct archive *arc, const char *name)
	attr_nonnull;

/*
 * Load data for an entry (if it is not already loaded). The caller owns a
 * reference to the entry when this function returns.
 *
 * This function is useful when iterating over the file list with
 * `archive_foreach`.
 */
bool archive_data_load(struct archive_data *data)
	attr_warn_unused_result
	attr_nonnull;

/*
 * Iterate over the list of files in an archive. The caller does NOT own a
 * reference to the entries it iterates over, and data is NOT loaded.
 *
 * The caller must use `archive_data_load` to create a reference and load
 * the file data.
 */
#define archive_foreach(var, arc) vector_foreach_p(var, (arc)->files)

#endif // AI5_ARC_H
