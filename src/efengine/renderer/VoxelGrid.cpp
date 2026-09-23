#include "efengine/renderer/VoxelGrid.h"

#include <efengine/core/Log.h>

#include <utility>
#include <vector>

namespace efengine {
namespace renderer {

    VoxelGrid VoxelGrid::Create(const VoxelGridDesc& desc) {
        efecom::Texture3DStorageDesc d;
        d.width = d.height = d.depth = desc.resolution;

        d.format = efecom::TextureFormat::RGBA8;
        const u32 albedo = efecom::CreateTexture3DStorage(d);

        d.format = efecom::TextureFormat::RG8;
        const u32 normal = efecom::CreateTexture3DStorage(d);

        VoxelGrid g(desc, albedo, normal);
        g.Clear();

        EF_LOG_INFO("VoxelGrid: %u^3 voxeles de %.3f m (%.1f m de lado), %.1f MB",
                    desc.resolution, desc.voxelSize, GridExtent(desc),
                    static_cast<f64>(g.memoryBytes()) / (1024.0 * 1024.0));
        return g;
    }

    VoxelGrid::VoxelGrid(const VoxelGridDesc& desc, u32 albedo, u32 normal)
        : m_desc(desc), m_albedo(albedo), m_normal(normal) {}

    VoxelGrid::~VoxelGrid() {
        if (m_albedo != 0u) efecom::DestroyTexture(m_albedo);
        if (m_normal != 0u) efecom::DestroyTexture(m_normal);
    }

    VoxelGrid::VoxelGrid(VoxelGrid&& o) noexcept
        : m_desc(o.m_desc)
        , m_albedo(std::exchange(o.m_albedo, 0u))
        , m_normal(std::exchange(o.m_normal, 0u)) {}

    VoxelGrid& VoxelGrid::operator=(VoxelGrid&& o) noexcept {
        if (this != &o) {
            if (m_albedo != 0u) efecom::DestroyTexture(m_albedo);
            if (m_normal != 0u) efecom::DestroyTexture(m_normal);
            m_desc   = o.m_desc;
            m_albedo = std::exchange(o.m_albedo, 0u);
            m_normal = std::exchange(o.m_normal, 0u);
        }
        return *this;
    }

    u64 VoxelGrid::memoryBytes() const {
        const u64 n = static_cast<u64>(m_desc.resolution);
        return n * n * n * (4u + 2u);   // RGBA8 + RG8
    }

    void VoxelGrid::Clear() {
        if (m_albedo != 0u) efecom::ClearTexture(m_albedo);
        if (m_normal != 0u) efecom::ClearTexture(m_normal);
    }

    void VoxelGrid::BindForWrite() const {
        efecom::BindImageLayered(kAlbedoImageUnit, m_albedo, 0,
                                 efecom::ImageAccess::ReadWrite, efecom::TextureFormat::RGBA8);
        efecom::BindImageLayered(kNormalImageUnit, m_normal, 0,
                                 efecom::ImageAccess::ReadWrite, efecom::TextureFormat::RG8);
    }

    void VoxelGrid::BindForSample(u32 albedoUnit, u32 normalUnit) const {
        efecom::BindTextureUnit(m_albedo, albedoUnit);
        efecom::BindTextureUnit(m_normal, normalUnit);
    }

}
}
