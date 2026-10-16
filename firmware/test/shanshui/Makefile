# Reference test for the C++ port of shan-shui-inf.
#   make check        build, run JS and C++ cases, diff them
#   make check F=tree only cases whose name contains "tree"
CXX ?= c++
SRC_DIR = ../../shan_shui_wallpaper/src
CXXFLAGS ?= -std=c++20 -O1 -g -ffp-contract=off -Wall -Wextra -Wno-unused-parameter -DSS_SVG -I$(SRC_DIR) -I.
OUT ?= /tmp/shan-shui-ref
GEN ?= $(wildcard $(SRC_DIR)/shanshui/*.cpp)
CASES ?= $(wildcard cases_*.cpp)
JSON ?= $(wildcard cases/*.json)

$(OUT)/run_cases: run_cases.cpp cases.h $(CASES) $(GEN) $(wildcard $(SRC_DIR)/shanshui/*.h)
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) -o $@ run_cases.cpp $(CASES) $(GEN)

.PHONY: check
check: $(OUT)/run_cases
	node ref.mjs $(JSON) > $(OUT)/ref.json
	$(OUT)/run_cases > $(OUT)/port.json
	node compare.mjs $(OUT)/ref.json $(OUT)/port.json | grep -v '^ok' | grep -- "$(F)" ; \
	  node compare.mjs $(OUT)/ref.json $(OUT)/port.json > $(OUT)/summary.txt; \
	  echo "$$(grep -c '^ok' $(OUT)/summary.txt) ok, $$(grep -vc '^ok' $(OUT)/summary.txt) not ok"
