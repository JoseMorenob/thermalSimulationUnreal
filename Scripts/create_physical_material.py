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
# SceneColorHDR is pre-exposed by UE5. Disable the automatic material scale so
# the validation render target keeps SensorRadiance in physical units.
try:
    material.set_editor_property('b_disable_pre_exposure_scale', True)
except Exception:
    material.set_editor_property('disable_pre_exposure_scale', True)

sensor_radiance = unreal.MaterialEditingLibrary.create_material_expression(
    material, unreal.MaterialExpressionScalarParameter, -400, 0)
sensor_radiance.set_editor_property('parameter_name', 'SensorRadiance')
sensor_radiance.set_editor_property('default_value', 0.0)

# SceneColorHDR is stored pre-exposed by UE5. Divide by the view's PreExposure
# so the engine's later pre-exposure multiplication cancels out and the R
# channel remains in W m^-2 sr^-1. This is a surface-material compensation;
# the post-process-only Disable Pre-Exposure Scale flag is not available here.
pre_exposure = unreal.MaterialEditingLibrary.create_material_expression(
    material, unreal.MaterialExpressionViewProperty, -400, 180)
# The Python wrapper drops the leading C++ "E" from the enum name.
view_property_enum = getattr(unreal, 'MaterialExposedViewProperty', None)
if view_property_enum is None:
    raise RuntimeError('UE Python API does not expose MaterialExposedViewProperty')
pre_exposure.set_editor_property(
    'property', view_property_enum.MEVP_PRE_EXPOSURE)
pre_exposure_guard = unreal.MaterialEditingLibrary.create_material_expression(
    material, unreal.MaterialExpressionMax, -180, 180)
pre_exposure_guard.set_editor_property('const_b', 0.0001)
unreal.MaterialEditingLibrary.connect_material_expressions(
    pre_exposure, '', pre_exposure_guard, 'A')
physical_output = unreal.MaterialEditingLibrary.create_material_expression(
    material, unreal.MaterialExpressionDivide, 40, 40)
unreal.MaterialEditingLibrary.connect_material_expressions(
    sensor_radiance, '', physical_output, 'A')
unreal.MaterialEditingLibrary.connect_material_expressions(
    pre_exposure_guard, '', physical_output, 'B')

unreal.MaterialEditingLibrary.connect_material_property(
    physical_output, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log('IR_NEW_MATERIAL created {} with physical linear output'.format(ASSET_PATH))
