#include "clock.h"

#include <ratio>

using namespace dx11_lessons;
using namespace std::chrono;

namespace
{
	using hrc = high_resolution_clock;
	using ns = duration<double, std::nano>;
	using us = duration<double, std::micro>;
	using ms = duration<double, std::milli>;
	using s = duration<double, std::ratio<1>>;
}

game_clock::game_clock()
{
	timepoint_prev = hrc::now();
}

game_clock::~game_clock() = default;

void game_clock::tick()
{
	auto timepoint_now = hrc::now();
	delta_time = timepoint_now - timepoint_prev;
	total_time += delta_time;
	timepoint_prev = timepoint_now;
}

void game_clock::reset()
{
	timepoint_prev = hrc::now();
	delta_time = hrc::duration{};
	total_time = hrc::duration{};
}

auto game_clock::get_delta_ns() const -> double
{
	return duration_cast<ns>(delta_time).count();
}

auto game_clock::get_delta_us() const -> double
{
	return duration_cast<us>(delta_time).count();
}

auto game_clock::get_delta_ms() const -> double
{
	return duration_cast<ms>(delta_time).count();
}

auto game_clock::get_delta_s() const -> double
{
	return duration_cast<s>(delta_time).count();
}

auto game_clock::get_total_ns() const -> double
{
	return duration_cast<ns>(total_time).count();
}

auto game_clock::get_total_us() const -> double
{
	return duration_cast<us>(total_time).count();
}

auto game_clock::get_total_ms() const -> double
{
	return duration_cast<ms>(total_time).count();
}

auto game_clock::get_total_s() const -> double
{
	return duration_cast<s>(total_time).count();
}
