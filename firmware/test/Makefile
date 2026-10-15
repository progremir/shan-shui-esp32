CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -DHOST_TEST -I../shan_shui_wallpaper
OUT ?= /tmp/shan-shui-host-test

SRC = host_test.cpp \
	../shan_shui_wallpaper/protocol.cpp \
	../shan_shui_wallpaper/st7305_fb.cpp \
	../shan_shui_wallpaper/strip.cpp

.PHONY: test
test: $(OUT)
	$(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(OUT) $(SRC)
