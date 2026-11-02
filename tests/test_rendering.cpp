#include "kinetix/ui/vector_canvas.hpp"
#include "third_party/sdl2_abi.h"
#include <cstring>
#include <filesystem>
#include <iostream>
#include <random>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

uint32_t pixel(const unsigned char* bytes, int stride, int x, int y) {
    uint32_t value = 0;
    std::memcpy(&value, bytes + y * stride + x * 4, 4);
    return value;
}

int main() {
    std::filesystem::path directory;
    try {
        std::random_device random;
        do {
            directory = std::filesystem::temp_directory_path() / ("kinetix-render-" + std::to_string(random()));
        } while (!std::filesystem::create_directory(directory));
        kinetix::ui::VectorCanvas canvas(96, 64);
        canvas.clear({1, 1, 1, 1});
        canvas.draw_rounded_rect({8, 8, 24, 24}, 0, {1, 0, 0, 1});
        require(pixel(canvas.pixels(), canvas.stride(), 16, 16) == 0xffff0000U, "Cairo must draw red pixels");
        require(pixel(canvas.pixels(), canvas.stride(), 0, 0) == 0xffffffffU, "Cairo must preserve the background");
        require(canvas.draw_svg_string("<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16'><rect width='16' height='16' fill='#00ff00'/></svg>", {40, 8, 16, 16}), "SVG renderer must succeed");
        require(pixel(canvas.pixels(), canvas.stride(), 48, 16) == 0xff00ff00U, "librsvg must draw green pixels");
        canvas.draw_text(2, 40, "Native text", 16, {0, 0, 0, 1}, false, "sans-serif");
        size_t changed = 0;
        for (int y = 40; y < 64; ++y) for (int x = 0; x < 96; ++x)
            if (pixel(canvas.pixels(), canvas.stride(), x, y) != 0xffffffffU) ++changed;
        require(changed > 10, "Pango must render visible glyphs");
        const auto png = (directory / "render.png").string();
        require(canvas.export_png(png), "PNG export failed");
        auto* decoded = cairo_image_surface_create_from_png(png.c_str());
        require(cairo_surface_status(decoded) == CAIRO_STATUS_SUCCESS, "PNG must fully decode");
        require(cairo_image_surface_get_width(decoded) == 96 && cairo_image_surface_get_height(decoded) == 64, "Decoded PNG dimensions differ");
        require(pixel(cairo_image_surface_get_data(decoded), cairo_image_surface_get_stride(decoded), 16, 16) == 0xffff0000U, "PNG must preserve rendered pixels");
        cairo_surface_destroy(decoded);
#if defined(_WIN32)
        SDL_SetMainReady();
#endif
        require(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == 0, SDL_GetError());
        auto* window = SDL_CreateWindow("Rendering contract", 0, 0, 96, 64, SDL_WINDOW_HIDDEN);
        require(window != nullptr, "SDL must create a real hidden window");
        int width = 0, height = 0;
        SDL_GetWindowSize(window, &width, &height);
        require(width == 96 && height == 64, "SDL window dimensions must be real");
        SDL_Event event{};
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_F2;
        require(SDL_PushEvent(&event) == 1, "SDL must accept events");
        bool delivered = false;
        while (SDL_PollEvent(&event))
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F2) delivered = true;
        require(delivered, "SDL must deliver queued keyboard events");
        SDL_DestroyWindow(window);
        SDL_Quit();
        std::filesystem::remove_all(directory);
        std::cout << "PASS: Cairo pixels, Pango glyphs, SVG pixels, decoded PNG, SDL window/events\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        if (!directory.empty()) { std::error_code ec; std::filesystem::remove_all(directory, ec); }
        SDL_Quit();
        return 1;
    }
}
