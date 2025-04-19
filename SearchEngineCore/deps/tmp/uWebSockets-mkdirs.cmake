# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "D:/Projects/hatef.ir/search-engine-core/SearchEngineCore/deps/src/uWebSockets")
  file(MAKE_DIRECTORY "D:/Projects/hatef.ir/search-engine-core/SearchEngineCore/deps/src/uWebSockets")
endif()
file(MAKE_DIRECTORY
  "D:/Projects/hatef.ir/search-engine-core/SearchEngineCore/deps/src/uWebSockets-build"
  "D:/Projects/hatef.ir/search-engine-core/SearchEngineCore/deps"
  "D:/Projects/hatef.ir/search-engine-core/SearchEngineCore/deps/tmp"
  "D:/Projects/hatef.ir/search-engine-core/SearchEngineCore/deps/src/uWebSockets-stamp"
  "D:/Projects/hatef.ir/search-engine-core/SearchEngineCore/deps/src"
  "D:/Projects/hatef.ir/search-engine-core/SearchEngineCore/deps/src/uWebSockets-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/Projects/hatef.ir/search-engine-core/SearchEngineCore/deps/src/uWebSockets-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/Projects/hatef.ir/search-engine-core/SearchEngineCore/deps/src/uWebSockets-stamp${cfgdir}") # cfgdir has leading slash
endif()
