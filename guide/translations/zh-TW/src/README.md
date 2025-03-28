# (中文 WIP) Intro

Vulkan is known for being explicit and verbose. But the _required_ verbosity has steadily reduced with each successive version, its new features, and previous extensions being absorbed into the core API. Similarly, RAII has been a pillar of C++ since its inception, yet most Vulkan guides do not utilize it, instead choosing to "extend" the explicitness by manually cleaning up resources.

To fill that gap, this guide has the following goals:

- Leverage modern C++, VulkanHPP, and Vulkan 1.3 features
- Focus on keeping it simple and straightforward, _not_ on performance
- Develop a basic but dynamic rendering foundation

To reiterate, the focus is _not on performance_, it is on a quick introduction to the current standard multi-platform graphics API while utilizing the modern paradigms and tools (at the time of writing). Even disregarding potential performance gains, Vulkan has a better and modern design and ecosystem than OpenGL, eg: there is no global state machine, parameters are passed by filling structs with meaningful member variable names, multi-threading is largely trivial (yes, it is actually easier to do on Vulkan than OpenGL), there are a comprehensive set of validation layers to catch misuse which can be enabled without _any_ changes to application code, etc.

## Target Audience

The guide is for you if you:

- Understand the principles of modern C++ and its usage
- Have created C++ projects using third-party libraries
- Are somewhat familiar with graphics
    - Having done OpenGL tutorials would be ideal
    - Experience with frameworks like SFML / SDL is great
- Don't mind if all the information you need isn't monolithically in one place (ie, this guide)

Some examples of what this guide _does not_ focus on:

- GPU-driven rendering
- Real-time graphics from ground-up
- Considerations for tiled GPUs (eg mobile devices / Android)

## Source

The source code for the project (as well as this guide) is located in [this repository](https://github.com/cpp-gamedev/learn-vulkan). A `section/*` branch intends to reflect the state of the code at the end of a particular section of the guide. Bugfixes / changes are generally backported, but there may be some divergence from the current state of the code (ie, in `main`). The source of the guide itself is only up-to-date on `main`, changes are not backported.

