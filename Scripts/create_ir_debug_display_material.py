import unreal

PACKAGE = '/IRSimPlugin/Materials'
ASSET_PATH = PACKAGE + '/M_IR_DebugDisplay'

material = unreal.load_asset(ASSET_PATH)
if not material:
    raise RuntimeError('Could not load {}'.format(ASSET_PATH))

material.set_editor_property('material_domain', unreal.MaterialDomain.MD_POST_PROCESS)
unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

def node(cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(material, cls, x, y)

texture = node(unreal.MaterialExpressionTextureSampleParameter2D, -700, 0)
texture.set_editor_property('parameter_name', 'RadianceTexture')

minimum = node(unreal.MaterialExpressionScalarParameter, -700, 180)
minimum.set_editor_property('parameter_name', 'DisplayRadianceMin')
minimum.set_editor_property('default_value', 0.0)

maximum = node(unreal.MaterialExpressionScalarParameter, -700, 340)
maximum.set_editor_property('parameter_name', 'DisplayRadianceMax')
maximum.set_editor_property('default_value', 70.0)

range_subtract = node(unreal.MaterialExpressionSubtract, -450, 300)
value_subtract = node(unreal.MaterialExpressionSubtract, -450, 80)
range_max = node(unreal.MaterialExpressionMax, -230, 300)
normalize = node(unreal.MaterialExpressionDivide, -20, 80)
saturate = node(unreal.MaterialExpressionSaturate, 180, 80)
low = node(unreal.MaterialExpressionConstant3Vector, -20, 220)
low.set_editor_property('constant', unreal.LinearColor(0.005, 0.01, 0.12, 1.0))
high = node(unreal.MaterialExpressionConstant3Vector, -20, 400)
high.set_editor_property('constant', unreal.LinearColor(1.0, 0.75, 0.08, 1.0))
lerp = node(unreal.MaterialExpressionLinearInterpolate, 400, 80)

connect = unreal.MaterialEditingLibrary.connect_material_expressions
connect(texture, 'R', value_subtract, 'A')
connect(minimum, '', value_subtract, 'B')
connect(maximum, '', range_subtract, 'A')
connect(minimum, '', range_subtract, 'B')
connect(range_subtract, '', range_max, 'A')
connect(value_subtract, '', normalize, 'A')
connect(range_max, '', normalize, 'B')
connect(normalize, '', saturate, '')
connect(low, '', lerp, 'A')
connect(high, '', lerp, 'B')
connect(saturate, '', lerp, 'Alpha')
unreal.MaterialEditingLibrary.connect_material_property(
    lerp, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log('IR_DEBUG_DISPLAY rebuilt: physical radiance mapped 25..70 to blue/yellow thermal palette')
