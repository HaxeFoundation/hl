#define HL_NAME(n) sdl_##n
#include <hl.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <shaderc/shaderc.h>

static VkInstance instance = NULL;

VkInstance vk_get_instance() {
	return instance;
}

HL_PRIM bool HL_NAME(vk_init)() {
	if( instance )
		return true;
	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "HashLink Vulkan",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "Heaps.io",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0,
	};
	VkInstanceCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = 2,
		.ppEnabledExtensionNames = (const char* const[]) { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME },
	};
	return vkCreateInstance(&info, NULL, &instance) == VK_SUCCESS;
}

// ------------------------------------------ CONTEXT INIT --------------------------------------------

#define MAX_SWAPCHAIN_IMAGES 3
#define FRAME_COUNT 2

typedef struct {
	VkCommandBuffer buffer;
	VkFence fence;
	VkSemaphore imageAvailable;
	VkSemaphore renderFinished;
} VkFrame;

typedef struct _VkContext {
	VkSurfaceKHR surface;
	VkPhysicalDevice pdevice;
	VkDevice device;
	VkQueue queue;
	int queueFamily;

	VkSwapchainKHR swapchain;
	int swapchainImageCount;
	VkImage swapchainImages[MAX_SWAPCHAIN_IMAGES];

	unsigned int currentFrame;
	unsigned int currentImage;
	VkCommandPool commandPool;
	VkFrame frames[FRAME_COUNT];

} *VkContext;



void *vk_init_context( VkSurfaceKHR surface ) {
	VkContext ctx = (VkContext)malloc(sizeof(struct _VkContext));
	memset(ctx,0,sizeof(struct _VkContext));
	ctx->surface = surface;

	int physicalDeviceCount;
#	define MAX_DEVICE_COUNT 16
#	define MAX_QUEUE_COUNT 16
	VkPhysicalDevice deviceHandles[MAX_DEVICE_COUNT];
	VkQueueFamilyProperties queueFamilyProperties[MAX_QUEUE_COUNT];
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, 0);
	physicalDeviceCount = physicalDeviceCount > MAX_DEVICE_COUNT ? MAX_DEVICE_COUNT : physicalDeviceCount;
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, deviceHandles);

	for(int i=0; i<physicalDeviceCount; i++) {
		int queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(deviceHandles[i], &queueFamilyCount, NULL);
		queueFamilyCount = queueFamilyCount > MAX_QUEUE_COUNT ? MAX_QUEUE_COUNT : queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(deviceHandles[i], &queueFamilyCount, queueFamilyProperties);
		vkGetPhysicalDeviceProperties(deviceHandles[i], &deviceProperties);
		vkGetPhysicalDeviceFeatures(deviceHandles[i], &deviceFeatures);
		vkGetPhysicalDeviceMemoryProperties(deviceHandles[i], &deviceMemoryProperties);
		for(int j=0; j<queueFamilyCount; j++) {
			VkBool32 supportsPresent = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(deviceHandles[i], j, ctx->surface, &supportsPresent);
			if (supportsPresent && (queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
				ctx->queueFamily = j;
				ctx->pdevice = deviceHandles[i];
				break;
			}
		}
		if( ctx->pdevice ) break;
	}

	VkDeviceQueueCreateInfo qinf = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = ctx->queueFamily,
		.queueCount = 1,
		.pQueuePriorities = (const float[]) { 1.0f }
	};
	VkDeviceCreateInfo dinf = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = 0,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = 0,
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = (const char* const[]) { VK_KHR_SWAPCHAIN_EXTENSION_NAME },
		.pEnabledFeatures = 0,
		.pQueueCreateInfos = &qinf,
	};

	if( vkCreateDevice(ctx->pdevice, &dinf, NULL, &ctx->device) != VK_SUCCESS )
		return NULL;

	vkGetDeviceQueue(ctx->device, ctx->queueFamily, 0, &ctx->queue);

	VkCommandPoolCreateInfo poolInf = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = ctx->queueFamily,
	};
	vkCreateCommandPool(ctx->device, &poolInf, 0, &ctx->commandPool);

	VkCommandBuffer cbuffers[FRAME_COUNT];
	VkCommandBufferAllocateInfo commandBufferAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = ctx->commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = FRAME_COUNT,
	};
	vkAllocateCommandBuffers(ctx->device, &commandBufferAllocInfo, cbuffers);

	VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	for(int i=0;i<FRAME_COUNT;i++) {
		VkFrame *f = &ctx->frames[i];
		f->buffer = cbuffers[i];
		vkCreateSemaphore(ctx->device, &semaphoreCreateInfo, 0, &f->imageAvailable);
		vkCreateSemaphore(ctx->device, &semaphoreCreateInfo, 0, &f->renderFinished);
		vkCreateFence(ctx->device, &fenceCreateInfo, 0, &f->fence);
	}
	return ctx;
}



bool HL_NAME(vk_init_swapchain)( VkContext ctx, int width, int height ) {

	vkDeviceWaitIdle(ctx->device);

	if( ctx->swapchain ) {
		vkDestroySwapchainKHR(ctx->device, ctx->swapchain, NULL);
		ctx->swapchain = NULL;
	}

	int formatCount = 1;
	VkSurfaceFormatKHR format;
	vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->pdevice, ctx->surface, &formatCount, 0); // suppress validation layer
	formatCount = 1;
	vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->pdevice, ctx->surface, &formatCount, &format);
	format.format = format.format == VK_FORMAT_UNDEFINED ? VK_FORMAT_B8G8R8A8_UNORM : format.format;

	int modeCount = 0;
#	define MAX_PRESENT_MODE_COUNT 16
	VkPresentModeKHR modes[MAX_PRESENT_MODE_COUNT];

	vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->pdevice, ctx->surface, &modeCount, NULL);
	modeCount = modeCount > MAX_PRESENT_MODE_COUNT ? MAX_PRESENT_MODE_COUNT : modeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->pdevice, ctx->surface, &modeCount, modes);

	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;   // always supported.
	ctx->swapchainImageCount = 2;

	// using MAILBOX will be same as no vsync
	/*
	for(int i=0; i<modeCount; i++) {
		if( modes[i] == VK_PRESENT_MODE_MAILBOX_KHR ) {
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			ctx->swapchainImageCount = 3;
			break;
		}
	}
	*/

	VkSurfaceCapabilitiesKHR scaps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->pdevice, ctx->surface, &scaps);

	VkExtent2D swapchainExtent = scaps.currentExtent;
#	define clamp(v,min,max) ( ((v) < (min)) ? (min) : ((v) < (max)) ? (max) : (v) )
	if( swapchainExtent.width == UINT32_MAX ) {
		swapchainExtent.width = clamp((unsigned)width, scaps.minImageExtent.width, scaps.maxImageExtent.width);
		swapchainExtent.height = clamp((unsigned)height, scaps.minImageExtent.height, scaps.maxImageExtent.height);
	}

	VkSwapchainCreateInfoKHR swapInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = ctx->surface,
		.minImageCount = ctx->swapchainImageCount,
		.imageFormat = format.format,
		.imageColorSpace = format.colorSpace,
		.imageExtent = swapchainExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = scaps.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
	};

	if( vkCreateSwapchainKHR(ctx->device, &swapInfo, 0, &ctx->swapchain) != VK_SUCCESS )
		return false;

	vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchainImageCount, NULL);
	vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchainImageCount, ctx->swapchainImages);

	return true;
}

const VkImageSubresourceRange RANGE_ALL = {
	.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.baseMipLevel = 0,
	.levelCount = VK_REMAINING_MIP_LEVELS,
	.baseArrayLayer = 0,
	.layerCount = VK_REMAINING_ARRAY_LAYERS,
};

HL_PRIM bool HL_NAME(vk_begin_frame)( VkContext ctx ) {
	VkFrame *frame = &ctx->frames[ctx->currentFrame];
	VkCommandBuffer buffer = frame->buffer;

	vkWaitForFences(ctx->device, 1, &frame->fence, VK_TRUE, UINT64_MAX);

	if( vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX, frame->imageAvailable, VK_NULL_HANDLE, &ctx->currentImage) != VK_SUCCESS )
		return false;

	vkResetFences(ctx->device, 1, &frame->fence);

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	vkBeginCommandBuffer(buffer, &beginInfo);

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		  .srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.srcQueueFamilyIndex = ctx->queueFamily,
			.dstQueueFamilyIndex = ctx->queueFamily,
			.image = ctx->swapchainImages[ctx->currentImage],
			.subresourceRange = RANGE_ALL,
	};
	vkCmdPipelineBarrier(buffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, NULL, 0, NULL, 1, &barrier
	);

	return true;
}

HL_PRIM void HL_NAME(vk_end_frame)( VkContext ctx ) {
	VkFrame *frame = &ctx->frames[ctx->currentFrame];

	VkImageMemoryBarrier barrier2 = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcQueueFamilyIndex = ctx->queueFamily,
		.dstQueueFamilyIndex = ctx->queueFamily,
		.image = ctx->swapchainImages[ctx->currentImage],
		.subresourceRange = RANGE_ALL,
	};
	vkCmdPipelineBarrier(frame->buffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0, 0, NULL, 0, NULL, 1,
		&barrier2
	);

	vkEndCommandBuffer(frame->buffer);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame->imageAvailable,
		.pWaitDstStageMask = (VkPipelineStageFlags[]) { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
		.commandBufferCount = 1,
		.pCommandBuffers = &frame->buffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &frame->renderFinished,
	};
	vkQueueSubmit(ctx->queue, 1, &submitInfo, frame->fence);

	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame->renderFinished,
		.swapchainCount = 1,
		.pSwapchains = &ctx->swapchain,
		.pImageIndices = &ctx->currentImage,
	};
	vkQueuePresentKHR(ctx->queue, &presentInfo);

	ctx->currentFrame++;
	ctx->currentFrame %= FRAME_COUNT;
}

VkShaderModule HL_NAME(vk_create_shader_module)( VkContext ctx, vbyte *data, int len ) {
	VkShaderModule module = NULL;
	VkShaderModuleCreateInfo inf = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = len,
		.pCode = (const uint32_t*)data,
	};
	vkCreateShaderModule(ctx->device,&inf,NULL,&module);
	return module;
}

VkPipelineLayout HL_NAME(vk_create_pipeline_layout)( VkContext ctx, VkPipelineLayoutCreateInfo *info ) {
	VkPipelineLayout p = NULL;
	vkCreatePipelineLayout(ctx->device, info, NULL, &p);
	return p;
}

VkPipeline HL_NAME(vk_create_graphics_pipeline)( VkContext ctx, VkGraphicsPipelineCreateInfo *info ) {
	VkPipeline p = NULL;
	vkCreateGraphicsPipelines(ctx->device, NULL, 1, info, NULL, &p);
	return p;
}

VkRenderPass HL_NAME(vk_create_render_pass)( VkContext ctx, VkRenderPassCreateInfo *info ) {
	VkRenderPass p = NULL;
	vkCreateRenderPass(ctx->device, info, NULL, &p);
	return p;
}

vbyte *HL_NAME(vk_make_array)( varray *a ) {
#ifdef HL_DEBUG
	if( a->at->kind != HSTRUCT ) hl_error("assert");
#endif
	int size = a->at->obj->rt->size;
	vbyte *ptr = hl_alloc_bytes(size * a->size);
	int i;
	for(i=0;i<a->size;i++)
		memcpy(ptr + i * size, hl_aptr(a,vbyte*)[i], size);
	return ptr;
}

#define _VCTX _ABSTRACT(vk_context)
#define _SHADER_MODULE _ABSTRACT(vk_shader_module)
#define _GPIPELINE _ABSTRACT(vk_gpipeline)
#define _PIPELAYOUT _ABSTRACT(vk_pipeline_layout)
#define _RENDERPASS _ABSTRACT(vk_render_pass)

DEFINE_PRIM(_BOOL, vk_init, _NO_ARG);
DEFINE_PRIM(_BOOL, vk_init_swapchain, _VCTX _I32 _I32);
DEFINE_PRIM(_BOOL, vk_begin_frame, _VCTX);
DEFINE_PRIM(_BYTES, vk_make_array, _ARR);
DEFINE_PRIM(_VOID, vk_end_frame, _VCTX);
DEFINE_PRIM(_SHADER_MODULE, vk_create_shader_module, _VCTX _BYTES _I32 );
DEFINE_PRIM(_GPIPELINE, vk_create_graphics_pipeline, _VCTX _STRUCT);
DEFINE_PRIM(_PIPELAYOUT, vk_create_pipeline_layout, _VCTX _STRUCT);
DEFINE_PRIM(_RENDERPASS, vk_create_render_pass, _VCTX _STRUCT);

// ------ COMMAND BUFFER OPERATIONS -----------------------

static VkContext current_context = NULL;
static VkCommandBuffer current_buffer = NULL;

HL_PRIM void HL_NAME(vk_set_current)( VkContext ctx ) {
	current_context = ctx;
	current_buffer = ctx->frames[ctx->currentFrame].buffer;
}

HL_PRIM void HL_NAME(vk_clear_color_image)( double r, double g, double b, double a ) {
	VkClearColorValue color = { (float)r, (float)g, (float)b, (float)a };
	vkCmdClearColorImage(current_buffer,
		current_context->swapchainImages[current_context->currentImage], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		&color, 1, &RANGE_ALL
	);
}

DEFINE_PRIM(_VOID, vk_set_current, _VCTX);
DEFINE_PRIM(_VOID, vk_clear_color_image, _F64 _F64 _F64 _F64);


// ------ SHADER COMPILATION ------------------------------

HL_PRIM vbyte *HL_NAME(vk_compile_shader)( vbyte *source, vbyte *shaderFile, vbyte *mainFunction, int shaderKind, int *outSize ) {
	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	shaderc_compile_options_t opts = shaderc_compile_options_initialize();
	shaderc_compile_options_set_optimization_level(opts, shaderc_optimization_level_size);
	shaderc_compilation_result_t result = shaderc_compile_into_spv(compiler, source, strlen(source), shaderKind, shaderFile, mainFunction, opts);
	shaderc_compiler_release(compiler);
	shaderc_compile_options_release(opts);
	
	if( shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success ) {
		const char *str = shaderc_result_get_error_message(result);
		vbyte *error = hl_copy_bytes(str, (int)strlen(str)+1);
		shaderc_result_release(result);
		*outSize = -1;
		return error;
	}

	int size = (int)shaderc_result_get_length(result);
	vbyte *data = hl_alloc_bytes(size);
	memcpy(data, shaderc_result_get_bytes(result), size);
	shaderc_result_release(result);
	
	*outSize = size;
	return data;
}

DEFINE_PRIM(_BYTES, vk_compile_shader, _BYTES _BYTES _BYTES _I32 _REF(_I32));
