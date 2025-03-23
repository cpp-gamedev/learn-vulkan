# Swapchain Loop

One part of rendering in the main loop is the Swapchain loop, which at a high level comprises of these steps:

1. Acquire a Swapchain Image (and its view)
1. Render to the acquired Image
1. Present the Image (this releases the image back to the Swapchain)

![WSI Engine](./wsi_engine.png)

There are a few nuances to deal with, for instance:

1. Acquiring (and/or presenting) will sometimes fail (eg because the Swapchain is out of date), in which case the remaining steps need to be skipped
1. The acquire command can return before the image is actually ready for use, rendering needs to be synchronized to only start after the image is ready
1. The images need appropriate Layout Transitions at each stage

Additionally, the number of swapchain images can vary, whereas the engine should use a fixed number of _virtual frames_: 2 for double buffering, 3 for triple (more is usually overkill). It's also possible for the main loop to acquire the same image before a previous render command has finished (or even started), if the Swapchain is using Mailbox Present Mode. While FIFO will block until the oldest submitted image is available (also known as vsync), we should still synchronize and wait until the acquired image has finished rendering.

## Virtual Frames

All the dynamic resources used during the rendering of a frame comprise a virtual frame. The application has a fixed number of virtual frames which it cycles through on each render pass. Each frame will be associated with a `vk::Fence` which will be waited on before rendering to it again. It will also have a pair of `vk::Semaphore`s to synchronize the acquire, render, and present calls on the GPU (we don't need to wait for them in the code). Lastly, there will be a Command Buffer per virtual frame, where all rendering commands for that frame (including layout transitions) will be recorded.

## Image Layouts

Vulkan Images have a property known as Image Layout. Most operations on images require them to be in certain specific layouts, requiring transitions before (and after). A layout transition conveniently also functions as a Pipeline Barrier (think memory barrier on the GPU), enabling us to synchronize operations before and after the transition.

Vulkan Synchronization is arguably the most complicated aspect of the API, a good amount of research is recommended. Here is an [article explaining barriers](https://gpuopen.com/learn/vulkan-barriers-explained/).
