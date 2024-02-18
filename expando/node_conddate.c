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
 * @page expando_node_conddate XXX
 *
 * XXX
 */

#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mutt/lib.h"
#include "node_conddate.h"
#include "node.h"
#include "parser.h"

/**
 * node_conddate_private_new - XXX
 */
static struct NodeCondDatePrivate *node_conddate_private_new(int count, char period, time_t multiplier)
{
  struct NodeCondDatePrivate *priv = mutt_mem_calloc(1, sizeof(struct NodeCondDatePrivate));

  priv->count = count;
  priv->period = period;
  priv->multiplier = multiplier;

  return priv;
}

/**
 * node_conddate_private_free - XXX
 * @param ptr XXX
 */
static void node_conddate_private_free(void **ptr)
{
  FREE(ptr);
}

/**
 * node_conddate_new - XXX
 * @param count      XXX
 * @param period     XXX
 * @param multiplier XXX
 * @param did        XXX
 * @param uid        XXX
 * @retval ptr XXX
 */
static struct ExpandoNode *node_conddate_new(int count, char period,
                                             time_t multiplier, int did, int uid)
{
  struct ExpandoNode *node = expando_node_new();
  node->type = ENT_CONDDATE;
  node->did = did;
  node->uid = uid;

  node->ndata = node_conddate_private_new(count, period, multiplier);
  node->ndata_free = node_conddate_private_free;

  return node;
}

/**
 * node_conddate_parse - XXX
 * @param s            XXX
 * @param parsed_until XXX
 * @param did          XXX
 * @param uid          XXX
 * @param error        XXX
 * @retval ptr XXX
 */
struct ExpandoNode *node_conddate_parse(const char *s, const char **parsed_until,
                                        int did, int uid, struct ExpandoParseError *error)
{
  int count = 1;
  char period = 0;
  time_t multiplier = 0;

  if (isdigit(*s))
  {
    char *end_ptr;
    int number = strtol(s, &end_ptr, 10);

    // NOTE(g0mb4): start is NOT null-terminated
    if (!end_ptr)
    {
      error->position = s;
      snprintf(error->message, sizeof(error->message), "Wrong number");
      return NULL;
    }

    count = number;
    s = end_ptr;
  };

  switch (*s)
  {
    case 'y':
      period = *s;
      multiplier = 60 * 60 * 24 * 365;
      break;

    case 'm':
      // NOTE(g0mb4): semi-broken (assuming 30 days in all months)
      period = *s;
      multiplier = 60 * 60 * 24 * 30;
      break;

    case 'w':
      period = *s;
      multiplier = 60 * 60 * 24 * 7;
      break;

    case 'd':
      period = *s;
      multiplier = 60 * 60 * 24;
      break;

    case 'H':
      period = *s;
      multiplier = 60 * 60;
      break;

    case 'M':
      period = *s;
      multiplier = 60;
      break;

    default:
    {
      error->position = s;
      snprintf(error->message, sizeof(error->message), "Wrong period: `%c`", *s);
      return NULL;
    }
  }

  *parsed_until = s + 1;

  return node_conddate_new(count, period, multiplier, did, uid);
}
