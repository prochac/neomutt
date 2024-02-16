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

void test_expando_nested_if_else(void)
{
  struct ExpandoParseError error = { 0 };

  {
    const char *input = "%<a?%<b?%c&%d>&%<e?%f&%g>>";

    struct ExpandoNode *root = NULL;

    expando_tree_parse(&root, input, NULL, &error);

    TEST_CHECK(error.position == NULL);

    struct ExpandoNode *node = get_nth_node(&root, 0);
    check_condition_node_head(node);

    struct ExpandoNode *condition = expando_node_get_child(node, ENC_CONDITION);
    struct ExpandoNode *if_true_tree = expando_node_get_child(node, ENC_TRUE);
    struct ExpandoNode *if_false_tree = expando_node_get_child(node, ENC_FALSE);

    check_expando_node(condition, "a", NULL);

    struct ExpandoNode *t = if_true_tree;
    check_condition_node_head(t);

    struct ExpandoNode *f = if_false_tree;
    check_condition_node_head(f);

    condition = expando_node_get_child(t, ENC_CONDITION);
    if_true_tree = expando_node_get_child(t, ENC_TRUE);
    if_false_tree = expando_node_get_child(t, ENC_FALSE);

    check_expando_node(condition, "b", NULL);
    check_expando_node(if_true_tree, "c", NULL);
    check_expando_node(if_false_tree, "d", NULL);

    condition = expando_node_get_child(f, ENC_CONDITION);
    if_true_tree = expando_node_get_child(f, ENC_TRUE);
    if_false_tree = expando_node_get_child(f, ENC_FALSE);

    check_expando_node(condition, "e", NULL);
    check_expando_node(if_true_tree, "f", NULL);
    check_expando_node(if_false_tree, "g", NULL);

    expando_tree_free(&root);
  }

  {
    const char *input = "%<a?%<b?%c&%d>&%<e?%f>>";

    struct ExpandoNode *root = NULL;

    expando_tree_parse(&root, input, NULL, &error);

    TEST_CHECK(error.position == NULL);

    struct ExpandoNode *node = get_nth_node(&root, 0);
    check_condition_node_head(node);

    struct ExpandoNode *condition = expando_node_get_child(node, ENC_CONDITION);
    struct ExpandoNode *if_true_tree = expando_node_get_child(node, ENC_TRUE);
    struct ExpandoNode *if_false_tree = expando_node_get_child(node, ENC_FALSE);

    check_expando_node(condition, "a", NULL);

    struct ExpandoNode *t = if_true_tree;
    check_condition_node_head(t);

    struct ExpandoNode *f = if_false_tree;
    check_condition_node_head(f);

    condition = expando_node_get_child(t, ENC_CONDITION);
    if_true_tree = expando_node_get_child(t, ENC_TRUE);
    if_false_tree = expando_node_get_child(t, ENC_FALSE);

    check_expando_node(condition, "b", NULL);
    check_expando_node(if_true_tree, "c", NULL);
    check_expando_node(if_false_tree, "d", NULL);

    condition = expando_node_get_child(f, ENC_CONDITION);
    if_true_tree = expando_node_get_child(f, ENC_TRUE);
    if_false_tree = expando_node_get_child(f, ENC_FALSE);

    check_expando_node(condition, "e", NULL);
    check_expando_node(if_true_tree, "f", NULL);
    TEST_CHECK(if_false_tree == NULL);

    expando_tree_free(&root);
  }

  {
    const char *input = "%<a?%<b?%c&%d>&%<e?&%f>>";

    struct ExpandoNode *root = NULL;

    expando_tree_parse(&root, input, NULL, &error);

    TEST_CHECK(error.position == NULL);

    struct ExpandoNode *node = get_nth_node(&root, 0);
    check_condition_node_head(node);

    struct ExpandoNode *condition = expando_node_get_child(node, ENC_CONDITION);
    struct ExpandoNode *if_true_tree = expando_node_get_child(node, ENC_TRUE);
    struct ExpandoNode *if_false_tree = expando_node_get_child(node, ENC_FALSE);

    check_expando_node(condition, "a", NULL);

    struct ExpandoNode *t = if_true_tree;
    check_condition_node_head(t);

    struct ExpandoNode *f = if_false_tree;
    check_condition_node_head(f);

    condition = expando_node_get_child(t, ENC_CONDITION);
    if_true_tree = expando_node_get_child(t, ENC_TRUE);
    if_false_tree = expando_node_get_child(t, ENC_FALSE);

    check_expando_node(condition, "b", NULL);
    check_expando_node(if_true_tree, "c", NULL);
    check_expando_node(if_false_tree, "d", NULL);

    condition = expando_node_get_child(f, ENC_CONDITION);
    if_true_tree = expando_node_get_child(f, ENC_TRUE);
    if_false_tree = expando_node_get_child(f, ENC_FALSE);

    check_expando_node(condition, "e", NULL);
    check_empty_node(if_true_tree);
    check_expando_node(if_false_tree, "f", NULL);

    expando_tree_free(&root);
  }

  {
    const char *input = "%<a?%<b?%c>&%<e?%f&%g>>";

    struct ExpandoNode *root = NULL;

    expando_tree_parse(&root, input, NULL, &error);

    TEST_CHECK(error.position == NULL);

    struct ExpandoNode *node = get_nth_node(&root, 0);
    check_condition_node_head(node);

    struct ExpandoNode *condition = expando_node_get_child(node, ENC_CONDITION);
    struct ExpandoNode *if_true_tree = expando_node_get_child(node, ENC_TRUE);
    struct ExpandoNode *if_false_tree = expando_node_get_child(node, ENC_FALSE);

    check_expando_node(condition, "a", NULL);

    struct ExpandoNode *t = if_true_tree;
    check_condition_node_head(t);

    struct ExpandoNode *f = if_false_tree;
    check_condition_node_head(f);

    condition = expando_node_get_child(t, ENC_CONDITION);
    if_true_tree = expando_node_get_child(t, ENC_TRUE);
    if_false_tree = expando_node_get_child(t, ENC_FALSE);

    check_expando_node(condition, "b", NULL);
    check_expando_node(if_true_tree, "c", NULL);
    TEST_CHECK(if_false_tree == NULL);

    condition = expando_node_get_child(f, ENC_CONDITION);
    if_true_tree = expando_node_get_child(f, ENC_TRUE);
    if_false_tree = expando_node_get_child(f, ENC_FALSE);

    check_expando_node(condition, "e", NULL);
    check_expando_node(if_true_tree, "f", NULL);
    check_expando_node(if_false_tree, "g", NULL);

    expando_tree_free(&root);
  }

  {
    const char *input = "%<a?%<b?&%c>&%<e?%f&%g>>";

    struct ExpandoNode *root = NULL;

    expando_tree_parse(&root, input, NULL, &error);

    TEST_CHECK(error.position == NULL);

    struct ExpandoNode *node = get_nth_node(&root, 0);
    check_condition_node_head(node);

    struct ExpandoNode *condition = expando_node_get_child(node, ENC_CONDITION);
    struct ExpandoNode *if_true_tree = expando_node_get_child(node, ENC_TRUE);
    struct ExpandoNode *if_false_tree = expando_node_get_child(node, ENC_FALSE);

    check_expando_node(condition, "a", NULL);

    struct ExpandoNode *t = if_true_tree;
    check_condition_node_head(t);

    struct ExpandoNode *f = if_false_tree;
    check_condition_node_head(f);

    condition = expando_node_get_child(t, ENC_CONDITION);
    if_true_tree = expando_node_get_child(t, ENC_TRUE);
    if_false_tree = expando_node_get_child(t, ENC_FALSE);

    check_expando_node(condition, "b", NULL);
    check_empty_node(if_true_tree);
    check_expando_node(if_false_tree, "c", NULL);

    condition = expando_node_get_child(f, ENC_CONDITION);
    if_true_tree = expando_node_get_child(f, ENC_TRUE);
    if_false_tree = expando_node_get_child(f, ENC_FALSE);

    check_expando_node(condition, "e", NULL);
    check_expando_node(if_true_tree, "f", NULL);
    check_expando_node(if_false_tree, "g", NULL);

    expando_tree_free(&root);
  }
}
