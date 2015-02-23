import os
import bpy

bl_info = {
    "name": "Export to Collada and Save File",
    "category": "Scene",
}

class ExportColladaSave(bpy.types.Operator):
    """Export to Collada and Save"""      # blender will use this as a tooltip for menu items and buttons.
    bl_idname = "scene.export_collada_and_save"        # unique identifier for buttons and menu items to reference.
    bl_label = "Export to Collada and Save File"         # display name in the interface.
    bl_options = {'REGISTER'}  # enable undo for the operator.

    def execute(self, context):        # execute() is called by blender when running the operator.
        dest = os.path.splitext(bpy.data.filepath)[0] + '.dae'
        bpy.ops.wm.collada_export(filepath=dest)
        bpy.ops.wm.save_mainfile()
        return {'FINISHED'}            # this lets blender know the operator finished successfully.

def menu_func(self, context):
    self.layout.operator(ExportColladaSave.bl_idname)

# store keymaps here to access after registration
addon_keymaps = []

def register():
    bpy.utils.register_class(ExportColladaSave)
    bpy.types.INFO_MT_file.append(menu_func)
	
	# handle the keymap
    wm = bpy.context.window_manager
    km = wm.keyconfigs.addon.keymaps.new(name='Object Mode', space_type='EMPTY')
    kmi = km.keymap_items.new(ExportColladaSave.bl_idname, 'E', 'PRESS', ctrl=True, shift=True)
    addon_keymaps.append((km, kmi))


def unregister():
    bpy.utils.unregister_class(ExportColladaSave)

	# handle the keymap
    for km, kmi in addon_keymaps:
        km.keymap_items.remove(kmi)
    addon_keymaps.clear()


# This allows you to run the script directly from blenders text editor
# to test the addon without having to install it.
if __name__ == "__main__":
    register()
