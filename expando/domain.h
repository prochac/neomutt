/**
 * @file
 * Expando Data Domains
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

#ifndef MUTT_EXPANDO_DOMAIN_H
#define MUTT_EXPANDO_DOMAIN_H

/**
 * ExpandoDomain - Expando Data Domains
 *
 * Each domain has UID associated with it.
 * Their prefixes are shown.
 */
enum ExpandoDomain
{
  ED_ALL = 0,      ///< All           ED_ALL_
  ED_ALIAS,        ///< Alias         ED_ALI_
  ED_ATTACH,       ///< Attach        ED_ATT_
  ED_AUTOCRYPT,    ///< Autocrypt     ED_AUT_
  ED_BODY,         ///< Body          ED_BOD_
  ED_COMPOSE,      ///< Compose       ED_COM_
  ED_COMPRESS,     ///< Compress      ED_CMP_
  ED_EMAIL,        ///< Email         ED_EMA_
  ED_ENVELOPE,     ///< Envelope      ED_ENV_
  ED_FOLDER,       ///< Folder        ED_FOL_
  ED_GLOBAL,       ///< Global        ED_GLO_
  ED_GPGME_KEY,    ///< Gpgme Key     ED_GPG_
  ED_HISTORY,      ///< History       ED_HIS_
  ED_INDEX,        ///< Index         ED_IND_
  ED_MAILBOX,      ///< Mailbox       ED_MBX_
  ED_MENU,         ///< Menu          ED_MEN_
  ED_MIXMASTER,    ///< Mixmaster     ED_MIX_
  ED_NNTP,         ///< Nntp          ED_NTP_
  ED_PATTERN,      ///< Pattern       ED_PAT_
  ED_PGP,          ///< Pgp           ED_PGP_
  ED_PGP_CMD,      ///< Pgp Command   ED_PGC_
  ED_PGP_KEY,      ///< Pgp_Key       ED_PGK_
  ED_SIDEBAR,      ///< Sidebar       ED_SID_
  ED_SMIME_CMD,    ///< Smime Command ED_SMI_
};

#endif /* MUTT_EXPANDO_DOMAIN_H */
