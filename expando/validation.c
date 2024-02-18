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
 * @page expando_validation XXX
 *
 * XXX
 */

#include "config.h"
#include <stddef.h>
#include <stdbool.h>
#include "mutt/lib.h"
#include "validation.h"
#include "expando.h"
#include "parser.h"

/**
 * expando_parse - Parse an Expando string
 * @param str  String to parse
 * @param defs Data defining Expando
 * @param err  Buffer for error messages
 * @retval ptr New Expando
 */
struct Expando *expando_parse(const char *str, const struct ExpandoDefinition *defs,
                              struct Buffer *err)
{
  if (!str || !defs)
    return NULL;

  struct Expando *exp = expando_new(str);

  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, exp->string, defs, &error);

  if (error.position)
  {
    buf_strcpy(err, error.message);
    expando_free(&exp);
    return NULL;
  }

  exp->tree = root;
  return exp;
}

/**
 * expando_equal - Compare two expandos
 * @param a First  Expando
 * @param b Second Expando
 * @retval true They are identical
 */
bool expando_equal(const struct Expando *a, const struct Expando *b)
{
  if (!a && !b) /* both empty */
    return true;
  if (!a ^ !b) /* one is empty, but not the other */
    return false;

  return mutt_str_equal(a->string, b->string);
}
