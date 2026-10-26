from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout


class KinetixL2Conan(ConanFile):
    name = "kinetix-l2"
    version = "2.0.0"
    license = "MIT"
    description = "Native C++20 4-Modality SLA Workbench (Photon Micro-GUI + Dear ImGui Dockspace + PangoCairo)"
    settings = "os", "compiler", "build_type", "arch"

    requires = [
        "sdl/2.26.5",
        "cairo/1.18.0",
        "sqlite3/3.45.0",
    ]

    generators = "CMakeDeps"

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
