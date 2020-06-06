#pragma once

#include <vector>
#include <array>
#include <string_view>
#include <filesystem>
#include <DirectXMath.h>

namespace dx11_lessons
{
	struct obj_data
	{
		using position = DirectX::XMFLOAT3; // std::array<float, 3>;
		using uv_coord = DirectX::XMFLOAT2; // std::array<float, 2>;
		using normal = DirectX::XMFLOAT3; // std::array<float, 3>;
		using file_path = std::filesystem::path;
		
		std::array<position, 8> bounding_box;
		std::vector<position> vertices;
		std::vector<normal> normals;
		std::vector<uv_coord> uv_coords;
		std::vector<uint32_t> indicies;

		struct group
		{
			std::string name;
			std::string material_name;
			uint32_t index_start;
			uint32_t index_count;
		};

		std::vector<group> groups;

		std::vector<file_path> mtl_files;
	};

	struct mtl_data
	{
		using color = std::array<float, 3>;
		using file_path = std::filesystem::path;

		struct material
		{
			std::string name;

			color color_ambient;
			color color_diffuse;
			color color_specular;

			float shininess;
			float transparency;
			uint16_t illumination_type;

			file_path tex_ambient;
			file_path tex_diffuse;
			file_path tex_specular;
			file_path tex_shininess;
			file_path tex_transparency;
			file_path tex_bump;
		};

		std::vector<material> materials;
	};

	auto parse_obj(const std::vector<uint8_t> &file_data) -> obj_data;
	auto parse_mtl(const std::vector<uint8_t> &file_data) -> mtl_data;

	class obj_parser
	{
		void parse_obj(const std::vector<uint8_t> &file_data);
	};

	class mtl_parser
	{
		void parse_mtl(const std::vector<uint8_t> &file_data);
	};
}