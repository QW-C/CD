set(CD_COMMON_SRC
	Common/Clock.cpp Common/Clock.hpp
	Common/Common.hpp
	Common/Debug.cpp Common/Debug.hpp
	Common/ResourcePool.hpp
	Common/Transform.hpp
	Common/Window.cpp Common/Window.hpp
)

set(CD_GPU_SRC
	GPU/CommandBuffer.hpp
	GPU/Common.hpp
	GPU/Device.hpp
	GPU/Factory.hpp
	GPU/Shader.cpp GPU/Shader.hpp
	GPU/Utils.cpp GPU/Utils.hpp
)

set(CD_D3D12_SRC
	GPU/D3D12/Allocator.cpp GPU/D3D12/Allocator.hpp
	GPU/D3D12/Common.cpp GPU/D3D12/Common.hpp
	GPU/D3D12/Device.cpp GPU/D3D12/Device.hpp
	GPU/D3D12/Engine.cpp GPU/D3D12/Engine.hpp
	GPU/D3D12/Factory.cpp GPU/D3D12/Factory.hpp
)

set(CD_GRAPHICS_SRC
	Graphics/Common.hpp
	Graphics/Frame.cpp Graphics/Frame.hpp
	Graphics/GraphicsManager.cpp Graphics/GraphicsManager.hpp
	Graphics/Lighting.cpp Graphics/Lighting.hpp
	Graphics/Material.cpp Graphics/Material.hpp
	Graphics/Model.cpp Graphics/Model.hpp
	Graphics/Renderer.cpp Graphics/Renderer.hpp
	Graphics/RenderPipeline.cpp Graphics/RenderPipeline.hpp
	Graphics/Scene.cpp Graphics/Scene.hpp
	Graphics/Sky.cpp Graphics/Sky.hpp
)

set(CD_LOADER_SRC
	Loader/Main.hpp
	Loader/ResourceLoader.cpp Loader/ResourceLoader.hpp
)

source_group("Common" FILES ${CD_COMMON_SRC}) 
source_group("GPU" FILES ${CD_GPU_SRC})
source_group("GPU\\D3D12" FILES ${CD_D3D12_SRC})
source_group("Graphics" FILES ${CD_GRAPHICS_SRC})
source_group("Loader" FILES ${CD_LOADER_SRC})

add_library(CD STATIC)
target_sources(CD PRIVATE ${CD_COMMON_SRC} ${CD_GPU_SRC} ${CD_D3D12_SRC} ${CD_GRAPHICS_SRC} ${CD_LOADER_SRC})
target_include_directories(CD PRIVATE ${PROJECT_SOURCE_DIR}/CD)

target_link_libraries(CD PRIVATE "d3d12.lib" "dxgi.lib")

target_include_directories(CD PRIVATE ${PROJECT_SOURCE_DIR}/External/assimp/include)
target_link_directories(CD PUBLIC ${PROJECT_SOURCE_DIR}/External/assimp/lib)
target_link_libraries(CD PUBLIC "assimp-vc142-mt.lib")

target_include_directories(CD PRIVATE ${PROJECT_SOURCE_DIR}/External/DirectXTex/include)
target_link_directories(CD PUBLIC ${PROJECT_SOURCE_DIR}/External/DirectXTex/lib)
target_link_libraries(CD PUBLIC debug "DirectXTex/Debug/DirectXTex" optimized "DirectXTex/Release/DirectXTex")

target_include_directories(CD PUBLIC ${PROJECT_SOURCE_DIR}/External/dxc/include)

target_include_directories(CD PRIVATE ${PROJECT_SOURCE_DIR}/External/Nuklear/include)

