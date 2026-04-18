## This file contains helper functions to be used when emulating the concept of interfaces/abstract classes (which are not directly supported by GDScript)

extends Object
class_name InterfaceUtils

class _Private:
	static func get_object_class_name(o: Object)->String:
		if o is Node:
			var scr = o.get_script();
			if(scr): o = scr;
		return ""+o.get_path();

## Should be called by the _init() function of an abstract class. 
## Checks if this instance is directly of the abstract class and if it is, throws an error.
static func report_interface_instantiated_error(interface_instance: Object, interface_type):
	if interface_instance.get_script() == interface_type:
		ErrorUtils.report_error("ERROR: Direct instantiation of interface type node '{0}'".format([interface_type.get_path()]))

## Should be called by a function with no implementation, that has to be overriden by a derived class 
static func report_not_implemented_error(method_descriptor: Callable):
	ErrorUtils.report_error("ERROR: Not implemented: '{0}::{1}'!".format([_Private.get_object_class_name(method_descriptor.get_object()), method_descriptor.get_method()]));
