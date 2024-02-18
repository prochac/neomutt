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
 * @page expando_helpers XXX
 *
 * XXX
 */

#include "config.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "mutt/lib.h"
#include "gui/lib.h"
#include "helpers.h"
#include "color/lib.h"
#include "mutt_thread.h"
#include "node.h"
#include "node_expando.h"

/**
 * memcpy_safe - XXX
 * @param dest     XXX
 * @param src      XXX
 * @param len      XXX
 * @param dest_len XXX
 */
void memcpy_safe(void *dest, const void *src, size_t len, size_t dest_len)
{
  if (!src || !dest || (len == 0) || (dest_len == 0))
    return;

  if (len > dest_len)
    len = dest_len;

  memcpy(dest, src, len);
}

/**
 * mutt_strwidth_nonnull - Measure a non null-terminated string's display width (in screen columns)
 * @param start Start of the string
 * @param end   End of the string
 * @return num Number of columns the string requires
 */
int mutt_strwidth_nonnull(const char *start, const char *end)
{
  assert(end >= start);

  char tmp[2048] = { 0 };
  const size_t len = end - start;

  assert(len < sizeof(tmp));
  memcpy(tmp, start, len);

  return mutt_strwidth(tmp);
}

/**
 * add_color - XXX
 */
static int add_color(char *buf, enum ColorId color)
{
  assert(color < MT_COLOR_MAX);

  buf[0] = MUTT_SPECIAL_INDEX;
  buf[1] = color;

  return 2;
}

/**
 * format_string - XXX
 */
int format_string(char *buffer, int buffer_len, const struct ExpandoNode *node,
                  struct Buffer *expando_buffer)
{
  const size_t expando_buffer_len = buf_len(expando_buffer);

  assert(buffer_len > 0);
  assert((size_t) buffer_len > expando_buffer_len);

  const struct NodeExpandoPrivate *p = node && node->type == ENT_EXPANDO ? node->ndata : NULL;
  const struct ExpandoFormatPrivate *format = p ? p->format : NULL;
  const int color = p ? p->color : -1;
  const bool has_tree = p ? p->has_tree : false;

  int printed = 0;

  if (color > -1)
  {
    const int n = add_color(buffer, color);

    buffer += n;
    buffer_len -= n;
    printed += n;
  }

  if (!buf_is_empty(expando_buffer))
  {
    if (format)
    {
      char tmp[1024];
      mutt_simple_format(tmp, sizeof(tmp), format->min, format->max,
                         format->justification, format->leader,
                         buf_string(expando_buffer), expando_buffer_len, has_tree);

      const int tmp_len = mutt_str_len(tmp);
      mutt_str_copy(buffer, tmp, buffer_len);
      buffer += tmp_len;
      printed += tmp_len;
    }
    else
    {
      mutt_strn_copy(buffer, buf_string(expando_buffer), expando_buffer_len, buffer_len);
      buffer += expando_buffer_len;
      printed += expando_buffer_len;
    }
  }

  if (color > -1)
  {
    const int n = add_color(buffer, MT_COLOR_INDEX);

    buffer += n;
    buffer_len -= n;
    printed += n;
  }

  return printed;
}
