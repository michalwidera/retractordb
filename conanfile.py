from conans import ConanFile, CMake

class Retractor(ConanFile):
   settings = "os", "compiler", "build_type", "arch"
   requires = ["cmake/3.21.3"]
   license = "MIT"
   author = "Michal Widera"
   description = "RetractorDB time series database"
   homepage = "https://retractordb.com"
   generators = "cmake", "gcc"
   testing = []

   options = { "boost": [1.74, 1.75, 1.76, 1.77],
               "gtest": [1.11]
            }

   default_options = {  "boost:shared": False, 
                        "boost:without_serialization": False, 
                        "boost:without_thread": False,
                        "boost:without_program_options": False,
                        "boost:without_test": False,
                        "boost:multithreading": True,
                        "boost:without_system": False,
                        "boost:without_filesystem": False,
                        "boost": options["boost"][0],
                        "gtest": options["gtest"][0]
                     }

   def package_info(self):
      self.cpp_info.libs = []
      self.cpp_info.system_libs = ["pthread","rt","dl"]
      
   def requirements(self):
      self.requires("boost/"+str(self.options.boost)+".0")
      self.requires("gtest/"+str(self.options.gtest)+".0")

   def build(self):
      cmake = CMake(self)
      cmake.configure() 
      cmake.build()
      cmake.install()
