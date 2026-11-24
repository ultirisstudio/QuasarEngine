#pragma once

#include <quasar/core/Base.h>
#include <quasar/renderer/TimestampQuery.h>

#include <cstdint>
#include <optional>

namespace Quasar
{
    class OpenGLTimestampQuery final : public TimestampQuery
    {
    public:
        static Scope<OpenGLTimestampQuery> Create();

        ~OpenGLTimestampQuery() override;

        OpenGLTimestampQuery(const OpenGLTimestampQuery&) = delete;
        OpenGLTimestampQuery& operator=(const OpenGLTimestampQuery&) = delete;
        OpenGLTimestampQuery(OpenGLTimestampQuery&&) = delete;
        OpenGLTimestampQuery& operator=(OpenGLTimestampQuery&&) = delete;

        void Record() override;

        [[nodiscard]] std::optional<std::uint64_t> GetNanoseconds() override;

    private:
        explicit OpenGLTimestampQuery(std::uint32_t query);

        std::uint32_t m_Query = 0;
        bool m_Recorded = false;
    };
}
