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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(_WIN32) || defined(__SWITCH__)
#define mmap(...) (ERROR("mmap not supported on this platform"), NULL)
#define munmap(...) (ERROR("munmap not supported on this platform"), -1)
#define MAP_FAILED ((void*)0)
#define PROT_READ 0x1
#define MAP_SHARED 0x01
#else
#include <sys/mman.h>
#endif

#include "nulib.h"
#include "nulib/file.h"
#include "nulib/hashtable.h"
#include "nulib/little_endian.h"
#include "nulib/string.h"
#include "nulib/utfsjis.h"
#include "nulib/vector.h"
#include "ai5/arc.h"
#include "ai5/lzss.h"
#include "ai5/game.h"

#define MAX_SANE_FILES 100000
#define DEFAULT_CACHE_SIZE 16

define_hashtable_string(arcindex, int);

static off_t get_file_size(FILE *fp)
{
	// get size of archive
	if (fseek(fp, 0, SEEK_END)) {
		WARNING("fseek: %s", strerror(errno));
		return 0;
	}
	off_t r = ftell(fp);
	if (fseek(fp, 0, SEEK_SET)) {
		WARNING("fseek: %s", strerror(errno));
		return 0;
	}
	return r;
}

static bool get_offset_and_size_key(
		uint32_t off_enc, uint32_t off_next_enc, uint32_t off_next2_enc,
		uint32_t size_enc, uint32_t size_next_enc, uint32_t size_next2_enc,
		uint32_t off_guess, struct arc_metadata *meta)
{
	uint32_t off_key = off_enc ^ off_guess;
	uint32_t off_next = off_next_enc ^ off_key;
	if (off_next <= off_guess)
		return false;

	uint32_t size_guess = off_next - off_guess;
	uint32_t size_key = size_enc ^ size_guess;
	uint32_t size_next = size_next_enc ^ size_key;
	if (off_guess + size_guess >= meta->arc_size || off_next + size_next >= meta->arc_size)
		return false;

	uint32_t off_next2 = off_next2_enc ^ off_key;
	uint32_t size_next2 = size_next2_enc ^ size_key;
	if (off_next + size_next != off_next2)
		return false;
	if (off_next2 + size_next2 >= meta->arc_size)
		return false;

	meta->offset_key = off_key;
	meta->size_key = size_key;
	return true;
}

static bool arc_get_size_and_count(FILE *fp, struct arc_metadata *meta)
{
	// get size of archive
	if (!(meta->arc_size = get_file_size(fp)))
		return false;

	// get number of files in archive
	uint8_t nr_files_buf[4];
	if (fread(nr_files_buf, 4, 1, fp) != 1) {
		WARNING("fread: %s", strerror(errno));
		return false;
	}
	if ((meta->nr_files = le_get32(nr_files_buf, 0)) > MAX_SANE_FILES) {
		WARNING("archive file count is not sane: %u", meta->nr_files);
		return false;
	}
	return true;
}

static bool arc_get_metadata(FILE *fp, struct arc_metadata *meta_out)
{
	struct arc_metadata meta = {
		.index_off = 4,
		.type = ARCHIVE_TYPE_ARC,
	};

	if (!arc_get_size_and_count(fp, &meta))
		return false;

	// check for game-specific cipher
	switch (ai5_target_game) {
	case GAME_DOUKYUUSEI2_DL:
		meta.entry_size = 39;
		meta.name_length = 31;
		meta.scheme = ARCHIVE_SCHEME_GAME_SPECIFIC;
		*meta_out = meta;
		return true;
	case GAME_KAKYUUSEI:
		meta.entry_size = 20;
		meta.name_length = 12;
		meta.scheme = ARCHIVE_SCHEME_GAME_SPECIFIC;
		*meta_out = meta;
		return true;
	case GAME_KISAKU_ANIM:
		meta.entry_size = 272;
		meta.name_length = 260;
		meta.scheme = ARCHIVE_SCHEME_GAME_SPECIFIC;
		*meta_out = meta;
		return true;
	default:
		break;
	}

	// read (at least) 3 entries
	uint8_t entry[0x318];
	if (fread(entry, 0x318, 1, fp) != 1) {
		WARNING("fread: %s", strerror(errno));
		return false;
	}

	// Most games use an XOR cipher where the name is XOR'd with an 8-bit
	// key and the offset and size are XOR'd with 32-bit keys.
	//
	// We can trivially determine the name key if we know the length of the
	// name field, since names are null-terminated.
	//
	// We know what the first offset should be (nr_files * entry_size),
	// which allows us to determine the offset key.
	//
	// Once we have the offset key, we can get the first size by taking the
	// difference of the first and second offsets, which allows us to
	// determine the size key.
	//
	// Some games have the offset and size fields reversed, so we have to
	// check both orders, and do a sanity-check against the third entry
	// since the reversed order can generate false-positives.
	const unsigned name_lengths[] = { 0xc, 0x10, 0x14, 0x1e, 0x20, 0x100 };
	for (int i = 0; i < ARRAY_SIZE(name_lengths); i++) {
		const unsigned len = name_lengths[i];
		uint8_t name_key = entry[len - 1];

		// check that name is valid with key
		for (int i = 0; i < len && entry[i] != name_key; i++) {
			if (!isprint(entry[i] ^ name_key))
				goto next;
		}

		uint8_t *entry2 = entry + len + 8;
		uint8_t *entry3 = entry + (len + 8) * 2;

		uint32_t fst_a = le_get32(entry, len);
		uint32_t fst_b = le_get32(entry, len + 4);
		uint32_t snd_a = le_get32(entry2, len);
		uint32_t snd_b = le_get32(entry2, len + 4);
		uint32_t thd_a = le_get32(entry3, len);
		uint32_t thd_b = le_get32(entry3, len + 4);
		uint32_t off_guess = 4 + meta.nr_files * (len + 8);

		// name / size / offset
		if (get_offset_and_size_key(fst_b, snd_b, thd_b, fst_a, snd_a, thd_a, off_guess, &meta)) {
			meta.name_length = len;
			meta.name_key = name_key;
			meta.offset_off = len + 4;
			meta.size_off = len;
			meta.name_off = 0;
			break;
		}
		// name / offset / size
		if (get_offset_and_size_key(fst_a, snd_a, thd_a, fst_b, snd_b, thd_b, off_guess, &meta)) {
			meta.name_length = len;
			meta.name_key = name_key;
			meta.offset_off = len;
			meta.size_off = len + 4;
			meta.name_off = 0;
			break;
		}
next:
	}
	if (!meta.name_length)
		return false;

	meta.entry_size = meta.name_length + 8;
	meta.scheme = ARCHIVE_SCHEME_TYPICAL;
	*meta_out = meta;
	return true;
}

static bool dat_get_metadata(FILE *fp, struct arc_metadata *meta_out)
{
	struct arc_metadata meta = {
		.index_off = 8,
		.entry_size = 28,
		.name_length = 20,
		.offset_off = 4,
		.size_off = 0,
		.name_off = 8,
		.scheme = ARCHIVE_SCHEME_TYPICAL,
		.type = ARCHIVE_TYPE_DAT,
	};

	// get size of archive
	if (!(meta.arc_size = get_file_size(fp)))
		return false;

	uint8_t buf[0x24];
	if (fread(buf, 0x24, 1, fp) != 1) {
		WARNING("fread: %s", strerror(errno));
		return false;
	}

	uint32_t key = le_get32(buf, 4);
	if ((meta.nr_files = le_get32(buf, 0) ^ key) > MAX_SANE_FILES) {
		WARNING("archive file count is not sane: %u", meta.nr_files);
		return false;
	}

	meta.offset_key = key;
	meta.size_key = key;
	meta.name_key = buf[0x23];
	*meta_out = meta;
	return true;
}

static bool awd_get_metadata(FILE *fp, struct arc_metadata *meta_out)
{
	struct arc_metadata meta = {
		.index_off = 4,
		.entry_size = 38,
		.name_length = 16,
		.name_off = 0,
		.offset_off = 18,
		.size_off = 22,
		.awd_type_off = 16,
		.loop_start_off = 26,
		.loop_end_off = 30,
		.scheme = ARCHIVE_SCHEME_TYPICAL,
		.type = ARCHIVE_TYPE_AWD,
	};

	// heuristic to detect older awd format (shuusaku cd version)
	uint8_t bytes[2];
	if (fseek(fp, 22, SEEK_SET)) {
		WARNING("fseek: %s", strerror(errno));
		return false;
	}
	if (fread(bytes, 2, 1, fp) != 1) {
		WARNING("fread: %s", strerror(errno));
		return false;
	}
	if (bytes[0] == 0 && bytes[1] == 0) {
		meta.offset_off += 2;
		meta.size_off += 2;
		meta.loop_start_off += 2;
		meta.loop_end_off += 2;
		meta.entry_size += 2;
	}

	if (!arc_get_size_and_count(fp, &meta))
		return false;
	*meta_out = meta;
	return true;
}

static bool awf_get_metadata(FILE *fp, struct arc_metadata *meta_out)
{
	struct arc_metadata meta = {
		.index_off = 4,
		.entry_size = 52,
		.name_length = 32,
		.name_off = 0,
		.offset_off = 32,
		.size_off = 36,
		.loop_start_off = 40,
		.loop_end_off = 44,
		.scheme = ARCHIVE_SCHEME_TYPICAL,
		.type = ARCHIVE_TYPE_AWF,
	};
	if (!arc_get_size_and_count(fp, &meta))
		return false;
	*meta_out = meta;
	return true;
}

static void create_index(struct archive *arc)
{
	for (int i = 0; i < vector_length(arc->files); i++) {
		// upcase filename
		char *name = vector_A(arc->files, i).name;
		for (int i = 0; name[i]; i++) {
			name[i] = toupper(name[i]);
		}

		int ret;
		hashtable_iter_t k = hashtable_put(arcindex, &arc->index,
				vector_A(arc->files, i).name,
				&ret);
		if (ret == HASHTABLE_KEY_PRESENT) {
			WARNING("skipping duplicate file name in archive");
			continue;
		}
		hashtable_val(&arc->index, k) = i;
	}
}

static bool read_index(FILE *fp, struct archive *arc,
		bool(*read_entry)(struct archive*,struct archive_data*,uint8_t*))
{
	const size_t buf_len = arc->meta.nr_files * arc->meta.entry_size;
	size_t buf_pos = 0;
	uint8_t *buf = xmalloc(buf_len);
	if (fread(buf, buf_len, 1, fp) != 1) {
		WARNING("fread: %s", strerror(errno));
		free(buf);
		return false;
	}

	// read file entries
	vector_init(arc->files);
	vector_resize(struct archive_data, arc->files, arc->meta.nr_files);
	for (int i = 0; i < arc->meta.nr_files; i++, buf_pos += arc->meta.entry_size) {
		struct archive_data *file = &vector_A(arc->files, i);
		*file = (struct archive_data){0};
		if (!read_entry(arc, file, buf + buf_pos)) {
			WARNING("Failed to read archive entry %d", i);
			vector_destroy(arc->files);
			vector_init(arc->files);
			free(buf);
			return false;
		}
		file->archive = arc;
	}

	free(buf);
	create_index(arc);
	return true;
}

static bool typical_read_entry(struct archive *arc, struct archive_data *file, uint8_t *buf)
{
	const struct arc_metadata *meta = &arc->meta;

	int name_len;
	uint8_t *name = buf + meta->name_off;

	// decode file name
	for (name_len = 0; name_len < meta->name_length; name_len++) {
		name[name_len] ^= meta->name_key;
		if (name[name_len] == 0)
			break;
	}

	file->offset = le_get32(buf, meta->offset_off) ^ meta->offset_key;
	file->raw_size = le_get32(buf, meta->size_off) ^ meta->size_key;
	// XXX: name is not necessarily null-terminated (happens in Kawa95)
	file->name = sjis_cstring_to_utf8((const char*)name, name_len);

	if (file->offset + file->raw_size > meta->arc_size) {
		WARNING("%s @ %x + %x extends beyond eof (%x)", file->name, file->offset,
				file->raw_size, (unsigned)meta->arc_size);
		string_free(file->name);
		return false;
	}

	if (arc->meta.type == ARCHIVE_TYPE_AWD) {
		file->meta.type = le_get16(buf, meta->awd_type_off);
		file->meta.loop_start = le_get32(buf, meta->loop_start_off);
		file->meta.loop_end = le_get32(buf, meta->loop_end_off);
	} else if (arc->meta.type == ARCHIVE_TYPE_AWF) {
		file->meta.type = AWD_PCM;
		file->meta.loop_start = le_get32(buf, meta->loop_start_off);
		file->meta.loop_end = le_get32(buf, meta->loop_end_off);
	}

	return true;
}

static uint8_t doukyuusei_2_dl_sbox[256] = {
	// 0
	0x63, 0x93,  0xB, 0xCD, 0x51, 0x8A, 0x60, 0xC5,
	0xB0, 0xF0, 0x26, 0xF6, 0xA5, 0x3D, 0x34, 0x9E,
	0x84, 0xFB, 0x1D, 0xDA, 0x62, 0xD1, 0xDC, 0xE1,
	0x24, 0x7C, 0xA3, 0x95, 0x48, 0xA1, 0x3B, 0xD9,
	// 32
	0x41, 0x1C, 0xEA, 0x90, 0xA9, 0xCE,  0x1, 0xF1,
	0x45, 0xFF, 0x92, 0x1F, 0x61, 0x50, 0x2F, 0xF5,
	0x8C, 0x85, 0x87, 0x71, 0x66, 0x8E, 0x17, 0x59,
	0x9C, 0x91, 0x79, 0xEB, 0xF2, 0x68, 0x69, 0x7F,
	// 64
	0x52, 0x42, 0xB7, 0xED, 0x4F, 0x14, 0x35, 0x94,
	0xAD, 0x4B, 0xCA, 0x4C, 0xA2, 0xD3, 0xD5,  0x9,
	0x64, 0x19, 0x5D, 0x27, 0x76, 0x31, 0x22, 0xD4,
	0xBB, 0xA6,  0xF,  0xD, 0x56, 0xDD, 0x80, 0x13,
	// 96
	0xBF, 0x72, 0x5B, 0xA4, 0x70, 0xF3, 0x4E, 0x53,
	0xB9, 0xC0, 0x5C, 0xFE, 0x55, 0xA8, 0xE3,  0x7,
	0xE,   0x0, 0x7E, 0xEF, 0x44, 0x20, 0x9F, 0xBE,
	0x3C,  0xA, 0x2E, 0xC7, 0x28, 0xB8, 0xE0, 0x33,
	// 128
	0x3E, 0xD8, 0x7D, 0x32, 0x11, 0x6F, 0xB2, 0x67,
	0x2C, 0x40, 0x1A, 0xE6, 0x8F, 0xB6, 0x49, 0x1B,
	0x46, 0x6C, 0xE2, 0xDB, 0x75, 0x77, 0x6E, 0x2D,
	0x89, 0xE8, 0x96, 0xA7, 0x97, 0x99, 0xE9, 0x47,
	// 160
	0x81, 0xD6, 0xFA, 0x36, 0xC3, 0xDE, 0x6D, 0xE4,
	0x5E, 0x58,  0x2, 0xE5, 0x18, 0xCF, 0xCC, 0x65,
	0xAB,  0x4, 0xF7, 0x54, 0x78, 0x30, 0x5A, 0xB3,
	0xA0,  0xC,  0x6, 0xD0, 0xFC, 0xC6,  0x3, 0xD7,
	// 192
	0x74, 0x3A, 0xBD, 0xB5, 0xC8, 0xB1, 0x6B, 0x6A,
	0x2B, 0x43, 0xC1, 0x8D, 0x12, 0x15, 0x8B, 0x88,
	0xC4, 0xBA, 0xCB, 0xDF, 0x3F, 0x38, 0x73, 0xF4,
	0x98, 0x23, 0x9D, 0x10, 0xD2, 0xAF, 0xEC, 0x7B,
	// 224
	0x1E, 0xF8, 0xB4, 0xC2, 0xF9, 0x82, 0x29, 0xEE,
	0x9B, 0x2A, 0x5F, 0xBC, 0x4D, 0x16, 0xFD, 0x9A,
	0x4A, 0xC9, 0xE7, 0x57, 0x21, 0x83,  0x5, 0x25,
	0xAE, 0x39, 0x7A,  0x8, 0xAC, 0x86, 0x37, 0xAA
};

static bool doukyuusei_2_dl_read_entry(struct archive *arc, struct archive_data *file,
		uint8_t *entry)
{
	// decode entry
	for (int i = 0; i < 39; i++) {
		uint8_t sbox_i = (uint8_t)41 - entry[i];
		entry[i] = doukyuusei_2_dl_sbox[sbox_i];
	}
	if (entry[38]) {
		WARNING("name is not null-terminated");
		return false;
	}

	file->offset = le_get32(entry, 0);
	file->raw_size = le_get32(entry, 4);
	file->name = sjis_cstring_to_utf8((char*)entry + 8, 0);
	return true;
}

static bool kakyuusei_read_index(FILE *fp, struct archive *arc)
{
	// encrypted bytes of index are out of order
	static uint8_t shuffle_table[20] = {
		17, 2, 8, 19, 0, 5, 10, 13, 1, 15, 6, 4, 11, 16, 3, 9, 18, 12, 7, 14
	};

	const size_t buf_len = arc->meta.nr_files * arc->meta.entry_size;
	size_t buf_pos = 0;
	uint8_t *buf = xmalloc(buf_len);
	if (fread(buf, buf_len, 1, fp) != 1) {
		WARNING("fread: %s", strerror(errno));
		free(buf);
		return false;
	}

	// read file entries
	uint8_t dec[20];
	uint8_t key = arc->meta.nr_files;
	vector_init(arc->files);
	vector_resize(struct archive_data, arc->files, arc->meta.nr_files);
	for (int i = 0; i < arc->meta.nr_files; i++, buf_pos += 20) {
		struct archive_data *file = &vector_A(arc->files, i);
		*file = (struct archive_data){0};
		for (int i = 0; i < 20; i++) {
			dec[shuffle_table[i]] = buf[buf_pos + i] ^ key;
			key = ((int)key * 3 + 1) & 0xff;
		}
		file->offset = le_get32(dec, 16);
		file->raw_size = le_get32(dec, 12);
		dec[12] = '\0';
		file->name = sjis_cstring_to_utf8((char*)dec, 0);
		file->archive = arc;
	}

	free(buf);
	create_index(arc);
	return true;
}

static bool kisaku_anim_read_index(FILE *fp, struct archive *arc)
{
	const size_t buf_len = arc->meta.nr_files * arc->meta.entry_size;
	size_t buf_pos = 0;
	uint8_t *buf = xmalloc(buf_len);
	if (fread(buf, buf_len, 1, fp) != 1) {
		WARNING("fread: %s", strerror(errno));
		free(buf);
		return false;
	}

	// read file entries
	uint8_t dec[260];
	vector_init(arc->files);
	vector_resize(struct archive_data, arc->files, arc->meta.nr_files);
	for (int i = 0; i < arc->meta.nr_files; i++, buf_pos += 272) {
		int name_len = strnlen((char*)buf + buf_pos, 260);
		for (int i = 0, key = name_len + 1; i < name_len; i++, key--) {
			dec[i] = buf[buf_pos + i] - key;
		}
		dec[name_len] = '\0';
		struct archive_data *file = &vector_A(arc->files, i);
		*file = (struct archive_data) {
			.raw_size = be_get32(buf, buf_pos + 260),
			.offset = be_get32(buf, buf_pos + 268),
			.name = sjis_cstring_to_utf8((char*)dec, 0),
			.archive = arc
		};
	}

	free(buf);
	create_index(arc);
	return true;
}

static bool arc_read_index(FILE *fp, struct archive *arc)
{
	if (fseek(fp, arc->meta.index_off, SEEK_SET)) {
		WARNING("fseek: %s", strerror(errno));
		return false;
	}

	if (arc->meta.scheme == ARCHIVE_SCHEME_GAME_SPECIFIC) {
		switch (ai5_target_game) {
		case GAME_DOUKYUUSEI2_DL:
			return read_index(fp, arc, doukyuusei_2_dl_read_entry);
		case GAME_KAKYUUSEI:
			return kakyuusei_read_index(fp, arc);
		case GAME_KISAKU_ANIM:
			return kisaku_anim_read_index(fp, arc);
		default:
			WARNING("Game-specific archive type but no game specified");
			return false;
		}
	}

	return read_index(fp, arc, typical_read_entry);
}

struct archive *archive_open(const char *path, unsigned flags)
{
#ifdef _WIN32
	flags &= ~ARCHIVE_MMAP;
#endif
	FILE *fp = NULL;
	struct archive *arc = xcalloc(1, sizeof(struct archive));
	TAILQ_INIT(&arc->cache);
	if (flags & ARCHIVE_CACHE)
		arc->cache_size = DEFAULT_CACHE_SIZE;

	// open archive file
	if (!(fp = file_open_utf8(path, "rb"))) {
		WARNING("file_open_utf8: %s", strerror(errno));
		goto error;
	}

	bool meta_ok;
	const char *ext = file_extension(path);
	if (!strcasecmp(ext, "dat")) {
		meta_ok = dat_get_metadata(fp, &arc->meta);
	} else if (!strcasecmp(ext, "awd")) {
		meta_ok = awd_get_metadata(fp, &arc->meta);
	} else if (!strcasecmp(ext, "awf")) {
		meta_ok = awf_get_metadata(fp, &arc->meta);
	} else {
		meta_ok = arc_get_metadata(fp, &arc->meta);
	}
	if (!meta_ok) {
		WARNING("Failed to read archive metadata");
		goto error;
	}
	if (!arc_read_index(fp, arc))
		goto error;

	// store either mmap ptr/size or FILE* depending on flags
	if (flags & ARCHIVE_MMAP) {
		int fd = fileno(fp);
		if (fd == -1) {
			WARNING("fileno: %s", strerror(errno));
			goto error;
		}
		arc->map.data = mmap(0, arc->meta.arc_size, PROT_READ, MAP_SHARED, fd, 0);
		arc->map.size = arc->meta.arc_size;
		if (arc->map.data == MAP_FAILED) {
			WARNING("mmap: %s", strerror(errno));
			goto error;
		}
		if (fclose(fp)) {
			WARNING("fclose: %s", strerror(errno));
			goto error;
		}
		arc->mapped = true;
	} else {
		arc->fp = fp;
	}

	arc->flags = flags;
	return arc;
error:
	vector_destroy(arc->files);
	free(arc);
	if (fp && fclose(fp))
		WARNING("fclose: %s", strerror(errno));
	return NULL;
}

void archive_close(struct archive *arc)
{
	if (arc->mapped) {
		if (munmap(arc->map.data, arc->map.size))
			WARNING("munmap: %s", strerror(errno));
	} else {
		if (fclose(arc->fp))
			WARNING("fclose: %s", strerror(errno));
	}
	for (unsigned i = 0; i < vector_length(arc->files); i++) {
		string_free(vector_A(arc->files, i).name);
	}
	vector_destroy(arc->files);
	hashtable_destroy(arcindex, &arc->index);
	free(arc);
}

static uint8_t *pack_wav(uint8_t *data_in, size_t size_in, size_t *size_out, bool stereo)
{
	uint8_t *data = xmalloc(size_in + 44);
	// master RIFF chunk
	memcpy(data, "RIFF", 4);
	le_put32(data, 4, size_in + 36);
	memcpy(data + 8, "WAVE", 4);
	// chunk describing the data format
	memcpy(data + 12, "fmt ", 4);
	le_put32(data, 16, 0x10);
	le_put16(data, 20, 1); // format (PCM integer)
	if (stereo) {
		le_put16(data, 22, 2);      // channels
		le_put32(data, 24, 44100);  // frequency
		le_put32(data, 28, 176400); // bytes per second
		le_put16(data, 32, 4);      // bytes per block
	} else {
		le_put16(data, 22, 1);     // channels
		le_put32(data, 24, 44100); // frequency
		le_put32(data, 28, 88200); // bytes per second
		le_put16(data, 32, 2);     // bytes per block
	}
	le_put16(data, 34, 16); // bits per sample
	// chunk containing the sampled data
	memcpy(data + 36, "data", 4);
	le_put32(data, 40, size_in);
	memcpy(data + 44, data_in, size_in);
	*size_out = size_in + 44;
	return data;
}

/*
 * Decompress compressed file types.
 */
static bool data_decompress(struct archive_data *file)
{
	uint8_t *data;
	size_t data_size;

	if (file->archive->meta.type == ARCHIVE_TYPE_AWD
			|| file->archive->meta.type == ARCHIVE_TYPE_AWF) {
		if (file->meta.type == AWD_PCM) {
			// raw s16le PCM data: convert to WAV
			bool stereo = file->archive->flags & ARCHIVE_STEREO;
			data = pack_wav(file->data, file->raw_size, &data_size, stereo);
		} else {
			// mp3: do nothing
			if (file->meta.type != AWD_MP3)
				WARNING("Unknown AWD file type: %u", file->meta.type);
			return true;
		}
	} else if (file->archive->flags & ARCHIVE_RAW) {
		return true;
	} else if (game_is_aiwin()) {
		// LZSS compressed (bitwise): decompress
		data = lzss_bw_decompress(file->data, file->raw_size, &data_size);
	} else {
		// LZSS compressed: decompress
		data = lzss_decompress(file->data, file->raw_size, &data_size);
	}

	if (!file->mapped)
		free(file->data);
	file->mapped = false;
	if (!data) {
		WARNING("lzss_decompress failed");
		file->data = NULL;
		file->size = 0;
		return false;
	}
	file->data = data;
	file->size = data_size;
	return true;
}

void archive_set_cache_size(struct archive *arc, unsigned cache_size)
{
	if (cache_size)
		arc->flags |= ARCHIVE_CACHE;
	else
		arc->flags &= ~ARCHIVE_CACHE;

	arc->cache_size = cache_size;
	while (arc->nr_cached > cache_size) {
		struct archive_data *evicted = TAILQ_LAST(&arc->cache, cache_head);
		TAILQ_REMOVE(&arc->cache, evicted, entry);
		evicted->cached = 0;
		archive_data_release(evicted);
		arc->nr_cached--;
	}
}

static void archive_cache_add(struct archive_data *data)
{
	struct archive *arc = data->archive;
	if (!arc || !(arc->flags & ARCHIVE_CACHE))
		return;

	// already cached: move to font
	if (data->cached) {
		assert(data->ref);
		if (data != TAILQ_FIRST(&arc->cache)) {
			TAILQ_REMOVE(&arc->cache, data, entry);
			TAILQ_INSERT_HEAD(&arc->cache, data, entry);
		}
		return;
	}

	// evict least recently used file
	if (arc->nr_cached == arc->cache_size) {
		struct archive_data *evicted = TAILQ_LAST(&arc->cache, cache_head);
		TAILQ_REMOVE(&arc->cache, evicted, entry);
		evicted->cached = 0;
		archive_data_release(evicted);
	} else {
		arc->nr_cached++;
	}

	// add to front of cache
	TAILQ_INSERT_HEAD(&arc->cache, data, entry);
	data->cached = 1;
	data->ref++;
}

bool archive_data_load(struct archive_data *data)
{
	// data already loaded by another caller
	if (data->ref) {
		archive_cache_add(data);
		data->ref++;
		return true;
	}
	assert(!data->cached);
	archive_cache_add(data);

	assert(!data->data);

	// load data
	if (data->archive->mapped) {
		data->data = data->archive->map.data + data->offset;
		data->size = data->raw_size;
		data->mapped = true;
	} else {
		if (fseek(data->archive->fp, data->offset, SEEK_SET)) {
			WARNING("fseek: %s", strerror(errno));
			return false;
		}
		data->data = xmalloc(data->raw_size);
		if (fread(data->data, data->raw_size, 1, data->archive->fp) != 1) {
			WARNING("fread: %s", strerror(errno));
			free(data->data);
			return false;
		}
		data->size = data->raw_size;
	}

	if (!data_decompress(data))
		return false;

	data->ref++;
	return true;
}

int archive_get_index(struct archive *arc, const char *name)
{
	// convert name to uppercase
	int i;
	char upname[256];
	for (i = 0; name[i] && i < 255; i++) {
		upname[i] = toupper(name[i]);
	}
	upname[i] = 0;

	hashtable_iter_t k = hashtable_get(arcindex, &arc->index, upname);
	if (k == hashtable_end(&arc->index))
		return -1;
	return hashtable_val(&arc->index, k);
}

struct archive_data *archive_get(struct archive *arc, const char *name)
{
	int i = archive_get_index(arc, name);
	if (i < 0)
		return NULL;

	// XXX: Because we're giving out pointers into arc->files, the vector must
	//      never be resized.
	struct archive_data *data = &vector_A(arc->files, i);
	if (archive_data_load(data))
		return data;
	return NULL;
}

struct archive_data *archive_get_by_index(struct archive *arc, unsigned i)
{
	if (i >= vector_length(arc->files))
		return NULL;

	struct archive_data *data = &vector_A(arc->files, i);
	if (archive_data_load(data))
		return data;
	return NULL;
}

void archive_data_release(struct archive_data *data)
{
	if (data->ref == 0)
		ERROR("double-free of archive data");
	if (--data->ref == 0) {
		if (!data->mapped)
			free(data->data);
		data->data = NULL;
		data->size = 0;
		if (data->allocated)
			free(data);
	}
}
