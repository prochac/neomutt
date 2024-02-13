/**
 * @file
 * XXX
 *
 * @authors
 * Copyright (C) 2023-2024 Tóth János <gomba007@gmail.com>
 * Copyright (C) 2023-2024 Richard Russon <rich@flatcap.org>
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

/**
 * @page expando_node_padding XXX
 *
 * XXX
 */

#include "config.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mutt/lib.h"
#include "gui/lib.h"
#include "color/lib.h"
#include "domain.h"
#include "format_callbacks.h"
#include "helpers.h"
#include "mutt_thread.h"
#include "node.h"
#include "node_padding.h"
#include "parser.h"
#include "uid.h"

/**
 * node_padding_private_new - XXX
 */
static struct NodePaddingPrivate *node_padding_private_new(enum ExpandoPadType pad_type)
{
  struct NodePaddingPrivate *priv = mutt_mem_calloc(1, sizeof(struct NodePaddingPrivate));

  priv->pad_type = pad_type;

  return priv;
}

/**
 * node_padding_private_free - XXX
 * @param ptr XXX
 */
static void node_padding_private_free(void **ptr)
{
  if (!ptr || !*ptr)
    return;

  FREE(ptr);
}

/**
 * node_padding_new - XXX
 * @param pad_type XXX
 * @param start    XXX
 * @param end      XXX
 * @retval ptr XXX
 */
static struct ExpandoNode *node_padding_new(enum ExpandoPadType pad_type,
                                            const char *start, const char *end)
{
  struct ExpandoNode *node = expando_node_new();

  node->type = ENT_PADDING;
  node->start = start;
  node->end = end;

  node->did = ED_ALL;
  node->uid = ED_ALL_PAD;

  switch (pad_type)
  {
    case EPT_FILL_EOL:
      node->render = node_padding_render_eol;
      break;
    case EPT_HARD_FILL:
      node->render = node_padding_render_hard;
      break;
    case EPT_SOFT_FILL:
      node->render = node_padding_render_soft;
      break;
    default:
      assert(0 && "Unknown pad type");
  };

  node->ndata = node_padding_private_new(pad_type);
  node->ndata_free = node_padding_private_free;

  return node;
}

/**
 * is_ascii_byte - XXX
 * @param c XXX
 */
static bool is_ascii_byte(uint8_t c)
{
  return (c & 0x80) == 0;
}

/**
 * is_utf8_2_byte_head - XXX
 * @param c XXX
 */
static bool is_utf8_2_byte_head(uint8_t c)
{
  return (c & 0xE0) == 0xC0;
}

/**
 * is_utf8_3_byte_head - XXX
 * @param c XXX
 */
static bool is_utf8_3_byte_head(uint8_t c)
{
  return (c & 0xF0) == 0xE0;
}

/**
 * is_utf8_4_byte_head - XXX
 * @param c XXX
 */
static bool is_utf8_4_byte_head(uint8_t c)
{
  return (c & 0xF8) == 0xF0;
}

/**
 * node_padding_parse - XXX - Implements ::expando_parser_t - @ingroup expando_parser_api
 */
struct ExpandoNode *node_padding_parse(const char *s, const char **parsed_until,
                                       int did, int uid, struct ExpandoParseError *error)
{
  enum ExpandoPadType pt = 0;
  if (*s == '|')
  {
    pt = EPT_FILL_EOL;
  }
  else if (*s == '>')
  {
    pt = EPT_HARD_FILL;
  }
  else if (*s == '*')
  {
    pt = EPT_SOFT_FILL;
  }
  else
  {
    error->position = s;
    snprintf(error->message, sizeof(error->message), "Unknown padding: `%c`", *s);
    return NULL;
  }
  s++;

  size_t consumed = 0;
  uint8_t c = (uint8_t) *s;

  if (is_ascii_byte(c))
  {
    consumed = 1;
  }
  else if (is_utf8_2_byte_head(c))
  {
    consumed = 2;
  }
  else if (is_utf8_3_byte_head(c))
  {
    consumed = 3;
  }
  else if (is_utf8_4_byte_head(c))
  {
    consumed = 4;
  }
  else
  {
    assert(0 && "Unreachable");
  }

  *parsed_until = s + consumed;
  return node_padding_new(pt, s, s + consumed);
}

/**
 * node_padding_render_eol - XXX
 * @param node     XXX
 * @param rdata    XXX
 * @param buf      XXX
 * @param buf_len  XXX
 * @param cols_len XXX
 * @param data     XXX
 * @param flags    XXX
 * @retval num XXX
 */
int node_padding_render_eol(const struct ExpandoNode *node,
                            const struct ExpandoRenderData *rdata, char *buf,
                            int buf_len, int cols_len, void *data, MuttFormatFlags flags)
{
  struct ExpandoNode *left = expando_node_get_child(node, ENP_LEFT);

  format_tree(left, rdata, buf, buf_len, buf_len, data, flags);

  const int left_len = mutt_str_len(buf);
  const int left_width = mutt_strwidth(buf);

  const int pad_len = node->end - node->start;
  const int pad_width = mutt_strwidth_nonnull(node->start, node->end);

  int len = buf_len - left_len;
  int cols = cols_len - left_width;

  buf += left_len;

  bool is_space_to_write = ((len - pad_len) > 0) && ((cols - pad_width) >= 0);
  while (is_space_to_write)
  {
    memcpy(buf, node->start, pad_len);

    buf += pad_len;
    len -= pad_len;
    cols -= pad_width;

    is_space_to_write = ((len - pad_len) > 0) && ((cols - pad_width) >= 0);
  }

  // formatting of the whole buffer is done
  return buf_len;
}

/**
 * node_padding_render_hard - XXX
 * @param node     XXX
 * @param rdata    XXX
 * @param buf      XXX
 * @param buf_len  XXX
 * @param cols_len XXX
 * @param data     XXX
 * @param flags    XXX
 * @retval num XXX
 */
int node_padding_render_hard(const struct ExpandoNode *node,
                             const struct ExpandoRenderData *rdata, char *buf,
                             int buf_len, int cols_len, void *data, MuttFormatFlags flags)
{
  struct ExpandoNode *left = expando_node_get_child(node, ENP_LEFT);

  format_tree(left, rdata, buf, buf_len, cols_len, data, flags);

  const int pad_len = node->end - node->start;
  const int pad_width = mutt_strwidth_nonnull(node->start, node->end);

  const int left_len = mutt_str_len(buf);
  const int left_width = mutt_strwidth(buf);

  struct ExpandoNode *right = expando_node_get_child(node, ENP_RIGHT);

  char right_str[1024] = { 0 };
  format_tree(right, rdata, right_str, sizeof(right_str), cols_len - left_width, data, flags);

  const int right_len = mutt_str_len(right_str);
  const int right_width = mutt_strwidth(right_str);

  int len = buf_len - left_len - right_len;
  int cols = cols_len - left_width - right_width;

  buf += left_len;

  bool is_space_to_write = ((len - pad_len) > 0) && ((cols - pad_width) >= 0);

  while (is_space_to_write)
  {
    memcpy(buf, node->start, pad_len);

    buf += pad_len;
    len -= pad_len;
    cols -= pad_width;

    is_space_to_write = ((len - pad_len) > 0) && ((cols - pad_width) >= 0);
  }

  mutt_strn_copy(buf, right_str, MIN(cols_len, right_len), len);

  // formatting of the whole buffer is done
  return buf_len;
}

/**
 * node_padding_render_soft - XXX
 * @param node     XXX
 * @param rdata    XXX
 * @param buf      XXX
 * @param buf_len  XXX
 * @param cols_len XXX
 * @param data     XXX
 * @param flags    XXX
 * @retval num XXX
 */
int node_padding_render_soft(const struct ExpandoNode *node,
                             const struct ExpandoRenderData *rdata, char *buf,
                             int buf_len, int cols_len, void *data, MuttFormatFlags flags)
{
  char right_str[1024] = { 0 };
  // int right_len = 0;
  int right_width = 0;

  struct ExpandoNode *right = expando_node_get_child(node, ENP_RIGHT);
  if (right)
  {
    format_tree(right, rdata, right_str, sizeof(right_str), cols_len, data, flags);

    // right_len = mutt_str_len(right_str);
    right_width = mutt_strwidth(right_str);

    cols_len -= right_width;
  }

  char left_str[1024] = { 0 };
  int left_len = 0;
  int left_width = 0;

  struct ExpandoNode *left = expando_node_get_child(node, ENP_LEFT);
  if (left)
  {
    format_tree(left, rdata, left_str, sizeof(left_str), cols_len, data, flags);

    left_len = mutt_str_len(left_str);
    left_width = mutt_strwidth(left_str);

    cols_len -= left_width;
  }

  mutt_str_copy(buf, left_str, buf_len);

  buf += left_len;
  buf_len -= left_len;

  const int pad_len = node->end - node->start;
  const int pad_width = mutt_strwidth_nonnull(node->start, node->end);

  int len = buf_len;

  // fill space
  bool is_space_to_write = ((len - pad_len) > 0) && ((cols_len - pad_width) >= 0);
  while (is_space_to_write)
  {
    memcpy(buf, node->start, pad_len);

    buf += pad_len;
    len -= pad_len;
    cols_len -= pad_width;

    is_space_to_write = ((len - pad_len) > 0) && ((cols_len - pad_width) >= 0);
  }

  mutt_str_copy(buf, right_str, buf_len);

  // formatting of the whole buffer is done
  return buf_len;
}

/**
 * node_padding_render - Callback for every pad node
 * @param node     XXX
 * @param rdata    XXX
 * @param buf      XXX
 * @param buf_len  XXX
 * @param cols_len XXX
 * @param data     XXX
 * @param flags    XXX
 * @retval num XXX
 */
int node_padding_render(const struct ExpandoNode *node,
                        const struct ExpandoRenderData *rdata, char *buf,
                        int buf_len, int cols_len, void *data, MuttFormatFlags flags)
{
  assert(node->type == ENT_PADDING);
  assert(node->ndata);

  struct NodePaddingPrivate *priv = node->ndata;

  switch (priv->pad_type)
  {
    case EPT_FILL_EOL:
      return node_padding_render_eol(node, rdata, buf, buf_len, cols_len, data, flags);
    case EPT_HARD_FILL:
      return node_padding_render_hard(node, rdata, buf, buf_len, cols_len, data, flags);
    case EPT_SOFT_FILL:
      return node_padding_render_soft(node, rdata, buf, buf_len, cols_len, data, flags);
    default:
      assert(0 && "Unknown pad type");
  };

  assert(0 && "Unreachable");
  return 0;
}
