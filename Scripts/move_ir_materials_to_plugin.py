import unreal

ASSETS = [
    "M_ThermalSurface",
    "M_ThermalDebug",
    "M_IR_DebugDisplay",
]

source_dir = "/Game/Materials"
target_dir = "/IRSimPlugin/Materials"

editor_asset_lib = unreal.EditorAssetLibrary

if not editor_asset_lib.does_directory_exist(target_dir):
    editor_asset_lib.make_directory(target_dir)

for asset_name in ASSETS:
    source = f"{source_dir}/{asset_name}"
    target = f"{target_dir}/{asset_name}"

    if editor_asset_lib.does_asset_exist(target):
        unreal.log(f"Deleting pre-existing copied plugin asset: {target}")
        editor_asset_lib.delete_asset(target)

    if editor_asset_lib.does_asset_exist(source):
        unreal.log(f"Moving {source} -> {target}")
        if not editor_asset_lib.rename_asset(source, target):
            raise RuntimeError(f"Could not move {source} to {target}")
    else:
        unreal.log_warning(f"Source asset not found, skipping: {source}")

editor_asset_lib.save_directory(target_dir, only_if_is_dirty=False, recursive=True)
editor_asset_lib.save_directory(source_dir, only_if_is_dirty=False, recursive=True)

unreal.log("IR material migration to plugin completed.")
