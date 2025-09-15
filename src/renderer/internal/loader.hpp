#pragma once
#include "../renderer.hpp"
#include "helpers.hpp"
#include "vkstructs.hpp"

#include <stb/stb_image.h>
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

namespace flux::renderer::vkutil {

    std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(RendererState* state, std::filesystem::path filePath) {
        log::debug(std::format("loading gltf: {}", filePath));

        fastgltf::GltfDataBuffer data;
        data.FromPath(filePath);

        constexpr auto gltfOptions =
            fastgltf::Options::LoadGLBBuffers |
            fastgltf::Options::LoadExternalBuffers;

        fastgltf::Asset gltf;
        fastgltf::Parser parser {};

        auto load = parser.loadGltfBinary(data, filePath.parent_path(), gltfOptions);
        if (load) {
            gltf = std::move(load.get());
        } else {
            log::warn(std::format("Failed to load glTF: {} \n", fastgltf::to_underlying(load.error())));
            return {};
        }

    }
    
}