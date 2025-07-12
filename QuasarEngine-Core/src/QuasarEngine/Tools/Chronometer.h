#pragma once

#include <chrono>

namespace QuasarEngine
{
	struct ElapsedTime
	{
		double seconds;
		double milliseconds;
		double microseconds;
	};

	class Chronometer
	{
	private:
		std::chrono::system_clock::time_point m_startTime;
		ElapsedTime m_elapsedTime;

		bool m_paused;

	public:
		 Chronometer(bool start = true);

		void start();
		void stop();
		void reset();
		void restart();

		ElapsedTime getElapsedTime() const;
	};
}