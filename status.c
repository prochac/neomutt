/**
 * @file
 * GUI display a user-configurable status line
 *
 * @authors
 * Copyright (C) 1996-2000,2007 Michael R. Elkins <me@mutt.org>
 * Copyright (C) 2017-2024 Richard Russon <rich@flatcap.org>
 * Copyright (C) 2018 Austin Ray <austin@austinray.io>
 * Copyright (C) 2020-2022 Pietro Cerutti <gahr@gahr.ch>
 * Copyright (C) 2021 Eric Blake <eblake@redhat.com>
 * Copyright (C) 2023 Dennis Schön <mail@dennis-schoen.de>
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
 * @page neo_status GUI display a user-configurable status line
 *
 * GUI display a user-configurable status line
 */

#include "config.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "core/lib.h"
#include "status.h"
#include "expando/lib.h"
#include "index/lib.h"
#include "menu/lib.h"
#include "postpone/lib.h"
#include "globals.h"
#include "mutt_mailbox.h"
#include "mutt_thread.h"
#include "muttlib.h"
#include "mview.h"

void status_f(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf);

/**
 * get_sort_str - Get the sort method as a string
 * @param buf    Buffer for the sort string
 * @param buflen Length of the buffer
 * @param method Sort method, see #SortType
 * @retval ptr Buffer pointer
 */
static char *get_sort_str(char *buf, size_t buflen, enum SortType method)
{
  snprintf(buf, buflen, "%s%s%s", (method & SORT_REVERSE) ? "reverse-" : "",
           (method & SORT_LAST) ? "last-" : "",
           mutt_map_get_name(method & SORT_MASK, SortMethods));
  return buf;
}

/**
 * struct MenuStatusLineData - Data for creating a Menu line
 */
struct MenuStatusLineData
{
  struct IndexSharedData *shared; ///< Data shared between Index, Pager and Sidebar
  struct Menu *menu;              ///< Current Menu
};

/**
 * status_r - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_r(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct Mailbox *m = shared->mailbox;

  size_t i = 0;

  if (m)
  {
    i = shared->attach_msg ? 3 :
                             ((m->readonly || m->dontwrite) ? 2 :
                              (m->changed ||
                               /* deleted doesn't necessarily mean changed in IMAP */
                               (m->type != MUTT_IMAP && m->msg_deleted)) ?
                                                              1 :
                                                              0);
  }

  const struct MbTable *c_status_chars = cs_subset_mbtable(NeoMutt->sub, "status_chars");

  char tmp[128] = { 0 };

  if (!c_status_chars || !c_status_chars->len)
    tmp[0] = '\0';
  else if (i >= c_status_chars->len)
    snprintf(tmp, sizeof(tmp), "%s", c_status_chars->chars[0]);
  else
    snprintf(tmp, sizeof(tmp), "%s", c_status_chars->chars[i]);

  buf_strcpy(buf, tmp);
}

/**
 * status_D - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_D(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct Mailbox *m = shared->mailbox;

  // If there's a descriptive name, use it. Otherwise, use %f
  if (m && m->name)
  {
    const char *s = m->name;
    buf_strcpy(buf, NONULL(s));
    return;
  }

  status_f(node, data, flags, max_width, buf);
}

/**
 * status_f - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_f(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct Mailbox *m = shared->mailbox;

  char tmp[128] = { 0 };

  if (m && m->compress_info && (m->realpath[0] != '\0'))
  {
    mutt_str_copy(tmp, m->realpath, sizeof(tmp));
    mutt_pretty_mailbox(tmp, sizeof(tmp));
  }
  else if (m && (m->type == MUTT_NOTMUCH) && m->name)
  {
    mutt_str_copy(tmp, m->name, sizeof(tmp));
  }
  else if (m && !buf_is_empty(&m->pathbuf))
  {
    mutt_str_copy(tmp, mailbox_path(m), sizeof(tmp));
    mutt_pretty_mailbox(tmp, sizeof(tmp));
  }
  else
  {
    mutt_str_copy(tmp, _("(no mailbox)"), sizeof(tmp));
  }

  buf_strcpy(buf, tmp);
}

/**
 * status_M - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_M(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct Mailbox *m = shared->mailbox;

  const int num = m ? m->vcount : 0;
  buf_printf(buf, "%d", num);
}

/**
 * status_m - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_m(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct Mailbox *m = shared->mailbox;

  const int num = m ? m->msg_count : 0;
  buf_printf(buf, "%d", num);
}

/**
 * status_n - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_n(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct Mailbox *m = shared->mailbox;

  const int num = m ? m->msg_new : 0;
  buf_printf(buf, "%d", num);
}

/**
 * status_o - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_o(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct Mailbox *m = shared->mailbox;

  const int num = m ? (m->msg_unread - m->msg_new) : 0;
  buf_printf(buf, "%d", num);
}

/**
 * status_d - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_d(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct Mailbox *m = shared->mailbox;

  const int num = m ? m->msg_deleted : 0;
  buf_printf(buf, "%d", num);
}

/**
 * status_F - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_F(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct Mailbox *m = shared->mailbox;

  const int num = m ? m->msg_flagged : 0;
  buf_printf(buf, "%d", num);
}

/**
 * status_t - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_t(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct Mailbox *m = shared->mailbox;

  const int num = m ? m->msg_tagged : 0;
  buf_printf(buf, "%d", num);
}

/**
 * status_p - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_p(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  struct Mailbox *m = shared->mailbox;

  const int num = mutt_num_postponed(m, false);
  buf_printf(buf, "%d", num);
}

/**
 * status_b - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_b(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  struct Mailbox *m = shared->mailbox;

  const int num = mutt_mailbox_check(m, MUTT_MAILBOX_CHECK_NO_FLAGS);
  buf_printf(buf, "%d", num);
}

/**
 * status_l - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_l(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct Mailbox *m = shared->mailbox;

  char tmp[128] = { 0 };

  const off_t num = m ? m->size : 0;
  mutt_str_pretty_size(tmp, sizeof(tmp), num);
  buf_strcpy(buf, tmp);
}

/**
 * status_T - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_T(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const enum UseThreads c_use_threads = mutt_thread_style();
  const char *s = get_use_threads_str(c_use_threads);
  buf_strcpy(buf, NONULL(s));
}

/**
 * status_s - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_s(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  char tmp[128] = { 0 };

  const enum SortType c_sort = cs_subset_sort(NeoMutt->sub, "sort");
  const char *s = get_sort_str(tmp, sizeof(tmp), c_sort);
  buf_strcpy(buf, NONULL(s));
}

/**
 * status_S - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_S(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  char tmp[128] = { 0 };

  const enum SortType c_sort_aux = cs_subset_sort(NeoMutt->sub, "sort_aux");
  const char *s = get_sort_str(tmp, sizeof(tmp), c_sort_aux);
  buf_strcpy(buf, NONULL(s));
}

/**
 * status_P - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_P(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct Menu *menu = msld->menu;

  char tmp[128] = { 0 };

  if (!menu)
  {
    buf_reset(buf);
    return;
  }

  char *cp = NULL;
  if (menu->top + menu->page_len >= menu->max)
  {
    cp = menu->top ?
             /* L10N: Status bar message: the end of the list emails is visible in the index */
             _("end") :
             /* L10N: Status bar message: all the emails are visible in the index */
             _("all");
  }
  else
  {
    int count = (100 * (menu->top + menu->page_len)) / menu->max;
    /* L10N: Status bar, percentage of way through index.
           `%d` is the number, `%%` is the percent symbol.
           They may be reordered, or space inserted, if you wish. */
    snprintf(tmp, sizeof(tmp), _("%d%%"), count);
    cp = tmp;
  }

  buf_strcpy(buf, NONULL(cp));
}

/**
 * status_h - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_h(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const char *s = ShortHostname;
  buf_strcpy(buf, NONULL(s));
}

/**
 * status_L - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_L(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct MailboxView *mailbox_view = shared->mailbox_view;

  char tmp[128] = { 0 };

  const off_t num = mailbox_view ? mailbox_view->vsize : 0;
  mutt_str_pretty_size(tmp, sizeof(tmp), num);
  buf_strcpy(buf, tmp);
}

/**
 * status_R - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_R(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct Mailbox *m = shared->mailbox;

  const int num = m ? (m->msg_count - m->msg_unread) : 0;
  buf_printf(buf, "%d", num);
}

/**
 * status_u - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_u(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct Mailbox *m = shared->mailbox;

  const int num = m ? m->msg_unread : 0;
  buf_printf(buf, "%d", num);
}

/**
 * status_v - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_v(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const char *s = mutt_make_version();
  buf_strcpy(buf, NONULL(s));
}

/**
 * status_V - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void status_V(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct MenuStatusLineData *msld = data;
  const struct IndexSharedData *shared = msld->shared;
  const struct MailboxView *mailbox_view = shared->mailbox_view;

  const char *s = mview_has_limit(mailbox_view) ? mailbox_view->pattern : "";
  buf_strcpy(buf, NONULL(s));
}

/**
 * menu_status_line - Create the status line
 * @param[out] buf      Buffer in which to save string
 * @param[in]  shared   Shared Index data
 * @param[in]  menu     Current menu
 * @param[in]  cols     Maximum number of columns to use
 * @param[in]  exp      Expando
 *
 * @sa status_format_str()
 */
void menu_status_line(struct Buffer *buf, struct IndexSharedData *shared,
                      struct Menu *menu, int cols, const struct Expando *exp)
{
  static const struct ExpandoRenderData StatusRenderData[] = {
    // clang-format off
    { ED_INDEX,  ED_IND_UNREAD_COUNT,    status_b },
    { ED_INDEX,  ED_IND_DELETED_COUNT,   status_d },
    { ED_INDEX,  ED_IND_DESCRIPTION,     status_D },
    { ED_INDEX,  ED_IND_FLAGGED_COUNT,   status_F },
    { ED_INDEX,  ED_IND_MAILBOX_PATH,    status_f },
    { ED_GLOBAL, ED_GLO_HOSTNAME,        status_h },
    { ED_INDEX,  ED_IND_LIMIT_SIZE,      status_L },
    { ED_INDEX,  ED_IND_MAILBOX_SIZE,    status_l },
    { ED_INDEX,  ED_IND_LIMIT_COUNT,     status_M },
    { ED_INDEX,  ED_IND_MESSAGE_COUNT,   status_m },
    { ED_INDEX,  ED_IND_NEW_COUNT,       status_n },
    { ED_INDEX,  ED_IND_OLD_COUNT,       status_o },
    { ED_MENU,   ED_MEN_PERCENTAGE,      status_P },
    { ED_INDEX,  ED_IND_POSTPONED_COUNT, status_p },
    { ED_INDEX,  ED_IND_READ_COUNT,      status_R },
    { ED_INDEX,  ED_IND_READONLY,        status_r },
    { ED_GLOBAL, ED_GLO_SORT,            status_s },
    { ED_GLOBAL, ED_GLO_SORT_AUX,        status_S },
    { ED_INDEX,  ED_IND_TAGGED_COUNT,    status_t },
    { ED_GLOBAL, ED_GLO_USE_THREADS,     status_T },
    { ED_INDEX,  ED_IND_UNREAD_COUNT,    status_u },
    { ED_INDEX,  ED_IND_LIMIT_PATTERN,   status_V },
    { ED_GLOBAL, ED_GLO_VERSION,         status_v },
    { -1, -1, NULL },
    // clang-format on
  };

  struct MenuStatusLineData data = { shared, menu };

  expando_render(exp, StatusRenderData, &data, MUTT_FORMAT_NO_FLAGS, cols, buf);
}
