from curses import keyname
from conans import tools, ConanFile, CMake

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
    settings = "os", "compiler", "build_type", "arch", "cppstd"
    license = "MIT"
    author = "Michal Widera"
    description = "RetractorDB time series database"
    homepage = "https://retractordb.com"
    generators = "cmake" , "cmake_find_package"
    testing = []

    options = {
        "boost": ["1.77.0","1.78.0","1.79.0","1.80.0","1.81.0"],
        "gtest": ["1.11.0","1.12.1","1.13.0"],
        "antlr4" : ["4.9.3","4.10.1","4.11.1"],
        "spdlog" : ["1.10.0","1.11.0"]
    }

    default_options = {"boost:shared": False,
                       "boost:without_serialization": False,
                       "boost:without_thread": False,
                       "boost:without_program_options": False,
                       "boost:without_test": False,
                       "boost:multithreading": True,
                       "boost:without_system": False,
                       "boost:without_filesystem": False, # this could go true - but conan does not have such default
                       "boost": "1.80.0", # 1.81.0 - Failure in debug mode-  boost/1.81.0: Error in package_info() method, line 1673
                       "gtest": "1.13.0",
                       "antlr4": "4.11.1",
                       "spdlog": "1.10.0"
                       }

    def validate(self):
        if not tools.valid_min_cppstd(self, "20"):
            self.output.error("at least C++20 is required.")

    def configure(self):
        self.settings.compiler.cppstd = 20
        self.settings.compiler.libcxx = "libstdc++11"

    def package_info(self):
        self.cpp_info.libs = []
        self.cpp_info.system_libs = ["pthread", "rt", "dl"]

    def requirements(self):
        self.requires("boost/"+str(self.options.boost))
        self.requires("gtest/"+str(self.options.gtest))
        self.requires("antlr4-cppruntime/"+str(self.options.antlr4))
        self.requires("spdlog/"+str(self.options.spdlog))

        # Auto-generation of antlr4call.sh script

        antlr4_version_file = open("../scripts/antlr4call.sh","w")
        antlr4_version_file.write(script.replace('VERSION',str(self.options.antlr4)))
        antlr4_version_file.close()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        cmake.install()
