import shutil
from pathlib import Path

from converters.pack_audio  import pack_audio
from converters.pack_atlas  import pack_atlas
from converters.pack_bitmap import pack_bitmap

def sync_and_convert(src_root, dst_root):
    src_root = Path(src_root)
    dst_root = Path(dst_root)

    for path in src_root.rglob('*'):
        relative_path = path.relative_to(src_root)
        target_path = dst_root / relative_path

        if path.is_dir():
            target_path.mkdir(parents=True, exist_ok=True)
            continue
        
        target_path.parent.mkdir(parents=True, exist_ok=True)
        
        if path.suffix == ".png":

            if path.name.endswith(".atlas.png"):
                print(f"Converting atlas: {path} -> {target_path}")
                out_path = target_path.with_suffix(".atl4")
                base_path = path.name.replace(".atlas.png", "")
                json_path = path.with_name(f"{base_path}.json")    
                pack_atlas(path, json_path, out_path)

            print(f"Converting bitmap: {path} -> {target_path}")
            out_path = target_path.with_suffix(".spr4");
            pack_bitmap(path, out_path)

        elif path.suffix in [".wav", ".mp3", ".ogg", ".flac"]:
            print(f"Converting audio: {path} -> {target_path}")
            out_path = target_path.with_suffix(".wav");
            pack_audio(path, target_path)
        
        else:
            print(f"Copying: {path} -> {target_path}")
            shutil.copy2(path, target_path)

