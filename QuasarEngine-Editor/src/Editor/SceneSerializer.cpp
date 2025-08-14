#include "SceneSerializer.h"
#include "Tools/Utils.h"

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/AllComponents.h>

#include <QuasarEngine/Core/Input.h>
#include <QuasarEngine/Core/KeyCodes.h>

#include "Importer/TextureConfigImporter.h"

#include <fstream>

namespace YAML
{
	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
			{
				return false;
			}
			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.r);
			node.push_back(rhs.g);
			node.push_back(rhs.b);
			node.push_back(rhs.a);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
			{
				return false;
			}
			rhs.r = node[0].as<float>();
			rhs.g = node[1].as<float>();
			rhs.b = node[2].as<float>();
			rhs.a = node[3].as<float>();
			return true;
		}
	};
}

namespace QuasarEngine
{
	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.r << v.g << v.b << v.a << YAML::EndSeq;
		return out;
	}

	void SceneSerializer::Serialize(const std::string& filepath) const {
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << m_SceneObject->GetName();
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		for (auto e : m_SceneObject->GetScene().GetAllEntitiesWith<IDComponent>()) {
			Entity entity(e, m_SceneObject->GetScene().GetRegistry());
			if (entity.GetComponent<HierarchyComponent>().m_Parent == UUID::Null())
				SerializeEntity(out, entity, m_AssetPath.string());
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;
		std::ofstream fout(filepath);
		if (!fout)
			throw std::runtime_error("Failed to open file for writing: " + filepath);
		fout << out.c_str();
	}

	bool SceneSerializer::Deserialize(const std::string& filepath) {
		std::ifstream stream(filepath);
		if (!stream)
			return false;
		std::stringstream strStream;
		strStream << stream.rdbuf();
		YAML::Node data;
		try {
			data = YAML::Load(strStream.str());
		}
		catch (const YAML::ParserException&) {
			return false;
		}
		if (!data["Scene"] || !data["Entities"])
			return false;
		
		m_SceneObject->SetName(data["Scene"].as<std::string>());
		m_SceneObject->SetPath(filepath);
		auto& scene = m_SceneObject->GetScene();
		auto entities = data["Entities"];
		if (!LoadEntities(entities, scene, m_AssetPath.string()))
			return false;
		if (!SetupHierarchy(entities, scene))
			return false;
		return true;
	}

	bool SceneSerializer::LoadEntities(const YAML::Node& entities, Scene& scene, const std::string& assetPath) {
		for (const auto& entityNode : entities) {
			try {
				std::string name = entityNode["Entity"] ? entityNode["Entity"].as<std::string>() : "Entity";
				UUID uuid = entityNode["ID"] ? entityNode["ID"].as<uint64_t>() : UUID();
				Entity entity = scene.CreateEntityWithUUID(uuid, name);

				const auto& components = entityNode["Components"];
				if (!components) continue;

				for (const auto& component : components) {
					if (!component.IsMap()) continue;
					for (const auto& it : component) {
						const std::string& key = it.first.as<std::string>();
						const YAML::Node& value = it.second;

						if (key == "TransformComponent") {
							auto& c = entity.AddOrReplaceComponent<TransformComponent>();
							c.Position = value["Position"] ? value["Position"].as<glm::vec3>() : glm::vec3(0.f);
							c.Rotation = value["Rotation"] ? value["Rotation"].as<glm::vec3>() : glm::vec3(0.f);
							c.Scale = value["Scale"] ? value["Scale"].as<glm::vec3>() : glm::vec3(1.f);
						}
						else if (key == "MeshRendererComponent") {
							auto& c = entity.AddOrReplaceComponent<MeshRendererComponent>();
							if (value["Rendered"])
								c.m_Rendered = value["Rendered"].as<bool>();
						}
						else if (key == "MeshComponent") {
							std::string path = assetPath + "/" + (value["Path"] ? value["Path"].as<std::string>() : "");
							std::string mname = value["Name"] ? value["Name"].as<std::string>() : "";
							if (!Renderer::m_SceneData.m_AssetManager->isAssetLoaded(path)) {
								AssetToLoad modelAsset;
								modelAsset.id = path;
								modelAsset.type = MODEL;
								Renderer::m_SceneData.m_AssetManager->loadAsset(modelAsset);
							}
							auto& comp = entity.AddOrReplaceComponent<MeshComponent>(mname, nullptr, path);
							AssetToLoad meshAsset;
							meshAsset.id = path;
							meshAsset.type = MESH;
							meshAsset.handle = &comp;
							Renderer::m_SceneData.m_AssetManager->loadAsset(meshAsset);
						}
						else if (key == "MaterialComponent") {
							MaterialSpecification spec;
							spec.Albedo = value["albedo"] ? value["albedo"].as<glm::vec4>() : glm::vec4(1.f);
							spec.Metallic = value["metallic"] ? value["metallic"].as<float>() : 0.f;
							spec.Roughness = value["roughness"] ? value["roughness"].as<float>() : 0.5f;
							spec.AO = value["ao"] ? value["ao"].as<float>() : 1.f;
							auto tryLoadTex = [&](const char* key, std::optional<std::string>& outTex) {
								if (value[key]) {
									std::string texPath = std::filesystem::path(assetPath + "/" + value[key].as<std::string>()).generic_string();
									outTex = texPath;
									if (!Renderer::m_SceneData.m_AssetManager->isAssetLoaded(texPath)) {
										TextureSpecification spec = TextureConfigImporter::ImportTextureConfig(texPath);
										AssetToLoad asset;
										asset.id = texPath;
										asset.type = TEXTURE;
										asset.spec = spec;
										Renderer::m_SceneData.m_AssetManager->loadAsset(asset);
									}
								}
								};
							tryLoadTex("albedoMap", spec.AlbedoTexture);
							tryLoadTex("normalMap", spec.NormalTexture);
							tryLoadTex("metallicMap", spec.MetallicTexture);
							tryLoadTex("roughnessMap", spec.RoughnessTexture);
							tryLoadTex("aoMap", spec.AOTexture);
							entity.AddOrReplaceComponent<MaterialComponent>(spec);
						}
						else if (key == "LightComponent") {
							std::string type = value["lightType"] ? value["lightType"].as<std::string>() : "";
							if (type == "Directional") {
								auto& c = entity.AddOrReplaceComponent<LightComponent>(LightComponent::LightType::DIRECTIONAL);
								c.directional_light.color = value["color"] ? value["color"].as<glm::vec3>() : glm::vec3(1.f);
								if (value["power"])
									c.directional_light.power = value["power"].as<float>();
							}
							if (type == "Point") {
								auto& c = entity.AddOrReplaceComponent<LightComponent>(LightComponent::LightType::POINT);
								c.point_light.color = value["color"] ? value["color"].as<glm::vec3>() : glm::vec3(1.f);
								if (value["attenuation"])
									c.point_light.attenuation = value["attenuation"].as<float>();
								if (value["power"])
									c.point_light.power = value["power"].as<float>();
							}
						}
						else if (key == "CameraComponent") {
							auto& c = entity.AddOrReplaceComponent<CameraComponent>();
							c.GetCamera().Init(&entity.GetComponent<TransformComponent>());
							if (value["fov"])
								c.GetCamera().SetFov(value["fov"].as<float>());
							if (value["primary"])
								c.Primary = value["primary"].as<bool>();
						}
						else if (key == "RigidBodyComponent") {
							auto& c = entity.AddOrReplaceComponent<RigidBodyComponent>();
							c.Init();
							if (value["enableGravity"])
								c.enableGravity = value["enableGravity"].as<bool>();
							if (value["bodyType"])
								c.bodyTypeString = value["bodyType"].as<std::string>();
							if (value["linearAxisFactor_X"])
								c.m_LinearAxisFactorX = value["linearAxisFactor_X"].as<bool>();
							if (value["linearAxisFactor_Y"])
								c.m_LinearAxisFactorY = value["linearAxisFactor_Y"].as<bool>();
							if (value["linearAxisFactor_Z"])
								c.m_LinearAxisFactorZ = value["linearAxisFactor_Z"].as<bool>();
							if (value["angularAxisFactor_X"])
								c.m_AngularAxisFactorX = value["angularAxisFactor_X"].as<bool>();
							if (value["angularAxisFactor_Y"])
								c.m_AngularAxisFactorY = value["angularAxisFactor_Y"].as<bool>();
							if (value["angularAxisFactor_Z"])
								c.m_AngularAxisFactorZ = value["angularAxisFactor_Z"].as<bool>();
							c.UpdateEnableGravity();
							c.UpdateBodyType();
							c.UpdateLinearAxisFactor();
							c.UpdateAngularAxisFactor();
						}
						else if (key == "MeshColliderComponent") {
							auto& c = entity.AddOrReplaceComponent<MeshColliderComponent>();
							c.m_IsConvex = value["IsConvex"] ? value["IsConvex"].as<bool>() : false;
							c.Generate();
						}
						else if (key == "BoxColliderComponent") {
							auto& c = entity.AddOrReplaceComponent<BoxColliderComponent>();
							c.Init();
							if (value["mass"])
								c.mass = value["mass"].as<float>();
							if (value["friction"])
								c.friction = value["friction"].as<float>();
							if (value["bounciness"])
								c.bounciness = value["bounciness"].as<float>();
							if (value["useEntityScale"])
								c.m_UseEntityScale = value["useEntityScale"].as<bool>();
							if (value["size"])
								c.m_Size = value["size"].as<glm::vec3>();
							c.UpdateColliderMaterial();
							c.UpdateColliderSize();
						}

						else if (key == "SphereColliderComponent") {
							auto& c = entity.AddOrReplaceComponent<SphereColliderComponent>();
							c.Init();
							if (value["mass"])
								c.mass = value["mass"].as<float>();
							if (value["friction"])
								c.friction = value["friction"].as<float>();
							if (value["bounciness"])
								c.bounciness = value["bounciness"].as<float>();
							if (value["radius"])
								c.Radius() = value["radius"].as<float>();
							c.UpdateColliderMaterial();
							c.UpdateColliderSize();
						}

						else if (key == "CapsuleColliderComponent") {
							auto& c = entity.AddOrReplaceComponent<CapsuleColliderComponent>();
							c.Init();
							if (value["mass"])
								c.mass = value["mass"].as<float>();
							if (value["friction"])
								c.friction = value["friction"].as<float>();
							if (value["bounciness"])
								c.bounciness = value["bounciness"].as<float>();
							if (value["radius"])
								c.m_Radius = value["radius"].as<float>();
							if (value["height"])
								c.m_Height = value["height"].as<float>();
							c.UpdateColliderMaterial();
							c.UpdateColliderSize();
						}

						else if (key == "ScriptComponent") {
							auto& scriptComponent = entity.AddOrReplaceComponent<ScriptComponent>();
							if (value["scriptPath"]) {
								std::string scriptPath = assetPath + "/" + value["scriptPath"].as<std::string>();
								scriptComponent.scriptPath = scriptPath;
							}
						}
					}
				}
			}
			catch (const std::exception& ex) {
				std::cerr << "Exception lors du chargement de l'entité " << ": " << ex.what() << std::endl;
				continue;
			}
			catch (...) {
				std::cerr << "Exception inconnue lors du chargement de l'entité " << std::endl;
				continue;
			}
		}
		return true;
	}

	bool SceneSerializer::SetupHierarchy(const YAML::Node& entities, Scene& scene) {
		for (const auto& entityNode : entities) {
			if (!entityNode["ID"] || !entityNode["ParentID"]) continue;
			UUID uuid = entityNode["ID"].as<uint64_t>();
			UUID parentUUID = entityNode["ParentID"].as<uint64_t>();
			if (parentUUID == UUID::Null()) continue;

			auto entityOpt = scene.GetEntityByUUID(uuid);
			auto parentOpt = scene.GetEntityByUUID(parentUUID);

			if (entityOpt && parentOpt) {
				Entity& entity = *entityOpt;
				Entity& parent = *parentOpt;
				entity.GetComponent<HierarchyComponent>().m_Parent = parent.GetUUID();
				parent.GetComponent<HierarchyComponent>().AddChild(parent.GetUUID(), entity.GetUUID());
			}
		}
		return true;
	}

	void SceneSerializer::SerializeEntity(YAML::Emitter& out, Entity entity, const std::string& assetPath) const {
		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << entity.GetName();
		out << YAML::Key << "ID" << YAML::Value << entity.GetUUID();
		uint64_t parentID = (entity.GetComponent<HierarchyComponent>().m_Parent != UUID::Null())
			? entity.GetComponent<HierarchyComponent>().m_Parent : UUID::Null();
		out << YAML::Key << "ParentID" << YAML::Value << parentID;
		out << YAML::Key << "Components" << YAML::Value << YAML::BeginSeq;
		if (entity.HasComponent<TransformComponent>()) {
			out << YAML::BeginMap;
			out << YAML::Key << "TransformComponent";
			out << YAML::Value << YAML::BeginMap;
			auto& tc = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Position" << YAML::Value << tc.Position;
			out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << tc.Scale;
			out << YAML::EndMap;
			out << YAML::EndMap;
		}
		if (entity.HasComponent<MaterialComponent>()) {
			out << YAML::BeginMap;
			out << YAML::Key << "MaterialComponent";
			out << YAML::Value << YAML::BeginMap;
			auto& mc = entity.GetComponent<MaterialComponent>();
			bool hasAlbedo = mc.GetMaterial().HasTexture(TextureType::Albedo);
			bool hasNormal = mc.GetMaterial().HasTexture(TextureType::Normal);
			bool hasMetallic = mc.GetMaterial().HasTexture(TextureType::Metallic);
			bool hasRoughness = mc.GetMaterial().HasTexture(TextureType::Roughness);
			bool hasAO = mc.GetMaterial().HasTexture(TextureType::AO);
			out << YAML::Key << "albedo" << YAML::Value << mc.GetMaterial().GetSpecification().Albedo;
			out << YAML::Key << "metallic" << YAML::Value << mc.GetMaterial().GetSpecification().Metallic;
			out << YAML::Key << "roughness" << YAML::Value << mc.GetMaterial().GetSpecification().Roughness;
			out << YAML::Key << "ao" << YAML::Value << mc.GetMaterial().GetSpecification().AO;
			if (hasAlbedo) {
				auto rel = Utils::getRelativePath(mc.GetMaterial().GetSpecification().AlbedoTexture.value(), assetPath);
				out << YAML::Key << "albedoMap" << YAML::Value << (rel.has_value() ? rel.value() : "");
			}
			if (hasNormal) {
				auto rel = Utils::getRelativePath(mc.GetMaterial().GetSpecification().NormalTexture.value(), assetPath);
				out << YAML::Key << "normalMap" << YAML::Value << (rel.has_value() ? rel.value() : "");
			}
			if (hasMetallic) {
				auto rel = Utils::getRelativePath(mc.GetMaterial().GetSpecification().MetallicTexture.value(), assetPath);
				out << YAML::Key << "metallicMap" << YAML::Value << (rel.has_value() ? rel.value() : "");
			}
			if (hasRoughness) {
				auto rel = Utils::getRelativePath(mc.GetMaterial().GetSpecification().RoughnessTexture.value(), assetPath);
				out << YAML::Key << "roughnessMap" << YAML::Value << (rel.has_value() ? rel.value() : "");
			}
			if (hasAO) {
				auto rel = Utils::getRelativePath(mc.GetMaterial().GetSpecification().AOTexture.value(), assetPath);
				out << YAML::Key << "aoMap" << YAML::Value << (rel.has_value() ? rel.value() : "");
			}
			out << YAML::EndMap;
			out << YAML::EndMap;
		}
		if (entity.HasComponent<MeshRendererComponent>()) {
			out << YAML::BeginMap;
			out << YAML::Key << "MeshRendererComponent";
			out << YAML::Value << YAML::BeginMap;
			auto& mrc = entity.GetComponent<MeshRendererComponent>();
			out << YAML::Key << "Rendered" << YAML::Value << mrc.m_Rendered;
			out << YAML::EndMap;
			out << YAML::EndMap;
		}
		if (entity.HasComponent<MeshComponent>()) {
			out << YAML::BeginMap;
			out << YAML::Key << "MeshComponent";
			out << YAML::Value << YAML::BeginMap;
			auto& mc = entity.GetComponent<MeshComponent>();
			auto rel = Utils::getRelativePath(mc.GetModelPath(), assetPath);
			out << YAML::Key << "Path" << YAML::Value << (rel.has_value() ? rel.value() : "");
			out << YAML::Key << "Name" << YAML::Value << mc.GetName();
			out << YAML::EndMap;
			out << YAML::EndMap;
		}
		if (entity.HasComponent<LightComponent>()) {
			out << YAML::BeginMap;
			out << YAML::Key << "LightComponent";
			out << YAML::Value << YAML::BeginMap;
			auto& lc = entity.GetComponent<LightComponent>();
			if (lc.lightType == LightComponent::LightType::DIRECTIONAL) {
				out << YAML::Key << "lightType" << YAML::Value << "Directional";
				out << YAML::Key << "color" << YAML::Value << lc.directional_light.color;
				out << YAML::Key << "power" << YAML::Value << lc.directional_light.power;
			}
			else if (lc.lightType == LightComponent::LightType::POINT) {
				out << YAML::Key << "lightType" << YAML::Value << "Point";
				out << YAML::Key << "color" << YAML::Value << lc.point_light.color;
				out << YAML::Key << "attenuation" << YAML::Value << lc.point_light.attenuation;
				out << YAML::Key << "power" << YAML::Value << lc.point_light.power;
			}
			out << YAML::EndMap;
			out << YAML::EndMap;
		}
		if (entity.HasComponent<CameraComponent>()) {
			out << YAML::BeginMap;
			out << YAML::Key << "CameraComponent";
			out << YAML::Value << YAML::BeginMap;
			auto& cc = entity.GetComponent<CameraComponent>();
			out << YAML::Key << "fov" << YAML::Value << cc.GetCamera().GetFov();
			out << YAML::Key << "primary" << YAML::Value << cc.Primary;
			out << YAML::EndMap;
			out << YAML::EndMap;
		}
		if (entity.HasComponent<RigidBodyComponent>()) {
			out << YAML::BeginMap;
			out << YAML::Key << "RigidBodyComponent";
			out << YAML::Value << YAML::BeginMap;
			auto& rbc = entity.GetComponent<RigidBodyComponent>();
			out << YAML::Key << "enableGravity" << YAML::Value << rbc.enableGravity;
			out << YAML::Key << "bodyType" << YAML::Value << rbc.bodyTypeString;
			out << YAML::Key << "linearAxisFactor_X" << YAML::Value << rbc.m_LinearAxisFactorX;
			out << YAML::Key << "linearAxisFactor_Y" << YAML::Value << rbc.m_LinearAxisFactorY;
			out << YAML::Key << "linearAxisFactor_Z" << YAML::Value << rbc.m_LinearAxisFactorZ;
			out << YAML::Key << "angularAxisFactor_X" << YAML::Value << rbc.m_AngularAxisFactorX;
			out << YAML::Key << "angularAxisFactor_Y" << YAML::Value << rbc.m_AngularAxisFactorY;
			out << YAML::Key << "angularAxisFactor_Z" << YAML::Value << rbc.m_AngularAxisFactorZ;
			out << YAML::EndMap;
			out << YAML::EndMap;
		}
		if (entity.HasComponent<MeshColliderComponent>()) {
			out << YAML::BeginMap;
			out << YAML::Key << "MeshColliderComponent";
			out << YAML::Value << YAML::BeginMap;
			auto& mcc = entity.GetComponent<MeshColliderComponent>();
			out << YAML::Key << "IsConvex" << YAML::Value << mcc.m_IsConvex;
			out << YAML::EndMap;
			out << YAML::EndMap;
		}
		if (entity.HasComponent<BoxColliderComponent>()) {
			out << YAML::BeginMap;
			out << YAML::Key << "BoxColliderComponent";
			out << YAML::Value << YAML::BeginMap;
			auto& bcc = entity.GetComponent<BoxColliderComponent>();
			out << YAML::Key << "mass" << YAML::Value << bcc.mass;
			out << YAML::Key << "friction" << YAML::Value << bcc.friction;
			out << YAML::Key << "bounciness" << YAML::Value << bcc.bounciness;
			out << YAML::Key << "useEntityScale" << YAML::Value << bcc.m_UseEntityScale;
			out << YAML::Key << "size" << YAML::Value << bcc.m_Size;
			out << YAML::EndMap;
			out << YAML::EndMap;
		}

		if(entity.HasComponent<SphereColliderComponent>()) {
			out << YAML::BeginMap;
			out << YAML::Key << "SphereColliderComponent";
			out << YAML::Value << YAML::BeginMap;
			auto& scc = entity.GetComponent<SphereColliderComponent>();
			out << YAML::Key << "mass" << YAML::Value << scc.mass;
			out << YAML::Key << "friction" << YAML::Value << scc.friction;
			out << YAML::Key << "bounciness" << YAML::Value << scc.bounciness;
			out << YAML::Key << "radius" << YAML::Value << scc.Radius();
			out << YAML::EndMap;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<CapsuleColliderComponent>()) {
			out << YAML::BeginMap;
			out << YAML::Key << "CapsuleColliderComponent";
			out << YAML::Value << YAML::BeginMap;
			auto& ccc = entity.GetComponent<CapsuleColliderComponent>();
			out << YAML::Key << "mass" << YAML::Value << ccc.mass;
			out << YAML::Key << "friction" << YAML::Value << ccc.friction;
			out << YAML::Key << "bounciness" << YAML::Value << ccc.bounciness;
			out << YAML::Key << "radius" << YAML::Value << ccc.m_Radius;
			out << YAML::Key << "height" << YAML::Value << ccc.m_Height;
			out << YAML::EndMap;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<ScriptComponent>())
		{
			out << YAML::BeginMap;
			out << YAML::Key << "ScriptComponent";
			out << YAML::Value << YAML::BeginMap;
			auto& sc = entity.GetComponent<ScriptComponent>();
			out << YAML::Key << "scriptPath" << YAML::Value << sc.scriptPath;
			out << YAML::EndMap;
			out << YAML::EndMap;
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;
		std::vector<UUID> childrens = entity.GetComponent<HierarchyComponent>().m_Childrens;
		for (auto& child : childrens) {
			std::optional<Entity> childEntity = Renderer::m_SceneData.m_Scene->GetEntityByUUID(child);
			if (childEntity.has_value())
			{
				SerializeEntity(out, childEntity.value(), assetPath);
			}
		}
	}
}