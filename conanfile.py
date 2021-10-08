from conans import ConanFile, CMake

class RetractorDBConan(ConanFile):
   settings = "os", "compiler", "build_type", "arch"
   requires = ["boost/1.74.0","gtest/1.11.0","cmake/3.21.3"]
   license = "MIT"
   author = "Michal Widera"
   description = "RetractorDB time series database"
   homepage = "https://retractordb.com"
   generators = "cmake", "gcc"
   default_options = {  "boost:shared": False, 
                        "boost:without_serialization": False, 
                        "boost:without_thread": False,
                        "boost:without_program_options": False,
                        "boost:without_test": False,
                        "boost:multithreading": True,
                        "boost:without_system": False,
                        "boost:without_filesystem": False }

   def package_info(self):
      self.cpp_info.libs = []
      self.cpp_info.system_libs = ["pthread","rt","dl"]

   def build(self):
      cmake = CMake(self)
      cmake.configure()
      cmake.build()
      cmake.install()
      #cmake.test()
