"""Module required for conan build system."""
from curses import keyname

from conan import ConanFile, Version, __version__ as conan_version
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout

script = """#!/bin/bash

# This file is auto-generted by retractordb/conanfile.py by conan install ..

export ANTLR_HOME="~/.local/bin"
export ANTLR_JAR="$ANTLR_HOME/antlr-VERSION-complete.jar"
export CLASSPATH=".:$ANTLR_JAR:$CLASSPATH"
alias antlr4="java -jar $ANTLR_JAR"
alias grun="java org.antlr.v4.gui.TestRig"

cd ~/.local/bin && [ ! -f "antlr-VERSION-complete.jar" ] && wget https://www.antlr.org/download/antlr-VERSION-complete.jar
cd -

if [ $1 ] ; then
  java -jar ~/.local/bin/antlr-VERSION-complete.jar -o lib/Parser -lib lib/Parser -encoding UTF-8 -Dlanguage=Cpp -listener $1
fi
"""


class Retractor(ConanFile):
    """This class is required for conan file build system."""

    settings = "os", "compiler", "build_type", "arch"
    license = "MIT"
    author = "Michal Widera"
    description = "RetractorDB - time series database and data processing engine"
    homepage = "https://retractordb.com"
    antlr_version = "4.13.0"
    requires = (
        "boost/1.82.0",
        "gtest/1.13.0",
        "antlr4-cppruntime/" + antlr_version,
        "spdlog/1.10.0",
    )
    generators = "CMakeToolchain"
    testing = []
    package_type = "application"

    default_options = {
        "boost/*:shared": False,
        "boost/*:without_serialization": False,
        "boost/*:without_thread": False,
        "boost/*:without_program_options": False,
        "boost/*:without_test": False,
        "boost/*:multithreading": True,
        "boost/*:without_system": False,
        "boost/*:without_filesystem": False,
        "spdlog/*:header_only": True,
    }

    def layout(self):
        """Choose cmake_layout output."""
        cmake_layout(self)

    def package_info(self):
        """Set libraries required for compile and execute."""
        self.cpp_info.system_libs = ["pthread", "rt", "dl"]
        self.cpp_info.compiler = "20"

    def requirements(self):
        """Auto-generation of antlr4call.sh script"""
        antlr4_version_file = open("scripts/antlr4call.sh", "w")
        antlr4_version_file.write(script.replace("VERSION", self.antlr_version))
        antlr4_version_file.close()

    def generate(self):
        """Generate."""
        deps = CMakeDeps(self)
        deps.check_components_exist = True
        deps.generate()

    def build(self):
        """Build."""
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if conan_version >= Version("2.0"):
            cmake.install()
