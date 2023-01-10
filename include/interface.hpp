#pragma once
#include <stdint.h>

struct fan_set_rpm_message final {
    constexpr static const uint32_t command = 1;
    float value;
};
struct fan_set_pwm_duty_message final {
    constexpr static const uint32_t command = 2;
    uint16_t value;
};

struct fan_get_message final {
    constexpr static const uint32_t command = 3;
    uint32_t dummy;
};

struct fan_get_response_message final {
    constexpr static const uint32_t command = 4;
    float max_rpm;
    float target_rpm;
    float rpm;
    uint16_t pwm_duty;
    uint8_t mode;
};
