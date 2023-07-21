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

#ifndef MUTT_EXPANDO_HELPERS_H
#define MUTT_EXPANDO_HELPERS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

struct Buffer;
struct ExpandoNode;

int mutt_strwidth_nonnull(const char *start, const char *end);

void memcpy_safe(void *dest, const void *src, size_t len, size_t dest_len);

/**
 * enum HasTreeChars - Signals if the string contains tree characters
 *
 * Characters like: '┌', '┴'.
 * More readable than a simple true / false.
 */
enum HasTreeChars
{
  NO_TREE = 0,    ///< XXX
  HAS_TREE        ///< XXX
};

int format_string(char * buffer, int buffer_len, const struct ExpandoNode *node, struct Buffer *expando_buffer);

bool is_ascii_byte(uint8_t c);
bool is_utf8_2_byte_head(uint8_t c);
bool is_utf8_3_byte_head(uint8_t c);
bool is_utf8_4_byte_head(uint8_t c);
bool is_utf8_cont_byte(uint8_t c);

#endif /* MUTT_EXPANDO_HELPERS_H */
