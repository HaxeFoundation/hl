package dx;
import haxe.EntryPoint;

private typedef WinPtr = hl.Abstract<"dx_window">;

@:enum abstract DisplayMode(Int) {
	var Windowed = 0;
	var Fullscreen = 1;
	/**
		Fullscreen not exclusive.
	**/
	var Borderless = 2;
	var FullscreenResize = 3;
}

@:hlNative("directx")
class Window {

	static var windows : Array<Window> = [];

	public static inline var CW_USEDEFAULT = 0x80000000;

	public static inline var HIDDEN    = 0x000001;
	public static inline var RESIZABLE = 0x000002;

	var win : WinPtr;
	var savedSize : { x : Int, y : Int, width : Int, height : Int };
	public var title(default, set) : String;
	public var width(get, never) : Int;
	public var height(get, never) : Int;
	public var minWidth(get, never) : Int;
	public var minHeight(get, never) : Int;
	public var maxWidth(get, never) : Int;
	public var maxHeight(get, never) : Int;
	public var x(get, never) : Int;
	public var y(get, never) : Int;
	public var displayMode(default, set) : DisplayMode;
	public var visible(default, set) : Bool = true;
	public var opacity(get, set) : Float;
	public var vsync : Bool;

	public function new( title : String, width : Int, height : Int, x : Int = CW_USEDEFAULT, y : Int = CW_USEDEFAULT, windowFlags : Int = RESIZABLE ) {
		win = winCreateEx(x, y, width, height, windowFlags);
		this.title = title;
		windows.push(this);
		vsync = true;
	}

	function set_title(name:String) {
		winSetTitle(win, @:privateAccess name.bytes);
		return title = name;
	}

	function set_displayMode(mode) {
		if( mode == displayMode )
			return mode;
		displayMode = mode;
		var fs = mode != Windowed;
		if( savedSize == null ) {
			if( !fs ) return mode;
			savedSize = { x : x, y : y, width : width, height : height };
			winSetFullscreen(win,true);
			Driver.fullScreen = mode == Fullscreen;
		} else {
			Driver.fullScreen = mode == Fullscreen;
			if( fs )
				return mode;
			winSetFullscreen(win, false);
			resize(savedSize.width, savedSize.height);
			setPosition(savedSize.x, savedSize.y);
			savedSize = null;
		}
		return mode;
	}

	function set_visible(b) {
		if( visible == b )
			return b;
		winResize(win, b ? 4 : 3);
		return visible = b;
	}

	public function resize( width : Int, height : Int ) {
		winSetSize(win, width, height);
	}

	public function setMinSize( width : Int, height : Int ) {
		winSetMinSize(win, width, height);
	}

	public function setMaxSize( width : Int, height : Int ) {
		winSetMaxSize(win, width, height);
	}

	public function setPosition( x : Int, y : Int ) {
		winSetPosition(win, x, y);
	}
	
	public function setCursorPosition( x : Int, y : Int ) {
		return winSetCursorPos(win, x, y);
	}
	
	public static function setCursorPositionGlobal( x : Int, y : Int ) {
		return setCursorPos(x, y);
	}
	
	public function setRelativeMouseMode( enabled : Bool ) : Bool {
		return winSetRelativeMouseMode(win, enabled);
	}
	
	public function getRelativeMouseMode() : Bool {
		return winGetRelativeMouseMode();
	}

	public function center( centerPrimary : Bool = true ) {
		winCenter(win, centerPrimary);
	}

	function get_width() {
		var w = 0;
		winGetSize(win, w, null);
		return w;
	}

	function get_height() {
		var h = 0;
		winGetSize(win, null, h);
		return h;
	}

	function get_minWidth() {
		var w = 0;
		winGetMinSize(win, w, null);
		return w;
	}

	function get_minHeight() {
		var h = 0;
		winGetMinSize(win, null, h);
		return h;
	}

	function get_maxWidth() {
		var w = 0;
		winGetMaxSize(win, w, null);
		return w;
	}

	function get_maxHeight() {
		var h = 0;
		winGetMaxSize(win, null, h);
		return h;
	}

	function get_x() {
		var x = 0;
		winGetPosition(win, x, null);
		return x;
	}

	function get_y() {
		var y = 0;
		winGetPosition(win, null, y);
		return y;
	}

	function get_opacity() {
		return winGetOpacity(win);
	}

	function set_opacity(v) {
		winSetOpacity(win, v);
		return v;
	}

	public function destroy() {
		winDestroy(win);
		win = null;
		windows.remove(this);
	}

	public function maximize() {
		winResize(win, 0);
	}

	public function minimize() {
		winResize(win, 1);
	}

	public function restore() {
		winResize(win, 2);
	}

	public function getNextEvent( e : Event ) : Bool {
		return winGetNextEvent(win, e);
	}

	public function clipCursor( enable : Bool ) : Void {
		winClipCursor(win, enable);
	}

	static function winCreateEx( x : Int, y : Int, width : Int, height : Int, windowFlags : Int ) : WinPtr {
		return null;
	}

	static function winCreate( width : Int, height : Int ) : WinPtr {
		return null;
	}

	static function winSetTitle( win : WinPtr, title : hl.Bytes ) {
	}

	static function winSetFullscreen( win : WinPtr, fs : Bool ) {
	}

	static function winSetSize( win : WinPtr, width : Int, height : Int ) {
	}

	static function winSetMinSize( win : WinPtr, width : Int, height : Int ) {
	}

	static function winSetMaxSize( win : WinPtr, width : Int, height : Int ) {
	}

	static function winSetPosition( win : WinPtr, x : Int, y : Int ) {
	}

	static function winCenter( win : WinPtr, centerPrimary : Bool ) {
	}

	static function winResize( win : WinPtr, mode : Int ) {
	}

	static function winGetSize( win : WinPtr, width : hl.Ref<Int>, height : hl.Ref<Int> ) {
	}

	static function winGetMinSize( win : WinPtr, width : hl.Ref<Int>, height : hl.Ref<Int> ) {
	}

	static function winGetMaxSize( win : WinPtr, width : hl.Ref<Int>, height : hl.Ref<Int> ) {
	}

	static function winGetPosition( win : WinPtr, x : hl.Ref<Int>, y : hl.Ref<Int> ) {
	}

	static function winGetOpacity( win : WinPtr ) : Float {
		return 0.0;
	}

	static function winSetOpacity( win : WinPtr, opacity : Float ) : Bool {
		return false;
	}

	static function winDestroy( win : WinPtr ) {
	}

	public static function getScreenWidth() {
		return 0;
	}

	public static function getScreenHeight() {
		return 0;
	}

	static function winGetNextEvent( win : WinPtr, event : Dynamic ) : Bool {
		return false;
	}

	static function winClipCursor( win : WinPtr, enable : Bool ) : Void {
	}
	
	static function setCursorPos( x : Int, y : Int ) : Bool {
		return false;
	}
	
	static function winSetCursorPos( win : WinPtr, x : Int, y : Int ) : Bool {
		return false;
	}
	
	static function winSetRelativeMouseMode( win : WinPtr, enable : Bool ) : Bool {
		return false;
	}
	
	static function winGetRelativeMouseMode() : Bool { 
		return false;
	}

}