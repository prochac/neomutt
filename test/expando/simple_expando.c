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

void test_expando_simple_expando(void)
{
  const char *input = "%a %b";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, &input, NULL, &error);

  TEST_CHECK(error.position == NULL);
  check_expando_node(get_nth_node(&root, 0), "a", NULL);
  check_text_node(get_nth_node(&root, 1), " ");
  check_expando_node(get_nth_node(&root, 2), "b", NULL);

  expando_tree_free(&root);
}
