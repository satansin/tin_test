#pragma once

#include <string>
#include <string_view>

namespace tin_gen {

enum class MeshFormat { Ply, Obj };

[[nodiscard]] MeshFormat parse_mesh_format(std::string_view value);
[[nodiscard]] std::string_view mesh_format_extension(MeshFormat format);
[[nodiscard]] std::string_view mesh_format_name(MeshFormat format);

}  // namespace tin_gen
