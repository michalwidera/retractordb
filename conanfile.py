from conans import tools, ConanFile, CMake

class Retractor(ConanFile):
    settings = "os", "compiler", "build_type", "arch", "cppstd"
    requires = ["cmake/3.21.3"]
    license = "MIT"
    author = "Michal Widera"
    description = "RetractorDB time series database"
    homepage = "https://retractordb.com"
    generators = "cmake" , "cmake_find_package"
    testing = []


    default_options = {"boost:shared": False,
                       "boost:without_serialization": False,
                       "boost:without_thread": False,
                       "boost:without_program_options": False,
                       "boost:without_test": False,
                       "boost:multithreading": True,
                       "boost:without_system": False,
                       "boost:without_filesystem": False
                       }

    def validate(self):
        if not tools.valid_min_cppstd(self, "17"):
            self.output.error("C++17 is required.")

    def configure(self):
        self.settings.compiler.cppstd = 20
        self.settings.compiler.libcxx = "libstdc++11"

    def package_info(self):
        self.cpp_info.libs = []
        self.cpp_info.system_libs = ["pthread", "rt", "dl"]

    def requirements(self):
        self.requires("boost/1.77.0")
        self.requires("gtest/1.11.0")
        self.requires("antlr4-cppruntime/4.9.3")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        cmake.install()
