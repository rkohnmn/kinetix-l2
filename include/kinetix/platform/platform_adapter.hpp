#pragma once
// Cross-Platform Operating System Adapter (Windows, macOS, Linux, iOS, Android)
// Provides native OS integration for high-DPI display scaling, reduced-motion accessibility,
// native app-data storage paths, platform identification, screen-reader announcements,
// and native 16-bit PCM RIFF/WAVE acoustic prosody synthesis.

#include <cstdint>
#include <string>
#include <vector>

namespace kinetix::platform {

struct PlatformCapabilities {
    std::string os_name;               // "Windows", "macOS", "Linux", "iOS", "Android"
    std::string rendering_backend;     // e.g., "SDL2 Native + Cairo 2D Vector Surface (Zero WebView)"
    std::string audio_backend;         // e.g., "WASAPI / CoreAudio / ALSA-PulseAudio / AAudio"
    double dpi_scale_factor = 1.0;
    bool prefers_dark_theme = true;
    bool prefers_reduced_motion = false;
    bool is_mobile_form_factor = false;
    bool touch_input_available = false;
};

class PlatformAdapter {
public:
    static PlatformCapabilities detect_capabilities();
    static std::string resolve_app_data_directory(const std::string& app_folder_name = "kinetix-l2");
    static void announce_accessibility_event(const std::string& control_role, const std::string& announcement);
    static void trigger_tactile_haptic_pulse(int duration_ms = 15);

    /**
     * @brief Synthesizes a playable 44.1 kHz, 16-bit mono RIFF/WAVE audio file
     *        from a Pimsleur back-chaining F0 pitch contour (Hz) using 3 vocal-tract
     *        formant harmonics and a smooth cosine ADSR envelope.
     */
    static bool synthesize_prosodic_wav(
        const std::string& output_wav_path,
        const std::vector<double>& pitch_contour_hz,
        int32_t duration_ms = 950
    );
};

} // namespace kinetix::platform
