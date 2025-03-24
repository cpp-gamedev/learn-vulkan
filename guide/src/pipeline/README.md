# Graphics Pipeline

A [Vulkan Graphics Pipeline](https://docs.vulkan.org/spec/latest/chapters/pipelines.html) is a large object that encompasses the entire graphics pipeline. It consists of many stages - all this happens during a single `draw()` call. We again constrain ourselves to caring about a small subset:

1. Input Assembly: vertex buffers are read here
1. Vertex Shader: shader is run for each vertex in the primitive
1. Early Fragment Tests (EFT): pre-shading tests
1. Fragment Shader: shader is run for each fragment
1. Late Fragment Tests (LFT): depth buffer is written here
1. Color Blending: transparency

A Graphics Pipeline's specification broadly includes configuration of the vertex attributes and fixed state (baked into the pipeline), dynamic states (must be set at draw-time), shader code (SPIR-V), and its layout (Descriptor Sets and Push Constants). Creation of a pipeline is verbose _and_ expensive, most engines use some sort of "hash and cache" approach to optimize reuse of existing pipelines. The Descriptor Set Layouts of a shader program need to be explicitly specified, engines can either dictate a static layout or use runtime reflection via [SPIR-V Cross](https://github.com/KhronosGroup/SPIRV-Cross).

We shall use a single Pipeline Layout that evolves over chapters.
