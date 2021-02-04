#pragma once

#include <CD/Loader/ResourceLoader.hpp>
#include <CD/Graphics/Frame.hpp>
#include <CD/Graphics/Model.hpp>
#include <CD/Graphics/Scene.hpp>
#include <CD/Common/Debug.hpp>
#include <CD/Common/Transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <DirectXTex/DirectXTex.h>

namespace CD {

ResourceLoader::ResourceLoader(Frame& frame) :
	frame(frame) {
}

ResourceLoader::~ResourceLoader() {
	for(auto& texture : textures) {
		frame.get_device().destroy_texture(texture->handle);
	}
}

const Texture* ResourceLoader::load_texture2d(const wchar_t* path, GPU::BufferFormat format, GPU::TextureDimension dimension) {
	using namespace DirectX;

	TexMetadata metadata;
	ScratchImage image;

	CD_ASSERT(SUCCEEDED(LoadFromDDSFile(path, DDS_FLAGS_FORCE_RGB, &metadata, image)));

	GPU::TextureDesc desc = GPU::texture_desc_defaults(static_cast<std::uint16_t>(metadata.width), static_cast<std::uint16_t>(metadata.height), format, dimension, static_cast<std::uint16_t>(metadata.mipLevels), static_cast<std::uint16_t>(metadata.arraySize));
	auto& texture = textures.emplace_back(std::make_unique<Texture>());
	texture->handle = frame.get_device().create_texture(desc);
	texture->desc = desc;

	CopyContext& copy_context = frame.get_copy_context();

	for(std::uint16_t mip = 0; mip < metadata.mipLevels; ++mip) {
		for(std::uint16_t index = 0; index < metadata.arraySize; ++index) {

			GPU::TextureView view = GPU::texture_view_defaults(texture->handle, texture->desc, mip, index, 0);
			const Image* img = image.GetImage(mip, index, 0);

			copy_context.upload_texture_slice(texture.get(), view, img->pixels, static_cast<std::uint16_t>(img->width), static_cast<std::uint16_t>(img->height), static_cast<std::uint32_t>(img->rowPitch));
		}
	}

	copy_context.flush();

	return texture.get();
}

MaterialInstance* ResourceLoader::create_material(MaterialSystem& allocator, const MaterialInstanceDesc& desc) {
	auto& material = materials.emplace_back(std::make_unique<MaterialInstance>(allocator, desc));
	return material.get();
}

constexpr Vector3 to_vector(const aiVector3D& u) {
	return Vector3(u.x, u.y, u.z);
}

constexpr std::uint32_t strides[] {
	sizeof(Vector3),
	sizeof(Vector3),
	sizeof(Vector3),
	sizeof(Vector2)
};

const Model* ResourceLoader::load_model(const char* path, const MaterialInstance& material) {
	Assimp::Importer importer;

	auto scene_model = importer.ReadFile(path, aiPostProcessSteps::aiProcess_CalcTangentSpace | aiPostProcessSteps::aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_FlipUVs | aiProcess_FlipWindingOrder | aiPostProcessSteps::aiProcess_GenBoundingBoxes);

	GPU::Device& device = frame.get_device();
	CopyContext& copy_context = frame.get_copy_context();

	auto& model = models.emplace_back(std::make_unique<Model>());

	for(std::size_t mesh_index = 0; mesh_index < scene_model->mNumMeshes; ++mesh_index) {
		const auto mesh = scene_model->mMeshes[mesh_index];

		std::vector<std::uint32_t> indices;
		indices.reserve(3 * mesh->mNumFaces);
		for(std::size_t face = 0; face < mesh->mNumFaces; ++face) {
			for(std::size_t i = 0; i < 3; ++i) {
				indices.push_back(mesh->mFaces[face].mIndices[i]);
			}
		}

		std::unique_ptr<IndexedInputBuffer> buffer = std::make_unique<IndexedInputBuffer>();

		GPU::BufferDesc index_buffer_desc {sizeof(std::uint32_t) * indices.size(), GPU::BufferStorage::Device, GPU::BindFlags_IndexBuffer};
		buffer->index_buffer = device.create_buffer(index_buffer_desc);
		buffer->index_buffer_size = static_cast<std::uint32_t>(index_buffer_desc.size);
		buffer->num_indices = static_cast<std::uint32_t>(indices.size());
		GPU::BufferView index_buffer_view {buffer->index_buffer, GPU::BufferFormat::R32_UINT, 0, buffer->index_buffer_size};
		copy_context.upload_buffer(index_buffer_view, indices.data());

		std::uint32_t input_buffer_size = 0;
		for(std::size_t i = 0; i < MeshVertexElement_Count; ++i) {
			buffer->vertex_buffer_strides[i] = strides[i];

			input_buffer_size += strides[i] * mesh->mNumVertices;
		}

		GPU::BufferDesc input_buffer_desc {
			input_buffer_size,
			GPU::BufferStorage::Device,
			GPU::BindFlags_VertexBuffer
		};
		buffer->input_buffer = device.create_buffer(input_buffer_desc);

		buffer->num_vertices = mesh->mNumVertices;

		GPU::BufferView views[MeshVertexElement_Count] {};

		buffer->vertex_buffer_sizes[MeshVertexElement_Position] = sizeof(Vector3) * mesh->mNumVertices;

		buffer->vertex_buffer_offsets[MeshVertexElement_Normal] = sizeof(Vector3) * mesh->mNumVertices;
		buffer->vertex_buffer_sizes[MeshVertexElement_Normal] = sizeof(Vector3) * mesh->mNumVertices;

		buffer->vertex_buffer_offsets[MeshVertexElement_Tangent] = 2 * sizeof(Vector3) * mesh->mNumVertices;
		buffer->vertex_buffer_sizes[MeshVertexElement_Tangent] = sizeof(Vector3) * mesh->mNumVertices;

		buffer->vertex_buffer_offsets[MeshVertexElement_TexCoords] = 3 * sizeof(Vector3) * mesh->mNumVertices;
		buffer->vertex_buffer_sizes[MeshVertexElement_TexCoords] = sizeof(Vector2) * mesh->mNumVertices;

		for(std::size_t i = 0; i < MeshVertexElement_Count; ++i) {
			views[i].buffer = buffer->input_buffer;
			views[i].offset = buffer->vertex_buffer_offsets[i];
			views[i].size = buffer->vertex_buffer_sizes[i];
		}

		std::vector<Vector2> uv_buffer;
		uv_buffer.reserve(mesh->mNumVertices);
		for(std::size_t i = 0; i < mesh->mNumVertices; ++i) {
			const auto& uv = mesh->mTextureCoords[0][i];
			uv_buffer.push_back({uv.x, uv.y});
		}
		copy_context.upload_buffer(views[MeshVertexElement_Position], mesh->mVertices);
		copy_context.upload_buffer(views[MeshVertexElement_Normal], mesh->mNormals);
		copy_context.upload_buffer(views[MeshVertexElement_Tangent], mesh->mTangents);
		copy_context.upload_buffer(views[MeshVertexElement_TexCoords], uv_buffer.data());

		copy_context.flush();

		AABB aabb = {to_vector(mesh->mAABB.mMin), to_vector(mesh->mAABB.mMax)};
		auto& m = meshes.emplace_back(std::make_unique<Mesh>(device, std::move(buffer), aabb));
		model->add_mesh(m.get(), &material);
	}

	return model.get();
}

}