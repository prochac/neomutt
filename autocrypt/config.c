/**
 * @file
 * Config used by libautocrypt
 *
 * @authors
 * Copyright (C) 2020-2024 Richard Russon <rich@flatcap.org>
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
 * @page autocrypt_config Autocrypt Config
 *
 * Config used by libautocrypt
 */

#include "config.h"
#include <stddef.h>
#include <stdbool.h>
#include "private.h"
#include "config/lib.h"
#include "expando/lib.h"

// clang-format off
char *AutocryptSignAs = NULL;     ///< Autocrypt Key id to sign as
char *AutocryptDefaultKey = NULL; ///< Autocrypt default key id (used for postponing messages)
// clang-format on

/**
 * AutocryptFormatData - Expando definitions
 *
 * Config:
 * - $autocrypt_acct_format
 */
static struct ExpandoDefinition AutocryptFormatData[] = {
  // clang-format off
  { "a", "mailbox",        ED_AUTOCRYPT, ED_AUT_MAILBOX,        E_TYPE_STRING, E_FLAGS_NO_FLAGS, NULL },
  { "k", "keyid",          ED_AUTOCRYPT, ED_AUT_KEYID,          E_TYPE_STRING, E_FLAGS_NO_FLAGS, NULL },
  { "n", "number",         ED_AUTOCRYPT, ED_AUT_NUMBER,         E_TYPE_NUMBER, E_FLAGS_NO_FLAGS, NULL },
  { "p", "prefer-encrypt", ED_AUTOCRYPT, ED_AUT_PREFER_ENCRYPT, E_TYPE_STRING, E_FLAGS_NO_FLAGS, NULL },
  { "s", "enabled",        ED_AUTOCRYPT, ED_AUT_ENABLED,        E_TYPE_STRING, E_FLAGS_NO_FLAGS, NULL },
  { NULL, NULL, 0, -1, -1, 0, NULL }
  // clang-format on
};

/**
 * AutocryptVars - Config definitions for the autocrypt library
 */
static struct ConfigDef AutocryptVars[] = {
  // clang-format off
  { "autocrypt", DT_BOOL, false, 0, NULL,
    "Enables the Autocrypt feature"
  },
  { "autocrypt_acct_format", DT_EXPANDO, IP "%4n %-30a %20p %10s", IP &AutocryptFormatData, NULL,
    "Format of the autocrypt account menu"
  },
  { "autocrypt_dir", DT_PATH|D_PATH_DIR, IP "~/.mutt/autocrypt", 0, NULL,
    "Location of autocrypt files, including the GPG keyring and SQLite database"
  },
  { "autocrypt_reply", DT_BOOL, true, 0, NULL,
    "Replying to an autocrypt email automatically enables autocrypt in the reply"
  },
  { NULL },
  // clang-format on
};

/**
 * config_init_autocrypt - Register autocrypt config variables - Implements ::module_init_config_t - @ingroup cfg_module_api
 */
bool config_init_autocrypt(struct ConfigSet *cs)
{
  bool rc = false;

#if defined(USE_AUTOCRYPT)
  rc |= cs_register_variables(cs, AutocryptVars);
#endif

  return rc;
}
