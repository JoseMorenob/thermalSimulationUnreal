import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    class_name = actor.get_class().get_name()
    if class_name in ("RadianceCaptureActor", "ThermalPipelineController", "SphereActor"):
        unreal.log("IR_SCENE actor={} class={}".format(actor.get_actor_label(), class_name))
        for property_name in (
            "b_show_render_target_on_player_camera",
            "b_follow_player_camera",
            "player_view_post_process_material",
            "display_radiance_min",
            "display_radiance_max",
            "radiance_capture_actor",
            "scene_environment_actor",
            "default_thermal_material",
        ):
            try:
                unreal.log("IR_SCENE {}={}".format(property_name, actor.get_editor_property(property_name)))
            except Exception:
                pass

thermal_material = unreal.load_asset('/IRSimPlugin/Materials/M_ThermalSurface')
if thermal_material:
    unreal.log('IR_MATERIAL package_filename={}'.format(
        unreal.EditorAssetLibrary.get_path_name_for_loaded_asset(thermal_material)))
    unreal.log('IR_MATERIAL domain={}'.format(thermal_material.get_editor_property('material_domain')))
    unreal.log('IR_MATERIAL shading_model={}'.format(thermal_material.get_editor_property('shading_model')))
    unreal.log('IR_MATERIAL scalar_parameters={}'.format(
        unreal.MaterialEditingLibrary.get_scalar_parameter_names(thermal_material)))
    unreal.log('IR_MATERIAL expression_count={}'.format(
        unreal.MaterialEditingLibrary.get_num_material_expressions(thermal_material)))
    editor_only_data = thermal_material.get_editor_property('editor_only_data')
    unreal.log('IR_MATERIAL editor_only={}'.format(
        [name for name in dir(editor_only_data) if 'expression' in name.lower()]))
    unreal.log('IR_MATERIAL editing_api={}'.format(
        [name for name in dir(unreal.MaterialEditingLibrary) if 'expression' in name.lower() or 'connect' in name.lower()]))

physical_material = unreal.load_asset('/IRSimPlugin/Materials/M_IR_PhysicalRadiance')
if physical_material:
    unreal.log('IR_PHYSICAL domain={}'.format(physical_material.get_editor_property('material_domain')))
    unreal.log('IR_PHYSICAL scalar_parameters={}'.format(
        unreal.MaterialEditingLibrary.get_scalar_parameter_names(physical_material)))
