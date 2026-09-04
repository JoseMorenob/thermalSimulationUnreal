import unreal

PACKAGE_PATH = '/IRSimPlugin/Materials'
ASSET_NAME = 'M_ThermalSurface_Physical'
ASSET_PATH = PACKAGE_PATH + '/' + ASSET_NAME

material = unreal.load_asset(ASSET_PATH)
if not material:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME, PACKAGE_PATH, unreal.Material, unreal.MaterialFactoryNew())
if not material:
    raise RuntimeError('Could not load or create {}'.format(ASSET_PATH))

unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
material.set_editor_property('material_domain', unreal.MaterialDomain.MD_SURFACE)
material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
# The physical capture must preserve SensorRadiance in linear units. Project
# setting r.UsePreExposure=0 and the explicit SceneCapture settings provide the
# invariant path; no exposure compensation belongs in this material.
try:
    material.set_editor_property('b_disable_pre_exposure_scale', True)
except Exception:
    material.set_editor_property('disable_pre_exposure_scale', True)

sensor_radiance = unreal.MaterialEditingLibrary.create_material_expression(
    material, unreal.MaterialExpressionScalarParameter, -400, 0)
sensor_radiance.set_editor_property('parameter_name', 'SensorRadiance')
sensor_radiance.set_editor_property('default_value', 0.0)

unreal.MaterialEditingLibrary.connect_material_property(
    sensor_radiance, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log('IR_NEW_MATERIAL created {} with physical linear output'.format(ASSET_PATH))
