add_library(efengine_renderer OBJECT)
target_sources(efengine_renderer PRIVATE Renderer.cpp Context.cpp VertexLayout.cpp Buffer.cpp Shader.cpp VertexArray.cpp IndexBuffer.cpp Texture.cpp stb_image_impl.cpp Material.cpp Vertex.cpp Mesh.cpp Model.cpp Framebuffer.cpp PostChain.cpp TonemapPass.cpp BloomPass.cpp FxaaPass.cpp Cubemap.cpp Environment.cpp SkyboxPass.cpp ShadowMath.cpp ShadowMap.cpp ShadowPass.cpp)
target_include_directories(efengine_renderer PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(efengine_renderer PUBLIC efengine_core glfw glad glm::glm stb)