extends Object
class_name CoroutineUtils


static func wait_for_seconds(this: Node, seconds: float):
	return this.get_tree().create_timer(seconds).timeout;

static func forget_and_fire_after_seconds(this: Node, seconds: float, callback: Callable):
	await wait_for_seconds(this, seconds);
	callback.call()


class Generator:
	var Current:
		get: return _current_value
	var IsFinished:bool:
		get: return !_did_yield
	func MoveNext()->bool:
		if IsFinished: return false
		_did_yield = false
		if !_did_start:
			_did_start = true
			self.callv("_impl", self._args) 
			_args = null  #cleanup unnecessary clutter
		else:
			_on_next_signal.emit()
		return _did_yield
		
		
	func _init(args: Array=[])->void:
		self._args = args
		
	var _args;
	var _did_start : bool = false;
	var _did_yield : bool = true;
	var _current_value;
		
	signal _on_next_signal();
	
	func _yield(value)->Signal:
		_current_value = value
		_did_yield = true
		return _on_next_signal;
		
	func take_first(count: int)->Generator: return GeneratorHelpers.TakeFirst.new([self, count])
	func where(predicate: Callable)->Generator: return GeneratorHelpers.Where.new([self, predicate])
	func select(transform: Callable)->Generator: return GeneratorHelpers.Select.new([self, transform])
	func to_array(array: Array = [])->Array:
		while(self.MoveNext()): array.append(self.Current);
		return array;
	static func from_array(array: Array)->Generator: return GeneratorHelpers.FromArray.new([array])
		

class GeneratorHelpers:
	class FromArray:
		extends Generator
		func _impl(array: Array)->void:
			for item in array: await _yield(item)
	class TakeFirst:
		extends Generator
		func _impl(generator: Generator,  number: int)->void:
			while generator.MoveNext() && number > 0:
				await _yield(generator.Current)
				number -= 1
	class Select:
		extends Generator
		func _impl(generator: Generator, transform: Callable)->void:
			while generator.MoveNext():
				await _yield(transform.call(generator.Current))
	class Where:
		extends Generator
		func _impl(generator: Generator, predicate: Callable)->void:
			while generator.MoveNext():
				if predicate.call(generator.Current): await _yield(generator.Current)
