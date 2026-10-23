#include <quasar/core/Log.h>
#include <quasar/core/ScopedTimer.h>
 
#include <utility>
 
namespace Quasar
{
    ScopedTimer::ScopedTimer(std::string name) : m_Name(std::move(name)), m_Timer() {}
 
    ScopedTimer::~ScopedTimer()
    {
        QUASAR_ENGINE_INFO("{} took {:.3f} ms", m_Name, m_Timer.ElapsedMilliseconds());
    }
}
