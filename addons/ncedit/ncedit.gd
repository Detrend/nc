@tool
extends EditorPlugin


func _enable_plugin() -> void:
	# Add autoloads here.
	pass


func _disable_plugin() -> void:
	# Remove autoloads here.
	pass

var dock : Control
func _enter_tree() -> void:
	# Initialization of the plugin goes here.
	dock = preload("res://addons/ncedit/export_dock/export_dock.tscn").instantiate()
	add_control_to_container(EditorPlugin.CONTAINER_CANVAS_EDITOR_MENU, dock)
	_init_preview_generator()

func _exit_tree() -> void:
	# Clean-up of the plugin goes here.
	remove_control_from_container(EditorPlugin.CONTAINER_CANVAS_EDITOR_MENU, dock)
	dock.queue_free()
	_destroy_preview_generator()



var preview_generator: CustomResourcePreviewGenerator

func _init_preview_generator() -> void:
	preview_generator = CustomResourcePreviewGenerator.new()
	EditorInterface.get_resource_previewer().add_preview_generator(preview_generator)

func _destroy_preview_generator() -> void:
	EditorInterface.get_resource_previewer().remove_preview_generator(preview_generator)
