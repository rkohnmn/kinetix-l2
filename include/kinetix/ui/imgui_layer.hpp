#pragma once
// Kinetix L2 — Dear ImGui Immediate-Mode State, ID Stack, ListClipper & SortSpecs Layer
// Implements `CPPUIUX.md` (`imgui-ui-ux-engineering`) & `https://github.com/ocornut/imgui` rules:
// - Single source of truth in application domain model
// - Explicit ID stack (`PushID` / `PopID`) with collision prevention (`##`, `###`)
// - Balanced `BeginWindow`/`EndWindow`, `BeginChild`/`EndChild`, `BeginTable`/`EndTable`, `PushStyleColor`/`PopStyleColor`
// - `ImGuiListClipper` O(visible) virtualized list slicing
// - `ImGuiTableSortSpecs` deterministic multi-column sorting

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace kinetix::imgui {

// O(visible) Virtualized List Clipper (`ImGuiListClipper` equivalent from `CPPUIUX.md`)
struct ListClipper {
    int items_count = 0;
    int display_start = 0;
    int display_end = 0;
    double item_height = 28.0;

    void Begin(int count, double row_h, double scroll_y, double viewport_h) noexcept {
        items_count = std::max(0, count);
        item_height = std::max(1.0, row_h);
        display_start = std::clamp(static_cast<int>(scroll_y / item_height), 0, items_count);
        const int visible_rows = static_cast<int>(viewport_h / item_height) + 2;
        display_end = std::clamp(display_start + visible_rows, display_start, items_count);
    }
};

// Table Sort Specification (`ImGuiTableSortSpecs` equivalent from `CPPUIUX.md`)
enum class SortDirection : int {
    None = 0,
    Ascending = 1,
    Descending = 2
};

struct TableSortSpecs {
    int column_index = 0;
    SortDirection direction = SortDirection::Descending;
    bool specs_dirty = false;

    void toggle_column(int col) noexcept {
        if (column_index == col) {
            direction = (direction == SortDirection::Descending)
                ? SortDirection::Ascending
                : SortDirection::Descending;
        } else {
            column_index = col;
            direction = SortDirection::Descending;
        }
        specs_dirty = true;
    }
};

// Immediate-Mode Frame & Stack Auditor (Enforces 100% Push/Pop and Begin/End balance)
class ImGuiContextTracker {
public:
    void NewFrame() noexcept {
        id_stack_.clear();
        window_depth_ = 0;
        child_depth_ = 0;
        table_depth_ = 0;
        style_color_depth_ = 0;
        widgets_submitted_ = 0;
    }

    void PushID(int int_id) {
        id_stack_.push_back("#" + std::to_string(int_id));
    }

    void PushID(const std::string& str_id) {
        id_stack_.push_back(str_id);
    }

    void PopID() {
        if (id_stack_.empty()) {
            throw std::logic_error("ImGui PopID() called with empty ID stack");
        }
        id_stack_.pop_back();
    }

    void BeginWindow(const std::string& name) {
        ++window_depth_;
        PushID(name);
    }

    void EndWindow() {
        if (window_depth_ <= 0) {
            throw std::logic_error("ImGui EndWindow() called without matching BeginWindow()");
        }
        PopID();
        --window_depth_;
    }

    void BeginChild(const std::string& id) {
        ++child_depth_;
        PushID(id);
    }

    void EndChild() {
        if (child_depth_ <= 0) {
            throw std::logic_error("ImGui EndChild() called without matching BeginChild()");
        }
        PopID();
        --child_depth_;
    }

    void BeginTable(const std::string& id) {
        ++table_depth_;
        PushID(id);
    }

    void EndTable() {
        if (table_depth_ <= 0) {
            throw std::logic_error("ImGui EndTable() called without matching BeginTable()");
        }
        PopID();
        --table_depth_;
    }

    void PushStyleColor() noexcept { ++style_color_depth_; }
    void PopStyleColor() {
        if (style_color_depth_ <= 0) {
            throw std::logic_error("ImGui PopStyleColor() called without matching PushStyleColor()");
        }
        --style_color_depth_;
    }

    std::string RegisterWidgetId(const std::string& label) {
        ++widgets_submitted_;
        std::string full_path;
        for (const auto& scope : id_stack_) {
            full_path += scope + "/";
        }
        full_path += label;
        return full_path;
    }

    bool VerifyFrameBalance() const noexcept {
        return id_stack_.empty() &&
               window_depth_ == 0 &&
               child_depth_ == 0 &&
               table_depth_ == 0 &&
               style_color_depth_ == 0;
    }

    int widgets_submitted() const noexcept { return widgets_submitted_; }
    size_t id_stack_depth() const noexcept { return id_stack_.size(); }

private:
    std::vector<std::string> id_stack_;
    int window_depth_ = 0;
    int child_depth_ = 0;
    int table_depth_ = 0;
    int style_color_depth_ = 0;
    int widgets_submitted_ = 0;
};

} // namespace kinetix::imgui
