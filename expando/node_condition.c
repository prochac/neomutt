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
 * @page expando_node_condition XXX
 *
 * XXX
 */

#include "config.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "mutt/lib.h"
#include "node_condition.h"
#include "domain.h"
#include "format_callbacks.h"
#include "helpers.h"
#include "mutt_thread.h"
#include "node.h"
#include "uid.h"

/**
 * node_condition_new - XXX
 * @param condition     XXX
 * @param if_true_tree  XXX
 * @param if_false_tree XXX
 * @retval ptr XXX
 */
struct ExpandoNode *node_condition_new(struct ExpandoNode *condition,
                                       struct ExpandoNode *if_true_tree,
                                       struct ExpandoNode *if_false_tree)
{
  assert(condition);
  assert(if_true_tree);

  struct ExpandoNode *node = expando_node_new();

  node->type = ENT_CONDITION;
  node->did = ED_ALL;
  node->uid = ED_ALL_CONDITION;

  ARRAY_SET(&node->children, ENC_CONDITION, condition);
  ARRAY_SET(&node->children, ENC_TRUE, if_true_tree);
  ARRAY_SET(&node->children, ENC_FALSE, if_false_tree);

  return node;
}

/**
 * is_equal - XXX
 * @param s XXX
 * @param c XXX
 * @retval true XXX
 */
static bool is_equal(const char *s, char c)
{
  // skip color
  if (*s == MUTT_SPECIAL_INDEX)
  {
    s += 2;
  }

  if (*s != c)
  {
    return false;
  }
  s++;

  // skip color
  if (*s == MUTT_SPECIAL_INDEX)
  {
    s += 2;
  }

  if (*s != '\0')
  {
    return false;
  }

  return true;
}

/**
 * node_condition_render - Callback for every conditional node
 * @param node     XXX
 * @param rdata    XXX
 * @param buf      XXX
 * @param buf_len  XXX
 * @param cols_len XXX
 * @param data     XXX
 * @param flags    XXX
 * @retval num XXX
 */
int node_condition_render(const struct ExpandoNode *node,
                          const struct ExpandoRenderData *rdata, char *buf,
                          int buf_len, int cols_len, void *data, MuttFormatFlags flags)
{
  assert(node->type == ENT_CONDITION);

  struct ExpandoNode *condition = expando_node_get_child(node, ENC_CONDITION);
  struct ExpandoNode *if_true_tree = expando_node_get_child(node, ENC_TRUE);
  struct ExpandoNode *if_false_tree = expando_node_get_child(node, ENC_FALSE);

  char tmp[1024] = { 0 };

  format_tree(condition, rdata, tmp, sizeof(tmp), sizeof(tmp), data, flags);

  /* true if:
    - not 0 (numbers)
    - not empty string (strings)
    - not ' ' (flags)
  */
  if (!is_equal(tmp, '0') && !is_equal(tmp, '\0') && !is_equal(tmp, ' '))
  {
    memset(tmp, 0, sizeof(tmp));
    format_tree(if_true_tree, rdata, tmp, sizeof(tmp), sizeof(tmp), data, flags);

    int copylen = strlen(tmp);
    memcpy_safe(buf, tmp, copylen, buf_len);

    return copylen;
  }
  else
  {
    if (if_false_tree)
    {
      memset(tmp, 0, sizeof(tmp));
      format_tree(if_false_tree, rdata, tmp, sizeof(tmp), sizeof(tmp), data, flags);

      int copylen = strlen(tmp);
      memcpy_safe(buf, tmp, copylen, buf_len);
      return copylen;
    }
    else
    {
      return 0;
    }
  }
  return 0;
}
