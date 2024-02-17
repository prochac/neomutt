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

void test_expando_formatted_expando(void)
{
  const char *input = "%X %8X %-8X %08X %.8X %8.8X %-8.8X %=8X";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, input, NULL, &error);

  TEST_CHECK(error.position == NULL);
  check_expando_node(get_nth_node(&root, 0), "X", NULL);
  check_text_node(get_nth_node(&root, 1), " ");

  {
    struct ExpandoFormat fmt = { 0 };
    fmt.min = 8;
    fmt.max = INT_MAX;
    fmt.justification = JUSTIFY_RIGHT;
    fmt.leader = ' ';
    check_expando_node(get_nth_node(&root, 2), "X", &fmt);
    check_text_node(get_nth_node(&root, 3), " ");
  }

  {
    struct ExpandoFormat fmt = { 0 };
    fmt.min = 8;
    fmt.max = INT_MAX;
    fmt.justification = JUSTIFY_LEFT;
    fmt.leader = ' ';
    check_expando_node(get_nth_node(&root, 4), "X", &fmt);
    check_text_node(get_nth_node(&root, 5), " ");
  }

  {
    struct ExpandoFormat fmt = { 0 };
    fmt.min = 8;
    fmt.max = INT_MAX;
    fmt.justification = JUSTIFY_RIGHT;
    fmt.leader = '0';
    check_expando_node(get_nth_node(&root, 6), "X", &fmt);
    check_text_node(get_nth_node(&root, 7), " ");
  }

  {
    struct ExpandoFormat fmt = { 0 };
    fmt.min = 0;
    fmt.max = 8;
    fmt.justification = JUSTIFY_RIGHT;
    fmt.leader = ' ';
    check_expando_node(get_nth_node(&root, 8), "X", &fmt);
    check_text_node(get_nth_node(&root, 9), " ");
  }

  {
    struct ExpandoFormat fmt = { 0 };
    fmt.min = 8;
    fmt.max = 8;
    fmt.justification = JUSTIFY_RIGHT;
    fmt.leader = ' ';
    check_expando_node(get_nth_node(&root, 10), "X", &fmt);
    check_text_node(get_nth_node(&root, 11), " ");
  }

  {
    struct ExpandoFormat fmt = { 0 };
    fmt.min = 8;
    fmt.max = 8;
    fmt.justification = JUSTIFY_LEFT;
    fmt.leader = ' ';
    check_expando_node(get_nth_node(&root, 12), "X", &fmt);
    check_text_node(get_nth_node(&root, 13), " ");
  }

  {
    struct ExpandoFormat fmt = { 0 };
    fmt.min = 8;
    fmt.max = INT_MAX;
    fmt.justification = JUSTIFY_CENTER;
    fmt.leader = ' ';
    check_expando_node(get_nth_node(&root, 14), "X", &fmt);
  }

  expando_tree_free(&root);
}
