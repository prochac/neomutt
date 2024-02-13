/**
 * @file
 * XXX
 *
 * @authors
 * Copyright (C) 2023-2024 Tóth János <gomba007@gmail.com>
 * Copyright (C) 2023-2024 Richard Russon <rich@flatcap.org>
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
 * @page expando_format XXX
 *
 * XXX
 */

#include "config.h"
#include <assert.h>
#include <stdbool.h>
#include "mutt/lib.h"
#include "format_callbacks.h"
#include "domain.h"
#include "expando.h"
#include "helpers.h"
#include "node.h"
#include "node_condition.h"
#include "node_padding.h"
#include "node_text.h"
#include "uid.h"

/**
 * format_tree - XXX
 * @param tree    XXX
 * @param rdata   XXX
 * @param buf     XXX
 * @param buf_len XXX
 * @param col_len XXX
 * @param data    XXX
 * @param flags   XXX
 */
void format_tree(const struct ExpandoNode *node, const struct ExpandoRenderData *rdata,
                 char *buf, int buf_len, int cols_len, void *data, MuttFormatFlags flags)
{
  buf_len--;

  int printed = 0;

  while (node && (buf_len > 0) && (cols_len > 0))
  {
    if (node->render)
    {
      printed = node->render(node, rdata, buf, buf_len, cols_len, data, flags);
    }

    cols_len -= mutt_strwidth_nonnull(buf, buf + printed);
    buf_len -= printed;
    buf += printed;

    node = node->next;
  }
}

/**
 * expando_render - Render an Expando + data into a string
 * @param[in]  cols     Number of screen columns
 * @param[in]  exp      Expando containing the expando tree
 * @param[in]  rdata    Expando render data
 * @param[in]  data     Callback data
 * @param[in]  flags    Callback flags
 * @param[out] buf      Buffer in which to save string
 */
void expando_render(const struct Expando *exp, const struct ExpandoRenderData *rdata,
                    void *data, MuttFormatFlags flags, int cols, struct Buffer *buf)
{
  if (!exp || !exp->tree || !rdata)
  {
    return;
  }

  struct ExpandoNode *root = exp->tree;
  format_tree(root, rdata, buf->data, buf->dsize, cols, data, flags);
}
