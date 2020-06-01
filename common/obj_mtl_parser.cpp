#include "obj_mtl_parser.h"

#include "helpers.h"

#include <cppitertools\zip.hpp>

#include <charconv>
#include <filesystem>
#include <sstream>
#include <string>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cassert>

using namespace dx11_lessons;

namespace
{
	using rule_function = std::function<void()>;
	using parsing_rules = std::unordered_map<std::string, rule_function>;
		
	using position = obj_data::position;
	using normal = obj_data::normal;
	using uv_coord = obj_data::uv_coord;
	using file_path = obj_data::file_path;

	using index_list = std::array<uint32_t, 3>;
	using vertex_list = std::vector<position>;
	using normal_list = std::vector<normal>;
	using uv_list = std::vector<uv_coord>;
	using mtl_list = std::vector<file_path>;

	struct obj_group
	{
		std::string name;
		std::string mtl_name;
		std::vector<index_list> face_indicies;
	};

	using group_list = std::vector<obj_group>;

	auto make_bounding_box(obj_data::position min_point, obj_data::position max_point)
		-> std::array<obj_data::position, 8>
	{
		return {
			min_point,
			{ min_point[0], max_point[1], max_point[2] },
			{ min_point[0], max_point[1], min_point[2] },
			{ max_point[0], max_point[1], min_point[2] },
			max_point,
			{ min_point[0], min_point[1], max_point[2] },
			{ min_point[0], min_point[1], min_point[2] },
			{ max_point[0], min_point[1], min_point[2] },
		};
	}

	auto to_obj_data(const position &min_point, const position &max_point, 
	                 const group_list &groups, const mtl_list &mtls,
	                 const vertex_list &verticies, const normal_list &normals, const uv_list &uvs)
		-> obj_data
	{
		auto output = obj_data{};

		output.bounding_box = make_bounding_box(min_point, max_point);
		output.mtl_files = mtls;

		for (auto &in_grp : groups)
		{
			auto mtl_name = in_grp.mtl_name;
			if (mtl_name == "")
			{
				mtl_name = output.groups.back().material_name;
			}

			auto &out_grp = output.groups.emplace_back();
			out_grp.name = in_grp.name;
			out_grp.material_name = mtl_name;

			out_grp.index_start = static_cast<uint32_t>(output.vertices.size());
			for (auto &&[vi, ti, ni] : in_grp.face_indicies)
			{
				auto v = verticies.at(vi);
				output.vertices.push_back(v);
				auto n = normals.at(ni);
				output.normals.push_back(n);
				auto t = uvs.at(ti);
				output.uv_coords.push_back(t);
			}
			out_grp.index_count = static_cast<uint32_t>(output.vertices.size()) - out_grp.index_start;
		}

		return output;
	}
}

auto dx11_lessons::parse_obj(const std::vector<uint8_t> &file_data) -> obj_data
{
	auto data_stream = memory_stream(reinterpret_cast<const char *>(file_data.data()),
	                                 file_data.size());

	auto min_point = position{};
	auto max_point = position{};
	auto obj_groups = group_list{};
	auto obj_v = vertex_list{};
	auto obj_vn = normal_list{};
	auto obj_vt = uv_list{};
	auto obj_mtls = mtl_list{};

	auto parse_face_string = [](const std::string_view face_str)
	{
		auto face_indicies = index_list{};

		for (auto start = std::size_t{}; auto &val : face_indicies)
		{
			auto next = face_str.find('/', start);
			auto index_str = face_str.substr(start, next - start);
			
			auto [p, ec] = std::from_chars(index_str.data(), index_str.data() + index_str.size(), val);
			assert(ec == std::errc());

			val--;
			start = ++next;
		}

		return face_indicies;
	};

	auto update_minmax_points = [&min_point, &max_point](const obj_data::position &v)
	{
		for (auto &&[ip, ap, vp] : iter::zip(min_point, max_point, v))
		{
			ip = std::min(vp, ip);
			ap = std::max(vp, ap);
		}
	};

	auto get_active_group = [&]() -> obj_group &
	{
		if (obj_groups.size() == 0)
		{
			return obj_groups.emplace_back();
		}

		return obj_groups.back();
	};
		
	auto obj_parsing_rules = parsing_rules
	{
		{ "mtllib", [&]() {
			auto path_str = std::string{};
			std::getline(data_stream, path_str);
			obj_mtls.emplace_back(trim(path_str));
		} },
		{ "usemtl", [&]() {
			auto &&grp = get_active_group();
			std::getline(data_stream, grp.mtl_name);
			grp.mtl_name = trim(grp.mtl_name);
		} },
		{ "g", [&]() {
			auto &grp = obj_groups.emplace_back();
			std::getline(data_stream, grp.name);
			grp.name = trim(grp.name);
		} },
		{ "f", [&]() {
			auto &&grp = get_active_group();
			auto face_str = std::string{};
			std::getline(data_stream, face_str);

			auto face_stream = std::stringstream(face_str);
			auto face_elem_str = std::string{};
			while (face_stream >> face_elem_str)
			{
				grp.face_indicies.push_back(parse_face_string(face_elem_str));
			}
		} },
		{ "v", [&]() {
			auto &pos = obj_v.emplace_back();
			data_stream >> pos[0] >> pos[1] >> pos[2];
			update_minmax_points(pos);
		} },
		{ "vn", [&]() {
			auto &nor = obj_vn.emplace_back();
			data_stream >> nor[0] >> nor[1] >> nor[2];
		} },
		{ "vt", [&](){
			auto &uv = obj_vt.emplace_back();
			data_stream >> uv[0] >> uv[1];
		} },
	};

	while (not data_stream.eof())
	{
		auto token = std::string{};
		data_stream >> token;

		auto apply_rule = obj_parsing_rules.find(token);
		if (apply_rule == obj_parsing_rules.end())
		{
			data_stream.ignore(file_data.size(), '\n');
			continue;
		}

		apply_rule->second();
	}

	return to_obj_data(min_point, max_point, obj_groups, obj_mtls, obj_v, obj_vn, obj_vt);
}

auto dx11_lessons::parse_mtl(const std::vector<uint8_t> &file_data) -> mtl_data
{
	auto data_stream = memory_stream(reinterpret_cast<const char *>(file_data.data()), 
	                                 file_data.size());

	auto read_color_into = [&](mtl_data::color &dest)
	{
		data_stream >> dest[0] >> dest[1] >> dest[2];
	};

	auto read_texture_into = [&](mtl_data::file_path &tex_path)
	{
		auto path_str = std::string{};
		std::getline(data_stream, path_str);
		tex_path = trim(path_str);
	};

	auto output = mtl_data{};

	auto get_active_mtl = [&]() -> mtl_data::material &
	{
		if (output.materials.size() == 0)
		{
			return output.materials.emplace_back();
		}

		return output.materials.back();
	};

	auto mtl_parsing_rules = parsing_rules
	{
		{ "newmtl", [&]() {
			auto &mtl = output.materials.emplace_back();
			std::getline(data_stream, mtl.name);
			mtl.name = trim(mtl.name);
		}},
		{ "Ka", [&]() {
			auto &&mtl = get_active_mtl();
			read_color_into(mtl.color_ambient);
		}},
		{ "Kd", [&]() {
			auto &&mtl = get_active_mtl();
			read_color_into(mtl.color_diffuse);
		}},
		{ "Ks", [&]() {
			auto &&mtl = get_active_mtl();
			read_color_into(mtl.color_specular);
		}},
		{ "Ns", [&]() {
			auto &&mtl = get_active_mtl();
			data_stream >> mtl.shininess;
		}},
		{ "Tr", [&]() {
			auto &&mtl = get_active_mtl();
			data_stream >> mtl.transparency;
		}},
		{ "d", [&]() {
			auto &&mtl = get_active_mtl();
			data_stream >> mtl.transparency;
		}},
		{ "illum", [&]() {
			auto &&mtl = get_active_mtl();
			data_stream >> mtl.illumination_type;
		}},
		{ "map_Ka", [&]() {
			auto &&mtl = get_active_mtl();
			read_texture_into(mtl.tex_ambient);
		}},
		{ "map_Kd", [&]() {
			auto &&mtl = get_active_mtl();
			read_texture_into(mtl.tex_diffuse);
		}},
		{ "map_Ks", [&]() {
			auto &&mtl = get_active_mtl();
			read_texture_into(mtl.tex_specular);
		}},
		{ "map_Ns", [&]() {
			auto &&mtl = get_active_mtl();
			read_texture_into(mtl.tex_shininess);
		}},
		{ "map_Tr", [&]() {
			auto &&mtl = get_active_mtl();
			read_texture_into(mtl.tex_transparency);
		}},
		{ "map_d", [&]() {
			auto &&mtl = get_active_mtl();
			read_texture_into(mtl.tex_transparency);
		}},
		{ "map_bump", [&]() {
			auto &&mtl = get_active_mtl();
			read_texture_into(mtl.tex_bump);
		}},
	};

	while (not data_stream.eof())
	{
		auto token = std::string{};
		data_stream >> token;

		auto rule = mtl_parsing_rules.find(token);
		if (rule == mtl_parsing_rules.end())
		{
			data_stream.ignore(file_data.size(), '\n');
			continue;
		}

		rule->second();
	}

	return output;
}
