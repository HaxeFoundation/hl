#define HL_NAME(n) uv_##n
#ifdef _WIN32
#	include <uv.h>
#	include <hl.h>
#else
#	include <hl.h>
#	include <uv.h>
# include <stdlib.h>
#endif

#if (UV_VERSION_HEX < (1 << 16 | 31 << 8))
#	error "Compiling HashLink requires libuv version 1.31.0+"
#endif

// ------------- TYPES ----------------------------------------------

/**
	Type aliases, used when defining FFI signatures.
**/

// This is not quite accurate, since the type is a pointer-pointer.
#define _UV(name) "X" #name "_"

// Handle types

#define _LOOP _UV(uv_loop_t)
#define _HANDLE _UV(uv_handle_t)
#define _DIR _UV(uv_dir_t)
#define _STREAM _UV(uv_stream_t)
#define _TCP _UV(uv_tcp_t)
#define _UDP _UV(uv_udp_t)
#define _PIPE _UV(uv_pipe_t)
#define _TTY _UV(uv_tty_t)
#define _POLL _UV(uv_poll_t)
#define _TIMER _UV(uv_timer_t)
#define _PREPARE _UV(uv_prepare_t)
#define _CHECK _UV(uv_check_t)
#define _IDLE _UV(uv_idle_t)
#define _ASYNC _UV(uv_async_t)
#define _PROCESS _UV(uv_process_t)
#define _FS_EVENT _UV(uv_fs_event_t)
#define _FS_POLL _UV(uv_fs_poll_t)
#define _SIGNAL _UV(uv_signal_t)
#define _MUTEX _UV(uv_mutex_t)

// Request types

#define _REQ _UV(uv_req_t)
#define _GETADDRINFO _UV(uv_getaddrinfo_t)
#define _GETNAMEINFO _UV(uv_getnameinfo_t)
#define _SHUTDOWN _UV(uv_shutdown_t)
#define _WRITE _UV(uv_write_t)
#define _CONNECT _UV(uv_connect_t)
#define _UDP_SEND _UV(uv_udp_send_t)
#define _FS _UV(uv_fs_t)
#define _WORK _UV(uv_work_t)

// Other types

#define _CPU_INFO _UV(uv_cpu_info_t)
#define _INTERFACE_ADDRESS _UV(uv_interface_address_t)
#define _DIRENT _UV(uv_dirent_t)
// #define _PASSWD _UV(uv_passwd_t)
#define _UTSNAME _UV(uv_utsname_t)
#define _FILE _UV(uv_file)
// #define _STAT _UV(uv_stat_t)
#define _BUF _UV(uv_buf_t)

// Haxe types

#define _ERROR _DYN
#define _STAT _OBJ(_I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32)

// Callback types

#define _CB_NOERR _FUN(_VOID, _NO_ARG)
#define _CB _FUN(_VOID, _ERROR)
#define _CB_STR _FUN(_VOID, _ERROR _BYTES)
#define _CB_BYTES _FUN(_VOID, _ERROR _BYTES _I32)
#define _CB_INT _FUN(_VOID, _ERROR _I32)
#define _CB_FILE _FUN(_VOID, _ERROR _FILE)
#define _CB_STAT _FUN(_VOID, _ERROR _STAT)
#define _CB_SCANDIR _FUN(_VOID, _ERROR _ARR)
#define _CB_FS_EVENT _FUN(_VOID, _ERROR _BYTES _I32)
#define _CB_GAI _FUN(_VOID, _ERROR _ARR)
#define _CB_GNI _FUN(_VOID, _ERROR _BYTES _BYTES)
#define _CB_UDP_RECV _FUN(_VOID, _ERROR _BYTES _I32 _DYN _I32)

// ------------- UTILITY MACROS -------------------------------------

/**
	The `data` field of handles and requests is used to store Haxe callbacks.
	These callbacks are called from the various `handle_...` functions, after
	pre-processing libuv results as necessary. At runtime, a callback is simply
	a `value`. To ensure it is not garbage-collected, we add the data pointer of
	the handle or request to HL's global GC roots, then remove it after the
	callback is called.

	Handle-specific macros are defined further, in the HANDLE DATA section.
**/

// access the data of a request
#define UV_REQ_DATA(r) (((uv_req_t *)(r))->data)
#define UV_REQ_DATA_A(r) ((void *)(&UV_REQ_DATA(r)))

// allocate a request, add its callback to GC roots
#define UV_ALLOC_REQ(name, type, cb) \
	UV_ALLOC_CHECK(name, type); \
	UV_REQ_DATA(UV_UNWRAP(name, type)) = (void *)cb; \
	hl_add_root(UV_REQ_DATA_A(UV_UNWRAP(name, type)));

// free a request, remove its callback from GC roots
#define UV_FREE_REQ(name) \
	hl_remove_root(UV_REQ_DATA_A(name)); \
	free(name);

// malloc a single value of the given type
#define UV_ALLOC(t) ((t *)malloc(sizeof(t)))

// unwrap an abstract block (see UV_ALLOC_CHECK notes below)
#define UV_UNWRAP(v, t) ((t *)*v)

#define Connect_val(v) UV_UNWRAP(v, uv_connect_t)
#define Fs_val(v) UV_UNWRAP(v, uv_fs_t)
#define FsEvent_val(v) UV_UNWRAP(v, uv_fs_event_t)
#define GetAddrInfo_val(v) UV_UNWRAP(v, uv_getaddrinfo_t)
#define Handle_val(v) UV_UNWRAP(v, uv_handle_t)
#define Loop_val(v) UV_UNWRAP(v, uv_loop_t)
#define Mutex_val(v) UV_UNWRAP(v, uv_mutex_t)
#define Pipe_val(v) UV_UNWRAP(v, uv_pipe_t)
#define Process_val(v) UV_UNWRAP(v, uv_process_t)
#define Shutdown_val(v) UV_UNWRAP(v, uv_shutdown_t)
#define Stream_val(v) UV_UNWRAP(v, uv_stream_t)
#define Tcp_val(v) UV_UNWRAP(v, uv_tcp_t)
#define Timer_val(v) UV_UNWRAP(v, uv_timer_t)
#define Udp_val(v) UV_UNWRAP(v, uv_udp_t)
#define UdpSend_val(v) UV_UNWRAP(v, uv_udp_send_t)
#define Work_val(v) UV_UNWRAP(v, uv_work_t)
#define Write_val(v) UV_UNWRAP(v, uv_write_t)

// ------------- HAXE CONSTRUCTORS ----------------------------------

/**
	To make it easier to construct values expected by the Haxe standard library,
	the library provides constructors for various types. These are called from
	the methods in this file and either returned or thrown back into Haxe code.
**/

static vdynamic * (*construct_error)(int);
static vdynamic * (*construct_fs_stat)(int, int, int, int, int, int, int, int, int, int, int, int);
static vdynamic * (*construct_fs_dirent)(const char *name, int type);
static vdynamic * (*construct_addrinfo_ipv4)(int ip);
static vdynamic * (*construct_addrinfo_ipv6)(vbyte *ip);
static vdynamic * (*construct_addrport)(vdynamic *addr, int port);
static vdynamic * (*construct_pipe_accept_socket)(uv_tcp_t **socket);
static vdynamic * (*construct_pipe_accept_pipe)(uv_pipe_t **pipe);

HL_PRIM void HL_NAME(glue_register)(
	vclosure *c_error,
	vclosure *c_fs_stat,
	vclosure *c_fs_dirent,
	vclosure *c_addrinfo_ipv4,
	vclosure *c_addrinfo_ipv6,
	vclosure *c_addrport,
	vclosure *c_pipe_accept_socket,
	vclosure *c_pipe_accept_pipe
) {
	construct_error = (vdynamic * (*)(int))c_error->fun;
	construct_fs_stat = (vdynamic * (*)(int, int, int, int, int, int, int, int, int, int, int, int))c_fs_stat->fun;
	construct_fs_dirent = (vdynamic * (*)(const char *, int))c_fs_dirent->fun;
	construct_addrinfo_ipv4 = (vdynamic * (*)(int))c_addrinfo_ipv4->fun;
	construct_addrinfo_ipv6 = (vdynamic * (*)(vbyte *))c_addrinfo_ipv6->fun;
	construct_addrport = (vdynamic * (*)(vdynamic *, int))c_addrport->fun;
	construct_pipe_accept_socket = (vdynamic * (*)(uv_tcp_t **))c_pipe_accept_socket->fun;
	construct_pipe_accept_pipe = (vdynamic * (*)(uv_pipe_t **))c_pipe_accept_pipe->fun;
}
DEFINE_PRIM(
	_VOID,
	glue_register,
		_FUN(_DYN, _I32)
		_FUN(_DYN, _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32)
		_FUN(_DYN, _BYTES _I32)
		_FUN(_DYN, _I32)
		_FUN(_DYN, _BYTES)
		_FUN(_DYN, _DYN _I32)
		_FUN(_DYN, _TCP)
		_FUN(_DYN, _PIPE)
);

HL_PRIM varray *HL_NAME(glue_file_open_flags)(void) {
	int values[] = {UV_FS_O_APPEND, UV_FS_O_CREAT, UV_FS_O_DIRECT, UV_FS_O_DIRECTORY, UV_FS_O_DSYNC, UV_FS_O_EXCL, UV_FS_O_NOATIME, UV_FS_O_NOCTTY, UV_FS_O_NOFOLLOW, UV_FS_O_NONBLOCK, UV_FS_O_RDONLY, UV_FS_O_RDWR, UV_FS_O_SYNC, UV_FS_O_TRUNC, UV_FS_O_WRONLY};
	varray *ret = hl_alloc_array(&hlt_i32, sizeof(values) / sizeof(values[0]));
	for (int i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
		hl_aptr(ret, int)[i] = values[i];
	}
	return ret;
}
DEFINE_PRIM(_ARR, glue_file_open_flags, _NO_ARG);

HL_PRIM varray *HL_NAME(glue_error_codes)(void) {
	int values[] = {UV_E2BIG, UV_EACCES, UV_EADDRINUSE, UV_EADDRNOTAVAIL, UV_EAFNOSUPPORT, UV_EAGAIN, UV_EAI_ADDRFAMILY, UV_EAI_AGAIN, UV_EAI_BADFLAGS, UV_EAI_BADHINTS, UV_EAI_CANCELED, UV_EAI_FAIL, UV_EAI_FAMILY, UV_EAI_MEMORY, UV_EAI_NODATA, UV_EAI_NONAME, UV_EAI_OVERFLOW, UV_EAI_PROTOCOL, UV_EAI_SERVICE, UV_EAI_SOCKTYPE, UV_EALREADY, UV_EBADF, UV_EBUSY, UV_ECANCELED, UV_ECHARSET, UV_ECONNABORTED, UV_ECONNREFUSED, UV_ECONNRESET, UV_EDESTADDRREQ, UV_EEXIST, UV_EFAULT, UV_EFBIG, UV_EHOSTUNREACH, UV_EINTR, UV_EINVAL, UV_EIO, UV_EISCONN, UV_EISDIR, UV_ELOOP, UV_EMFILE, UV_EMSGSIZE, UV_ENAMETOOLONG, UV_ENETDOWN, UV_ENETUNREACH, UV_ENFILE, UV_ENOBUFS, UV_ENODEV, UV_ENOENT, UV_ENOMEM, UV_ENONET, UV_ENOPROTOOPT, UV_ENOSPC, UV_ENOSYS, UV_ENOTCONN, UV_ENOTDIR, UV_ENOTEMPTY, UV_ENOTSOCK, UV_ENOTSUP, UV_EPERM, UV_EPIPE, UV_EPROTO, UV_EPROTONOSUPPORT, UV_EPROTOTYPE, UV_ERANGE, UV_EROFS, UV_ESHUTDOWN, UV_ESPIPE, UV_ESRCH, UV_ETIMEDOUT, UV_ETXTBSY, UV_EXDEV, UV_UNKNOWN, UV_EOF, UV_ENXIO, UV_EMLINK, UV_EHOSTDOWN, 0};
	varray *ret = hl_alloc_array(&hlt_i32, sizeof(values) / sizeof(values[0]));
	for (int i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
		hl_aptr(ret, int)[i] = values[i];
	}
	return ret;
}
DEFINE_PRIM(_ARR, glue_error_codes, _NO_ARG);

// ------------- ERROR HANDLING -------------------------------------

/**
	UV_ERROR throws an error with the given error code.

	UV_ALLOC_CHECK tries to allocate a variable of the given type with the given
	name and throws an error if this fails. UV_ALLOC_CHECK_C is the same, but
	allows specifying custom clean-up code before the error result is returned.
	Allocation returns a value that is a pointer-pointer to the malloc'ed native
	value.

	UV_ERROR_CHECK checks for a libuv error in the given int expression (indicated
	by a negative value), and in case of an error, throws an error. Once again,
	UV_ERROR_CHECK_C is the same, but allows specifying custom clean-up code.
**/

#define UV_ERROR(errno) hl_throw(construct_error(errno))

#define UV_ALLOC_CHECK_C(var, type, cleanup) \
	type *_native = UV_ALLOC(type); \
	if (_native == NULL) { \
		cleanup; \
		UV_ERROR(0); \
	} \
	type **var = hl_gc_alloc_noptr(sizeof(type *)); \
	*var = _native;

#define UV_ALLOC_CHECK(var, type) UV_ALLOC_CHECK_C(var, type, )

#define UV_ERROR_CHECK_C(expr, cleanup) do { \
		int __tmp_result = expr; \
		if (__tmp_result < 0) { \
			cleanup; \
			UV_ERROR(__tmp_result); \
		} \
	} while (0)

#define UV_ERROR_CHECK(expr) UV_ERROR_CHECK_C(expr, )

// ------------- LOOP -----------------------------------------------

HL_PRIM uv_loop_t ** HL_NAME(w_loop_init)(void) {
	UV_ALLOC_CHECK(loop, uv_loop_t);
	UV_ERROR_CHECK_C(uv_loop_init(Loop_val(loop)), free(Loop_val(loop)));
	return loop;
}
DEFINE_PRIM(_LOOP, w_loop_init, _NO_ARG);

HL_PRIM void HL_NAME(w_loop_close)(uv_loop_t **loop) {
	UV_ERROR_CHECK(uv_loop_close(Loop_val(loop)));
	free(Loop_val(loop));
}
DEFINE_PRIM(_VOID, w_loop_close, _LOOP);

HL_PRIM bool HL_NAME(w_run)(uv_loop_t **loop, uv_run_mode mode) {
	return uv_run(Loop_val(loop), mode) != 0;
}
DEFINE_PRIM(_BOOL, w_run, _LOOP _I32);

HL_PRIM void HL_NAME(w_stop)(uv_loop_t **loop) {
	uv_stop(Loop_val(loop));
}
DEFINE_PRIM(_VOID, w_stop, _LOOP);

HL_PRIM bool HL_NAME(w_loop_alive)(uv_loop_t **loop) {
	return uv_loop_alive(Loop_val(loop)) != 0;
}
DEFINE_PRIM(_BOOL, w_loop_alive, _LOOP);

// ------------- FILESYSTEM -----------------------------------------

/**
	FS handlers all have the same structure.

	The async version (no suffix) calls the callback with either the result in
	the second argument, or an error in the first argument.

	The sync version (`_sync` suffix) returns the result directly.
**/

static void handle_fs_cb(uv_fs_t *req) {
	vclosure *cb = UV_REQ_DATA(req);
	if (req->result < 0)
		hl_call1(void, cb, vdynamic *, construct_error(req->result));
	else
		hl_call1(void, cb, vdynamic *, NULL);
	uv_fs_req_cleanup(req);
	UV_FREE_REQ(req);
}
static void handle_fs_cb_sync(uv_fs_t **req_w) {
	uv_fs_t *req = Fs_val(req_w);
	/* TODO: should we call uv_fs_req_cleanup on error here? */
	UV_ERROR_CHECK_C(req->result, free(req));
	uv_fs_req_cleanup(req);
	free(req);
}

#define UV_FS_HANDLER(name, type2, setup) \
	static void name(uv_fs_t *req) { \
		vclosure *cb = UV_REQ_DATA(req); \
		if (req->result < 0) \
			hl_call2(void, cb, vdynamic *, construct_error(req->result), type2, (type2)0); \
		else { \
			type2 value2; \
			setup; \
			hl_call2(void, cb, vdynamic *, NULL, type2, value2); \
		} \
		uv_fs_req_cleanup(req); \
		UV_FREE_REQ(req); \
	} \
	static type2 name ## _sync(uv_fs_t **req_w) { \
		uv_fs_t *req = Fs_val(req_w); \
		/* TODO: should we call uv_fs_req_cleanup on error here? */ \
		UV_ERROR_CHECK_C(req->result, free(req)); \
		type2 value2; \
		setup; \
		uv_fs_req_cleanup(req); \
		free(req); \
		return value2; \
	}

// TODO: return unchanged bytes, do String.fromUTF8 on Haxe side
UV_FS_HANDLER(handle_fs_cb_bytes, vbyte *, value2 = (vbyte *)hl_to_utf16((const char *)req->ptr));
UV_FS_HANDLER(handle_fs_cb_path, vbyte *, value2 = (vbyte *)hl_to_utf16((const char *)req->path));
UV_FS_HANDLER(handle_fs_cb_int, int, value2 = req->result);
UV_FS_HANDLER(handle_fs_cb_file, uv_file, value2 = req->result);
UV_FS_HANDLER(handle_fs_cb_stat, vdynamic *, value2 = construct_fs_stat(
		req->statbuf.st_dev,
		req->statbuf.st_mode,
		req->statbuf.st_nlink,
		req->statbuf.st_uid,
		req->statbuf.st_gid,
		req->statbuf.st_rdev,
		req->statbuf.st_ino,
		req->statbuf.st_size,
		req->statbuf.st_blksize,
		req->statbuf.st_blocks,
		req->statbuf.st_flags,
		req->statbuf.st_gen
	));
UV_FS_HANDLER(handle_fs_cb_scandir, varray *, {
		uv_dirent_t ent;
		vlist *last = NULL;
		int count = 0;
		while (uv_fs_scandir_next(req, &ent) != UV_EOF) {
			count++;
			vlist *node = (vlist *)malloc(sizeof(vlist));
			node->v = construct_fs_dirent(ent.name, ent.type);
			node->next = last;
			last = node;
		}
		value2 = hl_alloc_array(&hlt_dyn, count);
		for (int i = 0; i < count; i++) {
			hl_aptr(value2, vdynamic *)[count - i - 1] = last->v;
			vlist *next = last->next;
			free(last);
			last = next;
		}
	});

/**
	Most FS functions from libuv can be wrapped with FS_WRAP (or one of the
	FS_WRAP# variants defined below) - create a request, register a callback for
	it, register the callback with the GC, perform request. Then, either in the
	handler function (synchronous or asynchronous), the result is checked and
	given to the Haxe callback if successful, with the appropriate value
	conversions done, as defined in the various UV_FS_HANDLERs above.
**/

#define FS_WRAP(name, ret, sign, precall, call, ffiret, ffi, ffihandler, handler, doret) \
	HL_PRIM void HL_NAME(w_fs_ ## name)(uv_loop_t **loop, sign, vclosure *cb) { \
		UV_ALLOC_REQ(req, uv_fs_t, cb); \
		precall \
		UV_ERROR_CHECK_C(uv_fs_ ## name(Loop_val(loop), Fs_val(req), call, handler), UV_FREE_REQ(Fs_val(req))); \
	} \
	DEFINE_PRIM(_VOID, w_fs_ ## name, _LOOP ffi ffihandler); \
	HL_PRIM ret HL_NAME(w_fs_ ## name ## _sync)(uv_loop_t **loop, sign) { \
		UV_ALLOC_CHECK(req, uv_fs_t); \
		precall \
		UV_ERROR_CHECK_C(uv_fs_ ## name(Loop_val(loop), Fs_val(req), call, NULL), free(Fs_val(req))); \
		doret handler ## _sync(req); \
	} \
	DEFINE_PRIM(ffiret, w_fs_ ## name ## _sync, _LOOP ffi);

#define COMMA ,
#define FS_WRAP1(name, ret, arg1, ffiret, ffi, ffihandler, handler, doret) \
	FS_WRAP(name, ret, arg1 _arg1, , _arg1, ffiret, ffi, ffihandler, handler, doret);
#define FS_WRAP2(name, ret, arg1, arg2, ffiret, ffi, ffihandler, handler, doret) \
	FS_WRAP(name, ret, arg1 _arg1 COMMA arg2 _arg2, , _arg1 COMMA _arg2, ffiret, ffi, ffihandler, handler, doret);
#define FS_WRAP3(name, ret, arg1, arg2, arg3, ffiret, ffi, ffihandler, handler, doret) \
	FS_WRAP(name, ret, arg1 _arg1 COMMA arg2 _arg2 COMMA arg3 _arg3, , _arg1 COMMA _arg2 COMMA _arg3, ffiret, ffi, ffihandler, handler, doret);
#define FS_WRAP4(name, ret, arg1, arg2, arg3, arg4, ffiret, ffi, ffihandler, handler, doret) \
	FS_WRAP(name, ret, arg1 _arg1 COMMA arg2 _arg2 COMMA arg3 _arg3 COMMA arg4 _arg4, , _arg1 COMMA _arg2 COMMA _arg3 COMMA _arg4, ffiret, ffi, ffihandler, handler, doret);

FS_WRAP1(close, void, uv_file, _VOID, _FILE, _CB, handle_fs_cb, );
FS_WRAP3(open, uv_file, const char*, int, int, _FILE, _BYTES _I32 _I32, _CB_FILE, handle_fs_cb_file, return);
FS_WRAP1(unlink, void, const char*, _VOID, _BYTES, _CB, handle_fs_cb, );
FS_WRAP2(mkdir, void, const char*, int, _VOID, _BYTES _I32, _CB, handle_fs_cb, );
FS_WRAP1(mkdtemp, vbyte *, const char*, _BYTES, _BYTES, _CB_STR, handle_fs_cb_path, return);
FS_WRAP1(rmdir, void, const char*, _VOID, _BYTES, _CB, handle_fs_cb, );
FS_WRAP2(scandir, varray *, const char *, int, _ARR, _BYTES _I32, _CB_SCANDIR, handle_fs_cb_scandir, return);
FS_WRAP1(stat, vdynamic *, const char*, _STAT, _BYTES, _CB_STAT, handle_fs_cb_stat, return);
FS_WRAP1(fstat, vdynamic *, uv_file, _STAT, _FILE, _CB_STAT, handle_fs_cb_stat, return);
FS_WRAP1(lstat, vdynamic *, const char*, _STAT, _BYTES, _CB_STAT, handle_fs_cb_stat, return);
FS_WRAP2(rename, void, const char*, const char*, _VOID, _BYTES _BYTES, _CB, handle_fs_cb, );
FS_WRAP1(fsync, void, uv_file, _VOID, _FILE, _CB, handle_fs_cb, );
FS_WRAP1(fdatasync, void, uv_file, _VOID, _FILE, _CB, handle_fs_cb, );
FS_WRAP2(ftruncate, void, uv_file, int64_t, _VOID, _FILE _I32, _CB, handle_fs_cb, );
FS_WRAP3(copyfile, void, const char *, const char *, int, _VOID, _BYTES _BYTES _I32, _CB, handle_fs_cb, );
FS_WRAP4(sendfile, void, uv_file, uv_file, int64_t, size_t, _VOID, _FILE _FILE _I32 _I32, _CB, handle_fs_cb, );
FS_WRAP2(access, void, const char*, int, _VOID, _BYTES _I32, _CB, handle_fs_cb, );
FS_WRAP2(chmod, void, const char*, int, _VOID, _BYTES _I32, _CB, handle_fs_cb, );
FS_WRAP2(fchmod, void, uv_file, int, _VOID, _FILE _I32, _CB, handle_fs_cb, );
FS_WRAP3(utime, void, const char*, double, double, _VOID, _BYTES _F64 _F64, _CB, handle_fs_cb, );
FS_WRAP3(futime, void, uv_file, double, double, _VOID, _FILE _F64 _F64, _CB, handle_fs_cb, );
FS_WRAP2(link, void, const char*, const char*, _VOID, _BYTES _BYTES, _CB, handle_fs_cb, );
FS_WRAP3(symlink, void, const char*, const char*, int, _VOID, _BYTES _BYTES _I32, _CB, handle_fs_cb, );
FS_WRAP1(readlink, vbyte *, const char*, _BYTES, _BYTES, _CB_STR, handle_fs_cb_bytes, return);
FS_WRAP1(realpath, vbyte *, const char*, _BYTES, _BYTES, _CB_STR, handle_fs_cb_bytes, return);
FS_WRAP3(chown, void, const char*, uv_uid_t, uv_gid_t, _VOID, _BYTES _I32 _I32, _CB, handle_fs_cb, );
FS_WRAP3(fchown, void, uv_file, uv_uid_t, uv_gid_t, _VOID, _FILE _I32 _I32, _CB, handle_fs_cb, );
FS_WRAP3(lchown, void, const char*, uv_uid_t, uv_gid_t, _VOID, _BYTES _I32 _I32, _CB, handle_fs_cb, );

/**
	`fs_read` and `fs_write` require a tiny bit of setup just before the libuv
	request is actually started; namely, a buffer structure needs to be set up,
	which is simply a wrapper of a pointer to the HL bytes value.

	libuv actually supports multiple buffers in both calls, but this is not
	mirrored in the Haxe API, so only a single-buffer call is used.
**/

FS_WRAP(read,
	int,
	uv_file file COMMA vbyte *data COMMA int offset COMMA int length COMMA int position,
	uv_buf_t buf = uv_buf_init((char *)(&data[offset]), length);,
	file COMMA &buf COMMA 1 COMMA position,
	_I32,
	_FILE _BYTES _I32 _I32 _I32,
	_CB_INT,
	handle_fs_cb_int,
	return);

FS_WRAP(write,
	int,
	uv_file file COMMA vbyte *data COMMA int offset COMMA int length COMMA int position,
	uv_buf_t buf = uv_buf_init((char *)(&data[offset]), length);,
	file COMMA &buf COMMA 1 COMMA position,
	_I32,
	_FILE _BYTES _I32 _I32 _I32,
	_CB_INT,
	handle_fs_cb_int,
	return);

// ------------- HANDLE DATA ----------------------------------------

/**
	There is a single `void *data` field on requests and handles. For requests,
	we use this to directly store the pointer to the callback function. For
	handles, however, it is sometimes necessary to register multiple different
	callbacks, hence a separate allocated struct is needed to hold them all.
	All of the fields of the struct are registered with the garbage collector
	immediately upon creation, although initially some of the callback fields are
	set to NULL pointers.
**/

#define UV_HANDLE_DATA(h) (((uv_handle_t *)(h))->data)
#define UV_HANDLE_DATA_SUB(h, t) (((uv_w_handle_t *)UV_HANDLE_DATA(h))->u.t)

typedef struct {
	vclosure *cb_close;
	union {
		struct {
			vclosure *cb1;
			vclosure *cb2;
		} all;
		struct {
			vclosure *cb_fs_event;
			vclosure *unused1;
		} fs_event;
		struct {
			vclosure *cb_read;
			vclosure *cb_connection;
		} stream;
		struct {
			vclosure *cb_read;
			vclosure *cb_connection;
		} tcp;
		struct {
			vclosure *cb_read;
			vclosure *unused1;
		} udp;
		struct {
			vclosure *cb_timer;
			vclosure *unused1;
		} timer;
		struct {
			vclosure *cb_exit;
			vclosure *unused1;
		} process;
		struct {
			vclosure *unused1;
			vclosure *unused2;
		} pipe;
	} u;
} uv_w_handle_t;

static uv_w_handle_t *alloc_data(void) {
	uv_w_handle_t *data = calloc(1, sizeof(uv_w_handle_t));
	if (data != NULL) {
		data->cb_close = NULL;
		hl_add_root(&(data->cb_close));
		data->u.all.cb1 = NULL;
		hl_add_root(&(data->u.all.cb1));
		data->u.all.cb2 = NULL;
		hl_add_root(&(data->u.all.cb2));
	}
	return data;
}

static void unalloc_data(uv_w_handle_t *data) {
	hl_remove_root(&(data->cb_close));
	hl_remove_root(&(data->u.all.cb1));
	hl_remove_root(&(data->u.all.cb2));
	free(data);
}

static void handle_close_cb(uv_handle_t *handle) {
	vclosure *cb = ((uv_w_handle_t *)UV_HANDLE_DATA(handle))->cb_close;
	hl_call1(void, cb, vdynamic *, NULL);
	unalloc_data(UV_HANDLE_DATA(handle));
	free(handle);
}

HL_PRIM void HL_NAME(w_close)(uv_handle_t **handle, vclosure *cb) {
	((uv_w_handle_t *)UV_HANDLE_DATA(Handle_val(handle)))->cb_close = cb;
	uv_close(Handle_val(handle), handle_close_cb);
}
DEFINE_PRIM(_VOID, w_close, _HANDLE _CB);

HL_PRIM void HL_NAME(w_ref)(uv_handle_t **handle) {
	uv_ref(Handle_val(handle));
}
DEFINE_PRIM(_VOID, w_ref, _HANDLE);

HL_PRIM void HL_NAME(w_unref)(uv_handle_t **handle) {
	uv_unref(Handle_val(handle));
}
DEFINE_PRIM(_VOID, w_unref, _HANDLE);

// ------------- FILESYSTEM EVENTS ----------------------------------

static void handle_fs_event_cb(uv_fs_event_t *handle, const char *filename, int events, int status) {
	vclosure *cb = UV_HANDLE_DATA_SUB(handle, fs_event).cb_fs_event;
	if (status < 0)
		hl_call3(void, cb, vdynamic *, construct_error(status), vbyte *, NULL, int, 0);
	else
		hl_call3(void, cb, vdynamic *, NULL, vbyte *, (vbyte *)hl_to_utf16((const char *)filename), int, events);
}

HL_PRIM uv_fs_event_t **HL_NAME(w_fs_event_start)(uv_loop_t **loop, const char *path, bool recursive, vclosure *cb) {
	UV_ALLOC_CHECK(handle, uv_fs_event_t);
	UV_ERROR_CHECK_C(uv_fs_event_init(Loop_val(loop), FsEvent_val(handle)), free(FsEvent_val(handle)));
	if ((UV_HANDLE_DATA(FsEvent_val(handle)) = alloc_data()) == NULL)
		UV_ERROR(0);
	UV_HANDLE_DATA_SUB(FsEvent_val(handle), fs_event).cb_fs_event = cb;
	UV_ERROR_CHECK_C(
		uv_fs_event_start(FsEvent_val(handle), handle_fs_event_cb, path, recursive ? UV_FS_EVENT_RECURSIVE : 0),
		{ unalloc_data(UV_HANDLE_DATA(FsEvent_val(handle))); free(FsEvent_val(handle)); }
		);
	return handle;
}
DEFINE_PRIM(_FS_EVENT, w_fs_event_start, _LOOP _BYTES _BOOL _CB_FS_EVENT);

HL_PRIM void HL_NAME(w_fs_event_stop)(uv_fs_event_t **handle, vclosure *cb) {
	UV_ERROR_CHECK_C(
		uv_fs_event_stop(FsEvent_val(handle)),
		{ unalloc_data(UV_HANDLE_DATA(FsEvent_val(handle))); free(FsEvent_val(handle)); }
		);
	((uv_w_handle_t *)UV_HANDLE_DATA(FsEvent_val(handle)))->cb_close = cb;
	uv_close(Handle_val(handle), handle_close_cb);
}
DEFINE_PRIM(_VOID, w_fs_event_stop, _FS_EVENT _CB);

// ------------- STREAM ---------------------------------------------

static void handle_stream_cb(uv_req_t *req, int status) {
	vclosure *cb = UV_REQ_DATA(req);
	if (status < 0)
		hl_call1(void, cb, vdynamic *, construct_error(status));
	else
		hl_call1(void, cb, vdynamic *, NULL);
	UV_FREE_REQ(req);
}

static void handle_stream_cb_connection(uv_stream_t *stream, int status) {
	vclosure *cb = UV_HANDLE_DATA_SUB(stream, stream).cb_connection;
	if (status < 0)
		hl_call1(void, cb, vdynamic *, construct_error(status));
	else
		hl_call1(void, cb, vdynamic *, NULL);
}

static void handle_stream_cb_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
	buf->base = hl_gc_alloc_noptr(suggested_size);
	buf->len = suggested_size;
}

static void handle_stream_cb_read(uv_stream_t *stream, long int nread, const uv_buf_t *buf) {
	vclosure *cb = UV_HANDLE_DATA_SUB(stream, stream).cb_read;
	if (nread < 0)
		hl_call3(void, cb, vdynamic *, construct_error(nread), vbyte *, NULL, int, 0);
	else
		hl_call3(void, cb, vdynamic *, NULL, vbyte *, (vbyte *)buf->base, int, nread);
}

HL_PRIM void HL_NAME(w_shutdown)(uv_stream_t **stream, vclosure *cb) {
	UV_ALLOC_REQ(req, uv_shutdown_t, cb);
	UV_ERROR_CHECK_C(uv_shutdown(Shutdown_val(req), Stream_val(stream), (void (*)(uv_shutdown_t *, int))handle_stream_cb), UV_FREE_REQ(Shutdown_val(req)));
}
DEFINE_PRIM(_VOID, w_shutdown, _STREAM _CB);

HL_PRIM void HL_NAME(w_listen)(uv_stream_t **stream, int backlog, vclosure *cb) {
	UV_HANDLE_DATA_SUB(Stream_val(stream), stream).cb_connection = cb;
	UV_ERROR_CHECK(uv_listen(Stream_val(stream), backlog, handle_stream_cb_connection));
}
DEFINE_PRIM(_VOID, w_listen, _STREAM _I32 _CB);

HL_PRIM void HL_NAME(w_write)(uv_stream_t **stream, const vbyte *data, int length, vclosure *cb) {
	UV_ALLOC_REQ(req, uv_write_t, cb);
	uv_buf_t buf = uv_buf_init((char *)data, length);
	UV_ERROR_CHECK_C(uv_write(Write_val(req), Stream_val(stream), &buf, 1, (void (*)(uv_write_t *, int))handle_stream_cb), UV_FREE_REQ(Write_val(req)));
}
DEFINE_PRIM(_VOID, w_write, _STREAM _BYTES _I32 _CB);

HL_PRIM void HL_NAME(w_read_start)(uv_stream_t **stream, vclosure *cb) {
	UV_HANDLE_DATA_SUB(Stream_val(stream), stream).cb_read = cb;
	UV_ERROR_CHECK(uv_read_start(Stream_val(stream), handle_stream_cb_alloc, handle_stream_cb_read));
}
DEFINE_PRIM(_VOID, w_read_start, _STREAM _CB_BYTES);

HL_PRIM void HL_NAME(w_read_stop)(uv_stream_t **stream) {
	UV_ERROR_CHECK(uv_read_stop(Stream_val(stream)));
}
DEFINE_PRIM(_VOID, w_read_stop, _STREAM);

// ------------- NETWORK MACROS -------------------------------------

#define UV_SOCKADDR_IPV4(var, host, port) \
	struct sockaddr_in var; \
	var.sin_family = AF_INET; \
	var.sin_port = htons((unsigned short)port); \
	var.sin_addr.s_addr = htonl(host);
#define UV_SOCKADDR_IPV6(var, host, port) \
	struct sockaddr_in6 var; \
	memset(&var, 0, sizeof(var)); \
	var.sin6_family = AF_INET6; \
	var.sin6_port = htons((unsigned short)port); \
	memcpy(var.sin6_addr.s6_addr, host, 16);

// ------------- TCP ------------------------------------------------

HL_PRIM uv_tcp_t **HL_NAME(w_tcp_init)(uv_loop_t **loop) {
	UV_ALLOC_CHECK(handle, uv_tcp_t);
	UV_ERROR_CHECK_C(uv_tcp_init(Loop_val(loop), Tcp_val(handle)), free(Tcp_val(handle)));
	if ((UV_HANDLE_DATA(Tcp_val(handle)) = alloc_data()) == NULL)
		UV_ERROR(0);
	return handle;
}
DEFINE_PRIM(_TCP, w_tcp_init, _LOOP);

HL_PRIM void HL_NAME(w_tcp_nodelay)(uv_tcp_t **handle, bool enable) {
	UV_ERROR_CHECK(uv_tcp_nodelay(Tcp_val(handle), enable ? 1 : 0));
}
DEFINE_PRIM(_VOID, w_tcp_nodelay, _TCP _BOOL);

HL_PRIM void HL_NAME(w_tcp_keepalive)(uv_tcp_t **handle, bool enable, unsigned int delay) {
	UV_ERROR_CHECK(uv_tcp_keepalive(Tcp_val(handle), enable ? 1 : 0, delay));
}
DEFINE_PRIM(_VOID, w_tcp_keepalive, _TCP _BOOL _I32);

HL_PRIM uv_tcp_t **HL_NAME(w_tcp_accept)(uv_loop_t **loop, uv_tcp_t **server) {
	uv_tcp_t **client = HL_NAME(w_tcp_init)(loop);
	UV_ERROR_CHECK_C(uv_accept(Stream_val(server), Stream_val(client)), free(Tcp_val(client)));
	return client;
}
DEFINE_PRIM(_TCP, w_tcp_accept, _LOOP _TCP);

HL_PRIM void HL_NAME(w_tcp_bind_ipv4)(uv_tcp_t **handle, int host, int port) {
	UV_SOCKADDR_IPV4(addr, host, port);
	UV_ERROR_CHECK(uv_tcp_bind(Tcp_val(handle), (const struct sockaddr *)&addr, 0));
}
DEFINE_PRIM(_VOID, w_tcp_bind_ipv4, _TCP _I32 _I32);

HL_PRIM void HL_NAME(w_tcp_bind_ipv6)(uv_tcp_t **handle, vbyte *host, int port, bool ipv6only) {
	UV_SOCKADDR_IPV6(addr, host, port);
	UV_ERROR_CHECK(uv_tcp_bind(Tcp_val(handle), (const struct sockaddr *)&addr, ipv6only ? UV_TCP_IPV6ONLY : 0));
}
DEFINE_PRIM(_VOID, w_tcp_bind_ipv6, _TCP _BYTES _I32 _BOOL);

HL_PRIM void HL_NAME(w_tcp_connect_ipv4)(uv_tcp_t **handle, int host, int port, vclosure *cb) {
	UV_SOCKADDR_IPV4(addr, host, port);
	UV_ALLOC_REQ(req, uv_connect_t, cb);
	UV_ERROR_CHECK_C(uv_tcp_connect(Connect_val(req), Tcp_val(handle), (const struct sockaddr *)&addr, (void (*)(uv_connect_t *, int))handle_stream_cb), UV_FREE_REQ(Connect_val(req)));
}
DEFINE_PRIM(_VOID, w_tcp_connect_ipv4, _TCP _I32 _I32 _CB);

HL_PRIM void HL_NAME(w_tcp_connect_ipv6)(uv_tcp_t **handle, vbyte *host, int port, vclosure *cb) {
	UV_SOCKADDR_IPV6(addr, host, port);
	UV_ALLOC_REQ(req, uv_connect_t, cb);
	UV_ERROR_CHECK_C(uv_tcp_connect(Connect_val(req), Tcp_val(handle), (const struct sockaddr *)&addr, (void (*)(uv_connect_t *, int))handle_stream_cb), UV_FREE_REQ(Connect_val(req)));
}
DEFINE_PRIM(_VOID, w_tcp_connect_ipv6, _TCP _BYTES _I32 _CB);

static vdynamic *w_getname(struct sockaddr_storage *addr) {
	if (addr->ss_family == AF_INET) {
		vdynamic *w_addr = construct_addrinfo_ipv4(ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr));
		return construct_addrport(w_addr, ntohs(((struct sockaddr_in *)addr)->sin_port));
	} else if (addr->ss_family == AF_INET6) {
		vdynamic *w_addr = construct_addrinfo_ipv6(((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr);
		return construct_addrport(w_addr, ntohs(((struct sockaddr_in6 *)addr)->sin6_port));
	}
	UV_ERROR(0);
}

HL_PRIM vdynamic *HL_NAME(w_tcp_getsockname)(uv_tcp_t **handle) {
	struct sockaddr_storage storage;
	int dummy = sizeof(struct sockaddr_storage);
	UV_ERROR_CHECK(uv_tcp_getsockname(Tcp_val(handle), (struct sockaddr *)&storage, &dummy));
	return w_getname(&storage);
}
DEFINE_PRIM(_DYN, w_tcp_getsockname, _TCP);

HL_PRIM vdynamic *HL_NAME(w_tcp_getpeername)(uv_tcp_t **handle) {
	struct sockaddr_storage storage;
	int dummy = sizeof(struct sockaddr_storage);
	UV_ERROR_CHECK(uv_tcp_getpeername(Tcp_val(handle), (struct sockaddr *)&storage, &dummy));
	return w_getname(&storage);
}
DEFINE_PRIM(_DYN, w_tcp_getpeername, _TCP);

// ------------- UDP ------------------------------------------------

static void handle_udp_cb_recv(uv_udp_t *handle, long int nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned int flags) {
	vclosure *cb = UV_HANDLE_DATA_SUB(handle, udp).cb_read;
	if (nread < 0)
		hl_call5(void, cb, vdynamic *, construct_error(nread), vbyte *, NULL, int, 0, vdynamic *, 0, int, 0);
	else {
		vdynamic *w_addr = NULL;
		int w_port = 0;
		if (addr != NULL) {
			if (addr->sa_family == AF_INET) {
				w_addr = construct_addrinfo_ipv4(ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr));
				w_port = ntohs(((struct sockaddr_in *)addr)->sin_port);
			} else if (addr->sa_family == AF_INET6) {
				w_addr = construct_addrinfo_ipv6(((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr);
				w_port = ntohs(((struct sockaddr_in6 *)addr)->sin6_port);
			}
		}
		hl_call5(void, cb, vdynamic *, NULL, vbyte *, (vbyte *)buf->base, int, nread, vdynamic *, w_addr, int, w_port);
	}
}

HL_PRIM uv_udp_t **HL_NAME(w_udp_init)(uv_loop_t **loop) {
	UV_ALLOC_CHECK(handle, uv_udp_t);
	UV_ERROR_CHECK_C(uv_udp_init(Loop_val(loop), Udp_val(handle)), free(Udp_val(handle)));
	if ((UV_HANDLE_DATA(Udp_val(handle)) = alloc_data()) == NULL)
		UV_ERROR(0);
	return handle;
}
DEFINE_PRIM(_UDP, w_udp_init, _LOOP);

HL_PRIM void HL_NAME(w_udp_bind_ipv4)(uv_udp_t **handle, int host, int port) {
	UV_SOCKADDR_IPV4(addr, host, port);
	UV_ERROR_CHECK(uv_udp_bind(Udp_val(handle), (const struct sockaddr *)&addr, 0));
}
DEFINE_PRIM(_VOID, w_udp_bind_ipv4, _UDP _I32 _I32);

HL_PRIM void HL_NAME(w_udp_bind_ipv6)(uv_udp_t **handle, vbyte *host, int port, bool ipv6only) {
	UV_SOCKADDR_IPV6(addr, host, port);
	UV_ERROR_CHECK(uv_udp_bind(Udp_val(handle), (const struct sockaddr *)&addr, ipv6only ? UV_UDP_IPV6ONLY : 0));
}
DEFINE_PRIM(_VOID, w_udp_bind_ipv6, _UDP _BYTES _I32 _BOOL);

HL_PRIM void HL_NAME(w_udp_send_ipv4)(uv_udp_t **handle, vbyte *data, int length, int host, int port, vclosure *cb) {
	UV_SOCKADDR_IPV4(addr, host, port);
	UV_ALLOC_REQ(req, uv_udp_send_t, cb);
	uv_buf_t buf = uv_buf_init((char *)data, length);
	UV_ERROR_CHECK_C(uv_udp_send(UdpSend_val(req), Udp_val(handle), &buf, 1, (const struct sockaddr *)&addr, (void (*)(uv_udp_send_t *, int))handle_stream_cb), UV_FREE_REQ(UdpSend_val(req)));
}
DEFINE_PRIM(_VOID, w_udp_send_ipv4, _UDP _BYTES _I32 _I32 _I32 _CB);

HL_PRIM void HL_NAME(w_udp_send_ipv6)(uv_udp_t **handle, vbyte *data, int length, vbyte *host, int port, vclosure *cb) {
	UV_SOCKADDR_IPV6(addr, host, port);
	UV_ALLOC_REQ(req, uv_udp_send_t, cb);
	uv_buf_t buf = uv_buf_init((char *)data, length);
	UV_ERROR_CHECK_C(uv_udp_send(UdpSend_val(req), Udp_val(handle), &buf, 1, (const struct sockaddr *)&addr, (void (*)(uv_udp_send_t *, int))handle_stream_cb), UV_FREE_REQ(UdpSend_val(req)));
}
DEFINE_PRIM(_VOID, w_udp_send_ipv6, _UDP _BYTES _I32 _BYTES _I32 _CB);

HL_PRIM void HL_NAME(w_udp_recv_start)(uv_udp_t **handle, vclosure *cb) {
	UV_HANDLE_DATA_SUB(Udp_val(handle), udp).cb_read = cb;
	UV_ERROR_CHECK(uv_udp_recv_start(Udp_val(handle), handle_stream_cb_alloc, handle_udp_cb_recv));
}
DEFINE_PRIM(_VOID, w_udp_recv_start, _UDP _CB_UDP_RECV);

HL_PRIM void HL_NAME(w_udp_recv_stop)(uv_udp_t **handle) {
	UV_ERROR_CHECK(uv_udp_recv_stop(Udp_val(handle)));
	UV_HANDLE_DATA_SUB(Udp_val(handle), udp).cb_read = NULL;
}
DEFINE_PRIM(_VOID, w_udp_recv_stop, _UDP);

HL_PRIM void HL_NAME(w_udp_set_membership)(uv_udp_t **handle, const char *address, const char *intfc, bool join) {
	UV_ERROR_CHECK(uv_udp_set_membership(Udp_val(handle), address, intfc, join ? UV_JOIN_GROUP : UV_LEAVE_GROUP));
}
DEFINE_PRIM(_VOID, w_udp_set_membership, _UDP _BYTES _BYTES _BOOL);

// TODO: why? just cast?
HL_PRIM void HL_NAME(w_udp_close)(uv_udp_t **handle, vclosure *cb) {
	HL_NAME(w_close)((uv_handle_t **)handle, cb);
}
DEFINE_PRIM(_VOID, w_udp_close, _UDP _CB);

HL_PRIM vdynamic *HL_NAME(w_udp_getsockname)(uv_udp_t **handle) {
	struct sockaddr_storage storage;
	int dummy = sizeof(struct sockaddr_storage);
	UV_ERROR_CHECK(uv_udp_getsockname(Udp_val(handle), (struct sockaddr *)&storage, &dummy));
	return w_getname(&storage);
}
DEFINE_PRIM(_DYN, w_udp_getsockname, _UDP);

HL_PRIM void HL_NAME(w_udp_set_broadcast)(uv_udp_t **handle, bool flag) {
	UV_ERROR_CHECK(uv_udp_set_broadcast(Udp_val(handle), flag ? 1 : 0));
}
DEFINE_PRIM(_VOID, w_udp_set_broadcast, _UDP _BOOL);

HL_PRIM void HL_NAME(w_udp_set_multicast_interface)(uv_udp_t **handle, const char *intfc) {
	UV_ERROR_CHECK(uv_udp_set_multicast_interface(Udp_val(handle), intfc));
}
DEFINE_PRIM(_VOID, w_udp_set_multicast_interface, _UDP _BYTES);

HL_PRIM void HL_NAME(w_udp_set_multicast_loopback)(uv_udp_t **handle, bool flag) {
	UV_ERROR_CHECK(uv_udp_set_multicast_loop(Udp_val(handle), flag ? 1 : 0));
}
DEFINE_PRIM(_VOID, w_udp_set_multicast_loopback, _UDP _BOOL);

HL_PRIM void HL_NAME(w_udp_set_multicast_ttl)(uv_udp_t **handle, int ttl) {
	UV_ERROR_CHECK(uv_udp_set_multicast_ttl(Udp_val(handle), ttl));
}
DEFINE_PRIM(_VOID, w_udp_set_multicast_ttl, _UDP _I32);

HL_PRIM void HL_NAME(w_udp_set_ttl)(uv_udp_t **handle, int ttl) {
	UV_ERROR_CHECK(uv_udp_set_ttl(Udp_val(handle), ttl));
}
DEFINE_PRIM(_VOID, w_udp_set_ttl, _UDP _I32);

HL_PRIM int HL_NAME(w_udp_get_recv_buffer_size)(uv_udp_t **handle) {
	int size = 0;
	return uv_recv_buffer_size(Handle_val(handle), &size);
}
DEFINE_PRIM(_I32, w_udp_get_recv_buffer_size, _UDP);

HL_PRIM int HL_NAME(w_udp_get_send_buffer_size)(uv_udp_t **handle) {
	int size = 0;
	return uv_send_buffer_size(Handle_val(handle), &size);
}
DEFINE_PRIM(_I32, w_udp_get_send_buffer_size, _UDP);

HL_PRIM int HL_NAME(w_udp_set_recv_buffer_size)(uv_udp_t **handle, int size) {
	return uv_recv_buffer_size(Handle_val(handle), &size);
}
DEFINE_PRIM(_I32, w_udp_set_recv_buffer_size, _UDP _I32);

HL_PRIM int HL_NAME(w_udp_set_send_buffer_size)(uv_udp_t **handle, int size) {
	return uv_send_buffer_size(Handle_val(handle), &size);
}
DEFINE_PRIM(_I32, w_udp_set_send_buffer_size, _UDP _I32);

// ------------- DNS ------------------------------------------------

static void handle_dns_gai_cb(uv_getaddrinfo_t *req, int status, struct addrinfo *res) {
	vclosure *cb = UV_REQ_DATA(req);
	if (status < 0)
		hl_call2(void, cb, vdynamic *, construct_error(status), varray *, NULL);
	else {
		// TODO: use linked list
		int count = 0;
		struct addrinfo *cur;
		for (cur = res; cur != NULL; cur = cur->ai_next) {
			if (cur->ai_family == AF_INET || cur->ai_family == AF_INET6)
				count++;
		}
		varray *arr = hl_alloc_array(&hlt_dyn, count);
		cur = res;
		for (int i = 0; i < count; i++) {
			if (cur->ai_family == AF_INET)
				hl_aptr(arr, vdynamic *)[i] = construct_addrinfo_ipv4(ntohl(((struct sockaddr_in *)cur->ai_addr)->sin_addr.s_addr));
			else if (cur->ai_family == AF_INET6)
				hl_aptr(arr, vdynamic *)[i] = construct_addrinfo_ipv6(((struct sockaddr_in6 *)cur->ai_addr)->sin6_addr.s6_addr);
			cur = cur->ai_next;
		}
		uv_freeaddrinfo(res);
		hl_call2(void, cb, vdynamic *, NULL, varray *, arr);
	}
	UV_FREE_REQ(req);
}

HL_PRIM void HL_NAME(w_dns_getaddrinfo)(uv_loop_t **loop, vbyte *node, bool flag_addrconfig, bool flag_v4mapped, int hint_family, vclosure *cb) {
	UV_ALLOC_REQ(req, uv_getaddrinfo_t, cb);
	int hint_flags_u = 0;
	if (flag_addrconfig)
		hint_flags_u |= AI_ADDRCONFIG;
	if (flag_v4mapped)
		hint_flags_u |= AI_V4MAPPED;
	int hint_family_u = AF_UNSPEC;
	if (hint_family == 4)
		hint_family_u = AF_INET;
	else if (hint_family == 6)
		hint_family_u = AF_INET6;
	struct addrinfo hints = {
		.ai_flags = hint_flags_u,
		.ai_family = hint_family_u,
		.ai_socktype = 0,
		.ai_protocol = 0,
		.ai_addrlen = 0,
		.ai_addr = NULL,
		.ai_canonname = NULL,
		.ai_next = NULL
	};
	UV_ERROR_CHECK_C(uv_getaddrinfo(Loop_val(loop), GetAddrInfo_val(req), handle_dns_gai_cb, (char *)node, NULL, &hints), UV_FREE_REQ(GetAddrInfo_val(req)));
}
DEFINE_PRIM(_VOID, w_dns_getaddrinfo, _LOOP _BYTES _BOOL _BOOL _I32 _CB_GAI);

/*
static void handle_dns_gni(uv_getnameinfo_t *req, int status, const char *hostname, const char *service) {
	vclosure *cb = UV_REQ_DATA(req);
	if (status < 0)
		hl_call3(void, cb, vdynamic *, construct_error(status), vbyte *, NULL, vbyte *, NULL);
	else 
		hl_call3(void, cb, vdynamic *, NULL, vbyte *, (vbyte *)hostname, vbyte *, (vbyte *)service);
	UV_FREE_REQ(req);
}

HL_PRIM void HL_NAME(w_getnameinfo_ipv4)(uv_loop_t *loop, int ip, int flags, vclosure *cb) {
	UV_SOCKADDR_IPV4(addr, ip, 0);
	UV_ALLOC_REQ(req, uv_getnameinfo_t, cb);
	UV_ERROR_CHECK_C(uv_getnameinfo(loop, req, handle_dns_gni, (const struct sockaddr *)&addr, flags), UV_FREE_REQ(GetNameInfo_val(req)));
}

HL_PRIM void HL_NAME(w_getnameinfo_ipv6)(uv_loop_t *loop, vbyte *ip, int flags, vclosure *cb) {
	UV_SOCKADDR_IPV6(addr, ip, 0);
	UV_ALLOC_REQ(req, uv_getnameinfo_t, cb);
	UV_ERROR_CHECK_C(uv_getnameinfo(loop, req, handle_dns_gni, (const struct sockaddr *)&addr, flags), UV_FREE_REQ(GetNameInfo_val(req)));
}

DEFINE_PRIM(_VOID, w_getnameinfo_ipv4, _LOOP _I32 _I32 _CB_GNI);
DEFINE_PRIM(_VOID, w_getnameinfo_ipv6, _LOOP _BYTES _I32 _CB_GNI);
*/

// ------------- TIMERS ---------------------------------------------

static void handle_timer_cb(uv_timer_t *handle) {
	vclosure *cb = UV_HANDLE_DATA_SUB(handle, timer).cb_timer;
	hl_call0(void, cb);
}

HL_PRIM uv_timer_t **HL_NAME(w_timer_start)(uv_loop_t **loop, int timeout, vclosure *cb) {
	UV_ALLOC_CHECK(handle, uv_timer_t);
	UV_ERROR_CHECK_C(uv_timer_init(Loop_val(loop), Timer_val(handle)), free(Timer_val(handle)));
	if ((UV_HANDLE_DATA(Timer_val(handle)) = alloc_data()) == NULL)
		UV_ERROR(0);
	UV_HANDLE_DATA_SUB(Timer_val(handle), timer).cb_timer = cb;
	UV_ERROR_CHECK_C(
		uv_timer_start(Timer_val(handle), handle_timer_cb, timeout, timeout),
		{ unalloc_data(UV_HANDLE_DATA(Timer_val(handle))); free(Timer_val(handle)); }
		);
	return handle;
}
DEFINE_PRIM(_TIMER, w_timer_start, _LOOP _I32 _CB_NOERR);

HL_PRIM void HL_NAME(w_timer_stop)(uv_timer_t **handle, vclosure *cb) {
	UV_ERROR_CHECK_C(
		uv_timer_stop(Timer_val(handle)),
		{ unalloc_data(UV_HANDLE_DATA(Timer_val(handle))); free(Timer_val(handle)); }
		);
	((uv_w_handle_t *)UV_HANDLE_DATA(Timer_val(handle)))->cb_close = cb;
	uv_close(Handle_val(handle), handle_close_cb);
}
DEFINE_PRIM(_VOID, w_timer_stop, _TIMER _CB);

// ------------- PROCESS --------------------------------------------

#define _CB_PROCESS_EXIT _FUN(_VOID, _I32 _I32)

static void handle_process_cb(uv_process_t *handle, int64_t exit_status, int term_signal) {
	vclosure *cb = UV_HANDLE_DATA_SUB(handle, process).cb_exit;
	// FIXME: int64 -> int conversion
	hl_call2(void, cb, int, exit_status, int, term_signal);
}

typedef struct {
	hl_type *t;
	int index;
	bool readable;
	bool writable;
	uv_stream_t **pipe;
} hx_process_io_pipe;

typedef struct {
	hl_type *t;
	int index;
	uv_stream_t **pipe;
} hx_process_io_ipc;

HL_PRIM uv_process_t **HL_NAME(w_spawn)(uv_loop_t **loop, vclosure *cb, const char *file, varray *args, varray *env, const char *cwd, int flags, varray *stdio, int uid, int gid) {
	UV_ALLOC_CHECK(handle, uv_process_t);
	if ((UV_HANDLE_DATA(Process_val(handle)) = alloc_data()) == NULL)
		UV_ERROR(0);
	UV_HANDLE_DATA_SUB(Process_val(handle), process).cb_exit = cb;
	char **args_u = malloc(sizeof(char *) * (args->size + 1));
	for (int i = 0; i < args->size; i++)
		args_u[i] = strdup(hl_aptr(args, char *)[i]);
	args_u[args->size] = NULL;
	char **env_u = malloc(sizeof(char *) * (env->size + 1));
	for (int i = 0; i < env->size; i++)
		env_u[i] = strdup(hl_aptr(env, char *)[i]);
	env_u[env->size] = NULL;
	uv_stdio_container_t *stdio_u = malloc(sizeof(uv_stdio_container_t) * stdio->size);
	for (int i = 0; i < stdio->size; i++) {
		venum *stdio_entry = hl_aptr(stdio, venum *)[i];
		switch (stdio_entry->index) {
			case 0: // Ignore
				stdio_u[i].flags = UV_IGNORE;
				break;
			case 1: // Inherit
				stdio_u[i].flags = UV_INHERIT_FD;
				stdio_u[i].data.fd = i;
				break;
			case 2: // Pipe
				stdio_u[i].flags = UV_CREATE_PIPE;
				if (((hx_process_io_pipe *)stdio_entry)->readable)
					stdio_u[i].flags |= UV_READABLE_PIPE;
				if (((hx_process_io_pipe *)stdio_entry)->writable)
					stdio_u[i].flags |= UV_WRITABLE_PIPE;
				stdio_u[i].data.stream = Stream_val(((hx_process_io_pipe *)stdio_entry)->pipe);
				break;
			default: // 3, Ipc
				stdio_u[i].flags = UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE;
				stdio_u[i].data.stream = Stream_val(((hx_process_io_ipc *)stdio_entry)->pipe);
				break;
		}
	}
	uv_process_options_t options = {
		.exit_cb = handle_process_cb,
		.file = file,
		.args = args_u,
		.env = env_u,
		.cwd = cwd,
		.flags = flags,
		.stdio_count = stdio->size,
		.stdio = stdio_u,
		.uid = uid,
		.gid = gid
	};
	UV_ERROR_CHECK_C(
		uv_spawn(Loop_val(loop), Process_val(handle), &options),
		{ free(args_u); free(env_u); free(stdio_u); unalloc_data(UV_HANDLE_DATA(Process_val(handle))); free(Process_val(handle)); }
		);
	free(args_u);
	free(env_u);
	free(stdio_u);
	return handle;
}
DEFINE_PRIM(_PROCESS, w_spawn, _LOOP _CB_PROCESS_EXIT _BYTES _ARR _ARR _BYTES _I32 _ARR _I32 _I32);

HL_PRIM void HL_NAME(w_process_kill)(uv_process_t **handle, int signum) {
	UV_ERROR_CHECK(uv_process_kill(Process_val(handle), signum));
}
DEFINE_PRIM(_VOID, w_process_kill, _PROCESS _I32);

HL_PRIM int HL_NAME(w_process_get_pid)(uv_process_t **handle) {
	return Process_val(handle)->pid;
}
DEFINE_PRIM(_I32, w_process_get_pid, _PROCESS);

// ------------- PIPES ----------------------------------------------

HL_PRIM uv_pipe_t **HL_NAME(w_pipe_init)(uv_loop_t **loop, bool ipc) {
	UV_ALLOC_CHECK(handle, uv_pipe_t);
	UV_ERROR_CHECK_C(uv_pipe_init(Loop_val(loop), Pipe_val(handle), ipc ? 1 : 0), free(Pipe_val(handle)));
	if ((UV_HANDLE_DATA(Pipe_val(handle)) = alloc_data()) == NULL)
		UV_ERROR(0);
	return handle;
}
DEFINE_PRIM(_PIPE, w_pipe_init, _LOOP _BOOL);

HL_PRIM void HL_NAME(w_pipe_open)(uv_pipe_t **pipe, int fd) {
	UV_ERROR_CHECK(uv_pipe_open(Pipe_val(pipe), fd));
}
DEFINE_PRIM(_VOID, w_pipe_open, _PIPE _I32);

HL_PRIM uv_pipe_t **HL_NAME(w_pipe_accept)(uv_loop_t **loop, uv_pipe_t **server) {
	UV_ALLOC_CHECK(client, uv_pipe_t);
	UV_ERROR_CHECK_C(uv_pipe_init(Loop_val(loop), Pipe_val(client), 0), free(Pipe_val(client)));
	if ((UV_HANDLE_DATA(Pipe_val(client)) = alloc_data()) == NULL)
		UV_ERROR(0);
	UV_ERROR_CHECK_C(uv_accept(Stream_val(server), Stream_val(client)), free(Pipe_val(client)));
	return client;
}
DEFINE_PRIM(_PIPE, w_pipe_accept, _LOOP _PIPE);

HL_PRIM void HL_NAME(w_pipe_bind_ipc)(uv_pipe_t **handle, const char *path) {
	UV_ERROR_CHECK(uv_pipe_bind(Pipe_val(handle), path));
}
DEFINE_PRIM(_VOID, w_pipe_bind_ipc, _PIPE _BYTES);

HL_PRIM void HL_NAME(w_pipe_connect_ipc)(uv_pipe_t **handle, const char *path, vclosure *cb) {
	UV_ALLOC_REQ(req, uv_connect_t, cb);
	uv_pipe_connect(Connect_val(req), Pipe_val(handle), path, (void (*)(uv_connect_t *, int))handle_stream_cb);
}
DEFINE_PRIM(_VOID, w_pipe_connect_ipc, _PIPE _BYTES _CB);

HL_PRIM int HL_NAME(w_pipe_pending_count)(uv_pipe_t **handle) {
	return uv_pipe_pending_count(Pipe_val(handle));
}
DEFINE_PRIM(_I32, w_pipe_pending_count, _PIPE);

HL_PRIM vdynamic *HL_NAME(w_pipe_accept_pending)(uv_loop_t **loop, uv_pipe_t **handle) {
	switch (uv_pipe_pending_type(Pipe_val(handle))) {
		case UV_NAMED_PIPE: {
			UV_ALLOC_CHECK(client, uv_pipe_t);
			UV_ERROR_CHECK_C(uv_pipe_init(Loop_val(loop), Pipe_val(client), 0), free(Pipe_val(client)));
			if ((UV_HANDLE_DATA(Pipe_val(client)) = alloc_data()) == NULL)
				UV_ERROR(0);
			UV_ERROR_CHECK_C(uv_accept(Stream_val(handle), Stream_val(client)), free(Pipe_val(client)));
			return construct_pipe_accept_pipe(client);
		} break;
		case UV_TCP: {
			UV_ALLOC_CHECK(client, uv_tcp_t);
			UV_ERROR_CHECK_C(uv_tcp_init(Loop_val(loop), Tcp_val(client)), free(Tcp_val(client)));
			if ((UV_HANDLE_DATA(Tcp_val(client)) = alloc_data()) == NULL)
				UV_ERROR(0);
			UV_ERROR_CHECK_C(uv_accept(Stream_val(handle), Stream_val(client)), free(Tcp_val(client)));
			return construct_pipe_accept_socket(client);
		} break;
		default:
			UV_ERROR(0);
			break;
	}
}
DEFINE_PRIM(_DYN, w_pipe_accept_pending, _LOOP _PIPE);

HL_PRIM vbyte *HL_NAME(w_pipe_getsockname)(uv_pipe_t **handle) {
	vbyte *path = hl_gc_alloc_noptr(256);
	size_t path_size = 255;
	UV_ERROR_CHECK(uv_pipe_getsockname(Pipe_val(handle), (char *)path, &path_size));
	path[path_size] = 0;
	return path;
}
DEFINE_PRIM(_BYTES, w_pipe_getsockname, _PIPE);

HL_PRIM vbyte *HL_NAME(w_pipe_getpeername)(uv_pipe_t **handle) {
	vbyte *path = hl_gc_alloc_noptr(256);
	size_t path_size = 255;
	UV_ERROR_CHECK(uv_pipe_getpeername(Pipe_val(handle), (char *)path, &path_size));
	path[path_size] = 0;
	return path;
}
DEFINE_PRIM(_BYTES, w_pipe_getpeername, _PIPE);

HL_PRIM void HL_NAME(w_pipe_write_handle)(uv_pipe_t **handle, vbyte *data, int length, uv_stream_t **send_handle, vclosure *cb) {
	UV_ALLOC_REQ(req, uv_write_t, cb);
	uv_buf_t buf = uv_buf_init((char *)data, length);
	UV_ERROR_CHECK_C(uv_write2(Write_val(req), Stream_val(handle), &buf, 1, Stream_val(send_handle), (void (*)(uv_write_t *, int))handle_stream_cb), UV_FREE_REQ(Write_val(req)));
}
DEFINE_PRIM(_VOID, w_pipe_write_handle, _PIPE _BYTES _I32 _STREAM _CB);

// ------------- THREADS --------------------------------------------

static void handle_thread_cb(uv_work_t *req) {
	vclosure *cb = UV_REQ_DATA(req);
	hl_call0(void, cb);
}

static void handle_thread_done_cb(uv_work_t *req, int status) {
	// TODO: what to do with status? throw in main thread if <0 ?
	UV_FREE_REQ(req);
}

HL_PRIM uv_work_t **HL_NAME(w_thread_create)(uv_loop_t **loop, vclosure *cb) {
	UV_ALLOC_REQ(req, uv_work_t, cb);
	UV_ERROR_CHECK_C(uv_queue_work(Loop_val(loop), Work_val(req), handle_thread_cb, handle_thread_done_cb), UV_FREE_REQ(Work_val(req)));
	return req;
}
DEFINE_PRIM(_WORK, w_thread_create, _CB);

HL_PRIM void HL_NAME(w_thread_kill)(uv_work_t **thread) {
	uv_cancel((uv_req_t *)Work_val(thread));
}
DEFINE_PRIM(_VOID, w_thread_kill, _WORK);

// ------------- MUTEX ----------------------------------------------

HL_PRIM uv_mutex_t **HL_NAME(w_mutex_init)(void) {
	UV_ALLOC_CHECK(handle, uv_mutex_t);
	UV_ERROR_CHECK_C(uv_mutex_init(Mutex_val(handle)), free(Mutex_val(handle)));
	return handle;
}
DEFINE_PRIM(_MUTEX, w_mutex_init, _NO_ARG);

HL_PRIM void HL_NAME(w_mutex_lock)(uv_mutex_t **handle) {
	uv_mutex_lock(Mutex_val(handle));
}
DEFINE_PRIM(_VOID, w_mutex_lock, _MUTEX);

HL_PRIM void HL_NAME(w_mutex_trylock)(uv_mutex_t **handle) {
	UV_ERROR_CHECK(uv_mutex_trylock(Mutex_val(handle)));
}
DEFINE_PRIM(_VOID, w_mutex_trylock, _MUTEX);

HL_PRIM void HL_NAME(w_mutex_unlock)(uv_mutex_t **handle) {
	uv_mutex_unlock(Mutex_val(handle));
}
DEFINE_PRIM(_VOID, w_mutex_unlock, _MUTEX);

// ------------- CASTS ----------------------------------------------

/**
	TODO: these might not be necessary if there is a way to cast HashLink
	abstracts in Haxe code.
**/

HL_PRIM uv_handle_t **HL_NAME(w_stream_handle)(uv_stream_t **handle) {
	return (uv_handle_t **)handle;
}
DEFINE_PRIM(_HANDLE, w_stream_handle, _STREAM);

HL_PRIM uv_handle_t **HL_NAME(w_fs_event_handle)(uv_fs_event_t **handle) {
	return (uv_handle_t **)handle;
}
DEFINE_PRIM(_HANDLE, w_fs_event_handle, _FS_EVENT);

HL_PRIM uv_handle_t **HL_NAME(w_timer_handle)(uv_timer_t **handle) {
	return (uv_handle_t **)handle;
}
DEFINE_PRIM(_HANDLE, w_timer_handle, _TIMER);

HL_PRIM uv_handle_t **HL_NAME(w_process_handle)(uv_process_t **handle) {
	return (uv_handle_t **)handle;
}
DEFINE_PRIM(_HANDLE, w_process_handle, _PROCESS);

HL_PRIM uv_stream_t **HL_NAME(w_tcp_stream)(uv_tcp_t **handle) {
	return (uv_stream_t **)handle;
}
DEFINE_PRIM(_STREAM, w_tcp_stream, _TCP);

HL_PRIM uv_stream_t **HL_NAME(w_udp_stream)(uv_udp_t **handle) {
	return (uv_stream_t **)handle;
}
DEFINE_PRIM(_STREAM, w_udp_stream, _UDP);

HL_PRIM uv_stream_t **HL_NAME(w_pipe_stream)(uv_pipe_t **handle) {
	return (uv_stream_t **)handle;
}
DEFINE_PRIM(_STREAM, w_pipe_stream, _PIPE);
