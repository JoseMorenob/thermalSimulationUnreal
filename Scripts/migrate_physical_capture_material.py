import unreal

source = unreal.load_asset('/Game/Materials/M_IR_PhysicalRadiance_Test')
if not source:
    raise RuntimeError('Could not load the physical capture material')

target_path = '/IRSimPlugin/Materials/M_IR_PhysicalRadiance'
if not unreal.EditorAssetLibrary.does_asset_exist(target_path):
    target = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
        'M_IR_PhysicalRadiance', '/IRSimPlugin/Materials', source)
    if not target:
        raise RuntimeError('Could not duplicate the physical capture material')
    unreal.EditorAssetLibrary.save_loaded_asset(target)
unreal.log('IR_PHYSICAL_CAPTURE material ready')
