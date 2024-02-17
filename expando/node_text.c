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
 * @page expando_node_text XXX
 *
 * XXX
 */

#include "config.h"
#include <assert.h>
#include "mutt/lib.h"
#include "domain.h"
#include "format_callbacks.h"
#include "helpers.h"
#include "node.h"
#include "uid.h"

/**
 * node_text_new - XXX
 * @param start XXX
 * @param end   XXX
 * @retval ptr XXX
 */
struct ExpandoNode *node_text_new(const char *start, const char *end)
{
  struct ExpandoNode *node = expando_node_new();

  node->type = ENT_TEXT;
  node->start = start;
  node->end = end;

  node->did = ED_ALL;
  node->uid = ED_ALL_TEXT;

  return node;
}

/**
 * skip_until_ch_or_end - XXX
 * @param start      XXX
 * @param terminator XXX
 * @param end        XXX
 * @retval ptr XXX
 */
static const char *skip_until_ch_or_end(const char *start, char terminator, const char *end)
{
  while (*start)
  {
    if (*start == terminator)
    {
      break;
    }

    if (end && (start > (end - 1)))
    {
      break;
    }

    start++;
  }

  return start;
}

/**
 * node_text_parse - XXX
 */
struct ExpandoNode *node_text_parse(const char *s, const char *end, const char **parsed_until)
{
  const char *text_end = skip_until_ch_or_end(s, '%', end);
  *parsed_until = text_end;
  return node_text_new(s, text_end);
}

/**
 * node_text_render - Callback for every text node
 * @param node     XXX
 * @param rdata    XXX
 * @param buf      XXX
 * @param buf_len  XXX
 * @param cols_len XXX
 * @param data     XXX
 * @param flags    XXX
 * @retval num XXX
 */
int node_text_render(const struct ExpandoNode *node,
                     const struct ExpandoRenderData *rdata, char *buf,
                     int buf_len, int cols_len, void *data, MuttFormatFlags flags)
{
  assert(node->type == ENT_TEXT);

  int copylen = MIN(node->end - node->start, cols_len);
  memcpy_safe(buf, node->start, copylen, buf_len);

  return copylen;
}
