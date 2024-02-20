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
#include "expando/lib.h"
#include "common.h" // IWYU pragma: keep
#include "test_common.h"

struct NullData
{
  int n;
};

void test_expando_padding_render(void)
{
  static const struct ExpandoDefinition FormatData[] = {
    // clang-format off
    { "*", "padding-soft", ED_GLOBAL, ED_GLO_PADDING_SOFT, E_TYPE_STRING, E_FLAGS_NO_FLAGS, node_padding_parse },
    { ">", "padding-hard", ED_GLOBAL, ED_GLO_PADDING_HARD, E_TYPE_STRING, E_FLAGS_NO_FLAGS, node_padding_parse },
    { "|", "padding-eol",  ED_GLOBAL, ED_GLO_PADDING_EOL,  E_TYPE_STRING, E_FLAGS_NO_FLAGS, node_padding_parse },
    { NULL, NULL, 0, -1, -1, 0, NULL }
    // clang-format on
  };

  struct ExpandoParseError error = { 0 };

  {
    const char *input = "text1%|-text2";
    struct ExpandoNode *root = NULL;

    expando_tree_parse(&root, input, FormatData, &error);

    TEST_CHECK(error.position == NULL);
    check_pad_node(root, "-", EPT_FILL_EOL);

    struct ExpandoNode *left = expando_node_get_child(root, ENP_LEFT);
    struct ExpandoNode *right = expando_node_get_child(root, ENP_RIGHT);

    TEST_CHECK(left != NULL);
    TEST_CHECK(right != NULL);

    check_text_node(left, "text1");
    check_text_node(right, "text2");

    const struct Expando expando = {
      .string = input,
      .tree = root,
    };

    const struct ExpandoRenderData render[] = {
      { -1, -1, NULL },
    };

    struct NullData data = { 0 };

    const char *expected = "text1---";
    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, E_FLAGS_NO_FLAGS, 8, buf);

    TEST_CHECK_STR_EQ(buf_string(buf), expected);
    TEST_MSG("Expected: %s", expected);
    TEST_MSG("Actual:   %s", buf_string(buf));

    expando_tree_free(&root);
    buf_pool_release(&buf);
  }

  {
    const char *input = "text1%|-text2";
    struct ExpandoNode *root = NULL;

    expando_tree_parse(&root, input, FormatData, &error);

    TEST_CHECK(error.position == NULL);
    check_pad_node(root, "-", EPT_FILL_EOL);

    struct ExpandoNode *left = expando_node_get_child(root, ENP_LEFT);
    struct ExpandoNode *right = expando_node_get_child(root, ENP_RIGHT);

    TEST_CHECK(left != NULL);
    TEST_CHECK(right != NULL);

    check_text_node(left, "text1");
    check_text_node(right, "text2");

    const struct Expando expando = {
      .string = input,
      .tree = root,
    };

    const struct ExpandoRenderData render[] = {
      { -1, -1, NULL },
    };

    struct NullData data = { 0 };

    const char *expected = "text1--------";
    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, E_FLAGS_NO_FLAGS, 13, buf);

    TEST_CHECK_STR_EQ(buf_string(buf), expected);

    expando_tree_free(&root);
    buf_pool_release(&buf);
  }

  {
    const char *input = "text1%>-text2";
    struct ExpandoNode *root = NULL;

    expando_tree_parse(&root, input, FormatData, &error);

    TEST_CHECK(error.position == NULL);
    check_pad_node(root, "-", EPT_HARD_FILL);

    struct ExpandoNode *left = expando_node_get_child(root, ENP_LEFT);
    struct ExpandoNode *right = expando_node_get_child(root, ENP_RIGHT);

    TEST_CHECK(left != NULL);
    TEST_CHECK(right != NULL);

    check_text_node(left, "text1");
    check_text_node(right, "text2");

    const struct Expando expando = {
      .string = input,
      .tree = root,
    };

    const struct ExpandoRenderData render[] = {
      { -1, -1, NULL },
    };

    struct NullData data = { 0 };

    const char *expected = "text1tex";
    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, E_FLAGS_NO_FLAGS, 8, buf);
    TEST_CHECK_STR_EQ(buf_string(buf), expected);
    TEST_MSG("Expected: %s", expected);
    TEST_MSG("Actual:   %s", buf_string(buf));

    expando_tree_free(&root);
    buf_pool_release(&buf);
  }

  {
    const char *input = "text1%>-text2";
    struct ExpandoNode *root = NULL;

    expando_tree_parse(&root, input, FormatData, &error);

    TEST_CHECK(error.position == NULL);
    check_pad_node(root, "-", EPT_HARD_FILL);

    struct ExpandoNode *left = expando_node_get_child(root, ENP_LEFT);
    struct ExpandoNode *right = expando_node_get_child(root, ENP_RIGHT);

    TEST_CHECK(left != NULL);
    TEST_CHECK(right != NULL);

    check_text_node(left, "text1");
    check_text_node(right, "text2");

    const struct Expando expando = {
      .string = input,
      .tree = root,
    };

    const struct ExpandoRenderData render[] = {
      { -1, -1, NULL },
    };

    struct NullData data = { 0 };

    const char *expected = "text1---text2";
    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, E_FLAGS_NO_FLAGS, 13, buf);

    TEST_CHECK_STR_EQ(buf_string(buf), expected);
    TEST_MSG("Expected: %s", expected);
    TEST_MSG("Actual:   %s", buf_string(buf));

    expando_tree_free(&root);
    buf_pool_release(&buf);
  }

  {
    const char *input = "text1%*-text2";
    struct ExpandoNode *root = NULL;

    expando_tree_parse(&root, input, FormatData, &error);

    TEST_CHECK(error.position == NULL);
    check_pad_node(root, "-", EPT_SOFT_FILL);

    struct ExpandoNode *left = expando_node_get_child(root, ENP_LEFT);
    struct ExpandoNode *right = expando_node_get_child(root, ENP_RIGHT);

    TEST_CHECK(left != NULL);
    TEST_CHECK(right != NULL);

    check_text_node(left, "text1");
    check_text_node(right, "text2");

    const struct Expando expando = {
      .string = input,
      .tree = root,
    };

    const struct ExpandoRenderData render[] = {
      { -1, -1, NULL },
    };

    struct NullData data = { 0 };

    const char *expected = "textext2";
    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, E_FLAGS_NO_FLAGS, 8, buf);

    TEST_CHECK_STR_EQ(buf_string(buf), expected);
    TEST_MSG("Expected: %s", expected);
    TEST_MSG("Actual:   %s", buf_string(buf));

    expando_tree_free(&root);
    buf_pool_release(&buf);
  }

  {
    const char *input = "text1%*-text2";
    struct ExpandoNode *root = NULL;

    expando_tree_parse(&root, input, FormatData, &error);

    TEST_CHECK(error.position == NULL);
    check_pad_node(root, "-", EPT_SOFT_FILL);

    struct ExpandoNode *left = expando_node_get_child(root, ENP_LEFT);
    struct ExpandoNode *right = expando_node_get_child(root, ENP_RIGHT);

    TEST_CHECK(left != NULL);
    TEST_CHECK(right != NULL);

    check_text_node(left, "text1");
    check_text_node(right, "text2");

    const struct Expando expando = {
      .string = input,
      .tree = root,
    };

    const struct ExpandoRenderData render[] = {
      { -1, -1, NULL },
    };

    struct NullData data = { 0 };

    const char *expected = "text1---text2";
    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, E_FLAGS_NO_FLAGS, 13, buf);

    TEST_CHECK_STR_EQ(buf_string(buf), expected);
    TEST_MSG("Expected: %s", expected);
    TEST_MSG("Actual:   %s", buf_string(buf));

    expando_tree_free(&root);
    buf_pool_release(&buf);
  }

  {
    const char *input = "text1%*-text2";
    struct ExpandoNode *root = NULL;

    expando_tree_parse(&root, input, FormatData, &error);

    TEST_CHECK(error.position == NULL);
    check_pad_node(root, "-", EPT_SOFT_FILL);

    struct ExpandoNode *left = expando_node_get_child(root, ENP_LEFT);
    struct ExpandoNode *right = expando_node_get_child(root, ENP_RIGHT);

    TEST_CHECK(left != NULL);
    TEST_CHECK(right != NULL);

    check_text_node(left, "text1");
    check_text_node(right, "text2");

    const struct Expando expando = {
      .string = input,
      .tree = root,
    };

    const struct ExpandoRenderData render[] = {
      { -1, -1, NULL },
    };

    struct NullData data = { 0 };

    const char *expected = "text2";
    struct Buffer *buf = buf_pool_get();
    expando_render(&expando, render, &data, E_FLAGS_NO_FLAGS, 5, buf);

    TEST_CHECK_STR_EQ(buf_string(buf), expected);
    TEST_MSG("Expected: %s", expected);
    TEST_MSG("Actual:   %s", buf_string(buf));

    expando_tree_free(&root);
    buf_pool_release(&buf);
  }
}
