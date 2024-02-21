/**
 * @file
 * Expando Data UIDs
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

#ifndef MUTT_EXPANDO_UID_H
#define MUTT_EXPANDO_UID_H

/**
 * ExpandoDataGlobal - Expando UIDs for Global
 *
 * UIDs used by the Global Domain
 * @sa #ED_GLOBAL
 */
enum ExpandoDataGlobal
{
  ED_GLO_ARROW = 1,            ///< XXX
  ED_GLO_CERTIFICATE_PATH,     ///< XXX
  ED_GLO_HOSTNAME,             ///< XXX
  ED_GLO_PADDING_EOL,          ///< XXX
  ED_GLO_PADDING_HARD,         ///< XXX
  ED_GLO_PADDING_SOFT,         ///< XXX
  ED_GLO_SORT,                 ///< XXX
  ED_GLO_SORT_AUX,             ///< XXX
  ED_GLO_USE_THREADS,          ///< XXX
  ED_GLO_VERSION,              ///< XXX
};

/**
 * ExpandoDataAll - UIDs used for all domains
 *
 * UIDs used by the All Domain
 * @sa #ED_ALL
 */
enum ExpandoDataAll
{
  ED_ALL_EMPTY = 0,                ///< XXX
  ED_ALL_TEXT,                     ///< XXX
  ED_ALL_PAD,                      ///< XXX
  ED_ALL_CONDITION,                ///< XXX
  ED_ALL_CONDITIONAL_DATE_HEAD,    ///< XXX
};

#endif /* MUTT_EXPANDO_UID_H */
