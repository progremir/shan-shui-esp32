#include <stdio.h>
#include <string.h>

#include "cases.h"

std::vector<RefCase>& ref_cases() {
  static std::vector<RefCase> v;
  return v;
}

std::string fixed_list(const std::vector<double>& v, int digits) {
  std::string s;
  for (size_t i = 0; i < v.size(); i++) s += (i ? "," : "") + ss::fixed(v[i], digits);
  return s;
}

static std::string pt_json(const ss::Pt& p, int digits) {
  return "[\"" + ss::fixed(p.x, digits) + "\",\"" + ss::fixed(p.y, digits) + "\"]";
}

std::string pts_json(const ss::Pts& p, int digits) {
  std::string s = "[";
  for (size_t i = 0; i < p.size(); i++) s += (i ? "," : "") + pt_json(p[i], digits);
  return s + "]";
}

std::string tris_json(const std::vector<ss::Pts>& t, int digits) {
  std::string s = "[";
  for (size_t i = 0; i < t.size(); i++) s += (i ? "," : "") + pts_json(t[i], digits);
  return s + "]";
}

static std::string json_str(const std::string& s) {
  std::string o = "\"";
  for (char c : s) {
    if (c == '"' || c == '\\') o += '\\';
    o += c;
  }
  return o + "\"";
}

int main(int argc, char** argv) {
  printf("{\n");
  bool first = true;
  for (auto& c : ref_cases()) {
    if (argc > 1 && strstr(c.name, argv[1]) == nullptr) continue;
    ss::seed(c.seed);
    std::string out = c.fn();
    printf("%s %s: %s", first ? "" : ",\n", json_str(c.name).c_str(), json_str(out).c_str());
    first = false;
  }
  printf("\n}\n");
}
