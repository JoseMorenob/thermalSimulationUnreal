"""Create empty, linear-output material shells for the IR auxiliary buffers.

The graphs are intentionally left empty.  They are authored manually in the
Unreal Material Editor so the physical data path remains visible and reviewable.
"""

import unreal

PACKAGE_PATH = "/IRSimPlugin/Materials/Buffers"

BUFFER_MATERIALS = (
    "M_IR_Output_Temperature",
    "M_IR_Output_Emissivity",
    "M_IR_Output_Depth",
    "M_IR_Output_Normal",
    "M_IR_Output_MaterialId",
)


def create_material(name):
    path = PACKAGE_PATH + "/" + name
    material = unreal.load_asset(path)
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, PACKAGE_PATH, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError("Could not create {}".format(path))

    # Do not delete expressions from an existing user-authored buffer material.
    # Newly created shells contain no expressions and are ready for manual wiring.
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    try:
        material.set_editor_property("b_disable_pre_exposure_scale", True)
    except Exception:
        try:
            material.set_editor_property("disable_pre_exposure_scale", True)
        except Exception:
            pass
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    unreal.log("IR buffer material ready: {}".format(path))


for material_name in BUFFER_MATERIALS:
    create_material(material_name)

