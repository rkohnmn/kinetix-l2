#pragma once
// Exact C ABI declarations for SDL2 (libSDL2-2.0.so.0 / SDL2.dll / libSDL2.dylib)
// Used for native cross-platform window management, hardware texture blitting,
// high-DPI scaling, keyboard/mouse/touch events, and PCM audio synthesis.

#include <cstdint>

extern "C" {

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Surface SDL_Surface;

typedef int32_t SDL_Keycode;
typedef int32_t SDL_Scancode;

struct SDL_version {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
};

struct SDL_Rect {
    int x, y;
    int w, h;
};

// Init flags
#define SDL_INIT_TIMER          0x00000001u
#define SDL_INIT_AUDIO          0x00000010u
#define SDL_INIT_VIDEO          0x00000020u
#define SDL_INIT_EVENTS         0x00004000u

// Window flags
#define SDL_WINDOW_OPENGL       0x00000002u
#define SDL_WINDOW_SHOWN        0x00000004u
#define SDL_WINDOW_HIDDEN       0x00000008u
#define SDL_WINDOW_RESIZABLE    0x00000020u
#define SDL_WINDOW_ALLOW_HIGHDPI 0x00002000u

#define SDL_WINDOWPOS_CENTERED  0x2FFF0000u

// Renderer flags
#define SDL_RENDERER_SOFTWARE      0x00000001u
#define SDL_RENDERER_ACCELERATED   0x00000002u
#define SDL_RENDERER_PRESENTVSYNC  0x00000004u

// Pixel format ARGB8888 (matches CAIRO_FORMAT_ARGB32 on little-endian x86_64/ARM64)
#define SDL_PIXELFORMAT_ARGB8888   0x16362004u
#define SDL_TEXTUREACCESS_STREAMING 1

// Event types
#define SDL_QUIT                0x100u
#define SDL_WINDOWEVENT         0x200u
#define SDL_KEYDOWN             0x300u
#define SDL_KEYUP               0x301u
#define SDL_TEXTINPUT           0x303u
#define SDL_MOUSEMOTION         0x400u
#define SDL_MOUSEBUTTONDOWN     0x401u
#define SDL_MOUSEBUTTONUP       0x402u
#define SDL_MOUSEWHEEL          0x403u

// Keycodes
#define SDLK_RETURN             '\r'
#define SDLK_ESCAPE             '\033'
#define SDLK_BACKSPACE          '\b'
#define SDLK_TAB                '\t'
#define SDLK_SPACE              ' '
#define SDLK_1                  '1'
#define SDLK_2                  '2'
#define SDLK_3                  '3'
#define SDLK_4                  '4'
#define SDLK_5                  '5'
#define SDLK_r                  'r'
#define SDLK_t                  't'
#define SDLK_a                  'a'
#define SDLK_s                  's'
#define SDLK_d                  'd'
#define SDLK_f                  'f'
#define SDLK_k                  'k'
#define SDLK_m                  'm'
#define KMOD_CTRL               0x00C0u
#define SDLK_F1                 (1073741882)
#define SDLK_F2                 (1073741883)
#define SDLK_F3                 (1073741884)
#define SDLK_F4                 (1073741885)
#define SDLK_F5                 (1073741886)
#define SDLK_RIGHT              (1073741903)
#define SDLK_LEFT               (1073741904)
#define SDLK_DOWN               (1073741905)
#define SDLK_UP                 (1073741906)

struct SDL_Keysym {
    SDL_Scancode scancode;
    SDL_Keycode sym;
    uint16_t mod;
    uint32_t unused;
};

struct SDL_KeyboardEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint8_t state;
    uint8_t repeat;
    uint8_t padding2;
    uint8_t padding3;
    SDL_Keysym keysym;
};

struct SDL_TextInputEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    char text[32];
};

struct SDL_MouseMotionEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint32_t which;
    uint32_t state;
    int32_t x;
    int32_t y;
    int32_t xrel;
    int32_t yrel;
};

struct SDL_MouseButtonEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint32_t which;
    uint8_t button;
    uint8_t state;
    uint8_t clicks;
    uint8_t padding1;
    int32_t x;
    int32_t y;
};

struct SDL_WindowEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint8_t event;
    uint8_t padding1;
    uint8_t padding2;
    uint8_t padding3;
    int32_t data1;
    int32_t data2;
};

union SDL_Event {
    uint32_t type;
    SDL_WindowEvent window;
    SDL_KeyboardEvent key;
    SDL_TextInputEvent text;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    uint8_t padding[56];
};

void SDL_GetVersion(SDL_version* ver);
const char* SDL_GetError(void);
const char* SDL_GetPlatform(void);
const char* SDL_GetCurrentVideoDriver(void);
int SDL_SetHint(const char* name, const char* value);
int SDL_Init(uint32_t flags);
void SDL_Quit(void);

SDL_Window* SDL_CreateWindow(
    const char* title,
    int x, int y, int w, int h,
    uint32_t flags
);
void SDL_DestroyWindow(SDL_Window* window);
void SDL_GetWindowSize(SDL_Window* window, int* w, int* h);
void SDL_SetWindowTitle(SDL_Window* window, const char* title);

SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, uint32_t flags);
void SDL_DestroyRenderer(SDL_Renderer* renderer);
int SDL_RenderClear(SDL_Renderer* renderer);
int SDL_RenderCopy(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    const SDL_Rect* srcrect,
    const SDL_Rect* dstrect
);
void SDL_RenderPresent(SDL_Renderer* renderer);

SDL_Texture* SDL_CreateTexture(
    SDL_Renderer* renderer,
    uint32_t format,
    int access,
    int w, int h
);
void SDL_DestroyTexture(SDL_Texture* texture);
int SDL_UpdateTexture(
    SDL_Texture* texture,
    const SDL_Rect* rect,
    const void* pixels,
    int pitch
);

int SDL_PollEvent(SDL_Event* event);
int SDL_PushEvent(SDL_Event* event);
void SDL_StartTextInput(void);
void SDL_StopTextInput(void);
void SDL_Delay(uint32_t ms);
uint64_t SDL_GetTicks64(void);

} // extern "C"
