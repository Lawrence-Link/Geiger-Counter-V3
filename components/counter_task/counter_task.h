#ifndef COUNTER_TASK_H
#define COUNTER_TASK_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int gpio_num; // 脉冲输入 GPIO
} counter_task_config_t;

/**
 * @brief 启动盖革计数器处理任务。
 *
 * @param config 任务配置。
 * @return true 启动成功，false 失败。
 */
bool start_counter_task(const counter_task_config_t* config);

/**
 * @brief 安全地停止并删除盖革计数器处理任务。
 *
 * @return true 停止成功，false 失败。
 */
bool stop_counter_task();

/**
 * @brief 获取当前的计数率 (CPM)。
 *
 * @return float 当前的计数率 (Counts Per Minute)。
 */
float get_current_cpm();

#endif // COUNTER_TASK_H