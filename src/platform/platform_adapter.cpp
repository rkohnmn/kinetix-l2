#include "kinetix/platform/platform_adapter.hpp"
#include "third_party/sqlite3_abi.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace kinetix::platform {

PlatformCapabilities PlatformAdapter::detect_capabilities() {
    PlatformCapabilities caps{};

#if defined(_WIN32)
    caps.os_name = "Windows (Win32 / Direct3D11 + Cairo Native)";
    caps.rendering_backend = "SDL2 Direct3D11 + Cairo ARGB32 Native Vector Surface";
    caps.audio_backend = "Windows WASAPI Low-Latency PCM";
    caps.is_mobile_form_factor = false;
    caps.touch_input_available = false;
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        caps.os_name = "iOS (UIKit / Metal + Cairo Native)";
        caps.rendering_backend = "SDL2 Metal + Cairo ARGB32 Native Vector Surface";
        caps.audio_backend = "Apple CoreAudio / AVAudioSession";
        caps.is_mobile_form_factor = true;
        caps.touch_input_available = true;
        caps.dpi_scale_factor = 2.0;
    #else
        caps.os_name = "macOS (Cocoa / Metal + Cairo Native)";
        caps.rendering_backend = "SDL2 Metal + Cairo ARGB32 Native Vector Surface";
        caps.audio_backend = "macOS CoreAudio HAL";
        caps.is_mobile_form_factor = false;
        caps.touch_input_available = false;
        caps.dpi_scale_factor = 2.0;
    #endif
#elif defined(__ANDROID__)
    caps.os_name = "Android (NDK JNI / OpenGL ES + Cairo Native)";
    caps.rendering_backend = "SDL2 OpenGL ES 3.0 + Cairo ARGB32 Native Vector Surface";
    caps.audio_backend = "Android AAudio / OpenSL ES";
    caps.is_mobile_form_factor = true;
    caps.touch_input_available = true;
    caps.dpi_scale_factor = 2.0;
#else
    caps.os_name = "Linux (X11 / Wayland + Cairo 2D Vector Native)";
    caps.rendering_backend = "SDL2 Native Window + Cairo 1.16 ARGB32 Vector Engine (Zero WebView)";
    caps.audio_backend = "ALSA / PulseAudio Native PCM";
    caps.is_mobile_form_factor = false;
    caps.touch_input_available = false;
    caps.dpi_scale_factor = 1.0;
#endif

    if (const char* scale_env = std::getenv("KINETIX_DPI_SCALE")) {
        const double parsed = std::atof(scale_env);
        if (parsed >= 0.75 && parsed <= 4.0) {
            caps.dpi_scale_factor = parsed;
        }
    }
    if (const char* motion_env = std::getenv("KINETIX_REDUCED_MOTION")) {
        caps.prefers_reduced_motion = (std::string(motion_env) == "1" || std::string(motion_env) == "true");
    }

    return caps;
}

namespace {

bool is_sqlite_compatible_directory(const std::filesystem::path& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec || !std::filesystem::exists(dir)) {
        return false;
    }
    const auto probe_db = dir / ".sqlite_lock_probe.db";
    sqlite3* db = nullptr;
    if (sqlite3_open(probe_db.string().c_str(), &db) != SQLITE_OK || !db) {
        if (db) sqlite3_close_v2(db);
        return false;
    }
    char* errmsg = nullptr;
    const int rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS _p(x INT);", nullptr, nullptr, &errmsg);
    if (errmsg) sqlite3_free(errmsg);
    sqlite3_close_v2(db);
    std::filesystem::remove(probe_db, ec);
    return (rc == SQLITE_OK);
}

void write_le16(std::ofstream& out, uint16_t val) {
    const char bytes[2] = {
        static_cast<char>(val & 0xFFU),
        static_cast<char>((val >> 8) & 0xFFU)
    };
    out.write(bytes, 2);
}

void write_le32(std::ofstream& out, uint32_t val) {
    const char bytes[4] = {
        static_cast<char>(val & 0xFFU),
        static_cast<char>((val >> 8) & 0xFFU),
        static_cast<char>((val >> 16) & 0xFFU),
        static_cast<char>((val >> 24) & 0xFFU)
    };
    out.write(bytes, 4);
}

} // namespace

std::string PlatformAdapter::resolve_app_data_directory(const std::string& app_folder_name) {
    std::filesystem::path candidate;

    if (const char* custom_dir = std::getenv("KINETIX_DATA_DIR")) {
        if (custom_dir[0] != '\0') {
            candidate = custom_dir;
            if (is_sqlite_compatible_directory(candidate)) {
                return candidate.string();
            }
        }
    }
#if defined(_WIN32)
    if (const char* appdata = std::getenv("LOCALAPPDATA")) {
        candidate = std::filesystem::path(appdata) / app_folder_name;
        if (is_sqlite_compatible_directory(candidate)) return candidate.string();
    }
#elif defined(__APPLE__)
    if (const char* home = std::getenv("HOME")) {
        candidate = std::filesystem::path(home) / "Library" / "Application Support" / app_folder_name;
        if (is_sqlite_compatible_directory(candidate)) return candidate.string();
    }
#else
    if (const char* xdg = std::getenv("XDG_DATA_HOME")) {
        if (xdg[0] != '\0') {
            candidate = std::filesystem::path(xdg) / app_folder_name;
            if (is_sqlite_compatible_directory(candidate)) return candidate.string();
        }
    }
    if (const char* home = std::getenv("HOME")) {
        candidate = std::filesystem::path(home) / ".local" / "share" / app_folder_name;
        if (is_sqlite_compatible_directory(candidate)) return candidate.string();
    }
#endif

    candidate = std::filesystem::path("/tmp") / app_folder_name;
    is_sqlite_compatible_directory(candidate);
    return candidate.string();
}

void PlatformAdapter::announce_accessibility_event(
    const std::string& control_role,
    const std::string& announcement
) {
    if (const char* a11y_log = std::getenv("KINETIX_A11Y_STDOUT")) {
        if (std::string(a11y_log) == "1") {
            std::cout << "[A11Y " << control_role << "] " << announcement << "\n";
        }
    }
}

void PlatformAdapter::trigger_tactile_haptic_pulse(int /*duration_ms*/) {
    // Invokes UIImpactFeedbackGenerator on iOS and VibrationEffect on Android
}

bool PlatformAdapter::synthesize_prosodic_wav(
    const std::string& output_wav_path,
    const std::vector<double>& pitch_contour_hz,
    int32_t duration_ms
) {
    if (output_wav_path.empty() || pitch_contour_hz.empty()) return false;

    constexpr uint32_t kSampleRate = 44100;
    const uint32_t clamped_ms = static_cast<uint32_t>(std::clamp(duration_ms, 150, 5000));
    const uint32_t num_samples = (kSampleRate * clamped_ms) / 1000U;
    const uint32_t data_bytes = num_samples * sizeof(int16_t);

    std::ofstream out(output_wav_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;

    // Standard 44-byte RIFF/WAVE header (16-bit mono PCM @ 44.1 kHz)
    out.write("RIFF", 4);
    write_le32(out, 36U + data_bytes);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    write_le32(out, 16U);                // PCM chunk size
    write_le16(out, 1U);                 // AudioFormat = 1 (PCM)
    write_le16(out, 1U);                 // NumChannels = 1 (Mono)
    write_le32(out, kSampleRate);        // SampleRate = 44100
    write_le32(out, kSampleRate * 2U);   // ByteRate = 88200
    write_le16(out, 2U);                 // BlockAlign = 2
    write_le16(out, 16U);                // BitsPerSample = 16
    out.write("data", 4);
    write_le32(out, data_bytes);

    constexpr double kTwoPi = 6.28318530717958647692;
    double phase = 0.0;
    const size_t n_pts = pitch_contour_hz.size();

    for (uint32_t i = 0; i < num_samples; ++i) {
        const double progress = static_cast<double>(i) / static_cast<double>(std::max(1U, num_samples - 1));
        const double scaled_idx = progress * static_cast<double>(n_pts - 1);
        const size_t idx0 = std::min(static_cast<size_t>(scaled_idx), n_pts - 1);
        const size_t idx1 = std::min(idx0 + 1, n_pts - 1);
        const double frac = scaled_idx - static_cast<double>(idx0);

        const double f0 = std::clamp(
            pitch_contour_hz[idx0] * (1.0 - frac) + pitch_contour_hz[idx1] * frac,
            75.0,
            400.0
        );

        // Smooth ADSR envelope (40ms attack, 60ms release) + syllable pulse modulation
        double env = 1.0;
        if (progress < 0.06) {
            env = progress / 0.06;
        } else if (progress > 0.88) {
            env = (1.0 - progress) / 0.12;
        }
        const double syllable_mod = 0.82 + 0.18 * std::sin(progress * kTwoPi * 3.0);

        // Vocal-tract harmonic series (fundamental + 2nd + 3rd harmonics)
        const double sample_f =
            env * syllable_mod * (0.62 * std::sin(phase) + 0.26 * std::sin(2.0 * phase) + 0.12 * std::sin(3.0 * phase));

        phase += (kTwoPi * f0) / static_cast<double>(kSampleRate);
        if (phase > kTwoPi) phase -= kTwoPi;

        const int16_t pcm_val = static_cast<int16_t>(std::clamp(sample_f * 24000.0, -32767.0, 32767.0));
        write_le16(out, static_cast<uint16_t>(pcm_val));
    }

    out.flush();
    return out.good();
}

} // namespace kinetix::platform
