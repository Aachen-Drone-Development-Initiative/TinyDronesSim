#include <mesh.hpp>

Mesh::~Mesh()
{
    // This was very wrong
    // free(m_vertex_buffer);
    // free(m_index_buffer);
}
