#pragma once

// Expansion of `{{name}}` placeholders in the string parameters of `to` events
// (`shell_command` and `set_notification_message.text`).
//
// The `{{...}}` syntax is used because it has no special meaning in `/bin/sh`
// (unlike `$name`, `${name}`, `$(...)` or `$[...]`), so it does not collide with
// anything that already appears in real shell commands.
//
// This header intentionally depends only on the standard library so that it can be unit tested in isolation.

#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace krbn::manipulator::event_variable_interpolation {

using variables = std::vector<std::pair<std::string_view, std::string_view>>;

// A value is substituted only if it consists solely of ASCII letters, digits and `_`.
//
// `shell_command` is passed to `/bin/sh -c`, so a value containing whitespace or shell metacharacters
// could change the meaning of the command. Key names are always in this character set,
// so this check never rejects a real key name; it only guards against unexpected values.
[[nodiscard]] inline bool is_safe_value(std::string_view value) {
  if (value.empty()) {
    return false;
  }

  for (unsigned char c : value) {
    if (!std::isalnum(c) && c != '_') {
      return false;
    }
  }

  return true;
}

// Replace each `{{name}}` in `text` with the value of `name` in `variables`.
// Spaces and tabs around `name` are ignored, so `{{ key_code }}` is the same as `{{key_code}}`.
//
// A placeholder is left exactly as written when
// - `name` is not in `variables`, or
// - the value is not safe (see `is_safe_value`), or
// - the placeholder is not terminated by `}}`.
//
// Nothing else is touched. In particular `$name`, `${name}` and other shell syntax are never expanded.
[[nodiscard]] inline std::string expand(const std::string& text,
                                        const variables& variables) {
  constexpr std::string_view open_token = "{{";
  constexpr std::string_view close_token = "}}";

  std::string result;
  result.reserve(text.size());

  std::size_t pos = 0;
  while (pos < text.size()) {
    auto open = text.find(open_token, pos);
    if (open == std::string::npos) {
      result.append(text, pos, std::string::npos);
      break;
    }

    result.append(text, pos, open - pos);

    auto name_begin = open + open_token.size();
    auto close = text.find(close_token, name_begin);
    if (close == std::string::npos) {
      // Unterminated placeholder: copy the rest as-is.
      result.append(text, open, std::string::npos);
      break;
    }

    std::string_view name(text.data() + name_begin, close - name_begin);
    while (!name.empty() && (name.front() == ' ' || name.front() == '\t')) {
      name.remove_prefix(1);
    }
    while (!name.empty() && (name.back() == ' ' || name.back() == '\t')) {
      name.remove_suffix(1);
    }

    bool replaced = false;
    for (const auto& [variable_name, value] : variables) {
      if (variable_name == name && is_safe_value(value)) {
        result.append(value);
        replaced = true;
        break;
      }
    }

    auto end = close + close_token.size();
    if (!replaced) {
      result.append(text, open, end - open);
    }

    pos = end;
  }

  return result;
}

} // namespace krbn::manipulator::event_variable_interpolation
