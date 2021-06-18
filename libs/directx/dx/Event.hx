package dx;

@:keep class Event {
	public var type : EventType;
	public var mouseX : Int;
	public var mouseY : Int;
	public var mouseXRel : Int;
	public var mouseYRel : Int;
	public var button : Int;
	public var wheelDelta : Int;
	public var state : WindowStateChange;
	public var keyCode : Int;
	public var scanCode : Int;
	public var keyRepeat : Bool;
	public var controller : Int;
	public var value : Int;
	public function new() {
	}
}

@:enum abstract EventType(Int) {
	var Quit		= 0;
	var MouseMove	= 1;
	var MouseLeave	= 2;
	var MouseDown	= 3;
	var MouseUp		= 4;
	var MouseWheel	= 5;
	var WindowState	= 6;
	var KeyDown		= 7;
	var KeyUp		= 8;
	var TextInput	= 9;
}

@:enum abstract WindowStateChange(Int) {
	var Show	= 0;
	var Hide	= 1;
	var Expose	= 2;
	var Move	= 3;
	var Resize	= 4;
	var Minimize= 5;
	var Maximize= 6;
	var Restore	= 7;
	var Enter	= 8;
	var Leave	= 9;
	var Focus	= 10;
	var Blur	= 11;
	var Close 	= 12;
}
