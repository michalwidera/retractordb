"""Module required for conan build system."""
from conan import ConanFile, tools
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout

# These modules are required for +x on generated script
import os
import stat

class Retractor(ConanFile):
    """This class is required for conan file build system."""
    name = "retractordb"
    version = "0.1.5"
    url = "https://retractordb.com"
    topics = ("time-series", "database", "timeseries", "rdb", "retractordb", "iot", "monitoring", "analytics")
    settings_build = ("os", "compiler", "build_type", "arch")

    generators = ("CMakeToolchain","CMakeDeps","VirtualBuildEnv")
    settings = ("os", "compiler", "build_type", "arch")
    license = "MIT"
    author = "Michal Widera"
    description = "RetractorDB - time series database and data processing engine"
    homepage = "https://retractordb.com"
    # Siatka zgodności licencyjnej (projekt: MIT). Wszystkie zależności są zgodne z MIT.
    #
    # | Komponent                  | Licencja                              | Zgodna z MIT | Uwagi                                |
    # |----------------------------|---------------------------------------|--------------|--------------------------------------|
    # | boost/1.91.0               | BSL-1.0                               | Tak          | permisywna                           |
    # | gtest/1.17.0               | BSD-3-Clause                          | Tak          | tylko testy                          |
    # | antlr4-cppruntime/4.13.2   | BSD-3-Clause                          | Tak          | runtime parsera                      |
    # | antlr4/4.13.1              | BSD-3-Clause                          | Tak          | generator (build-time)               |
    # | spdlog/1.17.0              | MIT                                   | Tak          | header-only                          |
    # | openjdk/21.0.1             | GPL-2.0 WITH Classpath-exception-2.0  | Tak (*)      | build-time, nielinkowany do binariów |
    # | magic_enum/0.9.7           | MIT                                   | Tak          | header-only                          |
    # | tomlplusplus/3.4.0         | MIT                                   | Tak          | header-only                          |
    # | fmt/12.1.0                 | MIT                                   | Tak          | tranzytywna (z spdlog)               |
    # | libbacktrace/cci.20210118  | BSD-3-Clause                          | Tak          | tranzytywna (z boost)                |
    # | b2/5.4.2                   | BSL-1.0                               | Tak          | narzędzie build-only                 |
    # | cmake/4.3.2                | BSD-3-Clause                          | Tak          | narzędzie build-only                 |
    #
    # (*) GPL-2.0 nie infekuje produktu: openjdk uruchamia jedynie ANTLR przy buildzie,
    #     nie jest linkowany do binariów, a Classpath-exception i tak na to zezwala.
    #     Permisywne licencje (MIT/BSD-3-Clause/BSL-1.0) wymagają tylko zachowania
    #     not o prawach autorskich w dystrybucji.
    requires = (
        "boost/1.91.0",
        "gtest/1.17.0",
        "antlr4-cppruntime/4.13.2",
        "antlr4/4.13.1",
        "spdlog/1.17.0",
        "openjdk/21.0.1",
        "magic_enum/0.9.7",
        "tomlplusplus/3.4.0"
    )
    testing = []
    package_type = "application"

    default_options = {
        "boost/*:shared": False,
        "boost/*:multithreading": True,
        "spdlog/*:header_only": True,
        # Boost compiled libraries used by this project
        "boost/*:without_container": False,
        "boost/*:without_program_options": False,
        "boost/*:without_regex": False,
        "boost/*:without_system": False,
        "boost/*:without_stacktrace": False,
        # Boost compiled libraries not used — disabled to reduce build time
        "boost/*:without_atomic": True,
        "boost/*:without_charconv": True,
        "boost/*:without_chrono": True,
        "boost/*:without_cobalt": True,
        "boost/*:without_context": True,
        "boost/*:without_contract": True,
        "boost/*:without_coroutine": True,
        "boost/*:without_date_time": True,
        "boost/*:without_fiber": True,
        "boost/*:without_filesystem": True,
        "boost/*:without_graph": True,
        "boost/*:without_graph_parallel": True,
        "boost/*:without_iostreams": True,
        "boost/*:without_json": True,
        "boost/*:without_locale": True,
        "boost/*:without_log": True,
        "boost/*:without_math": True,
        "boost/*:without_mpi": True,
        "boost/*:without_nowide": True,
        "boost/*:without_process": True,
        "boost/*:without_python": True,
        "boost/*:without_random": True,
        "boost/*:without_serialization": True,
        "boost/*:without_test": True,
        "boost/*:without_thread": True,
        "boost/*:without_timer": True,
        "boost/*:without_type_erasure": True,
        "boost/*:without_url": True,
        "boost/*:without_wave": True,
        "boost/*:without_wserialization": True,
    }

    def layout(self):
        """Choose cmake_layout output."""
        cmake_layout(self, src_folder=".")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.25]")

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        self.copy(pattern="LICENSE", dst="licenses")

    def package_info(self):
        """Set libraries required for compile and execute."""
        self.cpp_info.libs = tools.collect_libs(self)
        self.cpp_info.system_libs = ["pthread", "rt", "dl"]
        self.cpp_info.compiler = "23"

    def build(self):
        """Build."""
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
