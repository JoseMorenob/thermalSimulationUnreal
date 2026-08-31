import unreal

MATERIAL_PATH = '/IRSimPlugin/Materials/M_ThermalSurface'
BACKUP_PATH = '/IRSimPlugin/Materials/M_ThermalSurface_PreRepair'

material = unreal.load_asset(MATERIAL_PATH)
if not material:
    raise RuntimeError('Could not load {}'.format(MATERIAL_PATH))

if not unreal.load_asset(BACKUP_PATH):
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    backup = asset_tools.duplicate_asset(
        'M_ThermalSurface_PreRepair', '/IRSimPlugin/Materials', material)
    if not backup:
        raise RuntimeError('Could not create material backup')
    unreal.EditorAssetLibrary.save_loaded_asset(backup)

unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
sensor_radiance = unreal.MaterialEditingLibrary.create_material_expression(
    material, unreal.MaterialExpressionScalarParameter, -400, 0)
sensor_radiance.set_editor_property('parameter_name', 'SensorRadiance')
sensor_radiance.set_editor_property('default_value', 0.0)

# The capture target is a physical data product. Do not divide by a display
# range and do not apply eye-adaptation compensation here: the render target
# must contain SensorRadiance in W m^-2 sr^-1. Display normalization belongs
# exclusively to M_IR_DebugDisplay in the player post-process.
unreal.MaterialEditingLibrary.connect_material_property(
    sensor_radiance, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log('IR_REPAIR material repaired: SensorRadiance -> Emissive Color (physical linear output)')
