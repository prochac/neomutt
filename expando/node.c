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
 * free_expando_private - XXX
 * @param ptr XXX
 */
void free_expando_private(void **ptr)
{
  FREE(ptr);
}

/**
 * free_expando_private_expando - XXX
 * @param ptr XXX
 */
void free_expando_private_expando(void **ptr)
{
  struct ExpandoExpandoPrivate *p = *ptr;

  FREE(&p->format);
  FREE(ptr);
}

/**
 * free_expando_private_condition_node - XXX
 * @param ptr XXX
 */
void free_expando_private_condition_node(void **ptr)
{
  struct ExpandoConditionPrivate *p = *ptr;

  free_node(p->condition);
  free_tree(p->if_true_tree);
  free_tree(p->if_false_tree);

  FREE(ptr);
}
