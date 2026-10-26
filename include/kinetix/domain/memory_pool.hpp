#pragma once
// Kinetix L2 — Modern C++20 Memory Management & RAII Resource Wrappers
// Implements `memcpp.md` & `CPPDEV.md` standards:
// 1. PoolAllocator<T, BlockSize> for zero-fragmentation, cache-friendly allocation
// 2. Move-only RAII smart handles for C ABI resources (SQLite3, Cairo, Pango, RsvgHandle)
// 3. Real-time pool telemetry for the performance profiler dock

#include "third_party/pango_abi.h"
#include "third_party/rsvg_abi.h"
#include "third_party/sqlite3_abi.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace kinetix::memory {

// RAII Wrapper for RsvgHandle (librsvg-2.so.2 GObject handle)
class ScopedRsvgHandle {
public:
    explicit ScopedRsvgHandle(RsvgHandle* handle) noexcept : handle_(handle) {}
    ~ScopedRsvgHandle() noexcept {
        if (handle_) g_object_unref(handle_);
    }
    ScopedRsvgHandle(const ScopedRsvgHandle&) = delete;
    ScopedRsvgHandle& operator=(const ScopedRsvgHandle&) = delete;
    ScopedRsvgHandle(ScopedRsvgHandle&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr)) {}
    ScopedRsvgHandle& operator=(ScopedRsvgHandle&& other) noexcept {
        if (this != &other) {
            if (handle_) g_object_unref(handle_);
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }
    RsvgHandle* get() const noexcept { return handle_; }
    explicit operator bool() const noexcept { return handle_ != nullptr; }
private:
    RsvgHandle* handle_ = nullptr;
};

// RAII Wrapper for PangoFontDescription
class ScopedFontDesc {
public:
    explicit ScopedFontDesc(const char* font_spec) noexcept
        : desc_(pango_font_description_from_string(font_spec)) {}
    ~ScopedFontDesc() noexcept {
        if (desc_) pango_font_description_free(desc_);
    }
    ScopedFontDesc(const ScopedFontDesc&) = delete;
    ScopedFontDesc& operator=(const ScopedFontDesc&) = delete;
    ScopedFontDesc(ScopedFontDesc&& other) noexcept
        : desc_(std::exchange(other.desc_, nullptr)) {}
    ScopedFontDesc& operator=(ScopedFontDesc&& other) noexcept {
        if (this != &other) {
            if (desc_) pango_font_description_free(desc_);
            desc_ = std::exchange(other.desc_, nullptr);
        }
        return *this;
    }
    const PangoFontDescription* get() const noexcept { return desc_; }
private:
    PangoFontDescription* desc_ = nullptr;
};

// RAII Wrapper for PangoLayout
class ScopedPangoLayout {
public:
    explicit ScopedPangoLayout(cairo_t* cr) noexcept
        : layout_(cr ? pango_cairo_create_layout(cr) : nullptr) {}
    ~ScopedPangoLayout() noexcept {
        if (layout_) g_object_unref(layout_);
    }
    ScopedPangoLayout(const ScopedPangoLayout&) = delete;
    ScopedPangoLayout& operator=(const ScopedPangoLayout&) = delete;
    ScopedPangoLayout(ScopedPangoLayout&& other) noexcept
        : layout_(std::exchange(other.layout_, nullptr)) {}
    ScopedPangoLayout& operator=(ScopedPangoLayout&& other) noexcept {
        if (this != &other) {
            if (layout_) g_object_unref(layout_);
            layout_ = std::exchange(other.layout_, nullptr);
        }
        return *this;
    }
    PangoLayout* get() const noexcept { return layout_; }
private:
    PangoLayout* layout_ = nullptr;
};

// RAII Wrapper for Cairo Pattern (Linear / Radial Gradients)
class ScopedCairoPattern {
public:
    explicit ScopedCairoPattern(cairo_pattern_t* pat) noexcept : pat_(pat) {}
    ~ScopedCairoPattern() noexcept {
        if (pat_) cairo_pattern_destroy(pat_);
    }
    ScopedCairoPattern(const ScopedCairoPattern&) = delete;
    ScopedCairoPattern& operator=(const ScopedCairoPattern&) = delete;
    cairo_pattern_t* get() const noexcept { return pat_; }
private:
    cairo_pattern_t* pat_ = nullptr;
};

// Production PoolAllocator<T, BlockSize> from `memcpp.md`
template <typename T, size_t BlockSize = 4096>
class PoolAllocator {
    struct Block {
        alignas(alignof(T)) std::array<std::byte, sizeof(T)> data;
        Block* next = nullptr;
    };

    Block* free_list_ = nullptr;
    std::vector<std::unique_ptr<Block[]>> blocks_;
    size_t active_allocations_ = 0;
    size_t total_capacity_ = 0;

public:
    PoolAllocator() = default;
    ~PoolAllocator() = default;

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    template <typename... Args>
    T* create(Args&&... args) {
        if (!free_list_) {
            allocate_block();
        }
        Block* block = free_list_;
        free_list_ = block->next;
        ++active_allocations_;
        T* obj = reinterpret_cast<T*>(block->data.data());
        std::construct_at(obj, std::forward<Args>(args)...);
        return obj;
    }

    void destroy(T* ptr) noexcept {
        if (!ptr) return;
        std::destroy_at(ptr);
        Block* block = reinterpret_cast<Block*>(ptr);
        block->next = free_list_;
        free_list_ = block;
        if (active_allocations_ > 0) --active_allocations_;
    }

    size_t active_allocations() const noexcept { return active_allocations_; }
    size_t total_capacity() const noexcept { return total_capacity_; }
    size_t block_chunks() const noexcept { return blocks_.size(); }

private:
    void allocate_block() {
        constexpr size_t count = (BlockSize / sizeof(Block)) > 0 ? (BlockSize / sizeof(Block)) : 8;
        auto new_blocks = std::make_unique<Block[]>(count);
        for (size_t i = 0; i + 1 < count; ++i) {
            new_blocks[i].next = &new_blocks[i + 1];
        }
        new_blocks[count - 1].next = free_list_;
        free_list_ = &new_blocks[0];
        total_capacity_ += count;
        blocks_.push_back(std::move(new_blocks));
    }
};

} // namespace kinetix::memory
