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
#include <assert.h>
#include <stdio.h>
#include "mutt/lib.h"
#include "expando/lib.h"
#include "common.h" // IWYU pragma: keep

struct NestedIfElseData
{
  int x;
  int y;
};

static void nested_x(const struct ExpandoNode *node, void *data,
                     MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct NestedIfElseData *sd = data;

  const int num = sd->x;
  buf_printf(buf, "%d", num);
}

static void nested_y(const struct ExpandoNode *node, void *data,
                     MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct NestedIfElseData *sd = data;

  const int num = sd->y;
  buf_printf(buf, "%d", num);
}

void test_expando_nested_if_else_render(void)
{
  const char *input = "%<x?%<y?XY&X>&%<y?Y&NONE>>";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  const struct ExpandoDefinition defs[] = {
    { "x", NULL, 1, 0, 0, 0, NULL },
    { "y", NULL, 1, 1, 0, 0, NULL },
    { NULL, NULL, 0, 0, 0, 0, NULL },
  };

  expando_tree_parse(&root, input, defs, &error);

  TEST_CHECK(error.position == NULL);

  struct ExpandoNode *node = get_nth_node(&root, 0);
  check_condition_node_head(node);
  struct NodeConditionPrivate *cond = node->ndata;
  check_expando_node(cond->condition, "x", NULL);

  struct ExpandoNode *t = cond->if_true_tree;
  check_condition_node_head(t);
  struct NodeConditionPrivate *tcond = t->ndata;
  check_expando_node(tcond->condition, "y", NULL);
  check_text_node(tcond->if_true_tree, "XY");
  check_text_node(tcond->if_false_tree, "X");

  struct ExpandoNode *f = cond->if_false_tree;
  check_condition_node_head(f);
  struct NodeConditionPrivate *fcond = f->ndata;
  check_expando_node(fcond->condition, "y", NULL);
  check_text_node(fcond->if_true_tree, "Y");
  check_text_node(fcond->if_false_tree, "NONE");

  const struct Expando expando = {
    .string = input,
    .tree = root,
  };

  const struct ExpandoRenderData render[] = {
    { 1, 0, nested_x },
    { 1, 1, nested_y },
    { -1, -1, NULL },
  };

  const char *expected_X = "X";
  struct NestedIfElseData data_X = {
    .x = 1,
    .y = 0,
  };

  struct Buffer *buf = buf_pool_get();
  expando_render(&expando, render, &data_X, E_FLAGS_NO_FLAGS, buf->dsize, buf);

  TEST_CHECK(mutt_str_equal(buf_string(buf), expected_X));

  const char *expected_Y = "Y";
  struct NestedIfElseData data_Y = {
    .x = 0,
    .y = 1,
  };

  buf_reset(buf);
  expando_render(&expando, render, &data_Y, E_FLAGS_NO_FLAGS, buf->dsize, buf);

  TEST_CHECK(mutt_str_equal(buf_string(buf), expected_Y));

  const char *expected_XY = "XY";
  struct NestedIfElseData data_XY = {
    .x = 1,
    .y = 1,
  };

  buf_reset(buf);
  expando_render(&expando, render, &data_XY, E_FLAGS_NO_FLAGS, buf->dsize, buf);

  TEST_CHECK(mutt_str_equal(buf_string(buf), expected_XY));

  const char *expected_NONE = "NONE";
  struct NestedIfElseData data_NONE = {
    .x = 0,
    .y = 0,
  };

  buf_reset(buf);
  expando_render(&expando, render, &data_NONE, E_FLAGS_NO_FLAGS, buf->dsize, buf);

  TEST_CHECK(mutt_str_equal(buf_string(buf), expected_NONE));

  expando_tree_free(&root);
  buf_pool_release(&buf);
}
