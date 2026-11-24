#pragma once

// Changes an environment variable during a test, then puts back its previous value. Only on Linux: changing the
// environment needs POSIX functions. SDL keeps its own copy of the environment once it has read it, so the copy is
// changed too.

#include <quasar/core/PlatformDetection.h>

#ifdef QUASAR_PLATFORM_LINUX

#include <SDL3/SDL_stdinc.h>

#include <cstdlib>
#include <optional>
#include <string>

namespace TestHelpers
{
    class ScopedEnvironmentVariable
    {
    public:
        ScopedEnvironmentVariable(const char* name, const std::optional<std::string>& value) : m_Name(name)
        {
            if (const char* previous = std::getenv(name); previous != nullptr)
            {
                m_Previous = previous;
            }
            Set(value);
        }

        ~ScopedEnvironmentVariable() { Set(m_Previous); }

        ScopedEnvironmentVariable(const ScopedEnvironmentVariable&) = delete;
        ScopedEnvironmentVariable& operator=(const ScopedEnvironmentVariable&) = delete;
        ScopedEnvironmentVariable(ScopedEnvironmentVariable&&) = delete;
        ScopedEnvironmentVariable& operator=(ScopedEnvironmentVariable&&) = delete;

    private:
        // Changing the environment is unsafe while another thread reads it. The tests that use it start no thread.
        void Set(const std::optional<std::string>& value) const
        {
            if (value)
            {
                // NOLINTNEXTLINE(concurrency-mt-unsafe)
                setenv(m_Name.c_str(), value->c_str(), 1);
                SDL_SetEnvironmentVariable(SDL_GetEnvironment(), m_Name.c_str(), value->c_str(), true);
            }
            else
            {
                // NOLINTNEXTLINE(concurrency-mt-unsafe)
                unsetenv(m_Name.c_str());
                SDL_UnsetEnvironmentVariable(SDL_GetEnvironment(), m_Name.c_str());
            }
        }

        std::string m_Name;
        std::optional<std::string> m_Previous;
    };
}

#endif
