/**
 * @file
 * Test the Expando Parser
 *
 * @authors
 * Copyright (C) 2024 Richard Russon <rich@flatcap.org>
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

#define TEST_NO_MAIN
#include "config.h"
#include "acutest.h"
#include <stddef.h>
#include "mutt/lib.h"
#include "email/lib.h"
#include "expando/lib.h"
#include "common.h"
#include "test_common.h"

void expando_serialise(const struct Expando *exp, struct Buffer *buf);

void test_expando_parser(void)
{
  static const struct ExpandoDefinition TestFormatData[] = {
    // clang-format off
    { "X", "attachment-count", ED_EMAIL, ED_EMA_ATTACHMENT_COUNT, E_TYPE_NUMBER, E_FLAGS_NO_FLAGS, NULL },
    { "[",  NULL,              ED_EMAIL, ED_EMA_STRF_LOCAL,       E_TYPE_STRING, E_FLAGS_NO_FLAGS, parse_date },
    { NULL, NULL, 0, -1, -1, 0, NULL }
    // clang-format on
  };

  static const char *TestStrings[][2] = {
    // clang-format off
    // Formatting
    { "",       "<EMPTY>" },
    { "%X",     "<EXP:'X'(EMAIL,ATTACHMENT_COUNT)>" },
    { "%5X",    "<EXP:'X'(EMAIL,ATTACHMENT_COUNT):{5,2147483647,RIGHT,' ',5}>" },
    { "%.7X",   "<EXP:'X'(EMAIL,ATTACHMENT_COUNT):{0,7,RIGHT,' ',.7}>" },
    { "%5.7X",  "<EXP:'X'(EMAIL,ATTACHMENT_COUNT):{5,7,RIGHT,' ',5.7}>" },
    { "%-5X",   "<EXP:'X'(EMAIL,ATTACHMENT_COUNT):{5,2147483647,LEFT,' ',-5}>" },
    { "%-.7X",  "<EXP:'X'(EMAIL,ATTACHMENT_COUNT):{0,7,LEFT,' ',-.7}>" },
    { "%-5.7X", "<EXP:'X'(EMAIL,ATTACHMENT_COUNT):{5,7,LEFT,' ',-5.7}>" },
    { "%05X",   "<EXP:'X'(EMAIL,ATTACHMENT_COUNT):{5,2147483647,RIGHT,'0',05}>" },

    // Conditional (old form)
    { "%?X?&?",       "<COND:<EXP:'X'(EMAIL,ATTACHMENT_COUNT)>|<EMPTY>|<EMPTY>>" },
    { "%?X?AAA&?",    "<COND:<EXP:'X'(EMAIL,ATTACHMENT_COUNT)>|<TEXT:'AAA'>|<EMPTY>>" },
    { "%?X?&BBB?",    "<COND:<EXP:'X'(EMAIL,ATTACHMENT_COUNT)>|<EMPTY>|<TEXT:'BBB'>>" },
    { "%?X?AAA&BBB?", "<COND:<EXP:'X'(EMAIL,ATTACHMENT_COUNT)>|<TEXT:'AAA'>|<TEXT:'BBB'>>" },

    // Conditional (new form)
    { "%<X?&>",       "<COND:<EXP:'X'(EMAIL,ATTACHMENT_COUNT)>|<EMPTY>|<EMPTY>>" },
    { "%<X?AAA&>",    "<COND:<EXP:'X'(EMAIL,ATTACHMENT_COUNT)>|<TEXT:'AAA'>|<EMPTY>>" },
    { "%<X?&BBB>",    "<COND:<EXP:'X'(EMAIL,ATTACHMENT_COUNT)>|<EMPTY>|<TEXT:'BBB'>>" },
    { "%<X?AAA&BBB>", "<COND:<EXP:'X'(EMAIL,ATTACHMENT_COUNT)>|<TEXT:'AAA'>|<TEXT:'BBB'>>" },

    // Dates
    { "%[%Y-%m-%d]",    "<EXP:'%Y-%m-%d'(EMAIL,STRF_LOCAL)>" },
    { "%-5[%Y-%m-%d]",  "<EXP:'%Y-%m-%d'(EMAIL,STRF_LOCAL):{5,2147483647,LEFT,' ',-5}>" },

    // Conditional dates
    { "%<[1M?AAA&BBB>",  "<COND:<DATE:(EMAIL,STRF_LOCAL):1:M:60>|<TEXT:'AAA'>|<TEXT:'BBB'>>" },
    { "%<[10M?AAA&BBB>", "<COND:<DATE:(EMAIL,STRF_LOCAL):10:M:60>|<TEXT:'AAA'>|<TEXT:'BBB'>>" },
    { "%<[1H?AAA&BBB>",  "<COND:<DATE:(EMAIL,STRF_LOCAL):1:H:3600>|<TEXT:'AAA'>|<TEXT:'BBB'>>" },
    { "%<[10H?AAA&BBB>", "<COND:<DATE:(EMAIL,STRF_LOCAL):10:H:3600>|<TEXT:'AAA'>|<TEXT:'BBB'>>" },
    { "%<[1d?AAA&BBB>",  "<COND:<DATE:(EMAIL,STRF_LOCAL):1:d:86400>|<TEXT:'AAA'>|<TEXT:'BBB'>>" },
    { "%<[10d?AAA&BBB>", "<COND:<DATE:(EMAIL,STRF_LOCAL):10:d:86400>|<TEXT:'AAA'>|<TEXT:'BBB'>>" },
    { "%<[1w?AAA&BBB>",  "<COND:<DATE:(EMAIL,STRF_LOCAL):1:w:604800>|<TEXT:'AAA'>|<TEXT:'BBB'>>" },
    { "%<[10w?AAA&BBB>", "<COND:<DATE:(EMAIL,STRF_LOCAL):10:w:604800>|<TEXT:'AAA'>|<TEXT:'BBB'>>" },
    { "%<[1m?AAA&BBB>",  "<COND:<DATE:(EMAIL,STRF_LOCAL):1:m:2592000>|<TEXT:'AAA'>|<TEXT:'BBB'>>" },
    { "%<[10m?AAA&BBB>", "<COND:<DATE:(EMAIL,STRF_LOCAL):10:m:2592000>|<TEXT:'AAA'>|<TEXT:'BBB'>>" },
    { "%<[1y?AAA&BBB>",  "<COND:<DATE:(EMAIL,STRF_LOCAL):1:y:31536000>|<TEXT:'AAA'>|<TEXT:'BBB'>>" },
    { "%<[10y?AAA&BBB>", "<COND:<DATE:(EMAIL,STRF_LOCAL):10:y:31536000>|<TEXT:'AAA'>|<TEXT:'BBB'>>" },

    // Padding
    { "AAA%>XBBB", "<TEXT:'AAA'><PAD:HARD:'X'><TEXT:'BBB'>" },
    { "AAA%|XBBB", "<TEXT:'AAA'><PAD:EOL:'X'><TEXT:'BBB'>" },
    { "AAA%*XBBB", "<TEXT:'AAA'><PAD:SOFT:'X'><TEXT:'BBB'>" },
    // clang-format on
  };

  struct Buffer *buf = buf_pool_get();
  struct Buffer *err = buf_pool_get();
  struct Expando *exp = NULL;

  for (int i = 0; i < mutt_array_size(TestStrings); i++)
  {
    buf_reset(buf);
    buf_reset(err);

    const char *format = TestStrings[i][0];
    const char *expected = TestStrings[i][1];
    TEST_CASE(format);

    exp = expando_parse(format, TestFormatData, err);
    TEST_CHECK(buf_is_empty(err));
    TEST_MSG(buf_string(err));
    expando_serialise(exp, buf);
    TEST_CHECK_STR_EQ(buf_string(buf), expected);
    expando_free(&exp);
  }

  buf_pool_release(&buf);
  buf_pool_release(&err);
}
