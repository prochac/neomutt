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

void test_expando_padding(void)
{
  static const struct ExpandoDefinition FormatData[] = {
    // clang-format off
    { "*", "padding-soft", ED_GLOBAL, ED_GLO_PADDING_SOFT, E_TYPE_STRING, E_FLAGS_NO_FLAGS, node_padding_parse },
    { ">", "padding-hard", ED_GLOBAL, ED_GLO_PADDING_HARD, E_TYPE_STRING, E_FLAGS_NO_FLAGS, node_padding_parse },
    { "|", "padding-eol",  ED_GLOBAL, ED_GLO_PADDING_EOL,  E_TYPE_STRING, E_FLAGS_NO_FLAGS, node_padding_parse },
    { NULL, NULL, 0, -1, -1, 0, NULL }
    // clang-format on
  };

  const char *input = "%|A %>B %*C";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, input, FormatData, &error);

  TEST_CHECK(error.position == NULL);
  check_pad_node(get_nth_node(root, 0), "A", EPT_FILL_EOL);

  struct ExpandoNode *left = expando_node_get_child(root, ENP_LEFT);
  struct ExpandoNode *right = expando_node_get_child(root, ENP_RIGHT);

  TEST_CHECK(left == NULL);
  TEST_CHECK(right != NULL);

  check_text_node(get_nth_node(right, 0), " ");
  check_pad_node(get_nth_node(right, 1), "B", EPT_HARD_FILL);
  check_text_node(get_nth_node(right, 2), " ");
  check_pad_node(get_nth_node(right, 3), "C", EPT_SOFT_FILL);

  expando_tree_free(&root);
}
