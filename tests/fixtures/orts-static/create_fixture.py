"""Run with Blender --background --factory-startup --python create_fixture.py.

Original synthetic fixture, CC0-1.0. No user models or route content involved.
All output remains beside this script. Units are metres.
"""
import hashlib
import json
from pathlib import Path

import bpy
from mathutils import Vector

HERE = Path(__file__).resolve().parent


def strip_png_metadata(data):
    """Remove Blender text metadata without changing encoded pixel chunks."""
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('Expected a PNG preview')
    result = bytearray(data[:8])
    offset = 8
    while offset < len(data):
        if offset + 12 > len(data):
            raise ValueError('Truncated PNG chunk')
        end = offset + int.from_bytes(data[offset:offset + 4], 'big') + 12
        if end > len(data):
            raise ValueError('Invalid PNG chunk length')
        if data[offset + 4:offset + 8] not in (b'tEXt', b'zTXt', b'iTXt'):
            result.extend(data[offset:end])
        offset = end
    return bytes(result)


bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.context.preferences.filepaths.save_version = 0
bpy.context.preferences.filepaths.file_preview_type = 'NONE'
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0


def solid(name, color):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    material.use_backface_culling = True
    nodes = material.node_tree.nodes
    nodes.clear()
    output = nodes.new('ShaderNodeOutputMaterial')
    rgb = nodes.new('ShaderNodeRGB')
    rgb.outputs[0].default_value = (*color, 1)
    # Direct color output is glTF-Blender-IO's supported unlit convention.
    material.node_tree.links.new(rgb.outputs[0], output.inputs['Surface'])
    return material


root = bpy.data.objects.new('ORTS_STATIC_PROBE_ROOT', None)
scene.collection.objects.link(root)


def cube(name, location, dimensions, material):
    bpy.ops.mesh.primitive_cube_add(size=1, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(material)
    obj.parent = root
    return obj


cube('Ground_pivot_base_4x6m', (0, 0, .15), (4, 6, .3), solid('Base_gray', (.18, .18, .18)))
cube('Tall_red_tower', (-1.3, -1, 1.5), (.5, .5, 2.7), solid('Red', (.8, .02, .02)))
cube('Short_green_tower', (1.1, -.5, .8), (.6, .6, 1), solid('Green', (.02, .65, .04)))

arrow_mesh = bpy.data.meshes.new('Forward_arrow_mesh')
arrow_mesh.from_pydata([(-.25, -.6, .32), (.25, -.6, .32), (.25, -1.7, .32),
                       (.7, -1.7, .32), (0, -2.7, .32), (-.7, -1.7, .32),
                       (-.25, -1.7, .32)], [], [(6, 5, 4, 3, 2, 1, 0)])
arrow_mesh.update()
arrow = bpy.data.objects.new('Yellow_FORWARD_minus_Blender_Y', arrow_mesh)
scene.collection.objects.link(arrow)
arrow.parent = root
arrow.data.materials.append(solid('Arrow_yellow', (.95, .75, .01)))

size = 128
image = bpy.data.images.new('Probe_quadrants_cutout', size, size, alpha=True)
pixels = []
for y in range(size):
    for x in range(size):
        colors = ((.04, .12, .95), (.95, .95, .95), (.95, .03, .03), (.03, .85, .08))
        color = colors[(2 if y >= size // 2 else 0) + (1 if x >= size // 2 else 0)]
        hole = (x - 64) ** 2 + (y - 64) ** 2 < 22 ** 2
        pixels.extend((*color, 0 if hole else 1))
image.pixels.foreach_set(pixels)
image.filepath_raw = str(HERE / 'probe_quadrants.png')
image.file_format = 'PNG'
image.save()

material = bpy.data.materials.new('Double_sided_MASK_quadrants')
material.use_nodes = True
material.use_backface_culling = False
nodes = material.node_tree.nodes
nodes.clear()
output = nodes.new('ShaderNodeOutputMaterial')
texture = nodes.new('ShaderNodeTexImage')
texture.image = image
texture.interpolation = 'Closest'
threshold = nodes.new('ShaderNodeMath')
threshold.operation = 'GREATER_THAN'
threshold.inputs[1].default_value = .5
transparent = nodes.new('ShaderNodeBsdfTransparent')
mix = nodes.new('ShaderNodeMixShader')
links = material.node_tree.links
links.new(texture.outputs['Alpha'], threshold.inputs[0])
links.new(threshold.outputs[0], mix.inputs[0])
links.new(transparent.outputs[0], mix.inputs[1])
links.new(texture.outputs['Color'], mix.inputs[2])
links.new(mix.outputs[0], output.inputs['Surface'])
panel_mesh = bpy.data.meshes.new('UV_panel_mesh')
panel_mesh.from_pydata([(-1.2, 1.5, .5), (1.2, 1.5, .5),
                        (1.2, 1.5, 2.5), (-1.2, 1.5, 2.5)], [], [(0, 1, 2, 3)])
panel_mesh.update()
uv = panel_mesh.uv_layers.new(name='UVMap')
for loop, coord in zip(uv.data, ((0, 0), (1, 0), (1, 1), (0, 1))):
    loop.uv = coord
panel = bpy.data.objects.new('Cutout_UV_panel', panel_mesh)
scene.collection.objects.link(panel)
panel.parent = root
panel.data.materials.append(material)

# No lights/cameras/animations/compression or extensions beyond ORTS unlit.
export_options = dict(export_yup=True, export_animations=False, export_cameras=False,
                      export_lights=False, export_extras=False, export_materials='EXPORT',
                      export_image_format='AUTO', export_draco_mesh_compression_enable=False)
for extension, export_format in (('glb', 'GLB'), ('gltf', 'GLTF_SEPARATE')):
    result = bpy.ops.export_scene.gltf(filepath=str(HERE / ('orts_static_probe.' + extension)),
                                       export_format=export_format, **export_options)
    if result != {'FINISHED'}:
        raise RuntimeError('glTF export failed')

document = json.loads((HERE / 'orts_static_probe.gltf').read_text(encoding='utf-8'))
assert set(document.get('extensionsUsed', [])) <= {'KHR_materials_unlit'}
assert not document.get('animations') and not document.get('skins')
assert any(m.get('alphaMode') == 'MASK' for m in document['materials'])
assert all('KHR_materials_unlit' in m.get('extensions', {}) for m in document['materials'])
assert all(p.get('mode', 4) == 4 for mesh in document['meshes'] for p in mesh['primitives'])

# Preview setup is retained in the .blend but excluded from exported models.
bpy.ops.object.camera_add(location=(7, -10, 7))
camera = bpy.context.object
camera.name = 'Preview_camera_not_exported'
camera.rotation_euler = (Vector((0, 0, 1)) - camera.location).to_track_quat('-Z', 'Y').to_euler()
camera.data.type = 'ORTHO'
camera.data.ortho_scale = 9
scene.camera = camera
scene.render.engine = 'CYCLES'
scene.cycles.samples = 8
scene.render.resolution_x = 800
scene.render.resolution_y = 650
scene.render.resolution_percentage = 100
scene.world.color = (.25, .25, .25)
scene.render.image_settings.file_format = 'PNG'
scene.render.filepath = str(HERE / 'fixture_preview.png')
scene.view_settings.view_transform = 'Standard'
bpy.ops.wm.save_as_mainfile(filepath=str(HERE / 'orts_static_probe.blend'))
bpy.ops.render.render(write_still=True)
preview = HERE / 'fixture_preview.png'
preview.write_bytes(strip_png_metadata(preview.read_bytes()))

manifest = {
    'license': 'CC0-1.0', 'generator': bpy.app.version_string,
    'units': 'metres', 'pivot': 'ground centre',
    'orts_minimum': [-2, 0, -3], 'orts_maximum': [2, 2.85, 3],
    'extensions': ['KHR_materials_unlit'],
    'files': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
              for p in sorted(HERE.iterdir()) if p.suffix in ('.glb', '.gltf', '.bin', '.png', '.blend')},
}
(HERE / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
print('ORTS_STATIC_FIXTURE_OK ' + json.dumps(manifest))
