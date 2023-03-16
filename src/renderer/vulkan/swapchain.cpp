#include <swapchain.h>
#include <context.h>

struct swapchain
{

};

swapchain* swapchain_init(renderer_context*)
{
	swapchain* out = new swapchain();



	return out;
}

void swapchian_destroy(swapchain* swapchain, renderer_context*)
{
	if (swapchain)
		delete swapchain;
}

void swapchian_update(swapchain*, renderer_context*)
{

}