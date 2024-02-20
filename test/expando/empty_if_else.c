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
#include "expando/lib.h"
#include "common.h" // IWYU pragma: keep

void test_expando_empty_if_else(void)
{
  struct ExpandoParseError error = { 0 };

  const char *input1 = "%<c?>";
  struct ExpandoNode *root1 = NULL;
  expando_tree_parse(&root1, input1, NULL, &error);
  TEST_CHECK(error.position == NULL);
  {
    struct ExpandoNode *node = get_nth_node(root1, 0);
    check_condition_node_head(node);

    struct ExpandoNode *condition = expando_node_get_child(node, ENC_CONDITION);
    struct ExpandoNode *if_true_tree = expando_node_get_child(node, ENC_TRUE);
    struct ExpandoNode *if_false_tree = expando_node_get_child(node, ENC_FALSE);

    check_expando_node(condition, "c", NULL);
    check_empty_node(if_true_tree);
    TEST_CHECK(if_false_tree == NULL);
  }
  expando_tree_free(&root1);

  const char *input2 = "%<c?&>";
  struct ExpandoNode *root2 = NULL;
  expando_tree_parse(&root2, input2, NULL, &error);
  TEST_CHECK(error.position == NULL);
  {
    struct ExpandoNode *node = get_nth_node(root2, 0);
    check_condition_node_head(node);

    struct ExpandoNode *condition = expando_node_get_child(node, ENC_CONDITION);
    struct ExpandoNode *if_true_tree = expando_node_get_child(node, ENC_TRUE);
    struct ExpandoNode *if_false_tree = expando_node_get_child(node, ENC_FALSE);

    check_expando_node(condition, "c", NULL);
    check_empty_node(if_true_tree);
    check_empty_node(if_false_tree);
  }
  expando_tree_free(&root2);

  const char *input3 = "%<c?%t&>";
  struct ExpandoNode *root3 = NULL;
  expando_tree_parse(&root3, input3, NULL, &error);
  TEST_CHECK(error.position == NULL);
  {
    struct ExpandoNode *node = get_nth_node(root3, 0);
    check_condition_node_head(node);

    struct ExpandoNode *condition = expando_node_get_child(node, ENC_CONDITION);
    struct ExpandoNode *if_true_tree = expando_node_get_child(node, ENC_TRUE);
    struct ExpandoNode *if_false_tree = expando_node_get_child(node, ENC_FALSE);

    check_expando_node(condition, "c", NULL);
    check_expando_node(if_true_tree, "t", NULL);
    check_empty_node(if_false_tree);
  }
  expando_tree_free(&root3);

  const char *input4 = "%<c?&%f>";
  struct ExpandoNode *root4 = NULL;
  expando_tree_parse(&root4, input4, NULL, &error);
  TEST_CHECK(error.position == NULL);
  {
    struct ExpandoNode *node = get_nth_node(root4, 0);
    check_condition_node_head(node);

    struct ExpandoNode *condition = expando_node_get_child(node, ENC_CONDITION);
    struct ExpandoNode *if_true_tree = expando_node_get_child(node, ENC_TRUE);
    struct ExpandoNode *if_false_tree = expando_node_get_child(node, ENC_FALSE);

    check_expando_node(condition, "c", NULL);
    check_empty_node(if_true_tree);
    check_expando_node(if_false_tree, "f", NULL);
  }
  expando_tree_free(&root4);
}
