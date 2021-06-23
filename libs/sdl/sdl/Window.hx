package sdl;

private typedef WinPtr = hl.Abstract<"sdl_window">;
private typedef GLContext = hl.Abstract<"sdl_gl">;

@:enum abstract DisplayMode(Int) {
	var Windowed = 0;
	var Fullscreen = 1;
	/**
		Fullscreen not exclusive.
	**/
	var Borderless = 2;
	var FullscreenResize = 3;
}

@:hlNative("sdl")
class Window {

	static var windows : Array<Window> = [];

	public static inline var SDL_WINDOWPOS_UNDEFINED = 0x1FFF0000;
	public static inline var SDL_WINDOWPOS_CENTERED  = 0x2FFF0000;

	public static inline var SDL_WINDOW_FULLSCREEN         = 0x00000001;
	public static inline var SDL_WINDOW_OPENGL             = 0x00000002;
	public static inline var SDL_WINDOW_SHOWN              = 0x00000004;
	public static inline var SDL_WINDOW_HIDDEN             = 0x00000008;
	public static inline var SDL_WINDOW_BORDERLESS         = 0x00000010;
	public static inline var SDL_WINDOW_RESIZABLE          = 0x00000020;
	public static inline var SDL_WINDOW_MINIMIZED          = 0x00000040;
	public static inline var SDL_WINDOW_MAXIMIZED          = 0x00000080;
	public static inline var SDL_WINDOW_INPUT_GRABBED      = 0x00000100;
	public static inline var SDL_WINDOW_INPUT_FOCUS        = 0x00000200;
	public static inline var SDL_WINDOW_MOUSE_FOCUS        = 0x00000400;
	public static inline var SDL_WINDOW_FULLSCREEN_DESKTOP = 0x00001001;
	public static inline var SDL_WINDOW_FOREIGN            = 0x00000800;
	public static inline var SDL_WINDOW_ALLOW_HIGHDPI      = 0x00002000;
	public static inline var SDL_WINDOW_MOUSE_CAPTURE      = 0x00004000;
	public static inline var SDL_WINDOW_ALWAYS_ON_TOP      = 0x00008000;
	public static inline var SDL_WINDOW_SKIP_TASKBAR       = 0x00010000;
	public static inline var SDL_WINDOW_UTILITY            = 0x00020000;
	public static inline var SDL_WINDOW_TOOLTIP            = 0x00040000;
	public static inline var SDL_WINDOW_POPUP_MENU         = 0x00080000;
	public static inline var SDL_WINDOW_VULKAN             = 0x10000000;

	var win : WinPtr;
	var glctx : GLContext;
	var lastFrame : Float;
	public var title(default, set) : String;
	public var vsync(default, set) : Bool;
	public var width(get, never) : Int;
	public var height(get, never) : Int;
	public var windowToPixelRatio(get, never) : Float;
	public var minWidth(get, never) : Int;
	public var minHeight(get, never) : Int;
	public var maxWidth(get, never) : Int;
	public var maxHeight(get, never) : Int;
	public var x(get, never) : Int;
	public var y(get, never) : Int;
	public var displayMode(default, set) : DisplayMode;
	public var visible(default, set) : Bool = true;
	public var opacity(get, set) : Float;

	public function new( title : String, width : Int, height : Int, x : Int = SDL_WINDOWPOS_CENTERED, y : Int = SDL_WINDOWPOS_CENTERED, sdlFlags : Int = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE ) {
		while( true ) {
			win = winCreateEx(x, y, width, height, sdlFlags);
			if( win == null ) throw "Failed to create window";
			glctx = winGetGLContext(win);
			if( glctx == null || !GL.init() || !testGL() ) {
				destroy();
				if( Sdl.onGlContextRetry() ) continue;
				Sdl.onGlContextError();
			}
			break;
		}
		this.title = title;
		windows.push(this);
		vsync = true;
	}

	function testGL() {
		try {

			var reg = ~/[0-9]+\.[0-9]+/;
			var v : String = GL.getParameter(GL.SHADING_LANGUAGE_VERSION);

			var glv : String = GL.getParameter(GL.VERSION);
			var isOpenGLES : Bool = ((glv!=null) && (glv.indexOf("ES") >= 0));

			var shaderVersion = 120;
			if (isOpenGLES) {
				if( reg.match(v) )
					shaderVersion = Std.int(Math.min( 100, Math.round( Std.parseFloat(reg.matched(0)) * 100 ) ));
			}
			else {
				shaderVersion = 130;
				if( reg.match(v) ) {
					var minVer = 150;
					shaderVersion = Math.round( Std.parseFloat(reg.matched(0)) * 100 );
					if( shaderVersion < minVer ) shaderVersion = minVer;
				}
			}

			var vertex = GL.createShader(GL.VERTEX_SHADER);
			GL.shaderSource(vertex, ["#version " + shaderVersion, "void main() { gl_Position = vec4(1.0); }"].join("\n"));
			GL.compileShader(vertex);
			if( GL.getShaderParameter(vertex, GL.COMPILE_STATUS) != 1 ) throw "Failed to compile VS ("+GL.getShaderInfoLog(vertex)+")";

			var fragment = GL.createShader(GL.FRAGMENT_SHADER);
			if (isOpenGLES)
				GL.shaderSource(fragment, ["#version " + shaderVersion, "lowp vec4 color; void main() { color = vec4(1.0); }"].join("\n"));
			else
				GL.shaderSource(fragment, ["#version " + shaderVersion, "out vec4 color; void main() { color = vec4(1.0); }"].join("\n"));
			GL.compileShader(fragment);
			if( GL.getShaderParameter(fragment, GL.COMPILE_STATUS) != 1 ) throw "Failed to compile FS ("+GL.getShaderInfoLog(fragment)+")";

			var p = GL.createProgram();
			GL.attachShader(p, vertex);
			GL.attachShader(p, fragment);
			GL.linkProgram(p);

			if( GL.getProgramParameter(p, GL.LINK_STATUS) != 1 ) throw "Failed to link ("+GL.getProgramInfoLog(p)+")";

			GL.deleteShader(vertex);
			GL.deleteShader(fragment);

		} catch( e : Dynamic ) {

			return false;
		}
		return true;
	}

	function set_title(name:String) {
		winSetTitle(win, @:privateAccess name.toUtf8());
		return title = name;
	}

	function set_displayMode(mode) {
		if( mode == displayMode )
			return mode;
		if( winSetFullscreen(win, cast mode) )
			displayMode = mode;
		return displayMode;
	}

	function set_visible(b) {
		if( visible == b )
			return b;
		winResize(win, b ? 3 : 4);
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

	public function center() {
		setPosition(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}

	function get_width() {
		var w = 0;
		winGetSize(win, w, null);
		return w;
	}

	function get_windowToPixelRatio() {
		var pixelHeight = 0;
		winGetPixelSize(win, null, pixelHeight);
		var h : Float = height;

		return h / pixelHeight;
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

	function set_vsync(v) {
		setVsync(v);
		return vsync = v;
	}

	function get_opacity() {
		return winGetOpacity(win);
	}

	function set_opacity(v) {
		winSetOpacity(win, v);
		return v;
	}

	/**
		Set the current window you will render to (in case of multiple windows)
	**/
	public function renderTo() {
		winRenderTo(win, glctx);
	}

	public function present() {
		if( vsync && @:privateAccess Sdl.isWin32 ) {
			// NVIDIA OpenGL windows driver does implement vsync as an infinite loop, causing high CPU usage
			// make sure to sleep a bit here based on how much time we spent since the last frame
			var spent = haxe.Timer.stamp() - lastFrame;
			if( spent < 0.005 ) Sys.sleep(0.005 - spent);
		}
		winSwapWindow(win);
		lastFrame = haxe.Timer.stamp();
	}

	public function destroy() {
		try winDestroy(win, glctx) catch( e : Dynamic ) {};
		win = null;
		glctx = null;
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

	static function winCreateEx( x : Int, y : Int, width : Int, height : Int, sdlFlags : Int ) : WinPtr {
		return null;
	}

	static function winCreate( width : Int, height : Int ) : WinPtr {
		return null;
	}

	static function winSetTitle( win : WinPtr, title : hl.Bytes ) {
	}

	static function winSetPosition( win : WinPtr, width : Int, height : Int ) {
	}

	static function winGetPosition( win : WinPtr, width : hl.Ref<Int>, height : hl.Ref<Int> ) {
	}

	static function winGetGLContext( win : WinPtr ) : GLContext {
		return null;
	}

	static function winSwapWindow( win : WinPtr ) {
	}

	static function winSetFullscreen( win : WinPtr, mode : DisplayMode ) {
		return false;
	}

	static function winSetSize( win : WinPtr, width : Int, height : Int ) {
	}

	static function winResize( win : WinPtr, mode : Int ) {
	}

	static function winSetMinSize( win : WinPtr, width : Int, height : Int ) {
	}

	static function winSetMaxSize( win : WinPtr, width : Int, height : Int ) {
	}

	static function winGetSize( win : WinPtr, width : hl.Ref<Int>, height : hl.Ref<Int> ) {
	}

	static function winGetPixelSize( win : WinPtr, width : hl.Ref<Int>, height : hl.Ref<Int> ) {
	}

	static function winGetMinSize( win : WinPtr, width : hl.Ref<Int>, height : hl.Ref<Int> ) {
	}

	static function winGetMaxSize( win : WinPtr, width : hl.Ref<Int>, height : hl.Ref<Int> ) {
	}

	static function winGetOpacity( win : WinPtr ) : Float {
		return 0.0;
	}

	static function winSetOpacity( win : WinPtr, opacity : Float ) : Bool {
		return false;
	}

	static function winRenderTo( win : WinPtr, gl : GLContext ) {
	}

	static function winDestroy( win : WinPtr, gl : GLContext ) {
	}

	static function setVsync( b : Bool ) {
	}

}
