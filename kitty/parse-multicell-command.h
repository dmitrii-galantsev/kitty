// This file is generated by apc_parsers.py do not edit!

#pragma once

#include "base64.h"

static inline void parse_multicell_code(PS *self, uint8_t *parser_buf,
                                        const size_t parser_buf_pos) {
  unsigned int pos = 0;
  size_t payload_start = 0;
  enum PARSER_STATES { KEY, EQUAL, UINT, INT, FLAG, AFTER_VALUE, PAYLOAD };
  enum PARSER_STATES state = KEY, value_state = FLAG;
  MultiCellCommand g = {0};
  unsigned int i, code;
  uint64_t lcode;
  int64_t accumulator;
  bool is_negative;
  (void)is_negative;
  size_t sz;

  enum KEYS { width = 'w', scale = 's', subscale = 'f' };

  enum KEYS key = 'a';
  if (parser_buf[pos] == ';')
    state = AFTER_VALUE;

  while (pos < parser_buf_pos) {
    switch (state) {
    case KEY:
      key = parser_buf[pos++];
      state = EQUAL;
      switch (key) {
      case width:
        value_state = UINT;
        break;
      case scale:
        value_state = UINT;
        break;
      case subscale:
        value_state = UINT;
        break;
      default:
        REPORT_ERROR("Malformed MultiCellCommand control block, invalid key "
                     "character: 0x%x",
                     key);
        return;
      }
      break;

    case EQUAL:
      if (parser_buf[pos++] != '=') {
        REPORT_ERROR("Malformed MultiCellCommand control block, no = after "
                     "key, found: 0x%x instead",
                     parser_buf[pos - 1]);
        return;
      }
      state = value_state;
      break;

    case FLAG:
      switch (key) {

      default:
        break;
      }
      state = AFTER_VALUE;
      break;

    case INT:
#define READ_UINT                                                              \
  for (i = pos, accumulator = 0; i < MIN(parser_buf_pos, pos + 10); i++) {     \
    int64_t n = parser_buf[i] - '0';                                           \
    if (n < 0 || n > 9)                                                        \
      break;                                                                   \
    accumulator += n * digit_multipliers[i - pos];                             \
  }                                                                            \
  if (i == pos) {                                                              \
    REPORT_ERROR("Malformed MultiCellCommand control block, expecting an "     \
                 "integer value for key: %c",                                  \
                 key & 0xFF);                                                  \
    return;                                                                    \
  }                                                                            \
  lcode = accumulator / digit_multipliers[i - pos - 1];                        \
  pos = i;                                                                     \
  if (lcode > UINT32_MAX) {                                                    \
    REPORT_ERROR(                                                              \
        "Malformed MultiCellCommand control block, number is too large");      \
    return;                                                                    \
  }                                                                            \
  code = lcode;

      is_negative = false;
      if (parser_buf[pos] == '-') {
        is_negative = true;
        pos++;
      }
#define I(x)                                                                   \
  case x:                                                                      \
    g.x = is_negative ? 0 - (int32_t)code : (int32_t)code;                     \
    break
      READ_UINT;
      switch (key) {
        ;
      default:
        break;
      }
      state = AFTER_VALUE;
      break;
#undef I
    case UINT:
      READ_UINT;
#define U(x)                                                                   \
  case x:                                                                      \
    g.x = code;                                                                \
    break
      switch (key) {
        U(width);
        U(scale);
        U(subscale);
      default:
        break;
      }
      state = AFTER_VALUE;
      break;
#undef U
#undef READ_UINT

    case AFTER_VALUE:
      switch (parser_buf[pos++]) {
      default:
        REPORT_ERROR("Malformed MultiCellCommand control block, expecting a : "
                     "or semi-colon after a value, found: 0x%x",
                     parser_buf[pos - 1]);
        return;
      case ':':
        state = KEY;
        break;
      case ';':
        state = PAYLOAD;
        break;
      }
      break;

    case PAYLOAD: {
      sz = parser_buf_pos - pos;
      payload_start = pos;
      g.payload_sz = MAX(BUF_EXTRA, sz);
      pos = parser_buf_pos;
    } break;

    } // end switch
  } // end while

  switch (state) {
  case EQUAL:
    REPORT_ERROR("Malformed MultiCellCommand control block, no = after key");
    return;
  case INT:
  case UINT:
    REPORT_ERROR(
        "Malformed MultiCellCommand control block, expecting an integer value");
    return;
  case FLAG:
    REPORT_ERROR(
        "Malformed MultiCellCommand control block, expecting a flag value");
    return;
  default:
    break;
  }

  REPORT_VA_COMMAND("K s { sI sI sI  sI} y#", self->window_id,
                    "multicell_command",

                    "width", (unsigned int)g.width, "scale",
                    (unsigned int)g.scale, "subscale", (unsigned int)g.subscale,

                    "payload_sz", g.payload_sz, parser_buf, g.payload_sz);

  screen_handle_multicell_command(self->screen, &g, parser_buf + payload_start);
}
