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

#ifndef MUTT_EXPANDO_FORMAT_CALLBACKS_H
#define MUTT_EXPANDO_FORMAT_CALLBACKS_H

#include <stddef.h>
#include <stdint.h>

struct Buffer;
struct Expando;
struct ExpandoNode;

typedef uint8_t MuttFormatFlags;         ///< Flags for expando_render(), e.g. #MUTT_FORMAT_FORCESUBJ
#define MUTT_FORMAT_NO_FLAGS          0  ///< No flags are set
#define MUTT_FORMAT_FORCESUBJ   (1 << 0) ///< Print the subject even if unchanged
#define MUTT_FORMAT_TREE        (1 << 1) ///< Draw the thread tree
#define MUTT_FORMAT_STAT_FILE   (1 << 2) ///< Used by attach_format_str
#define MUTT_FORMAT_ARROWCURSOR (1 << 3) ///< Reserve space for arrow_cursor
#define MUTT_FORMAT_INDEX       (1 << 4) ///< This is a main index entry
#define MUTT_FORMAT_PLAIN       (1 << 5) ///< Do not prepend DISP_TO, DISP_CC ...

/**
 * @defgroup expando_callback_api Expando Callback API
 *
 * Prototype for an Expando callback function
 *
 * @param[in]  node      ExpandoNode containing the callback
 * @param[in]  data      Private data
 * @param[in]  flags     Flags, see #MuttFormatFlags
 * @param[in]  max_width Maximum width in screen columns
 * @param[out] buf       Buffer in which to save string
 *
 * Each callback function implements some expandos, e.g.
 *
 * | Expando | Description
 * | :------ | :----------
 * | \%t     | Title
 */
typedef void (*expando_callback_t)(const struct ExpandoNode *node, void *data, MuttFormatFlags flags, int max_width, struct Buffer *buf);

/**
 * ExpandoRenderData - XXX
 */
struct ExpandoRenderData
{
  int did;                       ///< Domain ID
  int uid;                       ///< Unique ID
  expando_callback_t callback;   ///< Function to get data
};

void format_tree(struct ExpandoNode **tree, const struct ExpandoRenderData *rdata, char *buf, size_t buf_len,
                 size_t col_len, void *data, MuttFormatFlags flags);

int text_format_callback(const struct ExpandoNode *node,
                         const struct ExpandoRenderData *rdata, char *buf, int buf_len,
                         int cols_len, void *data, MuttFormatFlags flags);

int conditional_format_callback(const struct ExpandoNode *node, const struct ExpandoRenderData *rdata, char *buf,
                                int buf_len, int cols_len, void *data, MuttFormatFlags flags);

int pad_format_callback(const struct ExpandoNode *node, const struct ExpandoRenderData *rdata, char *buf,
                         int buf_len, int cols_len, void *data, MuttFormatFlags flags);

void expando_render(const struct Expando *exp, const struct ExpandoRenderData *rdata,
                    void *data, MuttFormatFlags flags, int cols, struct Buffer *buf);

int pad_format_fill_eol (const struct ExpandoNode *node, const struct ExpandoRenderData *rdata, char *buf, int buf_len, int cols_len, void *data, MuttFormatFlags flags);
int pad_format_hard_fill(const struct ExpandoNode *node, const struct ExpandoRenderData *rdata, char *buf, int buf_len, int cols_len, void *data, MuttFormatFlags flags);
int pad_format_soft_fill(const struct ExpandoNode *node, const struct ExpandoRenderData *rdata, char *buf, int buf_len, int cols_len, void *data, MuttFormatFlags flags);

#endif /* MUTT_EXPANDO_FORMAT_CALLBACKS_H */
