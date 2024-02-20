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
#include "mutt/lib.h"
#include "common.h"

struct ExpandoNode *get_nth_node(struct ExpandoNode *node, int n)
{
  TEST_CHECK(node != NULL);

  int i = 0;

  while (node)
  {
    if (i++ == n)
    {
      return node;
    }

    node = node->next;
  }

  if (!TEST_CHECK(0))
  {
    TEST_MSG("Node is not found\n");
  }

  return NULL;
}

void check_empty_node(struct ExpandoNode *node)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == ENT_EMPTY);
}

void check_text_node(struct ExpandoNode *node, const char *text)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == ENT_TEXT);

  const size_t n = mutt_str_len(text);
  const size_t m = (node->end - node->start);
  TEST_CHECK(n == m);
  TEST_CHECK(mutt_strn_equal(node->start, text, n));
}

void check_expando_node(struct ExpandoNode *node, const char *expando,
                        const struct ExpandoFormat *format)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == ENT_EXPANDO);
  TEST_CHECK(node->ndata != NULL);

  const size_t n = mutt_str_len(expando);
  const size_t m = node->end - node->start;

  TEST_CHECK(n == m);
  TEST_CHECK(mutt_strn_equal(node->start, expando, n));

  struct ExpandoFormat *f = node->format;

  if (format == NULL)
  {
    TEST_CHECK(f == NULL);
  }
  else
  {
    TEST_CHECK(f != NULL);
    TEST_CHECK(f->justification == format->justification);
    TEST_CHECK(f->leader == format->leader);
    TEST_CHECK(f->min == format->min);
    TEST_CHECK(f->max == format->max);
  }
}

void check_pad_node(struct ExpandoNode *node, const char *pad_char, enum ExpandoPadType pad_type)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == ENT_PADDING);

  const size_t n = mutt_str_len(pad_char);
  const size_t m = node->end - node->start;

  TEST_CHECK(n == m);
  TEST_CHECK(mutt_strn_equal(node->start, pad_char, n));

  TEST_CHECK(node->ndata != NULL);
  struct NodePaddingPrivate *p = node->ndata;
  TEST_CHECK(p->pad_type == pad_type);
}

void check_condition_node_head(struct ExpandoNode *node)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == ENT_CONDITION);
}

void check_conditional_date_node(struct ExpandoNode *node, int count, char period)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == ENT_CONDDATE);

  TEST_CHECK(node->ndata != NULL);
  struct NodeCondDatePrivate *p = node->ndata;

  TEST_CHECK(p->count == count);
  TEST_CHECK(p->period == period);
}

struct ExpandoNode *parse_date(const char *s, const char **parsed_until,
                               int did, int uid, struct ExpandoParseError *error)
{
  // s-1 is always something valid
  if (*(s - 1) == '<')
  {
    return node_conddate_parse(s + 1, parsed_until, did, uid, error);
  }

  return expando_parse_enclosed_expando(s, parsed_until, did, uid, ']', error);
}
