# Open Rails static scenery fixture

Original synthetic test asset generated with Blender 5.2.1 LTS. All model,
texture and preview assets in this directory are dedicated to the public domain
under CC0-1.0: https://creativecommons.org/publicdomain/zero/1.0/ .
The generator is also CC0-1.0. No private model or route content is included.

Use `orts_static_probe.glb` for the first placement test. It embeds its texture
and geometry. The equivalent `.gltf` requires `orts_static_probe.bin` and
`probe_quadrants.png` beside it. `orts_static_probe.blend` is the editable source.
`manifest.json` records generated file hashes. `fixture_preview.png` is a Blender
render, not evidence of successful TSRE rendering.

The fixture uses metres, a ground-centre pivot, indexed triangles and only
`KHR_materials_unlit`. It has five material primitives, a four-colour UV panel
with an alpha-mask hole, opaque single-sided solids and a double-sided panel.
No animation, skinning, morphs, compression, lights, cameras or PBR dependencies
are exported. Its OR-local bounds are (-2, 0, -3) through (2, 2.85, 3).

The 4 m by 6 m base, tall red post, short green post and yellow arrow make scale
and orientation mistakes visible. The yellow arrow points along OR-local -Z
after OR's +Z-to-forward conversion. Looking at the panel from the arrow side,
its top quadrants are red/green and its bottom quadrants are blue/white. Its
centre is transparent. Compare these features against the pinned OR runtime.

After an operator build and the registered model probe pass:

1. Copy the GLB to a disposable test route's Shapes directory.
2. Open that route in the rebuilt TSRE. In Objects, select Route/ShapesDirectory
   and `orts_static_probe.glb`; inspect its preview, then place it as Static.
3. Place a known `.s` beside it. Check ground pivot, metres, front/back culling,
   texture quadrants, transparency and selection outline. Click through the
   hole to an object behind it; solid parts must select the GLB itself.
4. Move, rotate, scale, clone, undo and redo. Save and reopen the disposable
   route. Verify the native `.glb` FileName, location, rotation and scale survive.
5. Repeat with the separate `.gltf` and compare against Open Rails. Try an
   unsupported/missing model: new placement must stop before adding an object.
   Existing missing references must remain saveable and appear in route errors.

To regenerate from the repository root:

```powershell
& 'Y:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --factory-startup --python-exit-code 1 --python TSREvcWIP/tests/fixtures/orts-static/create_fixture.py
```

No personal route is modified by the generator.
