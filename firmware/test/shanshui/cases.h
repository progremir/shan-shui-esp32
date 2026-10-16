#pragma once
// Reference cases: each SS_CASE must match the case of the same name in
// cases/*.json, which runs the JavaScript original.
#include <functional>
#include <string>
#include <vector>

#include "shanshui/core.h"

struct RefCase {
  const char* name;
  const char* seed;
  std::function<std::string()> fn;
};
std::vector<RefCase>& ref_cases();
struct RefCaseAdd {
  RefCaseAdd(const char* n, const char* s, std::function<std::string()> f) {
    ref_cases().push_back({n, s, f});
  }
};
#define SS_CAT2(a, b) a##b
#define SS_CAT(a, b) SS_CAT2(a, b)
#define SS_CASE(name, seed) \
  static std::string SS_CAT(case_fn_, __LINE__)(); \
  static RefCaseAdd SS_CAT(case_add_, __LINE__)(name, seed, SS_CAT(case_fn_, __LINE__)); \
  static std::string SS_CAT(case_fn_, __LINE__)()

// Helpers that format like the JS snippets in the cases.
std::string fixed_list(const std::vector<double>& v, int digits);    // v.map(toFixed).join(',')
std::string pts_json(const ss::Pts& p, int digits);                  // JSON of [[x,y]...] toFixed
std::string tris_json(const std::vector<ss::Pts>& t, int digits);
