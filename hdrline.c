/**
 * @file
 * String processing routines to generate the mail index
 *
 * @authors
 * Copyright (C) 1996-2000,2002,2007 Michael R. Elkins <me@mutt.org>
 * Copyright (C) 2016 Ian Zimmerman <itz@primate.net>
 * Copyright (C) 2016-2024 Richard Russon <rich@flatcap.org>
 * Copyright (C) 2017 Stefan Assmann <sassmann@kpanic.de>
 * Copyright (C) 2019 Victor Fernandes <criw@pm.me>
 * Copyright (C) 2019-2023 Pietro Cerutti <gahr@gahr.ch>
 * Copyright (C) 2021 Ashish Panigrahi <ashish.panigrahi@protonmail.com>
 * Copyright (C) 2023-2024 Tóth János <gomba007@gmail.com>
 * Copyright (C) 2023 наб <nabijaczleweli@nabijaczleweli.xyz>
 * Copyright (C) 2024 Dennis Schön <mail@dennis-schoen.de>
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
 * @page neo_hdrline String processing routines to generate the mail index
 *
 * String processing routines to generate the mail index
 */

#include "config.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mutt/lib.h"
#include "address/lib.h"
#include "config/lib.h"
#include "email/lib.h"
#include "core/lib.h"
#include "alias/lib.h"
#include "attach/lib.h"
#include "color/lib.h"
#include "expando/lib.h"
#include "ncrypt/lib.h"
#include "hdrline.h"
#include "hook.h"
#include "maillist.h"
#include "mutt_thread.h"
#include "muttlib.h"
#include "mx.h"
#include "sort.h"
#include "subjectrx.h"
#ifdef USE_NOTMUCH
#include "notmuch/lib.h"
#endif

/**
 * struct HdrFormatInfo - Data passed to index_format_str()
 */
struct HdrFormatInfo
{
  struct Mailbox *mailbox;    ///< Current Mailbox
  int msg_in_pager;           ///< Index of Email displayed in the Pager
  struct Email *email;        ///< Current Email
  const char *pager_progress; ///< String representing Pager position through Email
  bool show_arrow;            ///< XXX
};

/**
 * enum FieldType - Header types
 *
 * Strings for printing headers
 */
enum FieldType
{
  DISP_TO,    ///< To: string
  DISP_CC,    ///< Cc: string
  DISP_BCC,   ///< Bcc: string
  DISP_FROM,  ///< From: string
  DISP_PLAIN, ///< Empty string
  DISP_MAX,
};

extern const struct ExpandoRenderData IndexRenderData[];

/**
 * make_from_prefix - Create a prefix for an author field
 * @param disp   Type of field
 * @retval ptr Prefix string (do not free it)
 *
 * If $from_chars is set, pick an appropriate character from it.
 * If not, use the default prefix: "To", "Cc", etc
 */
static const char *make_from_prefix(enum FieldType disp)
{
  /* need 2 bytes at the end, one for the space, another for NUL */
  static char padded[8];
  static const char *long_prefixes[DISP_MAX] = {
    [DISP_TO] = "To ", [DISP_CC] = "Cc ", [DISP_BCC] = "Bcc ",
    [DISP_FROM] = "",  [DISP_PLAIN] = "",
  };

  const struct MbTable *c_from_chars = cs_subset_mbtable(NeoMutt->sub, "from_chars");

  if (!c_from_chars || !c_from_chars->chars || (c_from_chars->len == 0))
    return long_prefixes[disp];

  const char *pchar = mbtable_get_nth_wchar(c_from_chars, disp);
  if (mutt_str_len(pchar) == 0)
    return "";

  snprintf(padded, sizeof(padded), "%s ", pchar);
  return padded;
}

/**
 * make_from - Generate a From: field (with optional prefix)
 * @param env      Envelope of the email
 * @param buf      Buffer to store the result
 * @param buflen   Size of the buffer
 * @param do_lists Should we check for mailing lists?
 * @param flags    Format flags, see #MuttFormatFlags
 *
 * Generate the %F or %L field in $index_format.
 * This is the author, or recipient of the email.
 *
 * The field can optionally be prefixed by a character from $from_chars.
 * If $from_chars is not set, the prefix will be, "To", "Cc", etc
 */
static void make_from(struct Envelope *env, char *buf, size_t buflen,
                      bool do_lists, MuttFormatFlags flags)
{
  if (!env || !buf)
    return;

  bool me;
  enum FieldType disp;
  struct AddressList *name = NULL;

  me = mutt_addr_is_user(TAILQ_FIRST(&env->from));

  if (do_lists || me)
  {
    if (check_for_mailing_list(&env->to, make_from_prefix(DISP_TO), buf, buflen))
      return;
    if (check_for_mailing_list(&env->cc, make_from_prefix(DISP_CC), buf, buflen))
      return;
  }

  if (me && !TAILQ_EMPTY(&env->to))
  {
    disp = (flags & MUTT_FORMAT_PLAIN) ? DISP_PLAIN : DISP_TO;
    name = &env->to;
  }
  else if (me && !TAILQ_EMPTY(&env->cc))
  {
    disp = DISP_CC;
    name = &env->cc;
  }
  else if (me && !TAILQ_EMPTY(&env->bcc))
  {
    disp = DISP_BCC;
    name = &env->bcc;
  }
  else if (!TAILQ_EMPTY(&env->from))
  {
    disp = DISP_FROM;
    name = &env->from;
  }
  else
  {
    *buf = '\0';
    return;
  }

  snprintf(buf, buflen, "%s%s", make_from_prefix(disp), mutt_get_name(TAILQ_FIRST(name)));
}

/**
 * make_from_addr - Create a 'from' address for a reply email
 * @param env      Envelope of current email
 * @param buf      Buffer for the result
 * @param buflen   Length of buffer
 * @param do_lists If true, check for mailing lists
 */
static void make_from_addr(struct Envelope *env, char *buf, size_t buflen, bool do_lists)
{
  if (!env || !buf)
    return;

  bool me = mutt_addr_is_user(TAILQ_FIRST(&env->from));

  if (do_lists || me)
  {
    if (check_for_mailing_list_addr(&env->to, buf, buflen))
      return;
    if (check_for_mailing_list_addr(&env->cc, buf, buflen))
      return;
  }

  if (me && !TAILQ_EMPTY(&env->to))
    snprintf(buf, buflen, "%s", buf_string(TAILQ_FIRST(&env->to)->mailbox));
  else if (me && !TAILQ_EMPTY(&env->cc))
    snprintf(buf, buflen, "%s", buf_string(TAILQ_FIRST(&env->cc)->mailbox));
  else if (!TAILQ_EMPTY(&env->from))
    mutt_str_copy(buf, buf_string(TAILQ_FIRST(&env->from)->mailbox), buflen);
  else
    *buf = '\0';
}

/**
 * user_in_addr - Do any of the addresses refer to the user?
 * @param al AddressList
 * @retval true Any of the addresses match one of the user's addresses
 */
static bool user_in_addr(struct AddressList *al)
{
  struct Address *a = NULL;
  TAILQ_FOREACH(a, al, entries)
  if (mutt_addr_is_user(a))
    return true;
  return false;
}

/**
 * user_is_recipient - Is the user a recipient of the message
 * @param e Email to test
 * @retval enum Character index into the `$to_chars` config variable
 */
static enum ToChars user_is_recipient(struct Email *e)
{
  if (!e || !e->env)
    return FLAG_CHAR_TO_NOT_IN_THE_LIST;

  struct Envelope *env = e->env;

  if (!e->recip_valid)
  {
    e->recip_valid = true;

    if (mutt_addr_is_user(TAILQ_FIRST(&env->from)))
    {
      e->recipient = FLAG_CHAR_TO_ORIGINATOR;
    }
    else if (user_in_addr(&env->to))
    {
      if (TAILQ_NEXT(TAILQ_FIRST(&env->to), entries) || !TAILQ_EMPTY(&env->cc))
        e->recipient = FLAG_CHAR_TO_TO; /* non-unique recipient */
      else
        e->recipient = FLAG_CHAR_TO_UNIQUE; /* unique recipient */
    }
    else if (user_in_addr(&env->cc))
    {
      e->recipient = FLAG_CHAR_TO_CC;
    }
    else if (check_for_mailing_list(&env->to, NULL, NULL, 0))
    {
      e->recipient = FLAG_CHAR_TO_SUBSCRIBED_LIST;
    }
    else if (check_for_mailing_list(&env->cc, NULL, NULL, 0))
    {
      e->recipient = FLAG_CHAR_TO_SUBSCRIBED_LIST;
    }
    else if (user_in_addr(&env->reply_to))
    {
      e->recipient = FLAG_CHAR_TO_REPLY_TO;
    }
    else
    {
      e->recipient = FLAG_CHAR_TO_NOT_IN_THE_LIST;
    }
  }

  return e->recipient;
}

/**
 * thread_is_new - Does the email thread contain any new emails?
 * @param e Email
 * @retval true Thread contains new mail
 */
static bool thread_is_new(struct Email *e)
{
  return e->collapsed && (e->num_hidden > 1) && (mutt_thread_contains_unread(e) == 1);
}

/**
 * thread_is_old - Does the email thread contain any unread emails?
 * @param e Email
 * @retval true Thread contains unread mail
 */
static bool thread_is_old(struct Email *e)
{
  return e->collapsed && (e->num_hidden > 1) && (mutt_thread_contains_unread(e) == 2);
}

/**
 * index_date_recv_local - Index: Local received date and time - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_date_recv_local(const struct ExpandoNode *node, void *data,
                           MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert((node->type == ENT_EXPANDO) || (node->type == ENT_CONDDATE));

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;
  struct tm tm = mutt_date_localtime(e->received);

  char tmp[128] = { 0 };
  char tmp2[128] = { 0 };

  if (node->type == ENT_EXPANDO)
  {
    int len = node->end - node->start;
    const char *start = node->start;

    bool use_c_locale = false;
    if (*start == '!')
    {
      use_c_locale = true;
      start++;
      len--;
    }
    assert(len < sizeof(tmp2));
    mutt_strn_copy(tmp2, start, len, sizeof(tmp2));

    if (use_c_locale)
    {
      strftime_l(tmp, sizeof(tmp), tmp2, &tm, NeoMutt->time_c_locale);
    }
    else
    {
      strftime(tmp, sizeof(tmp), tmp2, &tm);
    }

    if (flags & MUTT_FORMAT_INDEX)
      node_expando_set_color(node, MT_COLOR_INDEX_DATE);
    buf_strcpy(buf, tmp);
  }
  else
  {
    assert(node->ndata);

    const struct NodeCondDatePrivate *priv = node->ndata;

    time_t t = mutt_date_now();
    t -= priv->count * priv->multiplier;
    struct tm condition = mutt_date_localtime(t);

    const time_t condt = mktime(&condition);
    const time_t checkt = mktime(&tm);

    const int num = checkt > condt;
    buf_printf(buf, "%d", num);
  }
}

/**
 * index_date_local - Index: Local date and time - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_date_local(const struct ExpandoNode *node, void *data,
                      MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert((node->type == ENT_EXPANDO) || (node->type == ENT_CONDDATE));

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;
  struct tm tm = mutt_date_localtime(e->date_sent);

  char tmp[128] = { 0 };
  char tmp2[128] = { 0 };

  if (node->type == ENT_EXPANDO)
  {
    int len = node->end - node->start;
    const char *start = node->start;

    bool use_c_locale = false;
    if (*start == '!')
    {
      use_c_locale = true;
      start++;
      len--;
    }
    assert(len < sizeof(tmp2));
    mutt_strn_copy(tmp2, start, len, sizeof(tmp2));

    if (use_c_locale)
    {
      strftime_l(tmp, sizeof(tmp), tmp2, &tm, NeoMutt->time_c_locale);
    }
    else
    {
      strftime(tmp, sizeof(tmp), tmp2, &tm);
    }

    if (flags & MUTT_FORMAT_INDEX)
      node_expando_set_color(node, MT_COLOR_INDEX_DATE);
    buf_strcpy(buf, tmp);
  }
  else
  {
    assert(node->ndata);

    const struct NodeCondDatePrivate *priv = node->ndata;

    time_t t = mutt_date_now();
    t -= priv->count * priv->multiplier;
    struct tm condition = mutt_date_localtime(t);

    const time_t condt = mktime(&condition);
    const time_t checkt = mktime(&tm);

    const int num = checkt > condt;
    buf_printf(buf, "%d", num);
  }
}

/**
 * index_date - Index: Sender's date and time - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_date(const struct ExpandoNode *node, void *data,
                MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert((node->type == ENT_EXPANDO) || (node->type == ENT_CONDDATE));

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  time_t now = e->date_sent;
  if (e->zoccident)
    now -= (e->zhours * 3600 + e->zminutes * 60);
  else
    now += (e->zhours * 3600 + e->zminutes * 60);

  struct tm tm = mutt_date_gmtime(now);

  char tmp[128] = { 0 };
  char tmp2[128] = { 0 };

  if (node->type == ENT_EXPANDO)
  {
    int len = node->end - node->start;
    const char *start = node->start;

    bool use_c_locale = false;
    if (*start == '!')
    {
      use_c_locale = true;
      start++;
      len--;
    }
    assert(len < sizeof(tmp2));
    mutt_strn_copy(tmp2, start, len, sizeof(tmp2));

    if (use_c_locale)
    {
      strftime_l(tmp, sizeof(tmp), tmp2, &tm, NeoMutt->time_c_locale);
    }
    else
    {
      strftime(tmp, sizeof(tmp), tmp2, &tm);
    }

    if (flags & MUTT_FORMAT_INDEX)
      node_expando_set_color(node, MT_COLOR_INDEX_DATE);
    buf_strcpy(buf, tmp);
  }
  else
  {
    assert(node->ndata);

    const struct NodeCondDatePrivate *priv = node->ndata;

    time_t t = mutt_date_now();
    t -= priv->count * priv->multiplier;

    if (e->zoccident)
      t -= (e->zhours * 3600 + e->zminutes * 60);
    else
      t += (e->zhours * 3600 + e->zminutes * 60);

    struct tm condition = mutt_date_gmtime(t);

    const time_t condt = mktime(&condition);
    const time_t checkt = mktime(&tm);

    const int num = checkt > condt;
    buf_printf(buf, "%d", num);
  }
}

/**
 * index_format_hook - Index: index-format-hook - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_format_hook(const struct ExpandoNode *node, void *data,
                       MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;
  struct Mailbox *m = hfi->mailbox;

  char tmp[128] = { 0 };
  const int len = node->end - node->start;

  mutt_strn_copy(tmp, node->start, len, sizeof(tmp));

  const struct Expando *exp = mutt_idxfmt_hook(tmp, m, e);
  if (!exp)
  {
    buf_reset(buf);
    return;
  }

  expando_render(exp, IndexRenderData, data, MUTT_FORMAT_NO_FLAGS, buf->dsize, buf);
}

/**
 * index_a - Index: Author Address - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_a(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  const struct Address *from = TAILQ_FIRST(&e->env->from);

  const char *s = NULL;
  if (from && from->mailbox)
  {
    s = mutt_addr_for_display(from);
  }

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_AUTHOR);
  buf_strcpy(buf, NONULL(s));
}

/**
 * index_A - Index: Reply-to address - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_A(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  const struct Address *reply_to = TAILQ_FIRST(&e->env->reply_to);

  if (reply_to && reply_to->mailbox)
  {
    if (flags & MUTT_FORMAT_INDEX)
      node_expando_set_color(node, MT_COLOR_INDEX_AUTHOR);
    const char *s = mutt_addr_for_display(reply_to);
    buf_strcpy(buf, NONULL(s));
    return;
  }

  index_a(node, data, flags, max_width, buf);
}

/**
 * index_b - Index: Filename - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_b(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Mailbox *m = hfi->mailbox;

  char tmp[128] = { 0 };
  char *p = NULL;

  if (m)
  {
    p = strrchr(mailbox_path(m), '/');

#ifdef USE_NOTMUCH
    struct Email *e = hfi->email;
    if (m->type == MUTT_NOTMUCH)
    {
      char *rel_path = nm_email_get_folder_rel_db(m, e);
      if (rel_path)
      {
        p = rel_path;
      }
    }
#endif /* USE_NOTMUCH */

    if (p)
    {
      mutt_str_copy(tmp, p + 1, sizeof(tmp));
    }
    else
    {
      mutt_str_copy(tmp, mailbox_path(m), sizeof(tmp));
    }
  }
  else
  {
    mutt_str_copy(tmp, "(null)", sizeof(tmp));
  }

  buf_strcpy(buf, tmp);
}

/**
 * index_B - Index: Email list - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_B(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  char tmp[128] = { 0 };

  if (first_mailing_list(tmp, sizeof(tmp), &e->env->to) ||
      first_mailing_list(tmp, sizeof(tmp), &e->env->cc))
  {
    buf_strcpy(buf, tmp);
    return;
  }

  index_b(node, data, flags, max_width, buf);
}

/**
 * index_c - Index: Number of bytes - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_c(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  char tmp[128] = { 0 };

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_SIZE);

  mutt_str_pretty_size(tmp, sizeof(tmp), e->body->length);
  buf_strcpy(buf, tmp);
}

/**
 * index_cr - Index: Number of raw bytes - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_cr(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  const struct Email *e = hfi->email;

  char tmp[128] = { 0 };

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_SIZE);

  mutt_str_pretty_size(tmp, sizeof(tmp), email_size(e));
  buf_strcpy(buf, tmp);
}

/**
 * index_C - Index: Index number - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_C(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_NUMBER);

  const int num = e->msgno + 1;
  buf_printf(buf, "%d", num);
}

/**
 * index_d - Index: Senders Date and time - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_d(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  const char *c_date_format = cs_subset_string(NeoMutt->sub, "date_format");
  const char *cp = NONULL(c_date_format);
  bool use_c_locale = false;
  if (*cp == '!')
  {
    use_c_locale = true;
    cp++;
  }

  /* restore sender's time zone */
  time_t now = e->date_sent;
  if (e->zoccident)
    now -= (e->zhours * 3600 + e->zminutes * 60);
  else
    now += (e->zhours * 3600 + e->zminutes * 60);

  struct tm tm = mutt_date_gmtime(now);
  char tmp[128] = { 0 };

  if (use_c_locale)
  {
    strftime_l(tmp, sizeof(tmp), cp, &tm, NeoMutt->time_c_locale);
  }
  else
  {
    strftime(tmp, sizeof(tmp), cp, &tm);
  }

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_DATE);

  buf_strcpy(buf, tmp);
}

/**
 * index_D - Index: Local Date and time - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_D(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  const char *c_date_format = cs_subset_string(NeoMutt->sub, "date_format");
  const char *cp = NONULL(c_date_format);
  bool use_c_locale = false;
  if (*cp == '!')
  {
    use_c_locale = true;
    cp++;
  }

  struct tm tm = mutt_date_localtime(e->date_sent);
  char tmp[128] = { 0 };

  if (use_c_locale)
  {
    strftime_l(tmp, sizeof(tmp), cp, &tm, NeoMutt->time_c_locale);
  }
  else
  {
    strftime(tmp, sizeof(tmp), cp, &tm);
  }

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_DATE);

  buf_strcpy(buf, tmp);
}

/**
 * index_e - Index: Thread index number - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_e(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;
  struct Mailbox *m = hfi->mailbox;

  const int num = mutt_messages_in_thread(m, e, MIT_POSITION);
  buf_printf(buf, "%d", num);
}

/**
 * index_E - Index: Number of messages thread - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_E(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;
  struct Mailbox *m = hfi->mailbox;

  const int num = mutt_messages_in_thread(m, e, MIT_NUM_MESSAGES);
  buf_printf(buf, "%d", num);
}

/**
 * index_f - Index: Sender - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_f(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;

  mutt_addrlist_write(&e->env->from, buf, true);
}

/**
 * index_F - Index: Author name - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_F(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;

  char tmp[128] = { 0 };

  make_from(e->env, tmp, sizeof(tmp), false, MUTT_FORMAT_NO_FLAGS);

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_AUTHOR);

  buf_strcpy(buf, tmp);
}

/**
 * index_Fp - Index: Plain author name - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_Fp(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = (const struct HdrFormatInfo *) data;
  struct Email *e = hfi->email;

  char tmp[128] = { 0 };

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_AUTHOR);

  make_from(e->env, tmp, sizeof(tmp), false, MUTT_FORMAT_PLAIN);

  buf_strcpy(buf, tmp);
}

/**
 * index_g - Index: Newsgroup name - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_g(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_TAGS);
  driver_tags_get_transformed(&e->tags, buf);
}

/**
 * index_G - Index: Individual tag - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_G(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  char x = '0'; //XXX needs to come from parsing the node string

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;

  char tag_format[3] = { 0 };

  tag_format[0] = 'G';
  tag_format[1] = x;
  tag_format[2] = '\0';

  char *tag = mutt_hash_find(TagFormats, tag_format);
  if (tag)
  {
    if (flags & MUTT_FORMAT_INDEX)
      node_expando_set_color(node, MT_COLOR_INDEX_TAG);
    driver_tags_get_transformed_for(&e->tags, tag, buf);
  }
  else
  {
    buf_reset(buf);
  }
}

/**
 * index_H - Index: Spam attributes - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_H(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;

  buf_copy(buf, &e->env->spam);
}

/**
 * index_i - Index: Message-id - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_i(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;

  const char *s = e->env->message_id ? e->env->message_id : "<no.id>";
  buf_strcpy(buf, NONULL(s));
}

/**
 * index_I - Index: Initials of author - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_I(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  const struct Address *from = TAILQ_FIRST(&e->env->from);

  char tmp[128] = { 0 };

  if (mutt_mb_get_initials(mutt_get_name(from), tmp, sizeof(tmp)))
  {
    if (flags & MUTT_FORMAT_INDEX)
      node_expando_set_color(node, MT_COLOR_INDEX_AUTHOR);

    buf_strcpy(buf, tmp);
    return;
  }

  index_a(node, data, flags, max_width, buf);
}

/**
 * index_J - Index: Tags - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_J(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;

  bool have_tags = true;
  struct Buffer *tags = buf_pool_get();
  driver_tags_get_transformed(&e->tags, tags);
  if (!buf_is_empty(tags))
  {
    if (flags & MUTT_FORMAT_TREE)
    {
      struct Buffer *parent_tags = buf_pool_get();
      if (e->thread->prev && e->thread->prev->message)
      {
        driver_tags_get_transformed(&e->thread->prev->message->tags, parent_tags);
      }
      if (!parent_tags && e->thread->parent && e->thread->parent->message)
      {
        driver_tags_get_transformed(&e->thread->parent->message->tags, parent_tags);
      }
      if (parent_tags && buf_istr_equal(tags, parent_tags))
        have_tags = false;
      buf_pool_release(&parent_tags);
    }
  }
  else
  {
    have_tags = false;
  }

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_TAGS);

  const char *s = have_tags ? buf_string(tags) : "";
  buf_strcpy(buf, NONULL(s));

  buf_pool_release(&tags);
}

/**
 * index_K - Index: Mailing list - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_K(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  char tmp[128] = { 0 };

  if (first_mailing_list(tmp, sizeof(tmp), &e->env->to) ||
      first_mailing_list(tmp, sizeof(tmp), &e->env->cc))
  {
    buf_strcpy(buf, tmp);
    return;
  }

  buf_reset(buf);
}

/**
 * index_l - Index: Number of lines - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_l(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_SIZE);

  const int num = e->lines;
  buf_printf(buf, "%d", num);
}

/**
 * index_L - Index: List address - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_L(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  char tmp[128] = { 0 };

  make_from(e->env, tmp, sizeof(tmp), true, flags);

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_AUTHOR);
  buf_strcpy(buf, tmp);
}

/**
 * index_m - Index: Total number of message - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_m(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Mailbox *m = hfi->mailbox;

  if (m)
  {
    const int num = m->msg_count;
    buf_printf(buf, "%d", num);
  }
  else
  {
    const char *s = "(null)";
    buf_strcpy(buf, NONULL(s));
  }
}

/**
 * index_M - Index: Number of hidden messages - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_M(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;
  const bool threads = mutt_using_threads();
  const bool is_index = (flags & MUTT_FORMAT_INDEX) != 0;

  if (threads && is_index && e->collapsed && (e->num_hidden > 1))
  {
    if (flags & MUTT_FORMAT_INDEX)
      node_expando_set_color(node, MT_COLOR_INDEX_COLLAPSED);
    const int num = e->num_hidden;
    buf_printf(buf, "%d", num);
  }
  else if (is_index && threads)
  {
    if (flags & MUTT_FORMAT_INDEX)
      node_expando_set_color(node, MT_COLOR_INDEX_COLLAPSED);
    const char *s = " ";
    buf_strcpy(buf, NONULL(s));
  }
  else
  {
    buf_reset(buf);
  }
}

/**
 * index_n - Index: Author's real name - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_n(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;
  const struct Address *from = TAILQ_FIRST(&e->env->from);

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_AUTHOR);

  const char *s = mutt_get_name(from);
  buf_strcpy(buf, NONULL(s));
}

/**
 * index_N - Index: Message score - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_N(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  const int num = e->score;
  buf_printf(buf, "%d", num);
}

/**
 * index_O - Index: Original save folder - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_O(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  char tmp[128] = { 0 };
  char *p = NULL;

  make_from_addr(e->env, tmp, sizeof(tmp), true);
  const bool c_save_address = cs_subset_bool(NeoMutt->sub, "save_address");
  if (!c_save_address && (p = strpbrk(tmp, "%@")))
  {
    *p = '\0';
  }

  buf_strcpy(buf, tmp);
}

/**
 * index_P - Index: Progress indicator - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_P(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;

  const char *s = hfi->pager_progress;
  buf_strcpy(buf, NONULL(s));
}

/**
 * index_q - Index: Newsgroup name - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_q(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  const char *s = e->env->newsgroups;
  buf_strcpy(buf, NONULL(s));
}

/**
 * index_r - Index: To recipients - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_r(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  mutt_addrlist_write(&e->env->to, buf, true);
}

/**
 * index_R - Index: Cc recipients - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_R(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  mutt_addrlist_write(&e->env->cc, buf, true);
}

/**
 * index_s - Index: Subject - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_s(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  subjrx_apply_mods(e->env);
  char *subj = NULL;

  if (e->env->disp_subj)
    subj = e->env->disp_subj;
  else
    subj = e->env->subject;

  if (flags & MUTT_FORMAT_TREE && !e->collapsed)
  {
    if (flags & MUTT_FORMAT_FORCESUBJ)
    {
      if (flags & MUTT_FORMAT_INDEX)
        node_expando_set_color(node, MT_COLOR_INDEX_SUBJECT);
      node_expando_set_has_tree(node, true);

      buf_printf(buf, "%s%s", e->tree, NONULL(subj));
    }
    else
    {
      if (flags & MUTT_FORMAT_INDEX)
        node_expando_set_color(node, MT_COLOR_INDEX_SUBJECT);
      node_expando_set_has_tree(node, true);

      const char *s = e->tree;
      buf_strcpy(buf, NONULL(s));
    }
  }
  else
  {
    if (flags & MUTT_FORMAT_INDEX)
      node_expando_set_color(node, MT_COLOR_INDEX_SUBJECT);
    const char *s = subj;
    buf_strcpy(buf, NONULL(s));
  }
}

/**
 * index_S - Index: Status flag - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_S(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  const struct MbTable *c_flag_chars = cs_subset_mbtable(NeoMutt->sub, "flag_chars");
  const int msg_in_pager = hfi->msg_in_pager;

  const char *wch = NULL;
  if (e->deleted)
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_DELETED);
  else if (e->attach_del)
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_DELETED_ATTACH);
  else if (e->tagged)
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_TAGGED);
  else if (e->flagged)
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_IMPORTANT);
  else if (e->replied)
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_REPLIED);
  else if (e->read && (msg_in_pager != e->msgno))
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_SEMPTY);
  else if (e->old)
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_OLD);
  else
    wch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_NEW);

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_FLAGS);

  buf_strcpy(buf, NONULL(wch));
}

/**
 * index_t - Index: To field - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_t(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  const struct Address *to = TAILQ_FIRST(&e->env->to);
  const struct Address *cc = TAILQ_FIRST(&e->env->cc);

  char tmp[128] = { 0 };

  if (!check_for_mailing_list(&e->env->to, "To ", tmp, sizeof(tmp)) &&
      !check_for_mailing_list(&e->env->cc, "Cc ", tmp, sizeof(tmp)))
  {
    if (to)
      snprintf(tmp, sizeof(tmp), "To %s", mutt_get_name(to));
    else if (cc)
      snprintf(tmp, sizeof(tmp), "Cc %s", mutt_get_name(cc));
    else
    {
      tmp[0] = '\0';
    }
  }

  buf_strcpy(buf, tmp);
}

/**
 * index_T - Index: $to_chars flag - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_T(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;

  const struct MbTable *c_to_chars = cs_subset_mbtable(NeoMutt->sub, "to_chars");

  int i;
  const char *s = (c_to_chars && ((i = user_is_recipient(e))) < c_to_chars->len) ?
                      c_to_chars->chars[i] :
                      " ";

  buf_strcpy(buf, NONULL(s));
}

/**
 * index_u - Index: User name - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_u(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  const struct Address *from = TAILQ_FIRST(&e->env->from);

  char tmp[128] = { 0 };
  char *p = NULL;

  if (from && from->mailbox)
  {
    mutt_str_copy(tmp, mutt_addr_for_display(from), sizeof(tmp));
    p = strpbrk(tmp, "%@");
    if (p)
    {
      *p = '\0';
    }
  }
  else
  {
    tmp[0] = '\0';
  }

  buf_strcpy(buf, tmp);
}

/**
 * index_v - Index: First name - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_v(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  const struct Address *from = TAILQ_FIRST(&e->env->from);
  const struct Address *to = TAILQ_FIRST(&e->env->to);
  const struct Address *cc = TAILQ_FIRST(&e->env->cc);

  char tmp[128] = { 0 };
  char *p = NULL;

  if (mutt_addr_is_user(from))
  {
    if (to)
    {
      const char *s = mutt_get_name(to);
      mutt_str_copy(tmp, NONULL(s), sizeof(tmp));
    }
    else if (cc)
    {
      const char *s = mutt_get_name(cc);
      mutt_str_copy(tmp, NONULL(s), sizeof(tmp));
    }
    else
    {
      tmp[0] = '\0';
    }
  }
  else
  {
    const char *s = mutt_get_name(from);
    mutt_str_copy(tmp, NONULL(s), sizeof(tmp));
  }
  p = strpbrk(tmp, " %@");
  if (p)
  {
    *p = '\0';
  }

  buf_strcpy(buf, tmp);
}

/**
 * index_W - Index: Organization - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_W(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  const char *s = e->env->organization;
  buf_strcpy(buf, NONULL(s));
}

/**
 * index_x - Index: X-Comment-To - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_x(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  const char *s = e->env->x_comment_to;
  buf_strcpy(buf, NONULL(s));
}

/**
 * index_X - Index: Number of MIME attachments - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_X(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;
  struct Mailbox *m = hfi->mailbox;

  struct Message *msg = mx_msg_open(m, e);
  if (msg)
  {
    const int num = mutt_count_body_parts(m, e, msg->fp);
    buf_printf(buf, "%d", num);

    mx_msg_close(m, &msg);
  }
  else
  {
    buf_reset(buf);
  }
}

/**
 * index_y - Index: X-Label - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_y(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_LABEL);

  const char *s = e->env->x_label;
  buf_strcpy(buf, NONULL(s));
}

/**
 * index_Y - Index: X-Label (if different) - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_Y(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  bool label = true;
  if (e->env->x_label)
  {
    struct Email *e_tmp = NULL;
    if (flags & MUTT_FORMAT_TREE && (e->thread->prev && e->thread->prev->message &&
                                     e->thread->prev->message->env->x_label))
    {
      e_tmp = e->thread->prev->message;
    }
    else if (flags & MUTT_FORMAT_TREE && (e->thread->parent && e->thread->parent->message &&
                                          e->thread->parent->message->env->x_label))
    {
      e_tmp = e->thread->parent->message;
    }

    if (e_tmp && mutt_istr_equal(e->env->x_label, e_tmp->env->x_label))
    {
      label = false;
    }
  }
  else
  {
    label = false;
  }

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_LABEL);

  if (label)
  {
    const char *s = e->env->x_label;
    buf_strcpy(buf, NONULL(s));
  }
  else
  {
    const char *s = "";
    buf_strcpy(buf, NONULL(s));
  }
}

/**
 * index_zc - Index: Message crypto flags - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_zc(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  const struct Email *e = hfi->email;

  const struct MbTable *c_crypt_chars = cs_subset_mbtable(NeoMutt->sub, "crypt_chars");

  const char *ch = NULL;
  if ((WithCrypto != 0) && (e->security & SEC_GOODSIGN))
  {
    ch = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_GOOD_SIGN);
  }
  else if ((WithCrypto != 0) && (e->security & SEC_ENCRYPT))
  {
    ch = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_ENCRYPTED);
  }
  else if ((WithCrypto != 0) && (e->security & SEC_SIGN))
  {
    ch = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_SIGNED);
  }
  else if (((WithCrypto & APPLICATION_PGP) != 0) && ((e->security & PGP_KEY) == PGP_KEY))
  {
    ch = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_CONTAINS_KEY);
  }
  else
  {
    ch = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_NO_CRYPTO);
  }

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_FLAGS);
  buf_strcpy(buf, NONULL(ch));
}

/**
 * index_zs - Index: Message status flags - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_zs(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;

  const bool threads = mutt_using_threads();
  const struct MbTable *c_flag_chars = cs_subset_mbtable(NeoMutt->sub, "flag_chars");
  const int msg_in_pager = hfi->msg_in_pager;

  const char *ch = NULL;
  if (e->deleted)
  {
    ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_DELETED);
  }
  else if (e->attach_del)
  {
    ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_DELETED_ATTACH);
  }
  else if (threads && thread_is_new(e))
  {
    ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_NEW_THREAD);
  }
  else if (threads && thread_is_old(e))
  {
    ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_OLD_THREAD);
  }
  else if (e->read && (msg_in_pager != e->msgno))
  {
    if (e->replied)
    {
      ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_REPLIED);
    }
    else
    {
      ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_ZEMPTY);
    }
  }
  else
  {
    if (e->old)
    {
      ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_OLD);
    }
    else
    {
      ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_NEW);
    }
  }

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_FLAGS);
  buf_strcpy(buf, NONULL(ch));
}

/**
 * index_zt - Index: Message tag flags - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_zt(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
              int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;

  const struct MbTable *c_flag_chars = cs_subset_mbtable(NeoMutt->sub, "flag_chars");
  const struct MbTable *c_to_chars = cs_subset_mbtable(NeoMutt->sub, "to_chars");

  const char *ch = NULL;
  if (e->tagged)
  {
    ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_TAGGED);
  }
  else if (e->flagged)
  {
    ch = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_IMPORTANT);
  }
  else
  {
    ch = mbtable_get_nth_wchar(c_to_chars, user_is_recipient(e));
  }

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_FLAGS);
  buf_strcpy(buf, NONULL(ch));
}

/**
 * index_Z - Index: Status flags - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_Z(const struct ExpandoNode *node, void *data, MuttFormatFlags flags,
             int max_width, struct Buffer *buf)
{
  assert(node->type == ENT_EXPANDO);

  const struct HdrFormatInfo *hfi = data;
  struct Email *e = hfi->email;
  const int msg_in_pager = hfi->msg_in_pager;

  const struct MbTable *c_crypt_chars = cs_subset_mbtable(NeoMutt->sub, "crypt_chars");
  const struct MbTable *c_flag_chars = cs_subset_mbtable(NeoMutt->sub, "flag_chars");
  const struct MbTable *c_to_chars = cs_subset_mbtable(NeoMutt->sub, "to_chars");
  const bool threads = mutt_using_threads();

  const char *first = NULL;
  if (threads && thread_is_new(e))
  {
    first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_NEW_THREAD);
  }
  else if (threads && thread_is_old(e))
  {
    first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_OLD_THREAD);
  }
  else if (e->read && (msg_in_pager != e->msgno))
  {
    if (e->replied)
    {
      first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_REPLIED);
    }
    else
    {
      first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_ZEMPTY);
    }
  }
  else
  {
    if (e->old)
    {
      first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_OLD);
    }
    else
    {
      first = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_NEW);
    }
  }

  /* Marked for deletion; deleted attachments; crypto */
  const char *second = NULL;
  if (e->deleted)
    second = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_DELETED);
  else if (e->attach_del)
    second = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_DELETED_ATTACH);
  else if ((WithCrypto != 0) && (e->security & SEC_GOODSIGN))
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_GOOD_SIGN);
  else if ((WithCrypto != 0) && (e->security & SEC_ENCRYPT))
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_ENCRYPTED);
  else if ((WithCrypto != 0) && (e->security & SEC_SIGN))
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_SIGNED);
  else if (((WithCrypto & APPLICATION_PGP) != 0) && (e->security & PGP_KEY))
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_CONTAINS_KEY);
  else
    second = mbtable_get_nth_wchar(c_crypt_chars, FLAG_CHAR_CRYPT_NO_CRYPTO);

  /* Tagged, flagged and recipient flag */
  const char *third = NULL;
  if (e->tagged)
    third = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_TAGGED);
  else if (e->flagged)
    third = mbtable_get_nth_wchar(c_flag_chars, FLAG_CHAR_IMPORTANT);
  else
    third = mbtable_get_nth_wchar(c_to_chars, user_is_recipient(e));

  if (flags & MUTT_FORMAT_INDEX)
    node_expando_set_color(node, MT_COLOR_INDEX_FLAGS);

  buf_printf(buf, "%s%s%s", first, second, third);
}

/**
 * index_arrow - Index -  - Implements ::expando_callback_t - @ingroup expando_callback_api
 */
void index_arrow(const struct ExpandoNode *node, void *data,
                 MuttFormatFlags flags, int max_width, struct Buffer *buf)
{
  const bool c_arrow_cursor = cs_subset_bool(NeoMutt->sub, "arrow_cursor");
  const struct HdrFormatInfo *hfi = data;

  if (c_arrow_cursor)
  {
    const bool show_arrow = hfi->show_arrow;
    // FIXME(g0mb4): Uncomment this, just for debugging.
    //const char *c_arrow_string = cs_subset_string(NeoMutt->sub, "arrow_string");
    const char *c_arrow_string = "XX";
    const int arrow_width = mutt_strwidth(c_arrow_string);

    if (show_arrow)
    {
      node_expando_set_color(node, MT_COLOR_INDICATOR);
      buf_strcpy(buf, NONULL(c_arrow_string));
    }
    else
    {
      node_expando_set_color(node, MT_COLOR_NONE);
      buf_printf(buf, "%*s", arrow_width, " ");
    }
  }
  else
  {
    node_expando_set_color(node, MT_COLOR_NONE);
    buf_reset(buf);
  }
}

/**
 * mutt_make_string - Create formatted strings using mailbox expandos
 * @param buf        Buffer for the result
 * @param cols       Number of screen columns (OPTIONAL)
 * @param exp        Expando containing expando tree
 * @param m          Mailbox
 * @param inpgr      Message shown in the pager
 * @param e          Email
 * @param flags      Flags, see #MuttFormatFlags
 * @param progress   Pager progress string
 * @param show_arrow true if this is the current Menu item
 */
void mutt_make_string(struct Buffer *buf, size_t cols, const struct Expando *exp,
                      struct Mailbox *m, int inpgr, struct Email *e,
                      MuttFormatFlags flags, const char *progress, bool show_arrow)
{
  assert(exp);

  struct HdrFormatInfo hfi = { 0 };

  hfi.email = e;
  hfi.mailbox = m;
  hfi.msg_in_pager = inpgr;
  hfi.pager_progress = progress;
  hfi.show_arrow = show_arrow;

  expando_render(exp, IndexRenderData, &hfi, flags, cols, buf);
}

const struct ExpandoRenderData IndexRenderData[] = {
  // clang-format off
  { ED_EMAIL,    ED_EMA_STRF_RECV_LOCAL,     index_date_recv_local },
  { ED_EMAIL,    ED_EMA_INDEX_HOOK,          index_format_hook },
  { ED_ENVELOPE, ED_ENV_FROM,                index_a           },
  { ED_ENVELOPE, ED_ENV_REPLY_TO,            index_A           },
  { ED_ENVELOPE, ED_ENV_LIST_ADDRESS,        index_B           },
  { ED_MAILBOX,  ED_MBX_MAILBOX_NAME,        index_b           },
  { ED_EMAIL,    ED_EMA_NUMBER,              index_C           },
  { ED_EMAIL,    ED_EMA_SIZE,                index_c           },
  { ED_EMAIL,    ED_EMA_DATE_FORMAT,         index_d           },
  { ED_EMAIL,    ED_EMA_DATE_FORMAT_LOCAL,   index_D           },
  { ED_EMAIL,    ED_EMA_THREAD_COUNT,        index_E           },
  { ED_EMAIL,    ED_EMA_THREAD_NUMBER,       index_e           },
  { ED_ENVELOPE, ED_ENV_FROM_FULL,           index_f           },
  { ED_ENVELOPE, ED_ENV_SENDER,              index_F           },
  { ED_EMAIL,    ED_EMA_TAGS,                index_g           },
  { ED_EMAIL,    ED_EMA_TAGS_TRANSFORMED,    index_G           },
  { ED_ENVELOPE, ED_ENV_SPAM,                index_H           },
  { ED_ENVELOPE, ED_ENV_INITIALS,            index_I           },
  { ED_ENVELOPE, ED_ENV_MESSAGE_ID,          index_i           },
  { ED_EMAIL,    ED_EMA_THREAD_TAGS,         index_J           },
  { ED_ENVELOPE, ED_ENV_LIST_EMPTY,          index_K           },
  { ED_EMAIL,    ED_EMA_FROM_LIST,           index_L           },
  { ED_EMAIL,    ED_EMA_LINES,               index_l           },
  { ED_MAILBOX,  ED_MBX_MESSAGE_COUNT,       index_m           },
  { ED_EMAIL,    ED_EMA_THREAD_HIDDEN_COUNT, index_M           },
  { ED_ENVELOPE, ED_ENV_NAME,                index_n           },
  { ED_EMAIL,    ED_EMA_SCORE,               index_N           },
  { ED_EMAIL,    ED_EMA_SAVE_FOLDER,         index_O           },
  { ED_MAILBOX,  ED_MBX_PERCENTAGE,          index_P           },
  { ED_ENVELOPE, ED_ENV_NEWSGROUP,           index_q           },
  { ED_ENVELOPE, ED_ENV_CC_ALL,              index_R           },
  { ED_ENVELOPE, ED_ENV_TO_ALL,              index_r           },
  { ED_EMAIL,    ED_EMA_STATUS_FLAGS,        index_S           },
  { ED_ENVELOPE, ED_ENV_SUBJECT,             index_s           },
  { ED_ENVELOPE, ED_ENV_TO,                  index_t           },
  { ED_EMAIL,    ED_EMA_TO_CHARS,            index_T           },
  { ED_ENVELOPE, ED_ENV_USERNAME,            index_u           },
  { ED_ENVELOPE, ED_ENV_FIRST_NAME,          index_v           },
  { ED_ENVELOPE, ED_ENV_ORGANIZATION,        index_W           },
  { ED_EMAIL,    ED_EMA_ATTACHMENT_COUNT,    index_X           },
  { ED_ENVELOPE, ED_ENV_X_COMMENT_TO,        index_x           },
  { ED_ENVELOPE, ED_ENV_THREAD_X_LABEL,      index_Y           },
  { ED_ENVELOPE, ED_ENV_X_LABEL,             index_y           },
  { ED_EMAIL,    ED_EMA_COMBINED_FLAGS,      index_Z           },
  { ED_EMAIL,    ED_EMA_CRYPTO_FLAGS,        index_zc          },
  { ED_EMAIL,    ED_EMA_STATUS_FLAGS,        index_zs          },
  { ED_EMAIL,    ED_EMA_MESSAGE_FLAGS,       index_zt          },
  { ED_EMAIL,    ED_EMA_STRF_LOCAL,          index_date_local  },
  { ED_EMAIL,    ED_EMA_STRF,                index_date        },
  { ED_GLOBAL,   ED_GLO_ARROW,               index_arrow       },
  { -1, -1, NULL },
  // clang-format on
};
