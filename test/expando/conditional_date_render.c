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
#include <string.h>
#include <time.h>
#include "mutt/lib.h"
#include "expando/lib.h"
#include "common.h" // IWYU pragma: keep
#include "test_common.h"

struct CondDateData
{
  time_t t;
};

static void cond_date(const struct ExpandoNode *node, void *data,
                      MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO || node->type == ENT_CONDDATE);

  const struct CondDateData *dd = data;
  struct tm tm = mutt_date_localtime(dd->t);

  char tmp[128] = { 0 };
  char tmp2[128] = { 0 };

  if (node->type == ENT_EXPANDO)
  {
    const int len = node->end - node->start;
    memcpy(tmp2, node->start, len);

    strftime(tmp, sizeof(tmp), tmp2, &tm);
    buf_strcpy(buf, tmp);
  }
  else // condition
  {
    assert(node->ndata);

    const struct NodeCondDatePrivate *priv = node->ndata;

    time_t t = mutt_date_now();
    t -= priv->count * priv->multiplier;
    struct tm condition = mutt_date_localtime(t);

    const time_t condt = mktime(&condition);
    const time_t checkt = mktime(&tm);

    const int num = checkt > condt;
    buf_printf(buf, "%d", num);
  }
}

void test_expando_conditional_date_render(void)
{
  struct ExpandoParseError error = { 0 };

  const char *input = "%<[1m?a&banana>";

  struct ExpandoNode *root = NULL;

  const struct ExpandoDefinition defs[] = {
    { "[", NULL, 1, 0, 0, 0, parse_date },
    { NULL, NULL, 0, 0, 0, 0, NULL },
  };

  expando_tree_parse(&root, input, defs, &error);
  TEST_CHECK(error.position == NULL);

  struct ExpandoNode *node = get_nth_node(root, 0);
  check_condition_node_head(node);

  struct ExpandoNode *condition = expando_node_get_child(node, ENC_CONDITION);
  struct ExpandoNode *if_true_tree = expando_node_get_child(node, ENC_TRUE);
  struct ExpandoNode *if_false_tree = expando_node_get_child(node, ENC_FALSE);

  check_conditional_date_node(condition, 1, 'm');
  check_text_node(if_true_tree, "a");
  check_text_node(if_false_tree, "banana");

  const struct Expando expando = {
    .string = input,
    .tree = root,
  };

  const struct ExpandoRenderData render[] = {
    { 1, 0, cond_date },
    { -1, -1, NULL },
  };

  {
    struct CondDateData data = {
      .t = mutt_date_now(),
    };

    char *expected = "a";
    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, E_FLAGS_NO_FLAGS, buf->dsize, buf);

    TEST_CHECK_STR_EQ(buf_string(buf), expected);
    buf_pool_release(&buf);
  }

  {
    struct CondDateData data = {
      .t = mutt_date_now() - (60 * 60 * 24 * 365),
    };

    char *expected = "banana";
    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, E_FLAGS_NO_FLAGS, buf->dsize, buf);

    TEST_CHECK_STR_EQ(buf_string(buf), expected);
    buf_pool_release(&buf);
  }

  expando_tree_free(&root);
}
