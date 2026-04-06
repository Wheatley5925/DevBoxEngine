import argparse
import os
import subprocess
from pathlib import Path

from asset_packer import sync_and_convert


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--project",
        required=True,
        help='Project directory, for example: "./MyGame"',
    )
    parser.add_argument(
        "--out",
        default="./build",
        help="Directory where the final build output will be placed",
    )

    args = parser.parse_args()

    project_dir = Path(args.project).resolve()
    out_dir = Path(args.out).resolve()
    assets_src = project_dir / "assets"
    assets_out = out_dir / "assets"
    firmware_out = project_dir / "esp_build"

    out_dir.mkdir(parents=True, exist_ok=True)
    assets_out.mkdir(parents=True, exist_ok=True)
    firmware_out.mkdir(parents=True, exist_ok=True)

    sync_and_convert(assets_src, assets_out)

    subprocess.run(
        [
            "arduino-cli",
            "compile",
            "--fqbn",
            "esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,UploadMode=default,JTAGAdapter=builtin",
            str(project_dir),
            "--output-dir",
            str(firmware_out),
        ],
        check=True,
    )

    project_name = project_dir.name
    bin_name = f"{project_name}.ino.bin"
    src_bin = firmware_out / bin_name
    dst_bin = out_dir / f"{project_name}.bin"

    os.replace(src_bin, dst_bin)


if __name__ == "__main__":
    main()
