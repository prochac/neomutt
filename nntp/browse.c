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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "core/lib.h"
#include "lib.h"
#include "browser/lib.h"
#include "expando/lib.h"
#include "format_flags.h"
#include "mdata.h"
#include "muttlib.h"

/**
 * group_index_format_str - Format a string for the newsgroup menu - Implements ::format_t - @ingroup expando_api
 *
 * | Expando | Description
 * | :------ | :-------------------------------------------------------
 * | \%a     | Alert: 1 if user is notified of new mail
 * | \%C     | Current newsgroup number
 * | \%d     | Description of newsgroup (becomes from server)
 * | \%f     | Newsgroup name
 * | \%M     | - if newsgroup not allowed for direct post (moderated for example)
 * | \%N     | N if newsgroup is new, u if unsubscribed, blank otherwise
 * | \%n     | Number of new articles in newsgroup
 * | \%p     | Poll: 1 if Mailbox is checked for new mail
 * | \%s     | Number of unread articles in newsgroup
 */
const char *group_index_format_str(char *buf, size_t buflen, size_t col, int cols,
                                   char op, const char *src, const char *prec,
                                   const char *if_str, const char *else_str,
                                   intptr_t data, MuttFormatFlags flags)
{
  char fn[128] = { 0 };
  char fmt[128] = { 0 };
  struct Folder *folder = (struct Folder *) data;
  bool optional = (flags & MUTT_FORMAT_OPTIONAL);

  switch (op)
  {
    case 'a':
      if (!optional)
      {
        snprintf(fmt, sizeof(fmt), "%%%sd", prec);
        snprintf(buf, buflen, fmt, folder->ff->notify_user);
      }
      else
      {
        if (folder->ff->notify_user == 0)
          optional = false;
      }
      break;

    case 'C':
      snprintf(fmt, sizeof(fmt), "%%%sd", prec);
      snprintf(buf, buflen, fmt, folder->num + 1);
      break;

    case 'd':
      if (folder->ff->nd->desc)
      {
        char *desc = mutt_str_dup(folder->ff->nd->desc);
        const char *const c_newsgroups_charset = cs_subset_string(NeoMutt->sub, "newsgroups_charset");
        if (c_newsgroups_charset)
          mutt_ch_convert_string(&desc, c_newsgroups_charset, cc_charset(), MUTT_ICONV_HOOK_FROM);
        mutt_mb_filter_unprintable(&desc);

        snprintf(fmt, sizeof(fmt), "%%%ss", prec);
        snprintf(buf, buflen, fmt, desc);
        FREE(&desc);
      }
      else
      {
        snprintf(fmt, sizeof(fmt), "%%%ss", prec);
        snprintf(buf, buflen, fmt, "");
      }
      break;

    case 'f':
      mutt_str_copy(fn, folder->ff->name, sizeof(fn));
      snprintf(fmt, sizeof(fmt), "%%%ss", prec);
      snprintf(buf, buflen, fmt, fn);
      break;

    case 'M':
      snprintf(fmt, sizeof(fmt), "%%%sc", prec);
      if (folder->ff->nd->deleted)
        snprintf(buf, buflen, fmt, 'D');
      else
        snprintf(buf, buflen, fmt, folder->ff->nd->allowed ? ' ' : '-');
      break;

    case 'N':
      snprintf(fmt, sizeof(fmt), "%%%sc", prec);
      if (folder->ff->nd->subscribed)
        snprintf(buf, buflen, fmt, ' ');
      else
        snprintf(buf, buflen, fmt, folder->ff->has_new_mail ? 'N' : 'u');
      break;

    case 'n':
    {
      const bool c_mark_old = cs_subset_bool(NeoMutt->sub, "mark_old");
      if (c_mark_old && (folder->ff->nd->last_cached >= folder->ff->nd->first_message) &&
          (folder->ff->nd->last_cached <= folder->ff->nd->last_message))
      {
        snprintf(fmt, sizeof(fmt), "%%%sd", prec);
        snprintf(buf, buflen, fmt, folder->ff->nd->last_message - folder->ff->nd->last_cached);
      }
      else
      {
        snprintf(fmt, sizeof(fmt), "%%%sd", prec);
        snprintf(buf, buflen, fmt, folder->ff->nd->unread);
      }
      break;
    }

    case 'p':
      if (!optional)
      {
        snprintf(fmt, sizeof(fmt), "%%%sd", prec);
        snprintf(buf, buflen, fmt, folder->ff->poll_new_mail);
      }
      else
      {
        if (folder->ff->poll_new_mail == 0)
          optional = false;
      }
      break;

    case 's':
      if (optional)
      {
        if (folder->ff->nd->unread != 0)
        {
          mutt_expando_format(buf, buflen, col, cols, if_str,
                              group_index_format_str, data, flags);
        }
        else
        {
          mutt_expando_format(buf, buflen, col, cols, else_str,
                              group_index_format_str, data, flags);
        }
      }
      else
      {
        snprintf(fmt, sizeof(fmt), "%%%sd", prec);
        snprintf(buf, buflen, fmt, folder->ff->nd->unread);
      }
      break;
  }
  return src;
}
