/* -*-  mode:c; tab-width:8; c-basic-offset:8; indent-tabs-mode:nil;  -*- */
/*
   Copyright (C) 2017 by Ronnie Sahlberg <ronniesahlberg@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef STDC_HEADERS
#include <stddef.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "compat.h"

#include "smb2.h"
#include "libsmb2.h"
#include "libsmb2-private.h"

int
smb2_decode_file_basic_info(struct smb2_context *smb2,
                            void *memctx,
                            struct smb2_file_basic_info *fs,
                            struct smb2_iovec *vec)
{
        uint64_t t;

        smb2_get_uint64(vec, 0, &t);
        smb2_win_to_timeval(t, &fs->creation_time);

        smb2_get_uint64(vec, 8, &t);
        smb2_win_to_timeval(t, &fs->last_access_time);

        smb2_get_uint64(vec, 16, &t);
        smb2_win_to_timeval(t, &fs->last_write_time);

        smb2_get_uint64(vec, 24, &t);
        smb2_win_to_timeval(t, &fs->change_time);

        smb2_get_uint32(vec, 32, &fs->file_attributes);

        return 0;
}

static uint64_t
smb2_tv_timeval_to_win(struct smb2_timeval *tv){
        if (tv->tv_sec == 0 &&
            tv->tv_usec == 0) {
                return 0;
        } else if (tv->tv_sec == 0xffffffff &&
                   tv->tv_usec == 0xffffffff) {
               return 0xffffffffffffffffULL;
        }
        return smb2_timeval_to_win(tv);
}

int
smb2_encode_file_basic_info(struct smb2_context *smb2,
                            struct smb2_file_basic_info *fs,
                            struct smb2_iovec *vec)
{
        uint64_t t;

        t = smb2_tv_timeval_to_win(&fs->creation_time);
        smb2_set_uint64(vec, 0, t);

        t = smb2_tv_timeval_to_win(&fs->last_access_time);
        smb2_set_uint64(vec, 8, t);

        t = smb2_tv_timeval_to_win(&fs->last_write_time);
        smb2_set_uint64(vec, 16, t);

        t = smb2_tv_timeval_to_win(&fs->change_time);
        smb2_set_uint64(vec, 24, t);

        smb2_set_uint32(vec, 32, fs->file_attributes);
        smb2_set_uint32(vec, 36, 0);

        return 40;
}

int
smb2_decode_file_standard_info(struct smb2_context *smb2,
                               void *memctx,
                               struct smb2_file_standard_info *fs,
                               struct smb2_iovec *vec)
{
        smb2_get_uint64(vec, 0, &fs->allocation_size);
        smb2_get_uint64(vec, 8, &fs->end_of_file);
        smb2_get_uint32(vec, 16, &fs->number_of_links);
        smb2_get_uint8(vec, 20, &fs->delete_pending);
        smb2_get_uint8(vec, 21, &fs->directory);

        return 0;
}

int
smb2_encode_file_standard_info(struct smb2_context *smb2,
                               struct smb2_file_standard_info *fs,
                               struct smb2_iovec *vec)
{
        smb2_set_uint64(vec, 0, fs->allocation_size);
        smb2_set_uint64(vec, 8, fs->end_of_file);
        smb2_set_uint32(vec, 16, fs->number_of_links);
        smb2_set_uint8(vec, 20, fs->delete_pending);
        smb2_set_uint8(vec, 21, fs->directory);
        smb2_set_uint16(vec, 22, 0);

        return 24;
}

int
smb2_decode_file_position_info(struct smb2_context *smb2,
                               void *memctx,
                               struct smb2_file_position_info *fs,
                               struct smb2_iovec *vec)
{
        smb2_get_uint64(vec, 0, &fs->current_byte_offset);
        return 0;
}

int
smb2_encode_file_position_info(struct smb2_context *smb2,
                               struct smb2_file_position_info *fs,
                               struct smb2_iovec *vec)
{
        smb2_set_uint64(vec, 0, fs->current_byte_offset);
        return 8;
}

int
smb2_decode_file_all_info(struct smb2_context *smb2,
                          void *memctx,
                          struct smb2_file_all_info *fs,
                          struct smb2_iovec *vec)
{
        struct smb2_iovec v _U_;
        uint32_t name_len;
        const char *name;

        if (vec->len < 40) {
                return -1;
        }

        v.buf = &vec->buf[0];
        v.len = 40;
        smb2_decode_file_basic_info(smb2, memctx, &fs->basic, &v);

        if (vec->len < 64) {
                return -1;
        }

        v.buf = &vec->buf[40];
        v.len = 24;
        smb2_decode_file_standard_info(smb2, memctx, &fs->standard, &v);

        smb2_get_uint64(vec, 64, &fs->index_number);
        smb2_get_uint32(vec, 72, &fs->ea_size);
        smb2_get_uint32(vec, 76, &fs->access_flags);
        smb2_get_uint64(vec, 80, &fs->current_byte_offset);
        smb2_get_uint32(vec, 88, &fs->mode);
        smb2_get_uint32(vec, 92, &fs->alignment_requirement);
        smb2_get_uint32(vec, 96, &name_len);

        if (name_len > 0) {
                name = smb2_utf16_to_utf8((uint16_t *)&vec->buf[100], name_len / 2);
                fs->name = smb2_alloc_data(smb2, memctx, strlen(name) + 1);
                if (fs->name == NULL) {
                        free(discard_const(name));
                        return -1;
                }
                strcpy(discard_const(fs->name), name);
                free(discard_const(name));
        } else {
                fs->name = NULL;
        }
        return 0;
}

int
smb2_encode_file_all_info(struct smb2_context *smb2,
                          struct smb2_file_all_info *fs,
                          struct smb2_iovec *vec)
{
        struct smb2_iovec v _U_;
        struct smb2_utf16 *name = NULL;
        int name_len;

        if (vec->len < 40) {
                return -1;
        }

        v.buf = &vec->buf[0];
        v.len = 40;
        smb2_encode_file_basic_info(smb2, &fs->basic, &v);

        if (vec->len < 64) {
                return -1;
        }

        v.buf = &vec->buf[40];
        v.len = 24;
        smb2_encode_file_standard_info(smb2, &fs->standard, &v);

        smb2_set_uint64(vec, 64, fs->index_number);
        smb2_set_uint32(vec, 72, fs->ea_size);
        smb2_set_uint32(vec, 76, fs->access_flags);
        smb2_set_uint64(vec, 80, fs->current_byte_offset);
        smb2_set_uint32(vec, 88, fs->mode);
        smb2_set_uint32(vec, 92, fs->alignment_requirement);
        if (fs->name) {
                name = smb2_utf8_to_utf16((const char*)fs->name);
                if (name) {
                        name_len = 2 * name->len;
                        smb2_set_uint32(vec, 96, name_len);
                        memcpy((uint16_t *)&vec->buf[100], name->val, name_len);
                        free(name);
                        return 100 + name_len;
                } else {
                        return -1;
                }
        } else {
                smb2_set_uint32(vec, 96, 0);
                return 100;
        }
}

int
smb2_decode_file_network_open_info(struct smb2_context *smb2,
                                   void *memctx,
                                   struct smb2_file_network_open_info *fs,
                                   struct smb2_iovec *vec)
{
        uint64_t t;

        if (vec->len < 56) {
                return -1;
        }

        smb2_get_uint64(vec, 0, &t);
        smb2_win_to_timeval(t, &fs->creation_time);

        smb2_get_uint64(vec, 8, &t);
        smb2_win_to_timeval(t, &fs->last_access_time);

        smb2_get_uint64(vec, 16, &t);
        smb2_win_to_timeval(t, &fs->last_write_time);

        smb2_get_uint64(vec, 24, &t);
        smb2_win_to_timeval(t, &fs->change_time);

        smb2_get_uint64(vec, 32, &fs->allocation_size);
        smb2_get_uint64(vec, 40, &fs->end_of_file);
        smb2_get_uint32(vec, 48, &fs->file_attributes);
        return 0;
}

int
smb2_encode_file_network_open_info(struct smb2_context *smb2,
                                   struct smb2_file_network_open_info *fs,
                                   struct smb2_iovec *vec)
{
        uint64_t t;
        if (vec->len < 56) {
                return -1;
        }


        t = smb2_tv_timeval_to_win(&fs->creation_time);
        smb2_set_uint64(vec, 0, t);

        t = smb2_tv_timeval_to_win(&fs->last_access_time);
        smb2_set_uint64(vec, 8, t);

        t = smb2_tv_timeval_to_win(&fs->last_write_time);
        smb2_set_uint64(vec, 16, t);

        t = smb2_tv_timeval_to_win(&fs->change_time);
        smb2_set_uint64(vec, 24, t);

        smb2_set_uint64(vec, 32, fs->allocation_size);
        smb2_set_uint64(vec, 40, fs->end_of_file);
        smb2_set_uint32(vec, 48, fs->file_attributes);
        return 52;
}


int
smb2_decode_file_normalized_name_info(struct smb2_context *smb2,
                          void *memctx,
                          struct smb2_file_name_info *fs,
                          struct smb2_iovec *vec)
{
        struct smb2_iovec v _U_;
        uint32_t name_len;
        const char *name;

        if (vec->len < 4) {
                return -1;
        }

        v.buf = &vec->buf[0];
        v.len = 4;
        smb2_get_uint32(vec, 0, &name_len);

        fs->file_name_length = name_len / 2;

        /* it is possible the data is truncated if the blob len < name_len
        * (status will be SMB2_STATUS_BUFFER_OVERFLOW), so allow this case
        */
        if (vec->len < (name_len + 4)) {
                name_len = vec->len - 4;
        }

        if (name_len > 0) {
                name = smb2_utf16_to_utf8((uint16_t *)&vec->buf[4], name_len / 2);
                fs->name = smb2_alloc_data(smb2, memctx, strlen(name) + 1);
                if (fs->name == NULL) {
                        free(discard_const(name));
                        return -1;
                }
                strcpy(discard_const(fs->name), name);
                free(discard_const(name));
        } else {
                fs->name = NULL;
        }
        return 0;
}

int
smb2_encode_file_normalized_name_info(struct smb2_context *smb2,
                          struct smb2_file_name_info *fs,
                          struct smb2_iovec *vec)
{
        struct smb2_utf16 *name = NULL;
        int name_len;

        if (vec->len < 4) {
                return -1;
        }

        if (fs->name) {
                name = smb2_utf8_to_utf16((const char*)fs->name);
                if (name) {
                        name_len = 2 * name->len;
                        if (vec->len < name_len + 4) {
                                return -1;
                        }
                        smb2_set_uint32(vec, 0, name_len);
                        memcpy((uint16_t *)&vec->buf[4], name->val, name_len);
                        free(name);
                        return 4 + name_len;
                } else {
                        return -1;
                }
        } else {
                smb2_set_uint32(vec, 0, 0);
                return 4;
        }
}

