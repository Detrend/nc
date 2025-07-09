extends Object
class_name ErrorUtils


static func report_error(message: String)-> void:
	push_error(message);
	printerr("ERROR: {0}".format([message]));

static func report_warning(message: String)->void:
	push_warning(message)
	printerr("warning: {0}".format([message]))
