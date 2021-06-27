LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := hl
LOCAL_MODULE_FILENAME := libhl
LOCAL_SRC_FILES :\
include/pcre/*.c \
src/std/array.c \
src/std/buffer.c \
src/std/bytes.c \
src/std/cast.c \
src/std/date.c \
src/std/error.c \
src/std/file.c \
src/std/fun.c \
src/std/maps.c \
src/std/math.c \
src/std/obj.c \
src/std/random.c \
src/std/regexp.c \
src/std/socket.c \
src/std/string.c \
src/std/sys.c \
src/std/types.c \
src/std/ucs2.c \
src/std/thread.c \
src/std/process.c \
src/std/sys_android.c \
src/alloc.c \
src/hl.h \
src/hlc.h \
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH) \
                    $(LOCAL_PATH)/. \
                    $(LOCAL_PATH)/./include \
                    $(LOCAL_PATH)/./include/bullet \
                    $(LOCAL_PATH)/./include/ffmpeg \
                    $(LOCAL_PATH)/./include/gl\
                    $(LOCAL_PATH)/./include/libuv \
                    $(LOCAL_PATH)/./include/mbedtls \
                    $(LOCAL_PATH)/./include/mikktspace \
                    $(LOCAL_PATH)/./include/openal\include \
                    $(LOCAL_PATH)/./include/pcre \
                    $(LOCAL_PATH)/./include/png \
                    $(LOCAL_PATH)/./include/sdl/include \
                    $(LOCAL_PATH)/./include/sqlite/src \
					$(LOCAL_PATH)/./include/turbojpeg \
					$(LOCAL_PATH)/./include/vorbis \
					$(LOCAL_PATH)/./include/zlib \
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(LOCAL_PATH)/./include \
                    $(LOCAL_PATH)/./include/bullet \
                    $(LOCAL_PATH)/./include/ffmpeg \
                    $(LOCAL_PATH)/./include/gl\
                    $(LOCAL_PATH)/./include/libuv \
                    $(LOCAL_PATH)/./include/mbedtls \
                    $(LOCAL_PATH)/./include/mikktspace \
                    $(LOCAL_PATH)/./include/openal\include \
                    $(LOCAL_PATH)/./include/pcre \
                    $(LOCAL_PATH)/./include/png \
                    $(LOCAL_PATH)/./include/sdl/include \
                    $(LOCAL_PATH)/./include/sqlite/src \
					$(LOCAL_PATH)/./include/turbojpeg \
					$(LOCAL_PATH)/./include/vorbis \
					$(LOCAL_PATH)/./include/zlib \
LOCAL_EXPORT_LDLIBS := -lGLESv2 \
                       -llog \
                       -landroid
LOCAL_CFLAGS   :=  -DUSE_FILE32API
LOCAL_CFLAGS   +=  -fexceptions
include $(BUILD_STATIC_LIBRARY)
#==============================================================
