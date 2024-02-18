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
void format_tree(struct ExpandoNode *tree, const struct ExpandoRenderData *rdata,
                 char *buf, size_t buf_len, size_t col_len, void *data, MuttFormatFlags flags)
{
  const struct ExpandoNode *node = tree;
  char *buffer = buf;
  int buffer_len = (int) buf_len - 1;
  int columns_len = (int) col_len;

  struct Buffer *expando_buf = buf_pool_get();

  int printed = 0;

  while (node && (buffer_len > 0) && (columns_len > 0))
  {
    // General formats
    if (node->did == ED_ALL)
    {
      switch (node->uid)
      {
        case ED_ALL_EMPTY:
          break;

        case ED_ALL_TEXT:
          printed = text_format_callback(node, rdata, buffer, buffer_len,
                                         columns_len, data, flags);
          break;

        case ED_ALL_PAD:
        {
          struct NodePaddingPrivate *pp = node->ndata;
          pp->buffer_start = buf;
          pp->buffer_len = buf_len;
          printed = node_padding_render(node, rdata, buffer, buffer_len,
                                        columns_len, data, flags);
        }
        break;

        case ED_ALL_CONDITION:
          printed = node_condition_render(node, rdata, buffer, buffer_len,
                                          columns_len, data, flags);
          break;

        default:
          assert(0 && "Invalid UID");
          return;
      }
    }
    else
    {
      const struct ExpandoRenderData *rd = &rdata[0];
      bool found = false;
      while (rd->did != -1)
      {
        if ((rd->did == node->did) && (rd->uid == node->uid))
        {
          found = true;
          rd->callback(node, data, flags, columns_len, expando_buf);
          break;
        }
        rd++;
      }

      if (!found)
      {
        assert(0 && "Unknown UID");
      }

      printed = format_string(buffer, buffer_len, node, expando_buf);
    }

    columns_len -= mutt_strwidth_nonnull(buffer, buffer + printed);
    buffer_len -= printed;
    buffer += printed;

    node = node->next;
  }

  // give softpad nodes a chance to act
  while (node)
  {
    if (node->type == ENT_PADDING)
    {
      struct NodePaddingPrivate *pp = node->ndata;
      pp->buffer_start = buf;
      pp->buffer_len = buf_len;

      node_padding_render(node, rdata, buffer, buffer_len, columns_len, data, flags);
    }
    node = node->next;
  }

  buf_pool_release(&expando_buf);

  *buffer = '\0';
}

/**
 * text_format_callback - Callback for every text node
 * @param node     XXX
 * @param rdata    XXX
 * @param buf      XXX
 * @param buf_len  XXX
 * @param cols_len XXX
 * @param data     XXX
 * @param flags    XXX
 * @retval num XXX
 */
int text_format_callback(const struct ExpandoNode *node,
                         const struct ExpandoRenderData *rdata, char *buf,
                         int buf_len, int cols_len, void *data, MuttFormatFlags flags)
{
  assert(node->type == ENT_TEXT);

  int copylen = node->end - node->start;
  memcpy_safe(buf, node->start, copylen, buf_len);

  return copylen;
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
