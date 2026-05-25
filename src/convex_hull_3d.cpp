#include "tin_gen/convex_hull_3d.hpp"

#include <libqhullcpp/Qhull.h>
#include <libqhullcpp/QhullFacet.h>
#include <libqhullcpp/QhullVertex.h>
#include <libqhullcpp/QhullVertexSet.h>

#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace tin_gen {

TinMesh convex_hull_3d(const std::vector<std::array<double, 3>>& input_points) {
  if (input_points.size() < 4) {
    throw std::runtime_error("Need at least 4 points for a 3D convex hull.");
  }

  std::vector<double> coords;
  coords.reserve(input_points.size() * 3);
  for (const auto& p : input_points) {
    coords.push_back(p[0]);
    coords.push_back(p[1]);
    coords.push_back(p[2]);
  }

  try {
    orgQhull::Qhull qhull;
    qhull.runQhull("", 3, static_cast<int>(input_points.size()), coords.data(), "Qt");

    TinMesh mesh;
    std::unordered_map<int, std::size_t> vertex_index;

    for (const orgQhull::QhullVertex& vertex : qhull.vertexList()) {
      const orgQhull::QhullPoint& point = vertex.point();
      const double* c = point.coordinates();
      const int id = point.id();
      vertex_index[id] = mesh.vertices.size();
      mesh.vertices.push_back({c[0], c[1], c[2]});
    }

    for (const orgQhull::QhullFacet& facet : qhull.facetList()) {
      if (!facet.isGood()) {
        continue;
      }
      orgQhull::QhullVertexSet vertices = facet.vertices();
      const int n = vertices.count();
      if (n < 3) {
        continue;
      }

      const auto index_for = [&](const orgQhull::QhullVertex& v) -> std::size_t {
        const auto it = vertex_index.find(v.point().id());
        if (it == vertex_index.end()) {
          throw std::runtime_error("Qhull returned an unexpected vertex id.");
        }
        return it->second;
      };

      const std::size_t v0 = index_for(vertices[0]);
      for (int i = 1; i < n - 1; ++i) {
        mesh.faces.push_back({v0, index_for(vertices[i]), index_for(vertices[i + 1])});
      }
    }

    if (mesh.faces.empty()) {
      throw std::runtime_error("Convex hull is empty.");
    }
    return mesh;
  } catch (const orgQhull::QhullError& ex) {
    throw std::runtime_error(std::string("Qhull failed: ") + ex.what());
  }
}

}  // namespace tin_gen
