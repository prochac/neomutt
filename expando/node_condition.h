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

#ifndef MUTT_EXPANDO_NODE_CONDITION_H
#define MUTT_EXPANDO_NODE_CONDITION_H

#include "format_callbacks.h"

struct ExpandoNode;

/**
 * ExpandoConditionStart - Signals parse_node() if the parsing started in a conditional statement or not
 *
 * Easier to read than a simple true, false.
 */
enum ExpandoConditionStart
{
  CON_NO_CONDITION, ///< XXX
  CON_START         ///< XXX
};

/**
 * enum ENCondition - XXX
 */
enum ENCondition
{
  ENC_CONDITION,      ///< Index of Condition Node
  ENC_TRUE,           ///< Index of True Node
  ENC_FALSE,          ///< Index of False Node
};

int node_condition_render(const struct ExpandoNode *node, const struct ExpandoRenderData *rdata, char *buf, int buf_len, int cols_len, void *data, MuttFormatFlags flags);
struct ExpandoNode *node_condition_new(struct ExpandoNode *condition, struct ExpandoNode *if_true_tree, struct ExpandoNode *if_false_tree);

#endif /* MUTT_EXPANDO_NODE_CONDITION_H */
