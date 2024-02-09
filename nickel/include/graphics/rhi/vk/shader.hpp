#pragma once

#include "graphics/rhi/impl/shader.hpp"
#include "graphics/rhi/vk/pch.hpp"
#include "graphics/rhi/device.hpp"

namespace nickel::rhi::vulkan {

class ShaderModuleImpl: public rhi::ShaderModuleImpl {
public:
    ShaderModuleImpl(vk::Device, const std::vector<char>& code);
    void Destroy(rhi::DeviceImpl&) override;

    vk::ShaderModule module;
};

}