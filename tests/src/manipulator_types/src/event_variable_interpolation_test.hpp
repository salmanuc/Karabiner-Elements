#include "manipulator/types/event_variable_interpolation.hpp"
#include <boost/ut.hpp>

void run_event_variable_interpolation_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  namespace interpolation = krbn::manipulator::event_variable_interpolation;

  "event_variable_interpolation::is_safe_value"_test = [] {
    expect(interpolation::is_safe_value("a"));
    expect(interpolation::is_safe_value("left_arrow"));
    expect(interpolation::is_safe_value("f12"));
    expect(interpolation::is_safe_value("keypad_1"));

    expect(!interpolation::is_safe_value(""));
    expect(!interpolation::is_safe_value("a b"));
    expect(!interpolation::is_safe_value("a;b"));
    expect(!interpolation::is_safe_value("$(id)"));
    expect(!interpolation::is_safe_value("a'b"));
    expect(!interpolation::is_safe_value("a\nb"));
  };

  "event_variable_interpolation::expand"_test = [] {
    interpolation::variables vars{{"key_code", "tab"}};

    // basic
    expect(interpolation::expand("{{key_code}}", vars) == "tab");
    expect(interpolation::expand("Pressed: [Hyper] + {{key_code}}", vars) == "Pressed: [Hyper] + tab");
    expect(interpolation::expand("my_program --key '{{key_code}}'", vars) == "my_program --key 'tab'");

    // spaces and tabs inside the braces are ignored
    expect(interpolation::expand("{{ key_code }}", vars) == "tab");
    expect(interpolation::expand("{{\tkey_code  }}", vars) == "tab");

    // multiple occurrences
    expect(interpolation::expand("{{key_code}}-{{key_code}}", vars) == "tab-tab");
    expect(interpolation::expand("{{key_code}}{{key_code}}", vars) == "tabtab");

    // no placeholder
    expect(interpolation::expand("", vars) == "");
    expect(interpolation::expand("open -a Safari", vars) == "open -a Safari");

    // unknown variables are kept as written
    expect(interpolation::expand("{{name}}", vars) == "{{name}}");
    expect(interpolation::expand("{{name}}/{{key_code}}", vars) == "{{name}}/tab");
    expect(interpolation::expand("{{}}", vars) == "{{}}");
    expect(interpolation::expand("{{ }}", vars) == "{{ }}");
    expect(interpolation::expand("{{KEY_CODE}}", vars) == "{{KEY_CODE}}");
    expect(interpolation::expand("{{key code}}", vars) == "{{key code}}");

    // existing shell syntax is never touched, including the old `${key_code}` form
    expect(interpolation::expand("echo ${HOME}", vars) == "echo ${HOME}");
    expect(interpolation::expand("echo ${key_code}", vars) == "echo ${key_code}");
    expect(interpolation::expand("echo $key_code", vars) == "echo $key_code");
    expect(interpolation::expand("echo $$ $(date) $[1+2]", vars) == "echo $$ $(date) $[1+2]");
    expect(interpolation::expand("echo {a,b} {1..3}", vars) == "echo {a,b} {1..3}");
    expect(interpolation::expand("find . -exec ls {} \\;", vars) == "find . -exec ls {} \\;");
    expect(interpolation::expand("awk '{print $1}'", vars) == "awk '{print $1}'");
    expect(interpolation::expand("[key_code]", vars) == "[key_code]");

    // malformed
    expect(interpolation::expand("{{key_code", vars) == "{{key_code");
    expect(interpolation::expand("{{key_code}", vars) == "{{key_code}");
    expect(interpolation::expand("abc {{", vars) == "abc {{");
    expect(interpolation::expand("{key_code}}", vars) == "{key_code}}");
    expect(interpolation::expand("}}{{key_code}}{{", vars) == "}}tab{{");
    expect(interpolation::expand("{{{key_code}}}", vars) == "{{{key_code}}}");

    // unsafe values are never substituted
    {
      interpolation::variables unsafe_vars{{"key_code", "a; rm -rf ~"}};
      expect(interpolation::expand("echo {{key_code}}", unsafe_vars) == "echo {{key_code}}");
    }
    {
      interpolation::variables empty_value_vars{{"key_code", ""}};
      expect(interpolation::expand("echo {{key_code}}", empty_value_vars) == "echo {{key_code}}");
    }

    // no variables
    expect(interpolation::expand("echo {{key_code}}", interpolation::variables{}) == "echo {{key_code}}");
  };
}
