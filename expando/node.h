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

#ifndef MUTT_EXPANDO_NODE_H
#define MUTT_EXPANDO_NODE_H

#include <stdbool.h>
#include <time.h>
#include "gui/lib.h"

/**
 * enum ExpandoNodeType - XXX
 */
enum ExpandoNodeType
{
  ENT_EMPTY = 0,           ///< XXX
  ENT_TEXT,                ///< XXX
  ENT_EXPANDO,             ///< XXX
  ENT_PADDING,                 ///< XXX
  ENT_CONDITION,           ///< XXX
  ENT_CONDITIONAL_DATE,    ///< XXX
};

/**
 * struct ExpandoNode - XXX
 */
struct ExpandoNode
{
  enum ExpandoNodeType type;       ///< Type of Node, e.g. #ENT_EXPANDO
  struct ExpandoNode  *next;       ///< XXX
  int                  did;        ///< Domain ID, e.g. #ED_EMAIL
  int                  uid;        ///< Unique ID, e.g. #ED_EMA_SIZE

  const char          *start;      ///< XXX
  const char          *end;        ///< XXX

  void *ndata;                     ///< XXX
  void (*ndata_free)(void **ptr);  ///< XXX
};

/**
 * struct ExpandoFormatPrivate - XXX
 */
struct ExpandoFormatPrivate
{
  int                min;             ///< XXX
  int                max;             ///< XXX
  enum FormatJustify justification;   ///< XXX
  // NOTE(g0mb4): multibyte leader?
  char               leader;          ///< XXX
  const char         *start;          ///< XXX
  const char         *end;            ///< XXX
};

/**
 * struct ExpandoExpandoPrivate - XXX
 */
struct ExpandoExpandoPrivate
{
  struct ExpandoFormatPrivate *format;  ///< XXX
  int color;                            ///< XXX
  bool has_tree;                        ///< XXX, used in $index_format's %s
};

/**
 * struct ExpandoConditionalDatePrivate - XXX
 */
struct ExpandoConditionalDatePrivate
{
  int    count;         ///< XXX
  char   period;        ///< XXX
  time_t multiplier;    ///< XXX
};

struct ExpandoNode *expando_node_new(void);

void free_node(struct ExpandoNode *node);
void free_tree(struct ExpandoNode *node);

void free_expando_private(void **ptr);
void free_expando_private_expando(void **ptr);

#endif /* MUTT_EXPANDO_NODE_H */
