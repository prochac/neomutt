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
 * @page expando_node_expando XXX
 *
 * XXX
 */

#include "config.h"
#include <assert.h>
#include "mutt/lib.h"
#include "node_expando.h"
#include "helpers.h"
#include "node.h"

/**
 * node_expando_private_new - XXX
 */
static struct NodeExpandoPrivate *node_expando_private_new(void)
{
  struct NodeExpandoPrivate *priv = mutt_mem_calloc(1, sizeof(struct NodeExpandoPrivate));

  // NOTE(g0mb4): Expando definition should contain this
  priv->color = -1;

  return priv;
}

/**
 * node_expando_private_free - XXX
 * @param ptr XXX
 */
static void node_expando_private_free(void **ptr)
{
  if (!ptr || !*ptr)
    return;

  FREE(ptr);
}

/**
 * node_expando_new - XXX
 * @param start  XXX
 * @param end    XXX
 * @param fmt    XXX
 * @param did    XXX
 * @param uid    XXX
 * @retval ptr XXX
 */
struct ExpandoNode *node_expando_new(const char *start, const char *end,
                                     struct ExpandoFormat *fmt, int did, int uid)
{
  struct ExpandoNode *node = expando_node_new();

  node->type = ENT_EXPANDO;
  node->start = start;
  node->end = end;

  node->did = did;
  node->uid = uid;
  node->render = node_expando_render;

  node->format = fmt;

  node->ndata = node_expando_private_new();
  node->ndata_free = node_expando_private_free;

  return node;
}

/**
 * node_expando_set_color - XXX
 */
void node_expando_set_color(const struct ExpandoNode *node, int cid)
{
  if (!node || (node->type != ENT_EXPANDO) || !node->ndata)
    return;

  struct NodeExpandoPrivate *priv = node->ndata;

  priv->color = cid;
}

/**
 * node_expando_set_has_tree - XXX
 */
void node_expando_set_has_tree(const struct ExpandoNode *node, bool has_tree)
{
  if (!node || (node->type != ENT_EXPANDO) || !node->ndata)
    return;

  struct NodeExpandoPrivate *priv = node->ndata;

  priv->has_tree = has_tree;
}

/**
 * find_callback - XXX
 */
static expando_callback_t find_callback(const struct ExpandoRenderData *rdata, int did, int uid)
{
  if (!rdata)
    return NULL;

  for (; rdata->callback; rdata++)
  {
    if ((rdata->did == did) && (rdata->uid == uid))
    {
      return rdata->callback;
    }
  }

  return NULL;
}

/**
 * node_expando_render - Callback for every expando node
 * @param node     XXX
 * @param rdata    XXX
 * @param buf      XXX
 * @param buf_len  XXX
 * @param cols_len XXX
 * @param data     XXX
 * @param flags    XXX
 * @retval num XXX
 */
int node_expando_render(const struct ExpandoNode *node,
                        const struct ExpandoRenderData *rdata, char *buf,
                        int buf_len, int cols_len, void *data, MuttFormatFlags flags)
{
  assert((node->type == ENT_EXPANDO) || (node->type == ENT_CONDDATE));

  const expando_callback_t callback = find_callback(rdata, node->did, node->uid);
  assert(callback && "Unknown UID");

  struct Buffer *expando_buf = buf_pool_get();

  callback(node, data, flags, cols_len, expando_buf);
  int printed = format_string(buf, buf_len, node, expando_buf);

  buf_pool_release(&expando_buf);
  return printed;
}
