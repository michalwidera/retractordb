"""Module required for conan build system."""
from conan import ConanFile, tools
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout


# These modules are required for +x on generated script
import os
import stat

class Retractor(ConanFile):
    """This class is required for conan file build system."""

    generators = ("CMakeToolchain","CMakeDeps","VirtualBuildEnv")
    settings = ("os", "compiler", "build_type", "arch")
    license = "MIT"
    author = "Michal Widera"
    description = "RetractorDB - time series database and data processing engine"
    homepage = "https://retractordb.com"
    antlr_version = "4.13.1"
    requires = (
        "boost/1.85.0", # 1.86.0 is broken - BOOST_HEADER_DEPRECATED("<boost/process/v1/environment.hpp>")
        "gtest/1.15.0",
        "antlr4-cppruntime/" + antlr_version,
        "antlr4/" + antlr_version,
        "spdlog/1.10.0",
        "openjdk/21.0.1",
    )
    testing = []
    package_type = "application"

    default_options = {
        "boost/*:shared": False,
        "boost/*:without_thread": False,
        "boost/*:without_program_options": False,
        "boost/*:multithreading": True,
        "boost/*:without_system": False,
        "boost/*:without_filesystem": False,
        "spdlog/*:header_only": True,
    }

    def layout(self):
        """Choose cmake_layout output."""
        cmake_layout(self)

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
