## This file contains helper functions for easy reporting of errors

extends Object
class_name ErrorUtils


## Prints an error message through all available error outputs
static func report_error(message: String)-> void:
	push_error(message);
	printerr("ERROR: {0}".format([message]));

## Prints a warning message through all available error outputs
static func report_warning(message: String)->void:
	push_warning(message)
	#printerr("warning: {0}".format([message]))

## Prints an error message that's related to a specific [Node]
static func node_error(node: Node, message: String)->void:
	push_error("{0}| {1}".format([NodeUtils.get_full_name(node), message]))

## Prints a warning message that's related to a specific [Node]
static func node_warning(node: Node, message: String)->void:
	push_warning("{0}| {1}".format([NodeUtils.get_full_name(node), message]))
