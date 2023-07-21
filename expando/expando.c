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
 * @page expando_expando XXX
 *
 * XXX
 */

#include "config.h"
#include "mutt/lib.h"
#include "expando.h"
#include "parser.h"

/**
 * expando_new - Create an Expando from a string
 * @param format Format string to parse
 * @retval ptr New Expando object
 */
struct Expando *expando_new(const char *format)
{
  struct Expando *exp = mutt_mem_calloc(1, sizeof(struct Expando));
  exp->string = mutt_str_dup(format);
  return exp;
}

/**
 * expando_free - Free an Expando object
 * @param[out] ptr Expando to free
 */
void expando_free(struct Expando **ptr)
{
  if (!ptr || !*ptr)
    return;

  struct Expando *exp = *ptr;

  expando_tree_free(&exp->tree);
  FREE(&exp->string);

  FREE(ptr);
}
