#include "TextureConfigImporter.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include <QuasarEngine/Renderer/Renderer.h>

namespace QuasarEngine
{
	TextureSpecification TextureConfigImporter::ImportTextureConfig(std::filesystem::path path)
	{
		std::string filePath = path.parent_path().string() + "\\" + path.stem().string() + ".ultconf";

		TextureSpecification spec;

		if (std::filesystem::exists(filePath))
		{
			std::ifstream stream(filePath);
			std::stringstream strStream;
			strStream << stream.rdbuf();

			YAML::Node data = YAML::Load(strStream.str());

			auto wrap_s = data["WrapS"];
			if (wrap_s)
			{
				spec.wrap_s = TextureUtils::CharToTextureWrap(wrap_s.as<std::string>().c_str());
			}

			auto wrap_t = data["WrapT"];
			if (wrap_t)
			{
				spec.wrap_t = TextureUtils::CharToTextureWrap(wrap_t.as<std::string>().c_str());
			}

			auto wrap_r = data["WrapR"];
			if (wrap_r)
			{
				spec.wrap_r = TextureUtils::CharToTextureWrap(wrap_r.as<std::string>().c_str());
			}

			auto min_filter = data["MinFilter"];
			if (min_filter)
			{
				spec.min_filter_param = TextureUtils::CharToTextureFilter(min_filter.as<std::string>().c_str());
			}

			auto mag_filter = data["MagFilter"];
			if (mag_filter)
			{
				spec.mag_filter_param = TextureUtils::CharToTextureFilter(mag_filter.as<std::string>().c_str());
			}

			auto alpha = data["Alpha"];
			if (alpha)
			{
				spec.alpha = alpha.as<bool>();
			}

			auto gamma = data["Gamma"];
			if (gamma)
			{
				spec.gamma = gamma.as<bool>();
			}

			auto flip = data["Flip"];
			if (flip)
			{
				spec.flip = flip.as<bool>();
			}

			auto samples = data["Samples"];
			if (samples)
			{
				spec.Samples = samples.as<uint32_t>();
			}
		}
		else
		{
			TextureSpecification spec;

			spec.flip = true;

			YAML::Emitter out;
			out << YAML::BeginMap;
			out << YAML::Key << "WrapS" << YAML::Value << TextureUtils::TextureWrapToChar(spec.wrap_s);
			out << YAML::Key << "WrapT" << YAML::Value << TextureUtils::TextureWrapToChar(spec.wrap_t);
			out << YAML::Key << "WrapR" << YAML::Value << TextureUtils::TextureWrapToChar(spec.wrap_r);
			out << YAML::Key << "MinFilter" << YAML::Value << TextureUtils::TextureFilterToChar(spec.min_filter_param);
			out << YAML::Key << "MagFilter" << YAML::Value << TextureUtils::TextureFilterToChar(spec.mag_filter_param);
			out << YAML::Key << "Alpha" << YAML::Value << spec.alpha;
			out << YAML::Key << "Gamma" << YAML::Value << spec.gamma;
			out << YAML::Key << "Flip" << YAML::Value << spec.flip;
			out << YAML::Key << "Samples" << YAML::Value << spec.Samples;
			out << YAML::EndMap;

			std::ofstream fout(filePath);
			fout << out.c_str();
		}

		return spec;
	}

	void TextureConfigImporter::ExportTextureConfig(std::filesystem::path path, const TextureSpecification& specification)
	{
		std::string filePath = path.parent_path().string() + "\\" + path.stem().string() + ".ultconf";

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "WrapS" << YAML::Value << TextureUtils::TextureWrapToChar(specification.wrap_s);
		out << YAML::Key << "WrapT" << YAML::Value << TextureUtils::TextureWrapToChar(specification.wrap_t);
		out << YAML::Key << "WrapR" << YAML::Value << TextureUtils::TextureWrapToChar(specification.wrap_r);
		out << YAML::Key << "MinFilter" << YAML::Value << TextureUtils::TextureFilterToChar(specification.min_filter_param);
		out << YAML::Key << "MagFilter" << YAML::Value << TextureUtils::TextureFilterToChar(specification.mag_filter_param);
		out << YAML::Key << "Alpha" << YAML::Value << specification.alpha;
		out << YAML::Key << "Gamma" << YAML::Value << specification.gamma;
		out << YAML::Key << "Flip" << YAML::Value << specification.flip;
		out << YAML::Key << "Samples" << YAML::Value << specification.Samples;
		out << YAML::EndMap;
		std::ofstream fout(filePath);
		fout << out.c_str();
	}
}