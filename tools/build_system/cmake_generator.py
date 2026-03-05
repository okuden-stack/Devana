"""
CMake build file generation for Devana.

Generates CMakeLists.txt files for the project.
"""

import logging
from pathlib import Path
from typing import List

from .components import Component
from .config import ProductConfig

logger = logging.getLogger(__name__)


def generate_main_cmake(
    components: List[Component],
    product_config: ProductConfig,
    output_path: Path
) -> bool:
    logger.info(f"Generating main CMakeLists.txt...")

    proj_root = output_path.parent
    lib_subdirs = []
    lib_targets = []
    for comp in components:
        try:
            rel = comp.path.relative_to(proj_root)
            if not str(rel).startswith('src/'):
                lib_subdirs.append(str(rel))
                lib_targets.append(f"{comp.name}::{comp.name}")
        except ValueError:
            pass

    lib_section = ""
    if lib_subdirs:
        lib_section = "# Library subdirectories\n"
        for subdir in lib_subdirs:
            lib_section += f"add_subdirectory({subdir})\n"
        lib_section += "\n"

    lib_link_section = "\n".join(f"    {t}" for t in lib_targets)

    cmake_content = f'''cmake_minimum_required(VERSION 3.16)

project({product_config.name}
    VERSION {product_config.version}
    DESCRIPTION "{product_config.description}"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD {product_config.cpp_standard})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

{lib_section}include_directories(
    ${{CMAKE_CURRENT_SOURCE_DIR}}/src
)

file(GLOB_RECURSE SOURCES
    src/ui/**/*.cpp
    src/common/**/*.cpp
)

file(GLOB_RECURSE HEADERS
    src/ui/**/*.h
    src/common/**/*.h
)

add_executable({product_config.output_name}
    src/main.cpp
    ${{SOURCES}}
    ${{HEADERS}}
)

target_link_libraries({product_config.output_name}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
{lib_link_section}
)

set_target_properties({product_config.output_name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${{CMAKE_BINARY_DIR}}/bin
)

if(APPLE)
    set_target_properties({product_config.output_name} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.{product_config.name.lower()}.app"
        MACOSX_BUNDLE_BUNDLE_NAME "{product_config.display_name}"
        MACOSX_BUNDLE_BUNDLE_VERSION "{product_config.version}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "{product_config.version}"
    )
endif()

if(WIN32)
    set_target_properties({product_config.output_name} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

install(TARGETS {product_config.output_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

message(STATUS "")
message(STATUS "=== {product_config.display_name} Configuration ===")
message(STATUS "Version: {product_config.version}")
message(STATUS "Build type: ${{CMAKE_BUILD_TYPE}}")
message(STATUS "C++ standard: {product_config.cpp_standard}")
message(STATUS "Qt version: ${{Qt6_VERSION}}")
message(STATUS "Install prefix: ${{CMAKE_INSTALL_PREFIX}}")
message(STATUS "==============================")
message(STATUS "")
'''

    try:
        output_path.write_text(cmake_content)
        logger.info(f"Generated: {output_path}")
        return True
    except Exception as e:
        logger.error(f"Failed to generate CMakeLists.txt: {e}")
        return False


def generate_component_cmake(component: Component) -> bool:
    if "/src/" in str(component.path):
        logger.debug(f"Skipping cmake generation for src component: {component.name}")
        return True

    name = component.name
    name_upper = name.upper()

    link_section = ""
    if component.depends_on:
        deps = " ".join(f"{d}::{d}" for d in component.depends_on)
        link_section = f"\ntarget_link_libraries({name} PUBLIC {deps})\n"

    cmake_content = f"""cmake_minimum_required(VERSION 3.16)
project({name} VERSION 1.0.0 LANGUAGES CXX)

file(GLOB_RECURSE {name_upper}_SOURCES CONFIGURE_DEPENDS
    "${{CMAKE_CURRENT_SOURCE_DIR}}/src/*.cpp"
)

add_library({name} STATIC ${{{name_upper}_SOURCES}})
add_library({name}::{name} ALIAS {name})

target_include_directories({name}
    PUBLIC
        $<BUILD_INTERFACE:${{CMAKE_CURRENT_SOURCE_DIR}}/include>
        $<INSTALL_INTERFACE:include>
)
{link_section}
target_compile_features({name} PUBLIC cxx_std_17)
"""

    cmake_path = component.path / "CMakeLists.txt"
    try:
        cmake_path.write_text(cmake_content)
        logger.info(f"Generated: {cmake_path}")
        return True
    except Exception as e:
        logger.error(f"Failed to generate {cmake_path}: {e}")
        return False
