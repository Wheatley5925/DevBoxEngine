import argparse
import json
import os
import shutil
import subprocess
from pathlib import Path

from asset_packer import sync_and_convert


ESP32_FQBN = (
    "esp32:esp32:esp32s3:"
    "USBMode=hwcdc,CDCOnBoot=cdc,UploadMode=default,JTAGAdapter=builtin,"
    "PSRAM=enabled"
)


def cmake_path(path):
    return str(path.resolve()).replace("\\", "/")


def discover_linux_programs(project_dir):
    programs = []
    main = project_dir / "main.cpp"
    if main.is_file():
        programs.append((project_dir.name, main, []))

    programs_dir = project_dir / "programs"
    if programs_dir.is_dir():
        for program_dir in sorted(programs_dir.iterdir()):
            entry = program_dir / "main.cpp"
            if program_dir.is_dir() and entry.is_file():
                extra_sources = sorted(
                    path for path in program_dir.rglob("*.cpp") if path != entry
                )
                programs.append((program_dir.name, entry, extra_sources))

    if not programs:
        raise RuntimeError(
            f"No Linux entry point found. Add {main} or programs/<name>/main.cpp"
        )
    return programs


def cmake_list(paths):
    return "\n".join(f'  "{cmake_path(path)}"' for path in paths)


def write_linux_cmake(cmake_source_dir, project_dir, programs, programs_out):
    engine_dir = Path(__file__).resolve().parent.parent
    libraries_dir = engine_dir.parent
    sdk_dir = libraries_dir / "DevBoxSDK"
    u8g2_dir = libraries_dir / "U8g2" / "src" / "clib"

    engine_sources = sorted((engine_dir / "src").rglob("*.cpp"))
    sdk_sources = sorted((sdk_dir / "src" / "backends" / "linux").glob("*.cpp"))
    shared_game_sources = sorted((project_dir / "src").rglob("*.cpp"))

    target_blocks = []
    for index, (program_name, entry, extra_sources) in enumerate(programs):
        target = f"devbox_game_{index}"
        sources = [entry, *shared_game_sources, *extra_sources,
                   *engine_sources, *sdk_sources]
        target_blocks.append(
            f'''add_executable({target}
{cmake_list(sources)}
)
target_compile_features({target} PRIVATE cxx_std_17)
target_compile_definitions({target} PRIVATE DEVBOX_TARGET=DEVBOX_TARGET_LINUX)
# Every executable in this project shares the same settings identity.
target_compile_options({target} PRIVATE -include "${{CMAKE_CURRENT_SOURCE_DIR}}/DevBoxGameId.h")
target_include_directories({target} PRIVATE
  "{cmake_path(engine_dir / 'src')}"
  "{cmake_path(sdk_dir / 'src')}"
  "{cmake_path(project_dir)}"
  "${{SDL2_IMAGE_INCLUDE_DIR}}"
)
target_link_libraries({target} PRIVATE
  PkgConfig::SDL2
  "${{SDL2_IMAGE_LIBRARY}}"
  devbox_u8g2
  Threads::Threads
)
set_target_properties({target} PROPERTIES OUTPUT_NAME "{program_name}")
'''
        )

    cmake_source_dir.mkdir(parents=True, exist_ok=True)
    (cmake_source_dir / "DevBoxGameId.h").write_text(
        "#pragma once\n#define DEVBOX_GAME_ID " + json.dumps(project_dir.name) + "\n"
    )
    (cmake_source_dir / "CMakeLists.txt").write_text(
        f'''cmake_minimum_required(VERSION 3.16)
project(DevBoxGameLinux LANGUAGES C CXX)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
find_path(SDL2_IMAGE_INCLUDE_DIR SDL_image.h PATH_SUFFIXES SDL2)
find_library(SDL2_IMAGE_LIBRARY SDL2_image)
if(NOT SDL2_IMAGE_INCLUDE_DIR OR NOT SDL2_IMAGE_LIBRARY)
  message(FATAL_ERROR "SDL2_image development files are required")
endif()

file(GLOB U8G2_SOURCES CONFIGURE_DEPENDS "{cmake_path(u8g2_dir)}/*.c")
add_library(devbox_u8g2 STATIC ${{U8G2_SOURCES}})
target_include_directories(devbox_u8g2 PUBLIC "{cmake_path(u8g2_dir)}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "{cmake_path(programs_out)}")

{''.join(target_blocks)}
'''
    )

    artwork_dir = sdk_dir / "src" / "backends" / "linux"
    for name in ("DevBoxPlayerBackground.png", "InputConfiguration.png"):
        shutil.copy2(artwork_dir / name, programs_out / name)


def convert_assets(project_dir, assets_out):
    assets_out.mkdir(parents=True, exist_ok=True)
    print("\n+++ ASSET CONVERSION STARTED +++")
    sync_and_convert(project_dir / "assets", assets_out)
    print("+++ ASSET CONVERSION FINISHED +++\n")


def build_esp32(project_dir, out_dir):
    firmware_out = project_dir / "esp_build"
    firmware_out.mkdir(parents=True, exist_ok=True)
    convert_assets(project_dir, out_dir / "assets")

    print("\n+++ ESP32 COMPILATION STARTED +++")
    subprocess.run(
        ["arduino-cli", "compile", "--fqbn", ESP32_FQBN, str(project_dir),
         "--output-dir", str(firmware_out)],
        check=True,
    )
    os.replace(
        firmware_out / f"{project_dir.name}.ino.bin",
        out_dir / "app.bin",
    )
    print("+++ ESP32 BUILD FINISHED +++\n")


def build_linux(project_dir, out_dir):
    linux_out = out_dir / "linux"
    programs_out = linux_out / "programs"
    sd_root = linux_out / "sdcard"
    assets_out = sd_root / "apps" / project_dir.name / "assets"
    cmake_source_dir = linux_out / "cmake-src"
    cmake_build_dir = linux_out / "cmake-build"

    programs_out.mkdir(parents=True, exist_ok=True)
    convert_assets(project_dir, assets_out)
    programs = discover_linux_programs(project_dir)
    write_linux_cmake(cmake_source_dir, project_dir, programs, programs_out)

    print("\n+++ LINUX COMPILATION STARTED +++")
    subprocess.run(
        ["cmake", "-S", str(cmake_source_dir), "-B", str(cmake_build_dir),
         "-DCMAKE_BUILD_TYPE=Debug"],
        check=True,
    )
    subprocess.run(
        ["cmake", "--build", str(cmake_build_dir), "--parallel"],
        check=True,
    )
    print("+++ LINUX BUILD FINISHED +++\n")

    print("Programs:")
    for program_name, _, _ in programs:
        executable = programs_out / program_name
        print(f'  DEVBOX_SD_ROOT="{sd_root}" "{executable}"')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--project", required=True,
        help='Project directory, for example: "./examples/test_game/"',
    )
    parser.add_argument(
        "--target", choices=("esp32", "linux"), default="esp32",
        help="Build target (default: esp32)",
    )
    parser.add_argument(
        "--out", default="./build",
        help="Directory where build output will be placed",
    )
    args = parser.parse_args()

    project_dir = Path(args.project).resolve()
    if not project_dir.is_dir():
        parser.error(f"Project directory does not exist: {project_dir}")

    out_dir = (Path(args.out) / project_dir.name).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    if args.target == "linux":
        build_linux(project_dir, out_dir)
    else:
        build_esp32(project_dir, out_dir)


if __name__ == "__main__":
    main()
