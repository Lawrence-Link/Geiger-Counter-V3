#pragma once

#include <stdint.h>
#include <etl/inplace_function.h>
#include "config.h"
#include "core/CommonTypes.h"

using InputCallback = etl::inplace_function<bool(InputEvent), CALLBACK_STORAGE_SIZE>;
using VoidCallback = etl::inplace_function<void(), CALLBACK_STORAGE_SIZE>;
using ValueCallback = etl::inplace_function<void(int32_t), CALLBACK_STORAGE_SIZE>;
using IsrTaskNotifyFunction = void (*)(void* context);
