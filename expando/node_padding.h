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

#ifndef MUTT_EXPANDO_NODE_PADDING_H
#define MUTT_EXPANDO_NODE_PADDING_H

#include <stddef.h>
#include "format_callbacks.h"

struct ExpandoNode;
struct ExpandoParseError;

/**
 * enum ExpandoPadType - XXX
 */
enum ExpandoPadType
{
  EPT_FILL_EOL,     ///< XXX
  EPT_HARD_FILL,    ///< XXX
  EPT_SOFT_FILL     ///< XXX
};

/**
 * enum ENPad - XXX
 */
enum ENPad
{
  ENP_LEFT,           ///< Index of Left-Hand Nodes
  ENP_RIGHT,          ///< Index of Right-Hand Nodes
};

/**
 * struct NodePaddingPrivate - XXX
 */
struct NodePaddingPrivate
{
  enum ExpandoPadType  pad_type;        ///< XXX
};

int node_padding_render (const struct ExpandoNode *node, const struct ExpandoRenderData *rdata, char *buf, int buf_len, int cols_len, void *data, MuttFormatFlags flags);
int node_padding_render_eol (const struct ExpandoNode *node, const struct ExpandoRenderData *rdata, char *buf, int buf_len, int cols_len, void *data, MuttFormatFlags flags);
int node_padding_render_hard(const struct ExpandoNode *node, const struct ExpandoRenderData *rdata, char *buf, int buf_len, int cols_len, void *data, MuttFormatFlags flags);
int node_padding_render_soft(const struct ExpandoNode *node, const struct ExpandoRenderData *rdata, char *buf, int buf_len, int cols_len, void *data, MuttFormatFlags flags);

struct ExpandoNode *node_padding_parse(const char *s, const char **parsed_until, int did, int uid, struct ExpandoParseError *error);

#endif /* MUTT_EXPANDO_NODE_PADDING_H */
