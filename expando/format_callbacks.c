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
 * @page expando_format XXX
 *
 * XXX
 */

#include "config.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "mutt/lib.h"
#include "gui/lib.h"
#include "format_callbacks.h"
#include "color/lib.h"
#include "domain.h"
#include "expando.h"
#include "helpers.h"
#include "mutt_thread.h"
#include "node.h"
#include "uid.h"

/**
 * format_tree - XXX
 * @param tree    XXX
 * @param rdata   XXX
 * @param buf     XXX
 * @param buf_len XXX
 * @param col_len XXX
 * @param data    XXX
 * @param flags   XXX
 */
void format_tree(struct ExpandoNode *tree, const struct ExpandoRenderData *rdata,
                 char *buf, size_t buf_len, size_t col_len, void *data, MuttFormatFlags flags)
{
  const struct ExpandoNode *node = tree;
  char *buffer = buf;
  int buffer_len = (int) buf_len - 1;
  int columns_len = (int) col_len;

  struct Buffer *expando_buf = buf_pool_get();

  int printed = 0;

  while (node && (buffer_len > 0) && (columns_len > 0))
  {
    // General formats
    if (node->did == ED_ALL)
    {
      switch (node->uid)
      {
        case ED_ALL_EMPTY:
          break;

        case ED_ALL_TEXT:
          printed = text_format_callback(node, rdata, buffer, buffer_len,
                                         columns_len, data, flags);
          break;

        case ED_ALL_PAD:
        {
          struct ExpandoPadPrivate *pp = node->ndata;
          pp->buffer_start = buf;
          pp->buffer_len = buf_len;
          printed = pad_format_callback(node, rdata, buffer, buffer_len,
                                        columns_len, data, flags);
        }
        break;

        case ED_ALL_CONDITION:
          printed = conditional_format_callback(node, rdata, buffer, buffer_len,
                                                columns_len, data, flags);
          break;

        default:
          assert(0 && "Invalid UID");
          return;
      }
    }
    else
    {
      const struct ExpandoRenderData *rd = &rdata[0];
      bool found = false;
      while (rd->did != -1)
      {
        if ((rd->did == node->did) && (rd->uid == node->uid))
        {
          found = true;
          rd->callback(node, data, flags, columns_len, expando_buf);
          break;
        }
        rd++;
      }

      if (!found)
      {
        assert(0 && "Unknown UID");
      }

      printed = format_string(buffer, buffer_len, node, expando_buf);
    }

    columns_len -= mutt_strwidth_nonnull(buffer, buffer + printed);
    buffer_len -= printed;
    buffer += printed;

    node = node->next;
  }

  // give softpad nodes a chance to act
  while (node)
  {
    if (node->type == ENT_PAD)
    {
      struct ExpandoPadPrivate *pp = node->ndata;
      pp->buffer_start = buf;
      pp->buffer_len = buf_len;

      pad_format_callback(node, rdata, buffer, buffer_len, columns_len, data, flags);
    }
    node = node->next;
  }

  buf_pool_release(&expando_buf);

  *buffer = '\0';
}

/**
 * text_format_callback - Callback for every text node
 * @param node     XXX
 * @param rdata    XXX
 * @param buf      XXX
 * @param buf_len  XXX
 * @param cols_len XXX
 * @param data     XXX
 * @param flags    XXX
 * @retval num XXX
 */
int text_format_callback(const struct ExpandoNode *node,
                         const struct ExpandoRenderData *rdata, char *buf,
                         int buf_len, int cols_len, void *data, MuttFormatFlags flags)
{
  assert(node->type == ENT_TEXT);

  int copylen = node->end - node->start;
  memcpy_safe(buf, node->start, copylen, buf_len);

  return copylen;
}

/**
 * is_equal - XXX
 * @param s XXX
 * @param c XXX
 * @retval true XXX
 */
static bool is_equal(const char *s, char c)
{
  // skip color
  if (*s == MUTT_SPECIAL_INDEX)
  {
    s += 2;
  }

  if (*s != c)
  {
    return false;
  }
  s++;

  // skip color
  if (*s == MUTT_SPECIAL_INDEX)
  {
    s += 2;
  }

  if (*s != '\0')
  {
    return false;
  }

  return true;
}

/**
 * conditional_format_callback - Callback for every conditional node
 * @param node     XXX
 * @param rdata    XXX
 * @param buf      XXX
 * @param buf_len  XXX
 * @param cols_len XXX
 * @param data     XXX
 * @param flags    XXX
 * @retval num XXX
 */
int conditional_format_callback(const struct ExpandoNode *node,
                                const struct ExpandoRenderData *rdata, char *buf,
                                int buf_len, int cols_len, void *data, MuttFormatFlags flags)
{
  assert(node->type == ENT_CONDITION);
  assert(node->ndata);
  struct ExpandoConditionPrivate *cp = node->ndata;

  assert(cp->condition);
  assert(cp->if_true_tree);

  char tmp[1024] = { 0 };

  format_tree(cp->condition, rdata, tmp, sizeof(tmp), sizeof(tmp), data, flags);

  /* true if:
    - not 0 (numbers)
    - not empty string (strings)
    - not ' ' (flags)
  */
  if (!is_equal(tmp, '0') && !is_equal(tmp, '\0') && !is_equal(tmp, ' '))
  {
    memset(tmp, 0, sizeof(tmp));
    format_tree(cp->if_true_tree, rdata, tmp, sizeof(tmp), sizeof(tmp), data, flags);

    int copylen = strlen(tmp);
    memcpy_safe(buf, tmp, copylen, buf_len);

    return copylen;
  }
  else
  {
    if (cp->if_false_tree)
    {
      memset(tmp, 0, sizeof(tmp));
      format_tree(cp->if_false_tree, rdata, tmp, sizeof(tmp), sizeof(tmp), data, flags);

      int copylen = strlen(tmp);
      memcpy_safe(buf, tmp, copylen, buf_len);
      return copylen;
    }
    else
    {
      return 0;
    }
  }
  return 0;
}

/**
 * pad_format_fill_eol - XXX
 * @param node     XXX
 * @param rdata    XXX
 * @param buf      XXX
 * @param buf_len  XXX
 * @param cols_len XXX
 * @param data     XXX
 * @param flags    XXX
 * @retval num XXX
 */
int pad_format_fill_eol(const struct ExpandoNode *node,
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
 * pad_format_hard_fill - XXX
 * @param node     XXX
 * @param rdata    XXX
 * @param buf      XXX
 * @param buf_len  XXX
 * @param cols_len XXX
 * @param data     XXX
 * @param flags    XXX
 * @retval num XXX
 */
int pad_format_hard_fill(const struct ExpandoNode *node,
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
 * pad_format_soft_fill - XXX
 * @param node     XXX
 * @param rdata    XXX
 * @param buf      XXX
 * @param buf_len  XXX
 * @param cols_len XXX
 * @param data     XXX
 * @param flags    XXX
 * @retval num XXX
 */
int pad_format_soft_fill(const struct ExpandoNode *node,
                         const struct ExpandoRenderData *rdata, char *buf,
                         int buf_len, int cols_len, void *data, MuttFormatFlags flags)
{
  const int pad_len = node->end - node->start;
  const int pad_width = mutt_strwidth_nonnull(node->start, node->end);

  const struct ExpandoPadPrivate *pp = node->ndata;

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
 * pad_format_callback - Callback for every pad node.
 * @param node     XXX
 * @param rdata    XXX
 * @param buf      XXX
 * @param buf_len  XXX
 * @param cols_len XXX
 * @param data     XXX
 * @param flags    XXX
 * @retval num XXX
 */
int pad_format_callback(const struct ExpandoNode *node,
                        const struct ExpandoRenderData *rdata, char *buf,
                        int buf_len, int cols_len, void *data, MuttFormatFlags flags)
{
  assert(node->type == ENT_PAD);
  assert(node->ndata);

  struct ExpandoPadPrivate *pp = node->ndata;

  switch (pp->pad_type)
  {
    case EPT_FILL_EOL:
      return pad_format_fill_eol(node, rdata, buf, buf_len, cols_len, data, flags);
    case EPT_HARD_FILL:
      return pad_format_hard_fill(node, rdata, buf, buf_len, cols_len, data, flags);
    case EPT_SOFT_FILL:
      return pad_format_soft_fill(node, rdata, buf, buf_len, cols_len, data, flags);
    default:
      assert(0 && "Unknown pad type");
  };

  assert(0 && "Unreachable");
  return 0;
}

/**
 * expando_render - Render an Expando + data into a string
 * @param[in]  cols     Number of screen columns
 * @param[in]  exp      Expando containing the expando tree
 * @param[in]  rdata    Expando render data
 * @param[in]  data     Callback data
 * @param[in]  flags    Callback flags
 * @param[out] buf      Buffer in which to save string
 */
void expando_render(const struct Expando *exp, const struct ExpandoRenderData *rdata,
                    void *data, MuttFormatFlags flags, int cols, struct Buffer *buf)
{
  if (!exp || !exp->tree || !rdata)
  {
    return;
  }

  struct ExpandoNode *root = exp->tree;
  format_tree(root, rdata, buf->data, buf->dsize, cols, data, flags);
}
