package sdl;

@:hlNative("sdl")
class Sdl {

	static var initDone = false;
	static var isWin32 = false;

	public static var requiredGLMajor(default,null) = 3;
	public static var requiredGLMinor(default,null) = 2;

	public static function init() {
		if( initDone ) return;
		initDone = true;
		if( !initOnce() ) throw "Failed to init SDL";
		isWin32 = detectWin32();
	}

	public static function setGLOptions( major : Int = 3, minor : Int = 2, depth : Int = 24, stencil : Int = 8, flags : Int = 1, samples : Int = 1 ) {
		requiredGLMajor = major;
		requiredGLMinor = minor;
		glOptions(major, minor, depth, stencil, flags, samples);
	}

	public static function setHint(name:String, value:String) {
		return @:privateAccess hintValue(name.toUtf8(), value.toUtf8());
	}

	public static dynamic function onGlContextRetry() {
		return false;
	}

	public static dynamic function onGlContextError() {
		var devices = Sdl.getDevices();
		var device = devices[0];
		if( device == null ) device = "Unknown";
		var flags = new haxe.EnumFlags<hl.UI.DialogFlags>();
		flags.set(IsError);
		var msg = 'The application was unable to create an OpenGL context\nfor your $device video card.\nOpenGL $requiredGLMajor.$requiredGLMinor+ is required, please update your driver.';
		hl.UI.dialog("OpenGL Error", msg, flags);
		Sys.exit( -1);
	}

	public static inline var DOUBLE_BUFFER            = 1 << 0;
	public static inline var GL_CORE_PROFILE          = 1 << 1;
	public static inline var GL_COMPATIBILITY_PROFILE = 1 << 2;
	public static inline var GL_ES                    = 1 << 3;

	static function glOptions( major : Int, minor : Int, depth : Int, stencil : Int, flags : Int, samples : Int ) {}

	static function initOnce() return false;
	static function eventLoop( e : Dynamic ) return false;
	static function hintValue( name : hl.Bytes, value : hl.Bytes ) return false;

	static var event = new Event();
	public static function processEvents( onEvent : Event -> Bool ) {
		while( true ) {
			if( !eventLoop(event) )
				break;
			var ret = onEvent(event);
			if( event.type == Quit && ret )
				return false;
		}
		return true;
	}

	public static function quit() {
	}

	public static function delay(time:Int) {
	}


	public static function getScreenWidth() : Int {
		return 0;
	}

	public static function getScreenHeight() : Int {
		return 0;
	}

	public static function message( title : String, text : String, error = false ) {
		@:privateAccess messageBox(title.toUtf8(), text.toUtf8(), error);
	}

	static function messageBox( title : hl.Bytes, text : hl.Bytes, error : Bool ) {
	}

	static function detectWin32() {
		return false;
	}

	static function get_devices() : hl.NativeArray<hl.Bytes> {
		return null;
	}

	public static function getDevices() {
		var a = [];
		var arr = get_devices();
		var dnames = new Map();
		for( v in arr ) {
			if( v == null ) break;
			var d = StringTools.trim(@:privateAccess String.fromUCS2(v));
			if( dnames.exists(d) || StringTools.startsWith(d,"RDP") /* RemoteDesktop */ ) continue;
			dnames.set(d, true);
			a.push(d);
		}
		return a;
	}

	public static function setRelativeMouseMode( enable : Bool ) : Int {
		return 0;
	}

	public static function getRelativeMouseMode() : Bool {
		return false;
	}
	
	public static function warpMouseGlobal( x : Int, y : Int ) : Int {
		return 0;
	}

	static function detect_keyboard_layout() : hl.Bytes {
		return null;
	}

	public static function detectKeyboardLayout(){
		return @:privateAccess String.fromUTF8(detect_keyboard_layout());
	}

	@:hlNative("?sdl", "set_clipboard_text")
	private static function _setClipboardText( text : hl.Bytes ) : Bool {
		return false;
	}

	public static function setClipboardText( text : String ) : Bool {
		if( text == null )
			return false;
		return @:privateAccess _setClipboardText( text.toUtf8() );
	}

	@:hlNative("?sdl", "get_clipboard_text")
	private static function _getClipboardText() : hl.Bytes {
		return null;
	}

	public static function getClipboardText() : String {
		var t = _getClipboardText();
		if( t == null )
			return null;
		else
			return @:privateAccess String.fromUTF8(t);
	}
}

@:enum
abstract SDLHint(String) from String to String {

	var SDL_HINT_FRAMEBUFFER_ACCELERATION =                 "SDL_FRAMEBUFFER_ACCELERATION";
	var SDL_HINT_RENDER_DRIVER =                            "SDL_RENDER_DRIVER";
	var SDL_HINT_RENDER_OPENGL_SHADERS =                    "SDL_RENDER_OPENGL_SHADERS";
	var SDL_HINT_RENDER_DIRECT3D_THREADSAFE =               "SDL_RENDER_DIRECT3D_THREADSAFE";
	var SDL_HINT_RENDER_DIRECT3D11_DEBUG =                  "SDL_RENDER_DIRECT3D11_DEBUG";
	var SDL_HINT_RENDER_SCALE_QUALITY =                     "SDL_RENDER_SCALE_QUALITY";
	var SDL_HINT_RENDER_VSYNC =                             "SDL_RENDER_VSYNC";
	var SDL_HINT_VIDEO_ALLOW_SCREENSAVER =                  "SDL_VIDEO_ALLOW_SCREENSAVER";
	var SDL_HINT_VIDEO_X11_XVIDMODE =                       "SDL_VIDEO_X11_XVIDMODE";
	var SDL_HINT_VIDEO_X11_XINERAMA =                       "SDL_VIDEO_X11_XINERAMA";
	var SDL_HINT_VIDEO_X11_XRANDR =                         "SDL_VIDEO_X11_XRANDR";
	var SDL_HINT_VIDEO_X11_NET_WM_PING =                    "SDL_VIDEO_X11_NET_WM_PING";
	var SDL_HINT_WINDOW_FRAME_USABLE_WHILE_CURSOR_HIDDEN =  "SDL_WINDOW_FRAME_USABLE_WHILE_CURSOR_HIDDEN";
	var SDL_HINT_WINDOWS_ENABLE_MESSAGELOOP =               "SDL_WINDOWS_ENABLE_MESSAGELOOP";
	var SDL_HINT_GRAB_KEYBOARD =                            "SDL_GRAB_KEYBOARD";
	var SDL_HINT_MOUSE_RELATIVE_MODE_WARP =                 "SDL_MOUSE_RELATIVE_MODE_WARP";
	var SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS =             "SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS";
	var SDL_HINT_IDLE_TIMER_DISABLED =                      "SDL_IOS_IDLE_TIMER_DISABLED";
	var SDL_HINT_ORIENTATIONS =                             "SDL_IOS_ORIENTATIONS";
	var SDL_HINT_ACCELEROMETER_AS_JOYSTICK =                "SDL_ACCELEROMETER_AS_JOYSTICK";
	var SDL_HINT_XINPUT_ENABLED =                           "SDL_XINPUT_ENABLED";
	var SDL_HINT_XINPUT_USE_OLD_JOYSTICK_MAPPING =          "SDL_XINPUT_USE_OLD_JOYSTICK_MAPPING";
	var SDL_HINT_GAMECONTROLLERCONFIG =                     "SDL_GAMECONTROLLERCONFIG";
	var SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS =         "SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS";
	var SDL_HINT_ALLOW_TOPMOST =                            "SDL_ALLOW_TOPMOST";
	var SDL_HINT_TIMER_RESOLUTION =                         "SDL_TIMER_RESOLUTION";
	var SDL_HINT_THREAD_STACK_SIZE =                        "SDL_THREAD_STACK_SIZE";
	var SDL_HINT_VIDEO_HIGHDPI_DISABLED =                   "SDL_VIDEO_HIGHDPI_DISABLED";
	var SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK =       "SDL_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK";
	var SDL_HINT_VIDEO_WIN_D3DCOMPILER =                    "SDL_VIDEO_WIN_D3DCOMPILER";
	var SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT =          "SDL_VIDEO_WINDOW_SHARE_PIXEL_FORMAT";
	var SDL_HINT_WINRT_PRIVACY_POLICY_URL =                 "SDL_WINRT_PRIVACY_POLICY_URL";
	var SDL_HINT_WINRT_PRIVACY_POLICY_LABEL =               "SDL_WINRT_PRIVACY_POLICY_LABEL";
	var SDL_HINT_WINRT_HANDLE_BACK_BUTTON =                 "SDL_WINRT_HANDLE_BACK_BUTTON";
	var SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES =              "SDL_VIDEO_MAC_FULLSCREEN_SPACES";
	var SDL_HINT_MAC_BACKGROUND_APP =                       "SDL_MAC_BACKGROUND_APP";
	var SDL_HINT_ANDROID_APK_EXPANSION_MAIN_FILE_VERSION =  "SDL_ANDROID_APK_EXPANSION_MAIN_FILE_VERSION";
	var SDL_HINT_ANDROID_APK_EXPANSION_PATCH_FILE_VERSION = "SDL_ANDROID_APK_EXPANSION_PATCH_FILE_VERSION";
	var SDL_HINT_IME_INTERNAL_EDITING =                     "SDL_IME_INTERNAL_EDITING";
	var SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH =         "SDL_ANDROID_SEPARATE_MOUSE_AND_TOUCH";
	var SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT =              "SDL_EMSCRIPTEN_KEYBOARD_ELEMENT";
	var SDL_HINT_NO_SIGNAL_HANDLERS =                       "SDL_NO_SIGNAL_HANDLERS";
	var SDL_HINT_WINDOWS_NO_CLOSE_ON_ALT_F4 =               "SDL_WINDOWS_NO_CLOSE_ON_ALT_F4";
}