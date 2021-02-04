#pragma once

#include <CD/GPU/Common.hpp>
#include <memory>
#include <vector>

namespace CD {

struct Texture;
class Frame;
class MaterialInstance;
class MaterialSystem;
struct MaterialInstanceDesc;
class Mesh;
class Model;

class ResourceLoader {
public:
	ResourceLoader(Frame&);
	~ResourceLoader();

	const Texture* load_texture2d(const wchar_t* path, GPU::BufferFormat, GPU::TextureDimension);
	MaterialInstance* create_material(MaterialSystem&, const MaterialInstanceDesc&);

	const Model* load_model(const char* path, const MaterialInstance&);
private:
	Frame& frame;

	std::vector<std::unique_ptr<Texture>> textures;
	std::vector<std::unique_ptr<MaterialInstance>> materials;

	std::vector<std::unique_ptr<Mesh>> meshes;
	std::vector<std::unique_ptr<Model>> models;
};

}