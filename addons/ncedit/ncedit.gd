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


func _exit_tree() -> void:
	# Clean-up of the plugin goes here.
	remove_control_from_container(EditorPlugin.CONTAINER_CANVAS_EDITOR_MENU, dock)
	dock.queue_free()
