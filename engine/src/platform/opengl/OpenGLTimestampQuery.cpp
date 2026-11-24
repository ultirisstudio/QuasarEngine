#include <platform/opengl/OpenGLTimestampQuery.h>

#include <quasar/log/Log.h>

#include <glad/gl.h>

namespace Quasar
{
    OpenGLTimestampQuery::OpenGLTimestampQuery(std::uint32_t query) : m_Query(query) {}

    OpenGLTimestampQuery::~OpenGLTimestampQuery()
    {
        glDeleteQueries(1, &m_Query);
    }

    Scope<OpenGLTimestampQuery> OpenGLTimestampQuery::Create()
    {
        GLuint query = 0;
        glCreateQueries(GL_TIMESTAMP, 1, &query);
        if (query == 0)
        {
            QUASAR_ENGINE_ERROR("Cannot create an OpenGL timestamp query");
            return nullptr;
        }
        return Scope<OpenGLTimestampQuery>(new OpenGLTimestampQuery(query));
    }

    void OpenGLTimestampQuery::Record()
    {
        glQueryCounter(m_Query, GL_TIMESTAMP);
        m_Recorded = true;
    }

    std::optional<std::uint64_t> OpenGLTimestampQuery::GetNanoseconds()
    {
        if (!m_Recorded)
        {
            return std::nullopt;
        }
        GLint available = GL_FALSE;
        glGetQueryObjectiv(m_Query, GL_QUERY_RESULT_AVAILABLE, &available);
        if (available == GL_FALSE)
        {
            return std::nullopt;
        }
        GLuint64 time = 0;
        glGetQueryObjectui64v(m_Query, GL_QUERY_RESULT, &time);
        return time;
    }
}
