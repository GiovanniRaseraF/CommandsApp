cmake_minimum_required(VERSION 3.15)
project(CommandsApp VERSION 1.0.0 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find wxWidgets (core and base modules are required)
find_package(wxWidgets REQUIRED COMPONENTS core base)

# Find SQLite3 database library
find_package(SQLite3 REQUIRED)

# Set compile flags, defines, and include paths from wxWidgets
include(${wxWidgets_USE_FILE})

# Collect source files
file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "src/*.h")

# Build target
# if(APPLE)
#     # Build as a macOS app bundle so GUI renders correctly without terminal blocking
#     add_executable(CommandsApp MACOSX_BUNDLE ${SOURCES} ${HEADERS})
# else()
add_executable(CommandsApp ${SOURCES} ${HEADERS})
# endif()

# Set up include directories
target_include_directories(CommandsApp PRIVATE src)

# Link libraries
target_link_libraries(CommandsApp PRIVATE ${wxWidgets_LIBRARIES} SQLite3::SQLite3)
