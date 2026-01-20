#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QuasarEngine/Entity/Component.h>

namespace QuasarEngine
{
    enum class CameraType : uint8_t {
        Perspective = 0,
        Orthographic
    };

    class CameraComponent : public Component {
    public:
        CameraComponent() = default;

        bool Primary = false;
        CameraType Type = CameraType::Perspective;

        float FovDeg = 60.0f;
        float MinFovDeg = 15.0f;
        float MaxFovDeg = 95.0f;

        float NearZ = 0.1f;
        float FarZ = 100.0f;

        float OrthoHeight = 1.0f;

        glm::vec2 ViewportSize = { 1.0f, 1.0f };

        const glm::mat4& Projection() const { return m_projection; }

        void SetType(CameraType t) { Type = t; RebuildProjection(); }
        void SetFov(float deg) { FovDeg = glm::clamp(deg, MinFovDeg, MaxFovDeg); RebuildProjection(); }
        void SetNearFar(float n, float f) { NearZ = n; FarZ = f; RebuildProjection(); }
        void SetViewport(float w, float h) { ViewportSize = { w, h }; RebuildProjection(); }
        void SetOrthoHeight(float h) { OrthoHeight = glm::max(0.0001f, h); RebuildProjection(); }

        float Aspect() const {
            const float w = glm::max(1.0f, ViewportSize.x);
            const float h = glm::max(1.0f, ViewportSize.y);
            return w / h;
        }

        void RebuildProjection();

    private:
        glm::mat4 m_projection{ 1.0f };
    };
}

/*#pragma once

#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Scene/Camera.h>

namespace QuasarEngine
{
	class CameraComponent : public Component
	{
	private:
		std::unique_ptr<Camera> m_Camera;
	public:
		CameraComponent();

		Camera& GetCamera() { return *m_Camera; }

		bool Primary = false;

		const char* item_type = "Perspective";

		void setType(CameraType type);
	};
}*/