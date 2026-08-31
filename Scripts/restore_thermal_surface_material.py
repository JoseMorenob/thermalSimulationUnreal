import unreal

PACKAGE = '/IRSimPlugin/Materials'
SOURCE = PACKAGE + '/M_ThermalSurface_PreRepair'
TARGET = PACKAGE + '/M_ThermalSurface'
TEMP = PACKAGE + '/M_ThermalSurface_Restored'

source = unreal.load_asset(SOURCE)
if not source:
    raise RuntimeError('Backup material not found: {}'.format(SOURCE))

# Make a restored copy first. This keeps the backup untouched if the target is
# locked or the editor refuses to replace it.
if unreal.load_asset(TEMP):
    unreal.EditorAssetLibrary.delete_asset(TEMP)

restored = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
    'M_ThermalSurface_Restored', PACKAGE, source)
if not restored:
    raise RuntimeError('Could not duplicate the backup material')
unreal.EditorAssetLibrary.save_loaded_asset(restored)

if unreal.load_asset(TARGET):
    if not unreal.EditorAssetLibrary.delete_asset(TARGET):
        raise RuntimeError('Could not delete the broken target material; close Unreal first')

final = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
    'M_ThermalSurface', PACKAGE, restored)
if not final:
    raise RuntimeError('Could not restore {}'.format(TARGET))
unreal.EditorAssetLibrary.save_loaded_asset(final)
unreal.log('IR_RESTORE restored M_ThermalSurface from M_ThermalSurface_PreRepair')
