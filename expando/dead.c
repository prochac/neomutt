/**
 * is_utf8_cont_byte - XXX
 * @param c XXX
 */
static bool is_utf8_cont_byte(uint8_t c)
{
  return (c & 0xC0) == 0x80;
}

/**
 * count_spec - XXX
 * @param s XXX
 * @retval num XXX
 */
static int count_spec(const char *s)
{
  int n = 0;
  while (*s)
  {
    if (*s == MUTT_SPECIAL_INDEX)
    {
      n++;
    }

    s++;
  }

  return n;
}

/**
 * count_spec_end - XXX
 * @param s   XXX
 * @param end XXX
 * @retval num XXX
 */
static int count_spec_end(const char *s, const char *end)
{
  int n = 0;
  while (*s && s <= end)
  {
    if (*s == MUTT_SPECIAL_INDEX)
    {
      n++;
    }

    s++;
  }

  return n;
}

/**
 * softpad_correct_utf8 - XXX
 * @param buf        XXX
 * @param copy_start XXX
 */
static void softpad_correct_utf8(const char *buf, char **copy_start)
{
  char *s = *copy_start;
  uint8_t c = (uint8_t) *s;

  if (is_ascii_byte(c))
  {
    return;
  }

  while (is_utf8_cont_byte(c))
  {
    assert(s - 1 >= buf);
    s--;
    c = (uint8_t) *s;
  }

  if (is_utf8_2_byte_head(c))
  {
    *copy_start = (char *) (s + 2);
  }
  else if (is_utf8_3_byte_head(c))
  {
    *copy_start = (char *) s + 3;
  }
  else if (is_utf8_4_byte_head(c))
  {
    *copy_start = (char *) s + 4;
  }
  else
  {
    assert(0 && "Unreachable");
  }
}

/**
 * softpad_move_markers - XXX
 * @param buf        XXX
 * @param copy_start XXX
 */
static void softpad_move_markers(char *buf, char *copy_start)
{
  const int n = count_spec_end(buf, copy_start);

  // all markers are closed
  if (n % 2 == 0)
  {
    // if position is a marker
    if (*copy_start == MUTT_SPECIAL_INDEX)
    {
      const char color = *(copy_start + 1);
      assert(color == MT_COLOR_INDEX);

      // move marker
      *(copy_start - 2) = MUTT_SPECIAL_INDEX;
      *(copy_start - 1) = MT_COLOR_INDEX;
    }
    // if position is a color
    else if (*(copy_start - 1) == MUTT_SPECIAL_INDEX)
    {
      const char color = *copy_start;
      assert(color == MT_COLOR_INDEX);

      // move marker
      *(copy_start - 2) = MUTT_SPECIAL_INDEX;
      *(copy_start - 1) = MT_COLOR_INDEX;
    }
  }
  // one marker is open
  else
  {
    // move marker
    *(copy_start - 2) = MUTT_SPECIAL_INDEX;
    *(copy_start - 1) = MT_COLOR_INDEX;
  }
}
