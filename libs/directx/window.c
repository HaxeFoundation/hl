#define HL_NAME(n) directx_##n
#include <hl.h>

#define MAX_EVENTS 1024

typedef enum {
	Quit		= 0,
	MouseMove	= 1,
	MouseLeave	= 2,
	MouseDown	= 3,
	MouseUp		= 4,
	MouseWheel	= 5,
	WindowState	= 6,
	KeyDown		= 7,
	KeyUp		= 8,
	TextInput	= 9
} EventType;

typedef enum {
	Show	= 0,
	Hide	= 1,
	Expose	= 2,
	Move	= 3,
	Resize	= 4,
	Minimize= 5,
	Maximize= 6,
	Restore	= 7,
	Enter	= 8,
	Leave	= 9,
	Focus	= 10,
	Blur	= 11,
	Close 	= 12
} WindowStateChange;

typedef enum {
	Hidden    = 0x000001,
	Resizable = 0x000002
} WindowFlags;

typedef struct {
	hl_type *t;
	EventType type;
	int mouseX;
	int mouseY;
	int mouseXRel;
	int mouseYRel;
	int button;
	int wheelDelta;
	WindowStateChange state;
	int keyCode;
	int scanCode;
	bool keyRepeat;
	int controller;
	int value;
} dx_event;

typedef struct {
	dx_event events[MAX_EVENTS];
	DWORD normal_style;
	double opacity;
	int min_width;
	int min_height;
	int max_width;
	int max_height;
	int event_count;
	int next_event;
	bool is_over;
	bool is_focused;
} dx_events;

typedef struct HWND__ dx_window;

static dx_window *cur_clip_cursor_window = NULL;
static DWORD capture_refresh_time = 0;
typedef enum {
	SkipUpdate = 1,
	InTitleClick = 2,
	LButton = 4,
	RButton = 8,
	MButton = 16,
	XButton1 = 32,
	XButton2 = 64
} mouse_capture_flags;
static int disable_capture = 0;
static bool capture_mouse = false;
static bool relative_mouse = false;

typedef HCURSOR dx_cursor;

static dx_cursor cur_cursor = NULL;
static bool show_cursor = true;

#define CURSOR_VISIBLE show_cursor && !relative_mouse

static dx_events *get_events(HWND wnd) {
	return (dx_events*)GetWindowLongPtr(wnd,GWLP_USERDATA);
}

static void updateClipCursor(HWND wnd) {
	if ( disable_capture ) {
		ClipCursor(NULL);
		capture_refresh_time = GetTickCount();
		return;
	}
	if ( !capture_mouse && !relative_mouse ) {
		cur_clip_cursor_window = NULL;
		ClipCursor(NULL);
	} else {
		cur_clip_cursor_window = wnd;
		RECT rect;

		if ( GetClientRect(wnd, &rect) && !IsRectEmpty(&rect) ) {
			ClientToScreen(wnd, (LPPOINT)&rect.left);
			ClientToScreen(wnd, (LPPOINT)&rect.right);
			ClipCursor(&rect);
		}
	}
	capture_refresh_time = GetTickCount();
}
static void checkCaptureFlags( HWND wnd ) {
	if ( !(GetAsyncKeyState(VK_LBUTTON) & 0x8000) )
		disable_capture &= ~LButton;
	if ( !(GetAsyncKeyState(VK_RBUTTON) & 0x8000) )
		disable_capture &= ~RButton;
	if ( !(GetAsyncKeyState(VK_MBUTTON) & 0x8000) )
		disable_capture &= ~MButton;
	if ( !(GetAsyncKeyState(VK_XBUTTON1) & 0x8000) )
		disable_capture &= ~XButton1;
	if ( !(GetAsyncKeyState(VK_XBUTTON2) & 0x8000) )
		disable_capture &= ~XButton2;
}

static bool setRelativeMode( HWND wnd, bool enabled ) {
	RAWINPUTDEVICE mouse = { 0x01, 0x02, 0, NULL }; /* Mouse: UsagePage = 1, Usage = 2 */

	if ( relative_mouse == enabled ) return true;
	
	if ( !enabled ) {
		mouse.dwFlags |= RIDEV_REMOVE;
		if ( show_cursor ) SetCursor(cur_cursor);
	} else {
		SetCursor(NULL);
	}
	
	relative_mouse = enabled;
	updateClipCursor(wnd);

	return RegisterRawInputDevices(&mouse, 1, sizeof(RAWINPUTDEVICE)) || !enabled;
}

static dx_event *addEvent( HWND wnd, EventType type ) {
	dx_events *buf = get_events(wnd);
	dx_event *e;
	if( buf->event_count == MAX_EVENTS )
		e = &buf->events[MAX_EVENTS-1];
	else
		e = &buf->events[buf->event_count++];
	e->type = type;
	return e;
}

#define addMouse(etype,but) { \
	e = addEvent(wnd,etype); \
	e->button = but; \
	e->mouseX = (short)LOWORD(lparam); \
	e->mouseY = (short)HIWORD(lparam); \
}

#define addState(wstate) { e = addEvent(wnd,WindowState); e->state = wstate; }

static bool shift_downs[] = { false, false };

static LRESULT CALLBACK WndProc( HWND wnd, UINT umsg, WPARAM wparam, LPARAM lparam ) {
	dx_event *e = NULL;
	switch(umsg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CREATE:
		SetWindowLongPtr(wnd,GWLP_USERDATA,(LONG_PTR)((CREATESTRUCT*)lparam)->lpCreateParams);
		break;
	case WM_SIZE:
		{
			dx_events *buf = get_events(wnd);
			if( buf->event_count > buf->next_event && buf->events[buf->event_count-1].type == WindowState && buf->events[buf->event_count-1].state == Resize )
				buf->event_count--;
		}
		addState(Resize);
		break;
	case WM_LBUTTONDOWN: addMouse(MouseDown,1); break;
	case WM_LBUTTONUP: addMouse(MouseUp,1); break;
	case WM_MBUTTONDOWN: addMouse(MouseDown,2); break;
	case WM_MBUTTONUP: addMouse(MouseUp,2); break;
	case WM_RBUTTONDOWN: addMouse(MouseDown,3); break;
	case WM_RBUTTONUP: addMouse(MouseUp,3); break;
	case WM_XBUTTONDOWN: addMouse(MouseDown,3 + HIWORD(wparam)); break;
	case WM_XBUTTONUP: addMouse(MouseUp,3 + HIWORD(wparam)); break;
	case WM_MOUSEWHEEL: addMouse(MouseWheel,0); e->wheelDelta = ((int)(short)HIWORD(wparam)) / 120; break;
	case WM_SYSCOMMAND:
		if( wparam == SC_KEYMENU && (lparam>>16) <= 0 )
			return 0;
		break;	
	case WM_MOUSEMOVE: 
		{
			dx_events *evt = get_events(wnd);
			if( !evt->is_over ) {
				TRACKMOUSEEVENT ev;
				ev.cbSize = sizeof(ev);
				ev.dwFlags = TME_LEAVE;
				ev.hwndTrack = wnd;
				TrackMouseEvent(&ev);
				evt->is_over = true;
				addState(Enter);
			}
			if ( !relative_mouse )
				addMouse(MouseMove, 0);
		}
		break;
	case WM_INPUT:
		{
			dx_events* evt = get_events(wnd);
			if ( evt->is_focused ) {
				// Only handle raw input when window is active
				HRAWINPUT hRawInput = (HRAWINPUT)lparam;
				RAWINPUT inp;
				UINT size = sizeof(inp);
				GetRawInputData(hRawInput, RID_INPUT, &inp, &size, sizeof(RAWINPUTHEADER));
				
				// Ignore pseudo-movement from touch events.
				if ( inp.header.dwType == RIM_TYPEMOUSE ) {
					RAWMOUSE* mouse = &inp.data.mouse;
					
					if ( (mouse->usFlags & 0x01) == MOUSE_MOVE_RELATIVE ) {
						if ( mouse->lLastX != 0 || mouse->lLastY != 0 ) {
							e = addEvent(wnd, MouseMove);
							e->mouseX = 0;
							e->mouseY = 0;
							e->button = 0;
							e->mouseXRel = mouse->lLastX;
							e->mouseYRel = mouse->lLastY;
						}
						
					} else if ( mouse->lLastX || mouse->lLastY ) {
						// Absolute movement - simulate relative movement
						
						static POINT lastMousePos;
						
						bool virtual_desktop = mouse->usFlags & MOUSE_VIRTUAL_DESKTOP;
						int w = GetSystemMetrics(virtual_desktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
						int h = GetSystemMetrics(virtual_desktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
						int x = (int)(((float)mouse->lLastX / 65535.0f) * w);
						int y = (int)(((float)mouse->lLastY / 65535.0f) * h);

						if (lastMousePos.x == 0 && lastMousePos.y == 0) {
							lastMousePos.x = x;
							lastMousePos.y = y;
						} else {
							e = addEvent(wnd, MouseMove);
							e->mouseX = 0;
							e->mouseY = 0;
							e->button = 0;
							e->mouseXRel = x - lastMousePos.x;
							e->mouseYRel = y - lastMousePos.y;
							lastMousePos.x = x;
							lastMousePos.y = y;
						}
						
					}
				}
			}
		}
		break;
	case WM_MOUSELEAVE:
		addState(Leave);
		get_events(wnd)->is_over = false;
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		// right alt has triggered a control !
		if( wparam == VK_MENU && lparam & (1<<24) ) {
			dx_events *buf = get_events(wnd);
			if( buf->event_count > buf->next_event && buf->events[buf->event_count-1].type == (umsg == WM_KEYUP ? KeyUp : KeyDown) && buf->events[buf->event_count-1].keyCode == (VK_CONTROL|256) ) {
				buf->event_count--;
				//printf("CANCEL\n");
			}
		}
		bool repeat = (umsg == WM_KEYDOWN || umsg == WM_SYSKEYDOWN) && (lparam & 0x40000000) != 0;
		// see 
		e = addEvent(wnd,(umsg == WM_KEYUP || umsg == WM_SYSKEYUP) ? KeyUp : KeyDown);
		e->keyCode = (int)wparam;
		e->scanCode = (lparam >> 16) & 0xFF;
		e->keyRepeat = repeat;
		// L/R location
		if( e->keyCode == VK_SHIFT ) {
			bool right = MapVirtualKey((lparam >> 16) & 0xFF, MAPVK_VSC_TO_VK_EX) == VK_RSHIFT;
			e->keyCode |= right ? 512 : 256;
			e->keyRepeat = false;
			shift_downs[right?1:0] = e->type == KeyDown;
		}
		if( e->keyCode == VK_SHIFT || e->keyCode == VK_CONTROL || e->keyCode == VK_MENU )
			e->keyCode |= (lparam & (1<<24)) ? 512 : 256;
		if( e->keyCode == 13 && (lparam & 0x1000000) )
			e->keyCode = 108; // numpad enter
		//printf("%.8X - %d[%s]%s%s\n",lparam,e->keyCode&255,e->type == KeyUp ? "UP":"DOWN",e->keyRepeat?" REPEAT":"",(e->keyCode&256) ? " LEFT" : (e->keyCode & 512) ? " RIGHT" : "");
		if( (e->keyCode & 0xFF) == VK_MENU )
			return 0;
		break;
	case WM_TIMER:
		// bugfix for shifts being considered as a single key (one single WM_KEYUP is received when both are down)
		if( shift_downs[0] && GetKeyState(VK_LSHIFT) >= 0 ) {
			//printf("LSHIFT RELEASED\n");
			shift_downs[0] = false;
			e = addEvent(wnd,KeyUp);
			e->keyCode = VK_SHIFT | 256;
		}			
		if( shift_downs[1] && GetKeyState(VK_RSHIFT) >= 0 ) {
			//printf("RSHIFT RELEASED\n");
			shift_downs[1] = false;
			e = addEvent(wnd,KeyUp);
			e->keyCode = VK_SHIFT | 512;
		}

		if ( ( capture_mouse || relative_mouse ) && get_events(wnd)->is_focused ) {
			// Refresh the cursor capture every 3s in case some app hijacks it.
			if ( disable_capture ) {
				checkCaptureFlags(wnd);
				if (!disable_capture || disable_capture == SkipUpdate) {
					disable_capture = 0;
					updateClipCursor(wnd);
				}
			} else if ( GetTickCount() - capture_refresh_time >= 3000 ) {
				disable_capture &= ~SkipUpdate;
				updateClipCursor(wnd);
			}
		}

		break;
	case WM_CHAR:
		e = addEvent(wnd,TextInput);
		e->keyCode = (int)wparam;
		e->keyRepeat = (lparam & 0xFFFF) != 0;
		break;
	case WM_NCACTIVATE:
		// Allow user to interact with the titlebar without clipping.
		disable_capture |= SkipUpdate;
		ClipCursor(NULL);
		break;
	case WM_ACTIVATE:
		// HIWORD(wparam) = minimized flag
		if (!(bool)HIWORD(wparam) && LOWORD(wparam) != WA_INACTIVE) {

			if ( LOWORD(wparam) == WA_CLICKACTIVE ) {
				if ( GetAsyncKeyState(VK_LBUTTON) )
					disable_capture |= LButton;
				if ( GetAsyncKeyState(VK_RBUTTON) )
					disable_capture |= RButton;
				if ( GetAsyncKeyState(VK_MBUTTON) )
					disable_capture |= MButton;
				if ( GetAsyncKeyState(VK_XBUTTON1) )
					disable_capture |= XButton1;
				if ( GetAsyncKeyState(VK_XBUTTON2) )
					disable_capture |= XButton2;
			}

			checkCaptureFlags(wnd);
			updateClipCursor(wnd);
		} else {
			ClipCursor(NULL);
		}
		break;
	case WM_NCLBUTTONDOWN:
		disable_capture |= InTitleClick;
		ClipCursor(NULL);
		break;
	case WM_CAPTURECHANGED:
		disable_capture &= ~InTitleClick;
		checkCaptureFlags(wnd);
		updateClipCursor(wnd);
		break;
	case WM_SETFOCUS:
		get_events(wnd)->is_focused = true;
		updateClipCursor(wnd);
		addState(Focus);
		break;
	case WM_KILLFOCUS:
		shift_downs[0] = false;
		shift_downs[1] = false;
		if ( capture_mouse || relative_mouse ) ClipCursor(NULL);
		get_events(wnd)->is_focused = false;
		addState(Blur);
		break;
	case WM_WINDOWPOSCHANGED:
		updateClipCursor(wnd);
		break;
	case WM_GETMINMAXINFO:
	{
		dx_events *buf = get_events(wnd);
		if( buf ) {
			long resizable_flags = (WS_MAXIMIZEBOX | WS_THICKFRAME);
			if( (buf->normal_style & resizable_flags) == resizable_flags ) {
				if( buf->min_width != 0 || buf->min_height != 0 ||
					buf->max_width != 0 || buf->max_height != 0 ) {
					MINMAXINFO *info = (MINMAXINFO*)lparam;

					RECT r;
					GetClientRect(wnd, &r);
					int curr_width = r.right;
					int curr_height = r.bottom;

					// remove curr_width/height which contain non-client dimensions
					bool apply_max = false;
					int min_width = buf->min_width - curr_width;
					int min_height = buf->min_height - curr_height;
					int max_width = buf->max_width;
					int max_height = buf->max_height;
					if( max_width && max_height ) {
						max_width -= curr_width;
						max_height -= curr_height;
						apply_max = true;
					}

					// fix curr_width/height to contain only client size
					{
						RECT size;
						LONG style = GetWindowLong(wnd, GWL_STYLE);
						BOOL menu = (style & WS_CHILDWINDOW) ? FALSE : (GetMenu(wnd) != NULL);
						size.top = 0;
						size.left = 0;
						size.bottom = curr_height;
						size.right = curr_width;

						AdjustWindowRectEx(&size, style, menu, 0);
						curr_width = size.right - size.left;
						curr_height = size.bottom - size.top;
					}

					// apply curr_width/height without client size
					info->ptMinTrackSize.x = min_width + curr_width;
					info->ptMinTrackSize.y = min_height + curr_height;
					if( apply_max ) {
						info->ptMaxTrackSize.x = max_width + curr_width;
						info->ptMaxTrackSize.y = max_height + curr_height;
					}

					return 0;
				}
			}
		}
		break;
	}
	case WM_SETCURSOR:
		if( LOWORD(lparam) == HTCLIENT ) {
			if( CURSOR_VISIBLE )
				SetCursor(cur_cursor != NULL ? cur_cursor : LoadCursor(NULL, IDC_ARROW));
			else
				SetCursor(NULL);
			return TRUE;
		}
		break;
	case WM_CLOSE:
		addEvent(wnd, Quit);
		return 0;
	}
	return DefWindowProc(wnd, umsg, wparam, lparam);
}

HL_PRIM dx_window *HL_NAME(win_create_ex)( int x, int y, int width, int height, WindowFlags windowFlags ) {
	static bool wnd_class_reg = false;
	HINSTANCE hinst = GetModuleHandle(NULL);
	if( !wnd_class_reg ) {
		WNDCLASSEX wc;
		wchar_t fileName[1024];
		GetModuleFileName(hinst,fileName,1024);
		wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.lpfnWndProc   = WndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = hinst;
		wc.hIcon		 = ExtractIcon(hinst, fileName, 0);
		wc.hIconSm       = wc.hIcon;
		wc.hCursor       = NULL;
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = USTR("HL_WIN");
		wc.cbSize        = sizeof(WNDCLASSEX);
		RegisterClassEx(&wc);
	}

	RECT r;
	DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	if( !(windowFlags & Resizable) ) {
		style &= ~( WS_MAXIMIZEBOX | WS_THICKFRAME );
	}
	r.left = r.top = 0;
	r.right = width;
	r.bottom = height;
	AdjustWindowRect(&r,style,false);
	dx_events *event_buffer = (dx_events*)malloc(sizeof(dx_events));
	memset(event_buffer,0, sizeof(dx_events));
	event_buffer->normal_style = style;
	event_buffer->opacity = 1.0;
	dx_window *win = CreateWindowEx(WS_EX_APPWINDOW, USTR("HL_WIN"), USTR(""), style, x, y, r.right - r.left, r.bottom - r.top, NULL, NULL, hinst, event_buffer);
	SetTimer(win,0,10,NULL);
	if( !(windowFlags & Hidden) ) {
		ShowWindow(win, SW_SHOW);
	}
	SetForegroundWindow(win);
	SetFocus(win);
	return win;
}

HL_PRIM dx_window *HL_NAME(win_create)( int width, int height ) {
	return HL_NAME(win_create_ex)(CW_USEDEFAULT, CW_USEDEFAULT, width, height, Resizable);
}

HL_PRIM void HL_NAME(win_set_title)(dx_window *win, vbyte *title) {
	SetWindowText(win,(LPCWSTR)title);
}

HL_PRIM void HL_NAME(win_set_size)(dx_window *win, int width, int height) {
	RECT r;
	GetWindowRect(win,&r);
	r.left = r.top = 0;
	r.right = width;
	r.bottom = height;
	AdjustWindowRectEx(&r,GetWindowLong(win,GWL_STYLE),GetMenu(win) != NULL,GetWindowLong(win,GWL_EXSTYLE));
	SetWindowPos(win,NULL,0,0,r.right - r.left,r.bottom - r.top,SWP_NOMOVE|SWP_NOOWNERZORDER);
}

HL_PRIM void HL_NAME(win_get_size)(dx_window *win, int *width, int *height) {
	RECT r;
	GetClientRect(win,&r);
	if( width ) *width = r.right;
	if( height ) *height = r.bottom;
}

HL_PRIM void HL_NAME(win_set_min_size)(dx_window *win, int width, int height) {
	dx_events *buf = get_events(win);

	if( width < 0 ) width = 0;
	if( height < 0 ) height = 0;
	if( buf->max_width != 0 && width > buf->max_width ) width = buf->max_width;
	if( buf->max_height != 0 && height > buf->max_height ) height = buf->max_height;

	if( buf->min_width != width || buf->min_height != height ) {
		buf->min_width = width;
		buf->min_height = height;

		int curr_width = 0;
		int curr_height = 0;
		HL_NAME(win_get_size)(win, &curr_width, &curr_height);
		if( curr_width < width || curr_height < height )
			HL_NAME(win_set_size)(win, max(curr_width, width), max(curr_height, height));
	}
}

HL_PRIM void HL_NAME(win_get_min_size)(dx_window *win, int *width, int *height) {
	dx_events *buf = get_events(win);
	if( width ) *width = buf->min_width;
	if( height ) *height = buf->min_height;
}

HL_PRIM void HL_NAME(win_set_max_size)(dx_window *win, int width, int height) {
	dx_events *buf = get_events(win);
	if( width == 0 && height == 0 ) {
		buf->max_width = 0;
		buf->max_height = 0;
	} else {
		if( width < buf->min_width ) width = buf->min_width;
		if( height < buf->min_height ) height = buf->min_height;
		if( width == 0 ) width = 1;
		if( height == 0 ) height = 1;
		if( buf->max_width != width || buf->max_height != height ) {
			buf->max_width = width;
			buf->max_height = height;

			int curr_width = 0;
			int curr_height = 0;
			HL_NAME(win_get_size)(win, &curr_width, &curr_height);
			if( curr_width > width || curr_height > height )
				HL_NAME(win_set_size)(win, min(curr_width, width), min(curr_height, height));
		}
	}
}

HL_PRIM void HL_NAME(win_get_max_size)(dx_window *win, int *width, int *height) {
	dx_events *buf = get_events(win);
	if( width ) *width = buf->max_width;
	if( height ) *height = buf->max_height;
}

HL_PRIM void HL_NAME(win_get_position)(dx_window *win, int *x, int *y) {
	RECT r;
	GetWindowRect(win,&r);
	if( x ) *x = r.left;
	if( y ) *y = r.top;
}

HL_PRIM void HL_NAME(win_set_position)(dx_window *win, int x, int y) {
	SetWindowPos(win,NULL,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
}

// initially written with the intent to center on closest monitor; however, SDL centers to primary, so both options are provided
HL_PRIM void HL_NAME(win_center)(dx_window *win, bool centerPrimary) {
	int scnX = 0;
	int scnY = 0;
	int scnWidth = -1;
	int scnHeight = -1;
	if( centerPrimary ) {
		scnWidth = GetSystemMetrics(SM_CXSCREEN);
		scnHeight = GetSystemMetrics(SM_CYSCREEN);
	} else {
		HMONITOR m = MonitorFromWindow(win, MONITOR_DEFAULTTONEAREST);
		if( m != NULL ) {
			MONITORINFO info;
			info.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(m, &info);
			RECT screen = info.rcMonitor; // "rcMonitor" to match SM_CXSCREEN/SM_CYSCREEN measurements
			scnX = screen.left;
			scnY = screen.top;
			scnWidth = (screen.right - scnX);
			scnHeight = (screen.bottom - scnY);
		}
	}
	if( scnWidth >= 0 && scnHeight >= 0 ) {
		int winWidth = 0;
		int winHeight = 0;
		HL_NAME(win_get_size)(win, &winWidth, &winHeight);
		HL_NAME(win_set_position)(win, scnX + ((scnWidth - winWidth) / 2), scnY + ((scnHeight - winHeight) / 2));
	}
}

HL_PRIM void HL_NAME(win_resize)(dx_window *win, int mode) {
	switch( mode ) {
	case 0:
		ShowWindow(win, SW_MAXIMIZE);
		break;
	case 1:
		ShowWindow(win, SW_MINIMIZE);
		break;
	case 2:
		ShowWindow(win, SW_RESTORE);
		break;
	case 3:
		ShowWindow(win, SW_HIDE);
		break;
	case 4:
		ShowWindow(win, SW_SHOW);
		break;
	default:
		break;
	}
}

HL_PRIM void HL_NAME(win_set_fullscreen)(dx_window *win, bool fs) {
	if( fs ) {
		MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(MonitorFromWindow(win,MONITOR_DEFAULTTOPRIMARY), &mi);
		SetWindowLong(win,GWL_STYLE,WS_POPUP | WS_VISIBLE);
		SetWindowPos(win,NULL,mi.rcMonitor.left,mi.rcMonitor.top,mi.rcMonitor.right - mi.rcMonitor.left,mi.rcMonitor.bottom - mi.rcMonitor.top,SWP_NOOWNERZORDER|SWP_FRAMECHANGED|SWP_SHOWWINDOW);
	} else {
		dx_events *buf = get_events(win);
		SetWindowLong(win,GWL_STYLE,buf->normal_style);
		SetWindowPos(win,NULL,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_FRAMECHANGED|SWP_SHOWWINDOW);
	}
}

HL_PRIM double HL_NAME(win_get_opacity)(dx_window *win) {
	dx_events *buf = get_events(win);
	return buf->opacity;
}

HL_PRIM bool HL_NAME(win_set_opacity)(dx_window *win, double opacity) {
	if( opacity > 1.0 )
		opacity = 1.0;
	else if( opacity < 0.0 )
		opacity = 0.0;

	dx_events *buf = get_events(win);
	if( buf->opacity != opacity ) {
		buf->opacity = opacity;
		LONG style = GetWindowLong(win, GWL_EXSTYLE);
		bool layered = (style & WS_EX_LAYERED) != 0;
		if( opacity >= 1.0 ) {
			if( layered )
				if( SetWindowLong(win, GWL_EXSTYLE, style & ~WS_EX_LAYERED) == 0 )
					return false;
		} else {
			if( !layered )
				if( SetWindowLong(win, GWL_EXSTYLE, style | WS_EX_LAYERED) == 0 )
					return false;
			if( SetLayeredWindowAttributes(win, 0, (BYTE)((int)(opacity * 255.0)), LWA_ALPHA) == 0 )
				return false;
		}
	}
	return true;
}

HL_PRIM void HL_NAME(win_destroy)(dx_window *win) {
	if (cur_clip_cursor_window == win) {
		cur_clip_cursor_window = NULL;
		ClipCursor(NULL);
	}
	dx_events *buf = get_events(win);
	free(buf);
	SetWindowLongPtr(win,GWLP_USERDATA,0);
	DestroyWindow(win);
}

HL_PRIM bool HL_NAME(win_get_next_event)( dx_window *win, dx_event *e ) {
	dx_events *buf = get_events(win);
	hl_type *save;
	if( !buf ) {
		e->type = Quit;
		return true;
	}
	if( buf->next_event == buf->event_count ) {
		buf->next_event = buf->event_count = 0;
		return false;
	}
	save = e->t;
	memcpy(e,&buf->events[buf->next_event++],sizeof(dx_event));
	e->t = save;
	return true;
}

HL_PRIM void HL_NAME(win_clip_cursor)(dx_window *win, bool enable) {
	capture_mouse = enable;
	updateClipCursor(win);
}

HL_PRIM bool HL_NAME(set_cursor_pos)( int x, int y ) {
	return SetCursorPos(x, y);
}

HL_PRIM bool HL_NAME(win_set_cursor_pos)( dx_window *wnd, int x, int y) {
	if ( wnd ) {
		POINT pt;
		pt.x = x;
		pt.y = y;
		ClientToScreen(wnd, &pt);
		return SetCursorPos(pt.x, pt.y);
	}
	return false;
}

HL_PRIM bool HL_NAME(win_set_relative_mouse_mode)( dx_window *wnd, bool enabled) {
	return setRelativeMode(wnd, enabled);
}

HL_PRIM bool HL_NAME(win_get_relative_mouse_mode)() {
	return relative_mouse;
}

HL_PRIM int HL_NAME(get_screen_width)() {
	return GetSystemMetrics(SM_CXSCREEN);
}

HL_PRIM int HL_NAME(get_screen_height)() {
	return GetSystemMetrics(SM_CYSCREEN);
}

#define TWIN _ABSTRACT(dx_window)
DEFINE_PRIM(TWIN, win_create_ex, _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(TWIN, win_create, _I32 _I32);
DEFINE_PRIM(_VOID, win_set_fullscreen, TWIN _BOOL);
DEFINE_PRIM(_VOID, win_resize, TWIN _I32);
DEFINE_PRIM(_VOID, win_set_title, TWIN _BYTES);
DEFINE_PRIM(_VOID, win_set_size, TWIN _I32 _I32);
DEFINE_PRIM(_VOID, win_set_min_size, TWIN _I32 _I32);
DEFINE_PRIM(_VOID, win_set_max_size, TWIN _I32 _I32);
DEFINE_PRIM(_VOID, win_set_position, TWIN _I32 _I32);
DEFINE_PRIM(_VOID, win_center, TWIN _BOOL);
DEFINE_PRIM(_VOID, win_get_size, TWIN _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, win_get_min_size, TWIN _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, win_get_max_size, TWIN _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, win_get_position, TWIN _REF(_I32) _REF(_I32));
DEFINE_PRIM(_F64, win_get_opacity, TWIN);
DEFINE_PRIM(_BOOL, win_set_opacity, TWIN _F64);
DEFINE_PRIM(_VOID, win_destroy, TWIN);
DEFINE_PRIM(_BOOL, win_get_next_event, TWIN _DYN);
DEFINE_PRIM(_VOID, win_clip_cursor, TWIN _BOOL);
DEFINE_PRIM(_BOOL, set_cursor_pos, _I32 _I32);
DEFINE_PRIM(_BOOL, win_set_cursor_pos, TWIN _I32 _I32);
DEFINE_PRIM(_BOOL, win_set_relative_mouse_mode, TWIN _BOOL);
DEFINE_PRIM(_BOOL, win_get_relative_mouse_mode, _NO_ARG);

DEFINE_PRIM(_I32, get_screen_width, _NO_ARG);
DEFINE_PRIM(_I32, get_screen_height, _NO_ARG);

HL_PRIM dx_cursor HL_NAME(load_cursor)( int res ) {
	return LoadCursor(NULL,MAKEINTRESOURCE(res));
}

HL_PRIM dx_cursor HL_NAME(create_cursor)( int width, int height, vbyte *data, int hotX, int hotY ) {
	int pad = sizeof(void*) << 3;
    HICON hicon;
    HDC hdc = GetDC(NULL);
    BITMAPV4HEADER bmh;
    void *pixels;
    void *maskbits;
    int maskbitslen;
    ICONINFO ii;

    ZeroMemory(&bmh,sizeof(bmh));
    bmh.bV4Size = sizeof(bmh);
    bmh.bV4Width = width;
    bmh.bV4Height = -height;
    bmh.bV4Planes = 1;
    bmh.bV4BitCount = 32;
    bmh.bV4V4Compression = BI_BITFIELDS;
    bmh.bV4AlphaMask = 0xFF000000;
    bmh.bV4RedMask   = 0x00FF0000;
    bmh.bV4GreenMask = 0x0000FF00;
    bmh.bV4BlueMask  = 0x000000FF;

    maskbitslen = ((width + (-width)%pad) >> 3) * height;
    maskbits = malloc(maskbitslen);
	if( maskbits == NULL )
		return NULL;
	memset(maskbits,0xFF,maskbitslen);

	memset(&ii,0,sizeof(ii));
    ii.fIcon = FALSE;
    ii.xHotspot = (DWORD)hotX;
    ii.yHotspot = (DWORD)hotY;
    ii.hbmColor = CreateDIBSection(hdc, (BITMAPINFO*)&bmh, DIB_RGB_COLORS, &pixels, NULL, 0);
    ii.hbmMask = CreateBitmap(width, height, 1, 1, maskbits);
    ReleaseDC(NULL, hdc);
    free(maskbits);

    memcpy(pixels, data, height * width * 4);
    hicon = CreateIconIndirect(&ii);

    DeleteObject(ii.hbmColor);
    DeleteObject(ii.hbmMask);
	return hicon;
}

HL_PRIM void HL_NAME(destroy_cursor)( dx_cursor c ) {
	DestroyIcon(c);
}

HL_PRIM void HL_NAME(set_cursor)( dx_cursor c ) {
	cur_cursor = c;
	if( CURSOR_VISIBLE )
		SetCursor(c);
}

HL_PRIM void HL_NAME(show_cursor)( bool visible ) {
	show_cursor = visible;
	SetCursor(CURSOR_VISIBLE ? cur_cursor : NULL);
}

HL_PRIM bool HL_NAME(is_cursor_visible)() {
	return show_cursor;
}

#define TCURSOR _ABSTRACT(dx_cursor)
DEFINE_PRIM(TCURSOR, load_cursor, _I32);
DEFINE_PRIM(TCURSOR, create_cursor, _I32 _I32 _BYTES _I32 _I32);
DEFINE_PRIM(_VOID, destroy_cursor, TCURSOR);
DEFINE_PRIM(_VOID, set_cursor, TCURSOR);
DEFINE_PRIM(_VOID, show_cursor, _BOOL);
DEFINE_PRIM(_BOOL, is_cursor_visible, _NO_ARG);

HL_PRIM vbyte *HL_NAME(detect_keyboard_layout)() {
	char q = MapVirtualKey(0x10, MAPVK_VSC_TO_VK);
	char w = MapVirtualKey(0x11, MAPVK_VSC_TO_VK);
	char y = MapVirtualKey(0x15, MAPVK_VSC_TO_VK);

	if (q == 'Q' && w == 'W' && y == 'Y') return "qwerty";
	if (q == 'A' && w == 'Z' && y == 'Y') return "azerty";
	if (q == 'Q' && w == 'W' && y == 'Z') return "qwertz";
	if (q == 'Q' && w == 'Z' && y == 'Y') return "qzerty";
	return "unknown";
}
DEFINE_PRIM(_BYTES, detect_keyboard_layout, _NO_ARG);
