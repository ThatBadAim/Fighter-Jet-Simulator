#include <iostream>
#include <cassert>

#define CGLTF_IMPLEMENTATION
#include "fastjet/graphics/cgltf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "fastjet/graphics/stb_image.h"

int main() {
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    const char* candidates[] = {
        "assets/models/f16.glb",
        "../assets/models/f16.glb",
        "../../assets/models/f16.glb"
    };
    const char* actual_path = nullptr;
    cgltf_result result = cgltf_result_file_not_found;

    for (const char* p : candidates) {
        result = cgltf_parse_file(&options, p, &data);
        if (result == cgltf_result_success) {
            actual_path = p;
            break;
        }
    }

    if (result != cgltf_result_success || !actual_path) {
        std::cerr << "Failed to parse assets/models/f16.glb: error " << result << "\n";
        return 1;
    }

    result = cgltf_load_buffers(&options, data, actual_path);
    if (result != cgltf_result_success) {
        std::cerr << "Failed to load buffers: error " << result << "\n";
        cgltf_free(data);
        return 1;
    }

    std::cout << "GLB loaded successfully!\n";
    std::cout << "Meshes: " << data->meshes_count << "\n";
    std::cout << "Nodes: " << data->nodes_count << "\n";
    std::cout << "Materials: " << data->materials_count << "\n";
    std::cout << "Images: " << data->images_count << "\n";

    for (cgltf_size i = 0; i < data->images_count; ++i) {
        cgltf_image& img = data->images[i];
        if (img.buffer_view) {
            const uint8_t* bytes = (const uint8_t*)img.buffer_view->buffer->data + img.buffer_view->offset;
            int w = 0, h = 0, comp = 0;
            unsigned char* pixels = stbi_load_from_memory(bytes, (int)img.buffer_view->size, &w, &h, &comp, 4);
            if (pixels) {
                std::cout << "Image " << i << ": " << w << "x" << h << " (" << comp << " channels), size " << img.buffer_view->size << " bytes\n";
                stbi_image_free(pixels);
            } else {
                std::cout << "Image " << i << " failed to decode: " << stbi_failure_reason() << "\n";
            }
        }
    }

    cgltf_free(data);
    std::cout << "All verification passed!\n";
    return 0;
}
