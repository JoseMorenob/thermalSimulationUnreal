import unreal

material = unreal.load_asset('/IRSimPlugin/Materials/M_ThermalSurface')
if not material:
    raise RuntimeError('Could not load M_ThermalSurface')

unreal.log('IR_EXPORT classes={}'.format(
    [name for name in dir(unreal) if 'export' in name.lower() and 'material' in name.lower()]))
