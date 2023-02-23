from curses import keyname

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.build import check_max_cppstd, check_min_cppstd

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
  java -jar ~/.local/bin/antlr-VERSION-complete.jar -o Parser -lib Parser -encoding UTF-8 -Dlanguage=Cpp -listener $1
fi
"""

class Retractor(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    license = "MIT"
    author = "Michal Widera"
    description = "RetractorDB time series database"
    homepage = "https://retractordb.com"
    requires = "boost/1.81.0", "gtest/1.13.0" , "antlr4-cppruntime/4.11.1" , "spdlog/1.11.0"
    # generators = "cmake" , "cmake_find_package"
    # generators = "CMakeDeps" , "CMakeToolchain"
    testing = []
    package_type = "application"

    default_options = {"boost/*:shared": False,
                       "boost/*:without_serialization": False,
                       "boost/*:without_thread" : False,
                       "boost/*:without_program_options" : False,
                       "boost/*:without_test" : False,
                       "boost/*:multithreading" : True,
                       "boost/*:without_system" : False,
                       "boost/*:without_filesystem" : False }

    def validate(self):
        check_min_cppstd(self, "17")
        # check_max_cppstd(self, "23")

    def layout(self):
        self.cpp.package.includedirs.append("other_includes")
        cmake_layout(self)

    def package_info(self):
        self.cpp_info.system_libs = ["pthread", "rt", "dl"]

    def requirements(self):
        # Auto-generation of antlr4call.sh script

        antlr4_version_file = open("scripts/antlr4call.sh","w")
        antlr4_version_file.write(script.replace('VERSION',str("4.11.1")))
        antlr4_version_file.close()

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
