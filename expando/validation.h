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

#ifndef MUTT_EXPANDO_VALIDATION_H
#define MUTT_EXPANDO_VALIDATION_H

#include <stdbool.h>
#include <stdint.h>

struct Buffer;
struct Expando;
struct ExpandoParseError;

/**
 * enum ExpandoDataType - XXX
 */
enum ExpandoDataType
{
  E_TYPE_STRING = 0,    ///< XXX
  E_TYPE_NUMBER,        ///< XXX
};

typedef uint8_t ExpandoFlags;             ///< Flags for Expando definitions
#define E_FLAGS_NO_FLAGS               0  ///< No flags are set
#define E_FLAGS_OPTIONAL         (1 << 0) ///< Expando can be used in conditionals
#define E_FLAGS_BOOL             (1 << 1) ///< Could be boolean

/**
 * @defgroup expando_parser_api Expando Parser API
 *
 * Prototype for a custom Expando parser function
 *
 * @param s            XXX
 * @param parsed_until XXX
 * @param did          XXX
 * @param uid          XXX
 * @param error        XXX
 * @retval ptr XXX
 */
typedef struct ExpandoNode *(*expando_parser_t)(const char *s, const char **parsed_until, int did, int uid, struct ExpandoParseError *error);

/**
 * struct ExpandoDefinition - XXX
 */
struct ExpandoDefinition
{
  const char           *short_name;      ///< XXX
  const char           *long_name;       ///< XXX
  short                 did;             ///< Domain ID
  short                 uid;             ///< Unique ID in domain
  enum ExpandoDataType  data_type;       ///< XXX
  ExpandoFlags          flags;           ///< XXX
  expando_parser_t      custom_parser;   ///< XXX
};

bool expando_equal(const struct Expando *a, const struct Expando *b);
struct Expando *expando_parse(const char *str, const struct ExpandoDefinition *defs, struct Buffer *err);

#endif /* MUTT_EXPANDO_VALIDATION_H */
