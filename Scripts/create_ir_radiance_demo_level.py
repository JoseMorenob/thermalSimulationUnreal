"""Create and save the controlled IR radiance demonstration level.

Run from Unreal Editor with Execute Python Script after the plugin is compiled.
The level deliberately uses real-world distances expressed in Unreal centimetres.
"""

import unreal

LEVEL_PATH = "/Game/Maps/IRRadianceDemo"
THERMAL_MATERIAL_PATH = "/IRSimPlugin/Materials/M_ThermalSurface"
DEBUG_MATERIAL_PATH = "/IRSimPlugin/Materials/M_IR_DebugDisplay"
CUBE_PATH = "/Engine/BasicShapes/Cube"


def asset(path):
    value = unreal.load_asset(path)
    if not value:
        raise RuntimeError("Could not load {}".format(path))
    return value


def spawn(actor_class, location, rotation=unreal.Rotator()):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class, unreal.Vector(*location), rotation)
    if not actor:
        raise RuntimeError("Could not spawn {}".format(actor_class))
    return actor


if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
    unreal.EditorLevelLibrary.load_level(LEVEL_PATH)
    for existing_actor in unreal.EditorLevelLibrary.get_all_level_actors():
        unreal.EditorLevelLibrary.destroy_actor(existing_actor)
else:
    unreal.EditorLevelLibrary.new_level(LEVEL_PATH)

environment = spawn(unreal.load_class(None, "/Script/IRSimPlugin.IRSceneEnvironmentActor"), (0, 0, 0))
environment.set_editor_property("atmospheric_extinction_coefficient", 0.015)
environment.set_editor_property("air_temperature_k", 293.15)
environment.set_editor_property("effective_sky_temperature_k", 240.0)

capture = spawn(unreal.load_class(None, "/Script/IRSimPlugin.RadianceCaptureActor"), (0, 0, 0))
capture.set_editor_property("show_render_target_on_player_camera", True)
capture.set_editor_property("player_view_post_process_material", asset(DEBUG_MATERIAL_PATH))
capture.set_editor_property("display_radiance_min", 0.0)
capture.set_editor_property("display_radiance_max", 70.0)

pipeline = spawn(unreal.load_class(None, "/Script/IRSimPlugin.ThermalPipelineController"), (0, 0, 0))
pipeline.set_actor_label("ThermalPipelineController")
pipeline.set_editor_property("scene_environment_actor", environment)
pipeline.set_editor_property("radiance_capture_actor", capture)
pipeline.set_editor_property("default_thermal_material", asset(THERMAL_MATERIAL_PATH))

view_camera = spawn(unreal.CameraActor, (0, 0, 120))
view_camera.set_actor_label("ThermalInspectionCamera")

def thermal_plate(name, x, temperature, emissivity, material_id):
    actor = spawn(unreal.load_class(None, "/Script/IRSimPlugin.IRThermalDemoObjectActor"), (x, 0, 0))
    actor.set_actor_label(name)
    mesh = actor.get_editor_property("mesh")
    mesh.set_static_mesh(asset(CUBE_PATH))
    mesh.set_world_scale3d(unreal.Vector(1.5, 4.0, 4.0))
    component = actor.get_editor_property("thermal_surface")
    component.set_editor_property("target_mesh", mesh)
    component.set_editor_property("debug_material", asset(THERMAL_MATERIAL_PATH))
    component.set_editor_property("temperature_k", temperature)
    component.set_editor_property("emissivity", emissivity)
    component.set_editor_property("material_id", material_id)
    return actor


thermal_plate("BlackPlateNear", 1000, 320.0, 1.0, 1)
thermal_plate("MetalPlateNear", 1500, 300.0, 0.35, 2)
thermal_plate("WarmObject", 2500, 340.0, 0.8, 3)
thermal_plate("FarObject_45m", 4500, 340.0, 0.8, 4)

unreal.EditorLevelLibrary.save_current_level()
unreal.log("IR demo level saved: {}".format(LEVEL_PATH))
