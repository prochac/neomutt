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
 * @page expando_node XXX
 *
 * XXX
 */

#include "config.h"
#include "mutt/lib.h"
#include "node.h"

/**
 * expando_node_new - XXX
 */
struct ExpandoNode *expando_node_new(void)
{
  return mutt_mem_calloc(1, sizeof(struct ExpandoNode));
}

/**
 * free_node - XXX
 * @param node XXX
 */
void free_node(struct ExpandoNode *node)
{
  if (!node)
    return;

  if (node->ndata_free)
  {
    node->ndata_free(&node->ndata);
  }

  struct ExpandoNode **enp = NULL;
  ARRAY_FOREACH(enp, &node->children)
  {
    if (enp)
      free_tree(*enp);
  }

  ARRAY_FREE(&node->children);

  FREE(&node);
}

/**
 * free_tree - XXX
 * @param node XXX
 */
void free_tree(struct ExpandoNode *node)
{
  while (node)
  {
    struct ExpandoNode *n = node;
    node = node->next;
    free_node(n);
  }
}

/**
 * expando_node_get_child - XXX
 */
struct ExpandoNode *expando_node_get_child(const struct ExpandoNode *node, int index)
{
  if (!node)
    return NULL;

  struct ExpandoNode **ptr = ARRAY_GET(&node->children, index);
  if (!ptr)
    return NULL;

  return *ptr;
}

/**
 * expando_node_set_child - XXX
 */
void expando_node_set_child(struct ExpandoNode *node, int index, struct ExpandoNode *child)
{
  if (!node)
    return;

  struct ExpandoNode *old = expando_node_get_child(node, index);
  free_tree(old);

  ARRAY_SET(&node->children, index, child);
}
