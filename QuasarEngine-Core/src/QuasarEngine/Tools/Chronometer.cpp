#include "qepch.h"
#include <QuasarEngine/Tools/Chronometer.h>

QuasarEngine::Chronometer::Chronometer(bool start) :
	m_startTime(),
	m_elapsedTime(),
	m_paused()
{
	reset();

	if(start)
		this->start();
}

void QuasarEngine::Chronometer::start()
{
	m_startTime = std::chrono::system_clock::now();

	m_paused = false;
}
void QuasarEngine::Chronometer::stop()
{
	std::chrono::microseconds duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - m_startTime);

	m_elapsedTime.seconds += static_cast<float>(duration.count()) / std::chrono::microseconds::period::den;
	m_elapsedTime.milliseconds += static_cast<float>(duration.count()) / std::chrono::milliseconds::period::den;
	m_elapsedTime.microseconds += static_cast<float>(duration.count()) / std::chrono::seconds::period::den;
	
	m_paused = true;
}
void QuasarEngine::Chronometer::reset()
{
	m_startTime = std::chrono::system_clock::time_point(std::chrono::microseconds::zero());

	m_elapsedTime.seconds = 0.0;
	m_elapsedTime.milliseconds = 0.0;
	m_elapsedTime.microseconds = 0.0;

	m_paused = true;
}

void QuasarEngine::Chronometer::restart()
{
	reset();
	start();
}

QuasarEngine::ElapsedTime QuasarEngine::Chronometer::getElapsedTime() const
{
	if (m_paused)
		return m_elapsedTime;

	std::chrono::microseconds duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - m_startTime);

	return ElapsedTime(
		{
			m_elapsedTime.seconds + static_cast<float>(duration.count()) / std::chrono::microseconds::period::den,
			m_elapsedTime.milliseconds + static_cast<float>(duration.count()) / std::chrono::milliseconds::period::den,
			m_elapsedTime.microseconds + static_cast<float>(duration.count()) / std::chrono::seconds::period::den
		}
	);
}