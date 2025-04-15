#pragma once

#include <utils/Entity.h>
#include <filameshio/MeshReader.h>

namespace futils = utils;

struct Environment;

struct Mesh {
    Mesh(futils::Entity renderable, filament::VertexBuffer* vertex_buffer, filament::IndexBuffer* index_buffer, Environment* env)
        : m_renderable(renderable), m_vertex_buffer(vertex_buffer), m_index_buffer(index_buffer), env(env) {}
    Mesh(filamesh::MeshReader::Mesh fmesh, Environment* env)
        : m_renderable(fmesh.renderable), m_vertex_buffer(fmesh.vertexBuffer), m_index_buffer(fmesh.indexBuffer), env(env) {}
    ~Mesh();
    
    futils::Entity m_renderable;
    filament::VertexBuffer* m_vertex_buffer;
    filament::IndexBuffer* m_index_buffer;

    Environment* env;
};
