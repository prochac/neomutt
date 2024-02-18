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

#ifndef MUTT_EXPANDO_NODE_CONDDATE_H
#define MUTT_EXPANDO_NODE_CONDDATE_H

#include <time.h>

struct ExpandoParseError;

/**
 * struct NodeCondDatePrivate - XXX
 */
struct NodeCondDatePrivate
{
  int    count;         ///< XXX
  char   period;        ///< XXX
  time_t multiplier;    ///< XXX
};

struct ExpandoNode *node_conddate_parse(const char *s, const char **parsed_until, int did, int uid, struct ExpandoParseError *error);

#endif /* MUTT_EXPANDO_NODE_CONDDATE_H */
