# 基于 ESP32-C6 与 PixelUI 的 盖革计数器 N1

[OSHWHub上的工程点这里](https://oshwhub.com/qstorey/geiger-counter-n1/)


> 此工程在ESP-IDF v5.5.1下进行开发构建。
至少使用内置4MB FLASH的ESP32-C6。

## 构造前的配置 (must read): 
> - 在 idf.py menuconfig 中请将分区表配置为预设"Single factory app(large), no OTA", 以充分利用闪存空间。

> - 硬件JTAG接口默认与LED冲突，请使用espefuse工具设定 efuse bit "DIS_PAD_JTAG" = 1 以禁用JTAG，解放GPIO (见下方bash script). 根据指示输入BURN 以永久禁用JTAG PAD。
> ```bash
> ./espefuse --port /dev/ttyACM1(你的COM口) --chip esp32c6 burn_efuse DIS_PAD_JTAG 1
> ```

## 一些小建议
- 这个设计使用C6的ADC实现电压环输入。考虑到每一个ESP32-C6的体质有些许差异，建议你在voltage_pid.cpp中调节CORRECTION_OFFSET, 让ADC在目标工作区间整体线性程度更好。

- 在CORRECTION_OFFSET = 0的情况下，启动计数应用，电压环应开始工作。 此时在4.7MOhm电阻之前测得实际电压V1',经过分压器后测得电压为V2。为了估计实际高压V1，应该使V1约等于V1' = V2 * (1/div) - CORRECTION_OFFSET. 简单来说， 如果实际数值比理论大，应下调这一数值。

- 你亦可在system_nvs_varibles/system_nvs_varibles.h里寻找电压环PID调试变量，这些变量目前不会在程序中被修改或存入nvs，可以直接在此处修改
```cpp
    float voltage_pid_Kp = 1.5f;
    float voltage_pid_Ki = 7.0f;
    float voltage_pid_Kd = 0.0f;
```

# LICENSED UNDER GPL3.0
此工程使用GPL3.0协议。