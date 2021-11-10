/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: TimeStamp utility implementation.
 */

#include "TimeStamp.h"

#include <chrono>
#include <iomanip>

namespace utils {
namespace detail {
template <typename Duration>
using SysTime = std::chrono::time_point<std::chrono::system_clock, Duration>;
using SysNanoSeconds = SysTime<std::chrono::nanoseconds>;

TimeType NanosecondsSinceEpoch()
{
    SysNanoSeconds t = std::chrono::system_clock::now();
    return t.time_since_epoch().count();
}
} // namespace detail

TimeStamp TimeStamp::Now()
{
    return TimeStamp(detail::NanosecondsSinceEpoch());
}

void TimeStamp::AppendAccurateTime(std::stringstream &ss, TimePrecision precision) const
{
    switch (precision) {
        case TimePrecision::SECOND: {
            ss << std::setw(3) << 0;
            break;
        }
        case TimePrecision::MILLI: {
            TimeType millis = (m_nanoSecSinceEpoch % NANO_SECS_PER_SECOND) / NANO_SECS_PER_MILLISECOND;
            ss << std::setw(3) << millis;
            break;
        }
        case TimePrecision::MICRO: {
            TimeType micros = (m_nanoSecSinceEpoch % NANO_SECS_PER_SECOND) / NANO_SECS_PER_MICROSECOND;
            ss << std::setw(6) << micros;
            break;
        }
        case TimePrecision::NANO: {
            TimeType nanos = (m_nanoSecSinceEpoch % NANO_SECS_PER_SECOND);
            ss << std::setw(9) << nanos;
            break;
        }
        default:
            break;
    }
}

const std::string TimeStamp::ToString(TimePrecision precision) const
{
    std::stringstream ss;
    TimeType seconds = m_nanoSecSinceEpoch / NANO_SECS_PER_SECOND;
    ss << seconds << "." << std::setfill('0');
    AppendAccurateTime(ss, precision);
    return ss.str();
}

const std::string TimeStamp::ToFormattedString(TimePrecision precision) const
{
    std::stringstream ss;
    TimeType seconds = m_nanoSecSinceEpoch / NANO_SECS_PER_SECOND;
#ifdef WIN32
    struct tm localTime;
    (void)::localtime_s(&localTime, &seconds);
    struct tm *tm_time = &localTime;
#else
    struct tm *tm_time = ::localtime(&seconds);
#endif
    if (tm_time == nullptr) {
        return "Get Time Error!";
    }
    ss << std::setw(4) << tm_time->tm_year + 1900 << "-" << std::setfill('0') << std::setw(2)
        << tm_time->tm_mon + 1 << "-" << tm_time->tm_mday << " "
        << tm_time->tm_hour << ":" << tm_time->tm_min << ":" << tm_time->tm_sec << ".";
    AppendAccurateTime(ss, precision);
    return ss.str();
}
} // namespace utils
