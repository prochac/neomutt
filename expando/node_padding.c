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
#include "node_padding.h"
#include "color/lib.h"
#include "domain.h"
#include "format_callbacks.h"
#include "helpers.h"
#include "mutt_thread.h"
#include "node.h"
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
 * is_utf8_cont_byte - XXX
 * @param c XXX
 */
static bool is_utf8_cont_byte(uint8_t c)
{
  return (c & 0xC0) == 0x80;
}

/**
 * count_spec - XXX
 * @param s XXX
 * @retval num XXX
 */
static int count_spec(const char *s)
{
  int n = 0;
  while (*s)
  {
    if (*s == MUTT_SPECIAL_INDEX)
    {
      n++;
    }

    s++;
  }

  return n;
}

/**
 * count_spec_end - XXX
 * @param s   XXX
 * @param end XXX
 * @retval num XXX
 */
static int count_spec_end(const char *s, const char *end)
{
  int n = 0;
  while (*s && s <= end)
  {
    if (*s == MUTT_SPECIAL_INDEX)
    {
      n++;
    }

    s++;
  }

  return n;
}

/**
 * softpad_correct_utf8 - XXX
 * @param buf        XXX
 * @param copy_start XXX
 */
static void softpad_correct_utf8(const char *buf, char **copy_start)
{
  char *s = *copy_start;
  uint8_t c = (uint8_t) *s;

  if (is_ascii_byte(c))
  {
    return;
  }

  while (is_utf8_cont_byte(c))
  {
    assert(s - 1 >= buf);
    s--;
    c = (uint8_t) *s;
  }

  if (is_utf8_2_byte_head(c))
  {
    *copy_start = (char *) (s + 2);
  }
  else if (is_utf8_3_byte_head(c))
  {
    *copy_start = (char *) s + 3;
  }
  else if (is_utf8_4_byte_head(c))
  {
    *copy_start = (char *) s + 4;
  }
  else
  {
    assert(0 && "Unreachable");
  }
}

/**
 * softpad_move_markers - XXX
 * @param buf        XXX
 * @param copy_start XXX
 */
static void softpad_move_markers(char *buf, char *copy_start)
{
  const int n = count_spec_end(buf, copy_start);

  // all markers are closed
  if (n % 2 == 0)
  {
    // if position is a marker
    if (*copy_start == MUTT_SPECIAL_INDEX)
    {
      const char color = *(copy_start + 1);
      assert(color == MT_COLOR_INDEX);

      // move marker
      *(copy_start - 2) = MUTT_SPECIAL_INDEX;
      *(copy_start - 1) = MT_COLOR_INDEX;
    }
    // if position is a color
    else if (*(copy_start - 1) == MUTT_SPECIAL_INDEX)
    {
      const char color = *copy_start;
      assert(color == MT_COLOR_INDEX);

      // move marker
      *(copy_start - 2) = MUTT_SPECIAL_INDEX;
      *(copy_start - 1) = MT_COLOR_INDEX;
    }
  }
  // one marker is open
  else
  {
    // move marker
    *(copy_start - 2) = MUTT_SPECIAL_INDEX;
    *(copy_start - 1) = MT_COLOR_INDEX;
  }
}

/**
 * node_padding_parse - XXX
 * @param s            XXX
 * @param parsed_until XXX
 * @param error        XXX
 * @retval ptr XXX
 */
struct ExpandoNode *node_padding_parse(const char *s, const char **parsed_until,
                                       struct ExpandoParseError *error)
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
  const int pad_len = node->end - node->start;
  const int pad_width = mutt_strwidth_nonnull(node->start, node->end);

  int len = buf_len;
  int cols = cols_len;

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
  const int pad_len = node->end - node->start;
  const int pad_width = mutt_strwidth_nonnull(node->start, node->end);

  char right[1024] = { 0 };
  struct ExpandoNode *root = node->next;
  format_tree(root, rdata, right, sizeof(right), sizeof(right), data, flags);
  const int right_len = mutt_str_len(right);
  const int right_width = mutt_strwidth(right);

  int len = buf_len;
  int cols = cols_len;
  bool is_space_to_write = ((len - pad_len - right_len) > 0) &&
                           ((cols - pad_width - right_width) >= 0);

  while (is_space_to_write)
  {
    memcpy(buf, node->start, pad_len);

    buf += pad_len;
    len -= pad_len;
    cols -= pad_width;

    is_space_to_write = ((len - pad_len - right_len) > 0) &&
                        ((cols - pad_width - right_width) >= 0);
  }

  mutt_strn_copy(buf, right, MIN(cols_len, right_len), len);

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
  const int pad_len = node->end - node->start;
  const int pad_width = mutt_strwidth_nonnull(node->start, node->end);

  const struct NodePaddingPrivate *pp = node->ndata;

  char right[1024] = { 0 };
  struct ExpandoNode *root = node->next;
  format_tree(root, rdata, right, sizeof(right), sizeof(right), data, flags);

  int right_len = mutt_str_len(right);

  int no_spec = 0;

  int len = buf_len;
  int cols = cols_len;

  // NOTE(g0mb4): Dirty hack...
  // somehow the colormarkers count as a column
  if (flags & MUTT_FORMAT_INDEX)
  {
    no_spec = count_spec(right);
    cols += no_spec * 2;
  }

  // fill space
  bool is_space_to_write = ((len - pad_len) > 0) && ((cols - pad_width) >= 0);
  while (is_space_to_write)
  {
    memcpy(buf, node->start, pad_len);

    buf += pad_len;
    len -= pad_len;
    cols -= pad_width;

    is_space_to_write = ((len - pad_len) > 0) && ((cols - pad_width) >= 0);
  }

  char *copy_start = buf - right_len;
  if (copy_start < pp->buffer_start)
  {
    mutt_strn_copy(pp->buffer_start, right, right_len, pp->buffer_len);
  }
  else
  {
    softpad_correct_utf8(pp->buffer_start, &copy_start);

    const size_t occupied = copy_start - pp->buffer_start;
    const size_t remaining = pp->buffer_len - occupied;

    if (flags & MUTT_FORMAT_INDEX)
    {
      softpad_move_markers(pp->buffer_start, copy_start);
    }

    mutt_strn_copy(copy_start, right, right_len, remaining);
  }

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

  struct NodePaddingPrivate *pp = node->ndata;

  switch (pp->pad_type)
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
