# Kinetix L2 v3.1 — Reproducible POSIX / Linux C++20 Makefile
# Links against SQLite3, Cairo 2D, librsvg-2.0, Pango + PangoCairo + HarfBuzz, GObject, and SDL2.

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Iinclude

ifneq ($(wildcard /usr/lib/x86_64-linux-gnu/libsqlite3.so.0),)
    LIBS := /usr/lib/x86_64-linux-gnu/libsqlite3.so.0 \
            /usr/lib/x86_64-linux-gnu/libcairo.so.2 \
            /usr/lib/x86_64-linux-gnu/librsvg-2.so.2 \
            /usr/lib/x86_64-linux-gnu/libpango-1.0.so.0 \
            /usr/lib/x86_64-linux-gnu/libpangocairo-1.0.so.0 \
            /usr/lib/x86_64-linux-gnu/libgobject-2.0.so.0 \
            /usr/lib/x86_64-linux-gnu/libSDL2-2.0.so.0 \
            -lm -lpthread
else
    LIBS := -lsqlite3 -lcairo -lrsvg-2 -lpango-1.0 -lpangocairo-1.0 -lgobject-2.0 -lSDL2 -lm -lpthread
endif

BUILD_DIR := build
CORE_SRCS := src/domain/fsrs_engine.cpp \
             src/domain/knowledge_graph.cpp \
             src/domain/pedagogical_loop.cpp \
             src/domain/habit_engine.cpp \
             src/persistence/database_manager.cpp \
             src/platform/platform_adapter.cpp \
             src/ui/vector_canvas.cpp \
             src/ui/main_window.cpp

APP_BIN   := $(BUILD_DIR)/kinetix-l2
TEST_BIN  := $(BUILD_DIR)/kinetix-tests

.PHONY: all test verify audit clean

all: $(APP_BIN) $(TEST_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(APP_BIN): $(BUILD_DIR) $(CORE_SRCS) src/main.cpp
	$(CXX) $(CXXFLAGS) $(CORE_SRCS) src/main.cpp $(LIBS) -o $(APP_BIN)

$(TEST_BIN): $(BUILD_DIR) $(CORE_SRCS) tests/test_main.cpp
	$(CXX) $(CXXFLAGS) $(CORE_SRCS) tests/test_main.cpp $(LIBS) -o $(TEST_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

verify: $(APP_BIN)
	./$(APP_BIN) --verify-all-features

audit: all
	python3 scripts/native_gui_audit.py . --binary $(APP_BIN)
	python3 scripts/check_imgui_cpp.py src/ui/main_window.cpp tests/test_main.cpp
	python3 scripts/validate_design_md.py .
	python3 scripts/validate_project.py .

clean:
	rm -rf $(BUILD_DIR)
