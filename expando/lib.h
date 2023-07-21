/**
 * @file
 * Parse Expando string
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
 * @page lib_expando Parse Expando string
 *
 * Parse Expando string
 *
 * | File                              | Description                    |
 * | :-------------------------------- | :----------------------------- |
 * | expando/config_type.c             | @subpage expando_config_type   |
 * | expando/expando.c                 | @subpage expando_expando       |
 * | expando/format_callbacks.c        | @subpage expando_format        |
 * | expando/helpers.c                 | @subpage expando_helpers       |
 * | expando/node.c                    | @subpage expando_node          |
 * | expando/parser.c                  | @subpage expando_parser        |
 * | expando/validation.c              | @subpage expando_validation    |
 */

#ifndef MUTT_EXPANDO_LIB_H
#define MUTT_EXPANDO_LIB_H

// IWYU pragma: begin_keep
#include "domain.h"
#include "expando.h"
#include "format_callbacks.h"
#include "helpers.h"
#include "node.h"
#include "parser.h"
#include "uid.h"
#include "validation.h"
// IWYU pragma: end_keep

struct ConfigSubset;

const struct Expando *cs_subset_expando(const struct ConfigSubset *sub, const char *name);

#endif /* MUTT_EXPANDO_LIB_H */
