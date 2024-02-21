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

#ifndef TEST_EXPANDO_COMMON_H
#define TEST_EXPANDO_COMMON_H

#include "expando/lib.h"

void                check_conditional_date_node(struct ExpandoNode *node, int count, char period);
void                check_condition_node_head  (struct ExpandoNode *node);
void                check_empty_node           (struct ExpandoNode *node);
void                check_expando_node         (struct ExpandoNode *node, const char *expando, const struct ExpandoFormat *fmt_expected);
void                check_pad_node             (struct ExpandoNode *node, const char *pad_char, enum ExpandoPadType pad_type);
void                check_text_node            (struct ExpandoNode *node, const char *text);
struct ExpandoNode *get_nth_node               (struct ExpandoNode *node, int n);

struct ExpandoNode *parse_date(const char *s, const char **parsed_until, int did, int uid, struct ExpandoParseError *error);

#endif /* TEST_EXPANDO_COMMON_H */
