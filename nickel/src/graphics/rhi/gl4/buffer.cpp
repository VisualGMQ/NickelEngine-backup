#include "graphics/rhi/gl4/buffer.hpp"
#include "graphics/rhi/gl4/glcall.hpp"

namespace nickel::rhi::gl4 {

GLenum getBufferType(const Buffer::Descriptor& desc) {
    if (desc.usage & BufferUsage::Vertex) {
        return GL_ARRAY_BUFFER;
    }
    if (desc.usage & BufferUsage::Index) {
        return GL_ELEMENT_ARRAY_BUFFER;
    }
    if (desc.usage & BufferUsage::Indirect) {
        return GL_DISPATCH_INDIRECT_BUFFER;
    }
    if (desc.usage & BufferUsage::Storage) {
        return GL_SHADER_STORAGE_BUFFER;
    }
    if (desc.usage & BufferUsage::Uniform) {
        return GL_UNIFORM_BUFFER;
    }
    if (desc.usage & BufferUsage::QueryResolve) {
        return GL_QUERY_BUFFER;
    }
    return GL_ARRAY_BUFFER;
}

BufferImpl::BufferImpl(const Buffer::Descriptor& desc)
    : type_{getBufferType(desc)}, size_{desc.size}, usage_{desc.usage} {
    GL_CALL(glGenBuffers(1, &id));
    Bind();
    GL_CALL(glBufferData(type_, desc.size, 0, GL_STATIC_DRAW));
    if (desc.mappedAtCreation) {
        if (desc.usage & BufferUsage::MapRead &&
            desc.usage & BufferUsage::MapWrite) {
            MapAsync(
                Flags<Buffer::Mode>(Buffer::Mode::Read) | Buffer::Mode::Write,
                0, desc.size);
        } else if (desc.usage & BufferUsage::MapRead) {
            MapAsync(Buffer::Mode::Read, 0, desc.size);
        } else if (desc.usage & BufferUsage::MapWrite) {
            MapAsync(Buffer::Mode::Write, 0, desc.size);
        } else {
            MapAsync(
                Buffer::Mode::Write, 0, desc.size);
        }
    }
    Unbind();
}

BufferImpl::~BufferImpl() {
    GL_CALL(glDeleteBuffers(1, &id));
}

void BufferImpl::Bind() const {
    GL_CALL(glBindBuffer(type_, id));
}

void BufferImpl::Unbind() const {
    GL_CALL(glBindBuffer(type_, 0));
}

enum Buffer::MapState BufferImpl::MapState() const {
    return mapState_;
};

uint64_t BufferImpl::Size() const {
    return size_;
};

void BufferImpl::Unmap() {
    Bind();
    glUnmapBuffer(type_);
    Unbind();
};

void BufferImpl::MapAsync(Flags<Buffer::Mode> mode, uint64_t offset,
                          uint64_t size) {
    Bind();
    if (mapState_ != Buffer::MapState::Unmapped) {
        Unmap();
    }
    GLenum access = 0;
    if (mode & Buffer::Mode::Read && mode & Buffer::Mode::Write) {
        access |= GL_MAP_READ_BIT|GL_MAP_WRITE_BIT;
    } else if (mode & Buffer::Mode::Read) {
        access = GL_MAP_READ_BIT;
    } else {
        access = GL_MAP_WRITE_BIT;
    }
    map_ = glMapBufferRange(type_, offset, size, access|GL_MAP_COHERENT_BIT);
    mapState_ = Buffer::MapState::Mapped;
    Unbind();
};

void* BufferImpl::GetMappedRange() {
    return GetMappedRange(0, size_);
};

void* BufferImpl::GetMappedRange(uint64_t offset) {
    return GetMappedRange(offset, size_ - offset);
};

void* BufferImpl::GetMappedRange(uint64_t offset, uint64_t size) {
    return (char*)map_ + offset;
};

}  // namespace nickel::rhi::gl4