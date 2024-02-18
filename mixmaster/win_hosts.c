/**
 * @file
 * Mixmaster Hosts Window
 *
 * @authors
 * Copyright (C) 2022-2024 Richard Russon <rich@flatcap.org>
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
 * @page mixmaster_win_hosts Mixmaster Hosts Window
 *
 * Display and editable list of selected Remailer Hosts.
 *
 * ## Windows
 *
 * | Name                   | Type    | See Also        |
 * | :--------------------- | :------ | :-------------- |
 * | Mixmaster Hosts Window | WT_MENU | win_hosts_new() |
 *
 * **Parent**
 * - @ref mixmaster_dlg_mixmaster
 *
 * **Children**
 * - None
 *
 * ## Data
 * - RemailerArray
 *
 * The Hosts Window stores its data (RemailerArray) in Menu::mdata.
 */

#include "config.h"
#include <stddef.h>
#include <assert.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "core/lib.h"
#include "gui/lib.h"
#include "expando/lib.h"
#include "menu/lib.h"
#include "remailer.h"

/**
 * mix_format_caps - Turn flags into a MixMaster capability string
 * @param r Remailer to use
 * @retval ptr Capability string
 *
 * @note The string is a static buffer
 */
static const char *mix_format_caps(const struct Remailer *r)
{
  // NOTE(g0mb4): use $to_chars?
  static char capbuf[10];
  char *t = capbuf;

  if (r->caps & MIX_CAP_COMPRESS)
    *t++ = 'C';
  else
    *t++ = ' ';

  if (r->caps & MIX_CAP_MIDDLEMAN)
    *t++ = 'M';
  else
    *t++ = ' ';

  if (r->caps & MIX_CAP_NEWSPOST)
  {
    *t++ = 'N';
    *t++ = 'p';
  }
  else
  {
    *t++ = ' ';
    *t++ = ' ';
  }

  if (r->caps & MIX_CAP_NEWSMAIL)
  {
    *t++ = 'N';
    *t++ = 'm';
  }
  else
  {
    *t++ = ' ';
    *t++ = ' ';
  }

  *t = '\0';

  return capbuf;
}

/**
 * mix_a - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void mix_a(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
           int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct Remailer *remailer = data;

  const char *s = remailer->addr;
  buf_strcpy(buf, NONULL(s));
}

/**
 * mix_c - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void mix_c(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
           int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct Remailer *remailer = data;

  const char *s = mix_format_caps(remailer);
  buf_strcpy(buf, NONULL(s));
}

/**
 * mix_n - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void mix_n(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
           int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct Remailer *remailer = data;

  const int num = remailer->num;
  buf_printf(buf, "%d", num);
}

/**
 * mix_s - XXX - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void mix_s(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
           int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct Remailer *remailer = data;

  const char *s = remailer->shortname;
  buf_strcpy(buf, NONULL(s));
}

/**
 * mix_make_entry - Format a Remailer for the Menu - Implements Menu::make_entry() - @ingroup menu_make_entry
 *
 * @sa $mix_entry_format
 */
static void mix_make_entry(struct Menu *menu, int line, struct Buffer *buf)
{
  struct RemailerArray *ra = menu->mdata;
  struct Remailer **r = ARRAY_GET(ra, line);
  if (!r)
    return;

  const char *const c_mix_entry_format = cs_subset_string(NeoMutt->sub, "mix_entry_format");
  // mutt_expando_format(buf->data, buf->dsize, 0, menu->win->state.cols,
  //                     NONULL(c_mix_entry_format), mix_format_str, (intptr_t) *r,
  //                     MUTT_FORMAT_ARROWCURSOR);
}

/**
 * win_hosts_new - Create a new Hosts Window
 * @param ra Array of Remailer hosts
 * @retval ptr New Hosts Window
 */
struct MuttWindow *win_hosts_new(struct RemailerArray *ra)
{
  struct MuttWindow *win_hosts = menu_window_new(MENU_MIXMASTER, NeoMutt->sub);
  win_hosts->focus = win_hosts;

  struct Menu *menu = win_hosts->wdata;

  menu->max = ARRAY_SIZE(ra);
  menu->make_entry = mix_make_entry;
  menu->tag = NULL;
  menu->mdata = ra;
  menu->mdata_free = NULL; // Menu doesn't own the data

  return win_hosts;
}

/**
 * win_hosts_get_selection - Get the current selection
 * @param win Hosts Window
 * @retval ptr Currently selected Remailer
 */
struct Remailer *win_hosts_get_selection(struct MuttWindow *win)
{
  if (!win || !win->wdata)
    return NULL;

  struct Menu *menu = win->wdata;
  if (!menu->mdata)
    return NULL;

  struct RemailerArray *ra = menu->mdata;

  const int sel = menu_get_index(menu);
  if (sel < 0)
    return NULL;

  struct Remailer **r = ARRAY_GET(ra, sel);
  if (!r)
    return NULL;

  return *r;
}
