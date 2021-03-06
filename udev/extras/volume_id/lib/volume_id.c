/*
 * volume_id - reads volume label and uuid
 *
 * Copyright (C) 2005-2007 Kay Sievers <kay.sievers@vrfy.org>
 *
 *	This program is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU General Public License as published by the
 *	Free Software Foundation version 2 of the License.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "libvolume_id.h"
#include "util.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

struct prober {
	volume_id_probe_fn_t prober;
	const char *name[4];
};

static const struct prober prober_raid[] = {
	{ volume_id_probe_linux_raid, { "linux_raid", } },
	{ volume_id_probe_ddf_raid, { "ddf_raid", } },
	{ volume_id_probe_intel_software_raid, { "isw_raid", } },
	{ volume_id_probe_lsi_mega_raid, { "lsi_mega_raid", } },
	{ volume_id_probe_via_raid, { "via_raid", } },
	{ volume_id_probe_silicon_medley_raid, { "silicon_medley_raid", } },
	{ volume_id_probe_nvidia_raid, { "nvidia_raid", } },
	{ volume_id_probe_promise_fasttrack_raid, { "promise_fasttrack_raid", } },
	{ volume_id_probe_highpoint_45x_raid, { "highpoint_raid", } },
	{ volume_id_probe_adaptec_raid, { "adaptec_raid", } },
	{ volume_id_probe_jmicron_raid, { "jmicron_raid", } },
	{ volume_id_probe_lvm1, { "lvm1", } },
	{ volume_id_probe_lvm2, { "lvm2", } },
	{ volume_id_probe_highpoint_37x_raid, { "highpoint_raid", } },
};

static const struct prober prober_filesystem[] = {
	{ volume_id_probe_vfat, { "vfat", } },
	{ volume_id_probe_linux_swap, { "swap", } },
	{ volume_id_probe_luks, { "luks", } },
	{ volume_id_probe_xfs, { "xfs", } },
	{ volume_id_probe_ext, { "ext2", "ext3", "jbd", } },
	{ volume_id_probe_reiserfs, { "reiserfs", "reiser4", } },
	{ volume_id_probe_jfs, { "jfs", } },
	{ volume_id_probe_udf, { "udf", } },
	{ volume_id_probe_iso9660, { "iso9660", } },
	{ volume_id_probe_hfs_hfsplus, { "hfs", "hfsplus", } },
	{ volume_id_probe_ufs, { "ufs", } },
	{ volume_id_probe_ntfs, { "ntfs", } },
	{ volume_id_probe_cramfs, { "cramfs", } },
	{ volume_id_probe_romfs, { "romfs", } },
	{ volume_id_probe_hpfs, { "hpfs", } },
	{ volume_id_probe_sysv, { "sysv", "xenix", } },
	{ volume_id_probe_minix, { "minix",  } },
	{ volume_id_probe_ocfs1, { "ocfs1", } },
	{ volume_id_probe_ocfs2, { "ocfs2", } },
	{ volume_id_probe_vxfs, { "vxfs", } },
	{ volume_id_probe_squashfs, { "squashfs", } },
	{ volume_id_probe_netware, { "netware", } },
};

/* the user can overwrite this log function */
static void default_log(int priority, const char *file, int line, const char *format, ...)
{
	return;
}

volume_id_log_fn_t volume_id_log_fn = default_log;

const volume_id_probe_fn_t *volume_id_get_prober_by_type(const char *type)
{
	unsigned int p, n;

	if (type == NULL)
		return NULL;

	for (p = 0; p < ARRAY_SIZE(prober_raid); p++)
		for (n = 0; prober_raid[p].name[n] !=  NULL; n++)
			if (strcmp(type, prober_raid[p].name[n]) == 0)
				return &prober_raid[p].prober;
	for (p = 0; p < ARRAY_SIZE(prober_filesystem); p++)
		for (n = 0; prober_filesystem[p].name[n] !=  NULL; n++)
			if (strcmp(type, prober_filesystem[p].name[n]) == 0)
				return &prober_filesystem[p].prober;
	return NULL;
}

int volume_id_get_label(struct volume_id *id, const char **label)
{
	if (id == NULL)
		return -EINVAL;
	if (label == NULL)
		return -EINVAL;
	if (id->usage_id == VOLUME_ID_UNUSED)
		return 0;

	*label = id->label;
	return 1;
}

int volume_id_get_label_raw(struct volume_id *id, const uint8_t **label, size_t *len)
{
	if (id == NULL)
		return -EINVAL;
	if (label == NULL)
		return -EINVAL;
	if (len == NULL)
		return -EINVAL;
	if (id->usage_id == VOLUME_ID_UNUSED)
		return 0;

	*label = id->label_raw;
	*len = id->label_raw_len;
	return 1;
}

int volume_id_get_uuid(struct volume_id *id, const char **uuid)
{
	if (id == NULL)
		return -EINVAL;
	if (uuid == NULL)
		return -EINVAL;
	if (id->usage_id == VOLUME_ID_UNUSED)
		return 0;

	*uuid = id->uuid;
	return 1;
}

int volume_id_get_uuid_raw(struct volume_id *id, const uint8_t **uuid, size_t *len)
{
	if (id == NULL)
		return -EINVAL;
	if (uuid == NULL)
		return -EINVAL;
	if (len == NULL)
		return -EINVAL;
	if (id->usage_id == VOLUME_ID_UNUSED)
		return 0;

	*uuid = id->uuid_raw;
	*len = id->uuid_raw_len;
	return 1;
}

int volume_id_get_usage(struct volume_id *id, const char **usage)
{
	if (id == NULL)
		return -EINVAL;
	if (usage == NULL)
		return -EINVAL;
	if (id->usage_id == VOLUME_ID_UNUSED)
		return 0;

	*usage = id->usage;
	return 1;
}

int volume_id_get_type(struct volume_id *id, const char **type)
{
	if (id == NULL)
		return -EINVAL;
	if (type == NULL)
		return -EINVAL;
	if (id->usage_id == VOLUME_ID_UNUSED)
		return 0;

	*type = id->type;
	return 1;
}

int volume_id_get_type_version(struct volume_id *id, const char **type_version)
{
	if (id == NULL)
		return -EINVAL;
	if (type_version == NULL)
		return -EINVAL;
	if (id->usage_id == VOLUME_ID_UNUSED)
		return 0;

	*type_version = id->type_version;
	return 1;
}

int volume_id_probe_raid(struct volume_id *id, uint64_t off, uint64_t size)
{
	unsigned int i;

	if (id == NULL)
		return -EINVAL;

	info("probing at offset 0x%llx, size 0x%llx",
	    (unsigned long long) off, (unsigned long long) size);

	for (i = 0; i < ARRAY_SIZE(prober_raid); i++)
		if (prober_raid[i].prober(id, off, size) == 0)
			goto found;
	return -1;

found:
	/* If recognized, we free the allocated buffers */
	volume_id_free_buffer(id);
	return 0;
}

int volume_id_probe_filesystem(struct volume_id *id, uint64_t off, uint64_t size)
{
	unsigned int i;

	if (id == NULL)
		return -EINVAL;

	info("probing at offset 0x%llx, size 0x%llx",
	    (unsigned long long) off, (unsigned long long) size);

	for (i = 0; i < ARRAY_SIZE(prober_filesystem); i++)
		if (prober_filesystem[i].prober(id, off, size) == 0)
			goto found;
	return -1;

found:
	/* If recognized, we free the allocated buffers */
	volume_id_free_buffer(id);
	return 0;
}

int volume_id_probe_all(struct volume_id *id, uint64_t off, uint64_t size)
{
	if (id == NULL)
		return -EINVAL;

	/* probe for raid first, because fs probes may be successful on raid members */
	if (volume_id_probe_raid(id, off, size) == 0)
		return 0;

	if (volume_id_probe_filesystem(id, off, size) == 0)
		return 0;

	return -1;
}

void volume_id_all_probers(all_probers_fn_t all_probers_fn,
			   struct volume_id *id, uint64_t off, uint64_t size,
			   void *data)
{
	unsigned int i;

	if (all_probers_fn == NULL)
		return;

	for (i = 0; i < ARRAY_SIZE(prober_raid); i++)
		if (all_probers_fn(prober_raid[i].prober, id, off, size, data) != 0)
			goto out;
	for (i = 0; i < ARRAY_SIZE(prober_filesystem); i++)
		if (all_probers_fn(prober_filesystem[i].prober, id, off, size, data) != 0)
			goto out;
out:
	return;
}

/* open volume by already open file descriptor */
struct volume_id *volume_id_open_fd(int fd)
{
	struct volume_id *id;

	id = malloc(sizeof(struct volume_id));
	if (id == NULL)
		return NULL;
	memset(id, 0x00, sizeof(struct volume_id));

	id->fd = fd;

	return id;
}

/* open volume by device node */
struct volume_id *volume_id_open_node(const char *path)
{
	struct volume_id *id;
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		dbg("unable to open '%s'", path);
		return NULL;
	}

	id = volume_id_open_fd(fd);
	if (id == NULL)
		return NULL;

	/* close fd on device close */
	id->fd_close = 1;

	return id;
}

void volume_id_close(struct volume_id *id)
{
	if (id == NULL)
		return;

	if (id->fd_close != 0)
		close(id->fd);

	volume_id_free_buffer(id);

	free(id);
}
