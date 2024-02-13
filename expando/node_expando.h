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

#ifndef MUTT_EXPANDO_NODE_EXPANDO_H
#define MUTT_EXPANDO_NODE_EXPANDO_H

#include <limits.h>
#include <stdbool.h>
#include "format_callbacks.h"

struct ExpandoFormat;

/**
 * struct NodeExpandoPrivate - XXX
 */
struct NodeExpandoPrivate
{
  int color;                            ///< XXX
  bool has_tree;                        ///< XXX, used in $index_format's %s
};

struct ExpandoNode *node_expando_new(const char *start, const char *end, struct ExpandoFormat *fmt, int did, int uid);

void node_expando_set_color   (const struct ExpandoNode *node, int cid);
void node_expando_set_has_tree(const struct ExpandoNode *node, bool has_tree);
int node_expando_render(const struct ExpandoNode *node, const struct ExpandoRenderData *rdata, char *buf, int buf_len, int cols_len, void *data, MuttFormatFlags flags);

#endif /* MUTT_EXPANDO_NODE_EXPANDO_H */
