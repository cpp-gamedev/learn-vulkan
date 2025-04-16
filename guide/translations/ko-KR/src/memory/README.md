# Memory Allocation

Being an explicit API, [allocating memory](https://docs.vulkan.org/guide/latest/memory_allocation.html) in Vulkan that can be used by the device is the application's responsibility. The specifics can get quite complicated, but as recommended by the spec, we shall simply defer all that to a library: [Vulkan Memory Allocator (VMA)](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator).

Vulkan exposes two kinds of objects that use such allocated memory: Buffers and Images, VMA offers transparent support for both: we just have to allocate/free buffers and images through VMA instead of the device directly. Unlike memory allocation / object construction on the CPU, there are many more parameters (than say alignment and size) to provide for the creation of buffers and images. As you might have guessed, we shall constrain ourselves to a subset that's relevant for shader resources: vertex buffers, uniform/storage buffers, and texture images.
