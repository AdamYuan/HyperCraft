cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(glfw)
add_subdirectory(FastNoise2)

add_library(client_dep STATIC
        vk_mem_alloc.h
        vk_mem_alloc.cpp

        volk.h
        volk.c

        # IMGUI
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_impl_glfw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_tables.cpp

        # MYVK
        myvk/DeviceObjectBase.hpp
        myvk/ImageBase.cpp
        myvk/ImageBase.hpp
        myvk/Image.cpp
        myvk/Image.hpp
        myvk/BufferBase.cpp
        myvk/BufferBase.hpp
        myvk/Buffer.cpp
        myvk/Buffer.hpp
        myvk/CommandBuffer.cpp
        myvk/CommandBuffer.hpp
        myvk/CommandPool.cpp
        myvk/CommandPool.hpp
        myvk/Device.cpp
        myvk/Device.hpp
        myvk/Instance.cpp
        myvk/Instance.hpp
        myvk/PhysicalDevice.cpp
        myvk/PhysicalDevice.hpp
        myvk/Queue.cpp
        myvk/Queue.hpp
        myvk/QueueSelector.cpp
        myvk/QueueSelector.hpp
        myvk/DeviceCreateInfo.cpp
        myvk/DeviceCreateInfo.hpp
        myvk/Surface.cpp
        myvk/Surface.hpp
        myvk/Swapchain.cpp
        myvk/Swapchain.hpp
        myvk/SwapchainImage.cpp
        myvk/SwapchainImage.hpp
        myvk/Fence.cpp
        myvk/Fence.hpp
        myvk/Semaphore.cpp
        myvk/Semaphore.hpp
        myvk/ImageView.cpp
        myvk/ImageView.hpp
        myvk/RenderPass.cpp
        myvk/RenderPass.hpp
        myvk/PipelineBase.cpp
        myvk/PipelineBase.hpp
        myvk/PipelineLayout.cpp
        myvk/PipelineLayout.hpp
        myvk/DescriptorSetLayout.cpp
        myvk/DescriptorSetLayout.hpp
        myvk/ShaderModule.cpp
        myvk/ShaderModule.hpp
        myvk/GraphicsPipeline.cpp
        myvk/GraphicsPipeline.hpp
        myvk/ComputePipeline.cpp
        myvk/ComputePipeline.hpp
        myvk/Framebuffer.cpp
        myvk/Framebuffer.hpp
        myvk/DescriptorPool.cpp
        myvk/DescriptorPool.hpp
        myvk/DescriptorSet.cpp
        myvk/DescriptorSet.hpp
        myvk/FrameManager.cpp
        myvk/FrameManager.hpp
        myvk/Sampler.cpp
        myvk/Sampler.hpp
        myvk/ObjectTracker.cpp
        myvk/ObjectTracker.hpp
        myvk/QueryPool.cpp
        myvk/QueryPool.hpp
        myvk/ImGuiRenderer.cpp
        myvk/ImGuiRenderer.hpp
        myvk/FramebufferBase.cpp
        myvk/FramebufferBase.hpp
        myvk/ImagelessFramebuffer.cpp
        myvk/ImagelessFramebuffer.hpp)

target_include_directories(client_dep PUBLIC .)
target_link_libraries(client_dep PUBLIC glfw FastNoise)
target_compile_definitions(client_dep PUBLIC VK_NO_PROTOTYPES)
