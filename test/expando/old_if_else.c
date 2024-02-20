/**
 * @file
 * Test code for XXX
 *
 * @authors
 * Copyright (C) 2023-2024 Tóth János <gomba007@gmail.com>
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

#define TEST_NO_MAIN
#include "config.h"
#include "acutest.h"
#include <stddef.h>
#include "gui/lib.h"
#include "expando/lib.h"
#include "common.h" // IWYU pragma: keep
#include "limits.h"

void test_expando_old_if_else(void)
{
  const char *input = "if: %?l?%4l?  if-else: %?l?%4l&%4c?";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, input, NULL, &error);

  TEST_CHECK(error.position == NULL);
  check_text_node(get_nth_node(root, 0), "if: ");

  {
    struct ExpandoNode *node = get_nth_node(root, 1);
    check_condition_node_head(node);

    struct ExpandoNode *condition = expando_node_get_child(node, ENC_CONDITION);
    struct ExpandoNode *if_true_tree = expando_node_get_child(node, ENC_TRUE);
    struct ExpandoNode *if_false_tree = expando_node_get_child(node, ENC_FALSE);

    check_expando_node(condition, "l", NULL);

    struct ExpandoFormat fmt = { 0 };
    fmt.min = 4;
    fmt.max = INT_MAX;
    fmt.justification = JUSTIFY_RIGHT;
    fmt.leader = ' ';
    check_expando_node(if_true_tree, "l", &fmt);
    TEST_CHECK(if_false_tree == NULL);
  }

  check_text_node(get_nth_node(root, 2), "  if-else: ");

  {
    struct ExpandoNode *node = get_nth_node(root, 3);
    check_condition_node_head(node);

    struct ExpandoNode *condition = expando_node_get_child(node, ENC_CONDITION);
    struct ExpandoNode *if_true_tree = expando_node_get_child(node, ENC_TRUE);
    struct ExpandoNode *if_false_tree = expando_node_get_child(node, ENC_FALSE);

    check_expando_node(condition, "l", NULL);

    struct ExpandoFormat fmt = { 0 };
    fmt.min = 4;
    fmt.max = INT_MAX;
    fmt.justification = JUSTIFY_RIGHT;
    fmt.leader = ' ';
    check_expando_node(if_true_tree, "l", &fmt);
    check_expando_node(if_false_tree, "c", &fmt);
  }

  expando_tree_free(&root);
}
