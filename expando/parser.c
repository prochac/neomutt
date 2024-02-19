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
 * @page expando_parser XXX
 *
 * XXX
 */

#include "config.h"
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "mutt/lib.h"
#include "gui/lib.h"
#include "parser.h"
#include "domain.h"
#include "node.h"
#include "node_condition.h"
#include "node_expando.h"
#include "node_padding.h"
#include "node_text.h"
#include "uid.h"
#include "validation.h"

/**
 * new_empty_node - XXX
 * @retval ptr XXX
 */
static struct ExpandoNode *new_empty_node(void)
{
  struct ExpandoNode *node = expando_node_new();

  node->type = ENT_EMPTY;
  node->did = ED_ALL;
  node->uid = ED_ALL_EMPTY;

  return node;
}

/**
 * append_node - XXX
 * @param root     XXX
 * @param new_node XXX
 */
static void append_node(struct ExpandoNode **root, struct ExpandoNode *new_node)
{
  if (!*root)
  {
    *root = new_node;
    return;
  }

  struct ExpandoNode *node = *root;
  while (node->next)
  {
    node = node->next;
  }

  node->next = new_node;
}

/**
 * is_valid_classic_expando - XXX
 * @param c XXX
 * @retval true XXX
 */
static bool is_valid_classic_expando(char c)
{
  // NOTE(g0mb4): Maybe rework this?
  // if special expandos are added this list must be updated!
  return isalpha(c) || (c == '!') || (c == '{') || (c == '(') || (c == '[') ||
         (c == '@') || (c == '^');
}

/**
 * skip_until_classic_expando - XXX
 * @param start XXX
 * @retval ptr XXX
 */
static const char *skip_until_classic_expando(const char *start)
{
  while (*start && !is_valid_classic_expando(*start))
  {
    start++;
  }

  return start;
}

/**
 * skip_classic_expando - XXX
 * @param s    XXX
 * @param defs XXX
 * @retval ptr XXX
 */
static const char *skip_classic_expando(const char *s, const struct ExpandoDefinition *defs)
{
  assert(*s != '\0');
  const struct ExpandoDefinition *definitions = defs;

  while (definitions && definitions->short_name)
  {
    const bool is_two_char = mutt_str_len(definitions->short_name) == 2;
    const char *name = definitions->short_name;

    if (is_two_char && (name[0] == *s) && (name[1] == *(s + 1)))
    {
      s++;
      break;
    }

    definitions++;
  }

  s++;
  return s;
}

/**
 * skip_until_ch - XXX
 * @param start      XXX
 * @param terminator XXX
 * @retval ptr XXX
 */
static const char *skip_until_ch(const char *start, char terminator)
{
  while (*start)
  {
    if (*start == terminator)
    {
      break;
    }

    start++;
  }

  return start;
}

/**
 * skip_until_if_true_end - XXX
 * @param start          XXX
 * @param end_terminator XXX
 * @retval ptr XXX
 */
static const char *skip_until_if_true_end(const char *start, char end_terminator)
{
  int ctr = 0;
  while (*start)
  {
    if ((ctr == 0) && ((*start == end_terminator) || (*start == '&')))
    {
      break;
    }

    // handle nested if-esle-s
    if (*start == '<')
    {
      ctr++;
    }

    if (*start == '>')
    {
      ctr--;
    }

    start++;
  }

  return start;
}

/**
 * skip_until_if_false_end - XXX
 * @param start          XXX
 * @param end_terminator XXX
 * @retval ptr XXX
 */
static const char *skip_until_if_false_end(const char *start, char end_terminator)
{
  int ctr = 0;
  while (*start)
  {
    if ((ctr == 0) && (*start == end_terminator))
    {
      break;
    }

    // handle nested if-esle-s
    if (*start == '<')
    {
      ctr++;
    }

    if (*start == '>')
    {
      ctr--;
    }

    start++;
  }

  return start;
}

/**
 * parse_format - XXX
 * @param start XXX
 * @param end   XXX
 * @param error XXX
 * @retval ptr XXX
 */
static struct ExpandoFormatPrivate *parse_format(const char *start, const char *end,
                                                 struct ExpandoParseError *error)
{
  if (start == end)
  {
    return NULL;
  }

  struct ExpandoFormatPrivate *format = mutt_mem_calloc(1, sizeof(struct ExpandoFormatPrivate));

  format->leader = ' ';
  format->start = start;
  format->end = end;
  format->justification = JUSTIFY_RIGHT;
  format->min = 0;
  format->max = INT_MAX;

  if (*start == '-')
  {
    format->justification = JUSTIFY_LEFT;
    start++;
  }
  else if (*start == '=')
  {
    format->justification = JUSTIFY_CENTER;
    start++;
  }

  if (*start == '0')
  {
    format->leader = '0';
    start++;
  }

  if (isdigit(*start))
  {
    char *end_ptr = NULL;
    int number = strtol(start, &end_ptr, 10);

    // NOTE(g0mb4): start is NOT null-terminated
    if (end_ptr > end)
    {
      error->position = start;
      snprintf(error->message, sizeof(error->message), "Wrong number");
      FREE(&format);
      return NULL;
    }

    format->min = number;
    start = end_ptr;
  };

  if (*start == '.')
  {
    start++;

    if (!isdigit(*start))
    {
      error->position = start;
      snprintf(error->message, sizeof(error->message), "Number is expected");
      FREE(&format);
      return NULL;
    }

    char *end_ptr;
    int number = strtol(start, &end_ptr, 10);

    // NOTE(g0mb4): start is NOT null-terminated
    if (end_ptr > end)
    {
      error->position = start;
      snprintf(error->message, sizeof(error->message), "Wrong number");
      FREE(&format);
      return NULL;
    }

    format->max = number;
    start = end_ptr;
  }

  return format;
}

/**
 * parse_expando_node - XXX
 * @param s            XXX
 * @param parsed_until XXX
 * @param defs         XXX
 * @param error        XXX
 * @retval ptr XXX
 */
static struct ExpandoNode *parse_expando_node(const char *s, const char **parsed_until,
                                              const struct ExpandoDefinition *defs,
                                              struct ExpandoParseError *error)
{
  const struct ExpandoDefinition *definition = defs;

  const char *format_end = skip_until_classic_expando(s);
  const char *expando_end = skip_classic_expando(format_end, defs);
  char expando[128] = { 0 };
  const int expando_len = expando_end - format_end;
  mutt_strn_copy(expando, format_end, expando_len, sizeof(expando));

  struct ExpandoFormatPrivate *format = parse_format(s, format_end, error);
  if (error->position)
  {
    FREE(&format);
    return NULL;
  }

  // for debugging
  if (!definition)
  {
    *parsed_until = expando_end;
    return node_expando_new(format_end, expando_end, format, ED_ALL, ED_ALL_EMPTY);
  }

  while (definition && definition->short_name)
  {
    if (mutt_str_equal(definition->short_name, expando))
    {
      if (definition->custom_parser)
      {
        FREE(&format);
        return definition->custom_parser(s, parsed_until, definition->did,
                                         definition->uid, error);
      }
      else
      {
        *parsed_until = expando_end;
        return node_expando_new(format_end, expando_end, format,
                                definition->did, definition->uid);
      }
    }

    definition++;
  }

  error->position = format_end;
  snprintf(error->message, sizeof(error->message), "Unknown expando: `%.*s`",
           expando_len, format_end);
  FREE(&format);
  return NULL;
}

/**
 * expando_parse_enclosed_expando - XXX
 * @param s            XXX
 * @param parsed_until XXX
 * @param did          XXX
 * @param uid          XXX
 * @param terminator   XXX
 * @param error        XXX
 * @retval ptr XXX
 */
struct ExpandoNode *expando_parse_enclosed_expando(const char *s, const char **parsed_until,
                                                   int did, int uid, char terminator,
                                                   struct ExpandoParseError *error)
{
  const char *format_end = skip_until_classic_expando(s);

  format_end++; // skip opening char

  const char *expando_end = skip_until_ch(format_end, terminator);

  if (*expando_end != terminator)
  {
    error->position = expando_end;
    snprintf(error->message, sizeof(error->message), "Missing: `%c`", terminator);
    return NULL;
  }

  // revert skipping for format
  struct ExpandoFormatPrivate *format = parse_format(s, format_end - 1, error);
  if (error->position)
  {
    FREE(&format);
    return NULL;
  }

  *parsed_until = expando_end + 1;
  return node_expando_new(format_end, expando_end, format, did, uid);
}

/**
 * parse_node - XXX
 * @param s               XXX
 * @param end             XXX
 * @param condition_start XXX
 * @param parsed_until    XXX
 * @param defs            XXX
 * @param error           XXX
 * @retval ptr XXX
 */
static struct ExpandoNode *parse_node(const char *s, const char *end,
                                      enum ExpandoConditionStart condition_start,
                                      const char **parsed_until,
                                      const struct ExpandoDefinition *defs,
                                      struct ExpandoParseError *error)
{
  while (*s && (end ? (s <= end) : 1))
  {
    // %X -> expando
    // if there is condition like <X..., the `%` is implicit
    if ((*s == '%') || ((condition_start == CON_START) && ((*s == '?') || (*s == '<'))))
    {
      s++;

      // %% -> "%"s
      if (*s == '%')
      {
        *parsed_until = s + 1;
        return node_text_new(s, s + 1);
      }
      // padding
      else if ((*s == '|') || (*s == '>') || (*s == '*'))
      {
        return node_padding_parse(s, parsed_until, error);
      } // conditional
      else if ((*s == '?') || (*s == '<'))
      {
        bool old_style = (*s == '?');
        char end_terminator = old_style ? '?' : '>';

        const char *cond_end = skip_until_ch(s, '?');
        const char *next = NULL;
        struct ExpandoNode *condition = parse_node(s, cond_end, CON_START, &next, defs, error);
        if (!condition)
          return NULL;

        if (*next != '?')
        {
          error->position = next;
          snprintf(error->message, sizeof(error->message), "Missing '?'");
          free_node(condition);
          return NULL;
        }

        s = next + 1;

        const char *if_true_start = s;
        // nested if-else only allowed in the new style
        const char *if_true_end = skip_until_if_true_end(s, end_terminator);
        bool only_true = *if_true_end == end_terminator;
        bool invalid = *if_true_end != '&' && !only_true;

        if (invalid)
        {
          error->position = if_true_end;
          snprintf(error->message, sizeof(error->message), "Missing '&' or '%c'", end_terminator);
          free_node(condition);
          return NULL;
        }

        const char *if_true_parsed = NULL;
        struct ExpandoNode *if_true_tree = NULL;

        while (if_true_start < if_true_end)
        {
          struct ExpandoNode *node = parse_node(if_true_start, if_true_end, CON_NO_CONDITION,
                                                &if_true_parsed, defs, error);
          if (!node)
          {
            free_node(condition);
            return NULL;
          }

          append_node(&if_true_tree, node);

          if_true_start = if_true_parsed;
        }

        if ((if_true_start == if_true_end) && !if_true_tree)
        {
          if_true_tree = new_empty_node();
        }

        if (only_true)
        {
          *parsed_until = if_true_end + 1;
          return node_condition_new(condition, if_true_tree, NULL);
        }
        else
        {
          const char *if_false_start = if_true_end + 1;
          // nested if-else only allowed in the new style
          const char *if_false_end = skip_until_if_false_end(if_false_start, end_terminator);

          if (*if_false_end != end_terminator)
          {
            error->position = if_false_start;
            snprintf(error->message, sizeof(error->message), "Missing '%c'", end_terminator);
            free_node(if_true_tree);
            free_node(condition);
            return NULL;
          }

          const char *if_false_parsed = NULL;
          struct ExpandoNode *if_false_tree = NULL;

          while (if_false_start < if_false_end)
          {
            struct ExpandoNode *node = parse_node(if_false_start, if_false_end, CON_NO_CONDITION,
                                                  &if_false_parsed, defs, error);
            if (!node)
            {
              free_node(if_true_tree);
              free_node(condition);
              return NULL;
            }

            append_node(&if_false_tree, node);

            if_false_start = if_false_parsed;
          }

          if ((if_false_start == if_false_end) && !if_false_tree)
          {
            if_false_tree = new_empty_node();
          }

          *parsed_until = if_false_end + 1;
          return node_condition_new(condition, if_true_tree, if_false_tree);
        }
      }
      // expando
      else
      {
        struct ExpandoNode *node = parse_expando_node(s, parsed_until, defs, error);
        if (!node || error->position)
        {
          free_node(node);
          return NULL;
        }

        return node;
      }
    }
    // text
    else
    {
      return node_text_parse(s, end, parsed_until);
    }
  }

  error->position = s;
  snprintf(error->message, sizeof(error->message), "Internal parsing error");
  return NULL;
}

/**
 * expando_tree_parse - XXX
 * @param root   XXX
 * @param string XXX
 * @param defs   XXX
 * @param error  XXX
 */
void expando_tree_parse(struct ExpandoNode **root, const char *string,
                        const struct ExpandoDefinition *defs, struct ExpandoParseError *error)
{
  if (!string || !*string)
  {
    append_node(root, new_empty_node());
    return;
  }

  const char *end = NULL;
  const char *start = string;

  while (*start)
  {
    struct ExpandoNode *node = parse_node(start, NULL, CON_NO_CONDITION, &end, defs, error);
    if (!node)
      break;

    append_node(root, node);
    start = end;
  }
}

/**
 * expando_tree_free - XXX
 * @param root XXX
 */
void expando_tree_free(struct ExpandoNode **root)
{
  if (!root || !*root)
    return;

  struct ExpandoNode *node = *root;
  free_tree(node);
}
