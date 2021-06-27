LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := hl_core

LOCAL_MODULE_FILENAME := libhl_core

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
alloc.c \
code.c \
debugger.c \
hl.h \
hlc_main.c \
hlc.h \
hlmodule.h \
jit.c \
main.c \
module.c \
opcodes.h \
std/array.c \
std/buffer.c \
std/bytes.c \
std/cast.c \
std/date.c \
std/debug.c \
std/error.c \
std/file.c \
std/fun.c \
std/maps.h \
std/math.c \
std/obj.c \
std/process.c \
std/random.c \
std/regexp.c \
std/socket.c \
std/sort.h \
std/string.c \
std/sys_android.c \
std/sys.c \
std/thread.c \
std/track.c \
std/types.c \
std/ucs2.c \
std/unicase.h \

