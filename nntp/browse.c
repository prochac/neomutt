/**
 * @file
 * Browse NNTP groups
 *
 * @authors
 * Copyright (C) 2018-2024 Richard Russon <rich@flatcap.org>
 * Copyright (C) 2020 Pietro Cerutti <gahr@gahr.ch>
 * Copyright (C) 2023-2024 Tóth János <gomba007@gmail.com>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @page nntp_browse Browse NNTP groups
 *
 * Browse NNTP groups
 */

#include "config.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "core/lib.h"
#include "lib.h"
#include "browser/lib.h"
#include "expando/lib.h"
#include "mdata.h"

/**
 * group_index_a - NNTP: Alert for new mail - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void group_index_a(const struct ExpandoNode *node, void *data,
                   MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct Folder *folder = data;

  const int num = folder->ff->notify_user;
  buf_printf(buf, "%d", num);
}

/**
 * group_index_C - NNTP: Index number - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void group_index_C(const struct ExpandoNode *node, void *data,
                   MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct Folder *folder = data;

  const int num = folder->num + 1;
  buf_printf(buf, "%d", num);
}

/**
 * group_index_d - NNTP: Description - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void group_index_d(const struct ExpandoNode *node, void *data,
                   MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct Folder *folder = data;

  char tmp[128] = { 0 };

  if (folder->ff->nd->desc)
  {
    char *desc = mutt_str_dup(folder->ff->nd->desc);
    const char *const c_newsgroups_charset = cs_subset_string(NeoMutt->sub, "newsgroups_charset");
    if (c_newsgroups_charset)
    {
      mutt_ch_convert_string(&desc, c_newsgroups_charset, cc_charset(), MUTT_ICONV_HOOK_FROM);
    }
    mutt_mb_filter_unprintable(&desc);
    mutt_str_copy(tmp, desc, sizeof(tmp));
    FREE(&desc);
  }
  else
  {
    tmp[0] = '\0';
  }

  buf_strcpy(buf, tmp);
}

/**
 * group_index_f - NNTP: Newsgroup name - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void group_index_f(const struct ExpandoNode *node, void *data,
                   MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct Folder *folder = data;

  const char *s = folder->ff->name;
  buf_strcpy(buf, NONULL(s));
}

/**
 * group_index_M - NNTP: Moderated flag - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void group_index_M(const struct ExpandoNode *node, void *data,
                   MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct Folder *folder = data;

  const char *s = NULL;
  // NOTE(g0mb4): use $flag_chars?
  if (folder->ff->nd->deleted)
  {
    s = "D";
  }
  else
  {
    s = folder->ff->nd->allowed ? " " : "-";
  }

  buf_strcpy(buf, NONULL(s));
}

/**
 * group_index_n - NNTP: Number of new articles - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void group_index_n(const struct ExpandoNode *node, void *data,
                   MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct Folder *folder = data;

  const bool c_mark_old = cs_subset_bool(NeoMutt->sub, "mark_old");
  int num = 0;

  if (c_mark_old && (folder->ff->nd->last_cached >= folder->ff->nd->first_message) &&
      (folder->ff->nd->last_cached <= folder->ff->nd->last_message))
  {
    num = folder->ff->nd->last_message - folder->ff->nd->last_cached;
  }
  else
  {
    num = folder->ff->nd->unread;
  }

  buf_printf(buf, "%d", num);
}

/**
 * group_index_N - NNTP: New flag - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void group_index_N(const struct ExpandoNode *node, void *data,
                   MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct Folder *folder = data;

  const char *s = NULL;
  // NOTE(g0mb4): use $flag_chars?
  if (folder->ff->nd->subscribed)
  {
    s = " ";
  }
  else
  {
    s = folder->ff->has_new_mail ? "N" : "u";
  }

  buf_strcpy(buf, NONULL(s));
}

/**
 * group_index_p - NNTP: Poll for new mail - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void group_index_p(const struct ExpandoNode *node, void *data,
                   MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct Folder *folder = data;

  const int num = folder->ff->poll_new_mail;
  buf_printf(buf, "%d", num);
}

/**
 * group_index_s - NNTP: Number of unread articles - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void group_index_s(const struct ExpandoNode *node, void *data,
                   MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct Folder *folder = data;

  // NOTE(g0mb4): is long required for unread?
  const int num = (int) folder->ff->nd->unread;
  buf_printf(buf, "%d", num);
}
