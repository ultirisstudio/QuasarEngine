#include <quasar/log/Log.h>
#include <quasar/time/ScopedTimer.h>

#include <utility>

namespace Quasar
{
    ScopedTimer::ScopedTimer(std::string name) : m_Name(std::move(name)) {}

    ScopedTimer::~ScopedTimer()
    {
        QUASAR_ENGINE_INFO("{} took {:.3f} ms", m_Name, m_Timer.ElapsedMilliseconds());
    }
}
