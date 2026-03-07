# Analysis and Implementation of Embedded Operating Systems — STM32F407VG with FreeRTOS

基於 STM32F407G-DISC1 開發板的初始專案，使用 FreeRTOS 實作一系列「1142 嵌入式作業系統分析與實作」課程的實驗。你可以使用 `git checkout <實驗分支名稱>` 切換到對應的實驗分支，查看每個實驗的具體實作細節。

## 開發環境

- IDE：STM32CubeIDE 1.19.0
- 硬體：STM32F407G-DISC1 開發板（ARM Cortex-M4F）
- 編譯器：arm-none-eabi-gcc (MCU ARM GCC)
- 程式碼產生器：STM32CubeMX 6.15.0
- 韌體：STM32Cube FW_F4 V1.28.3

## 系統核心設置

- 偵錯模式：Serial Wire
- HAL Timebase：TIM7
- NVIC 優先順序：4 位搶占優先級

## 其他路徑配置

- 需包含：
  - ./FreeRTOS/include
  - ./FreeRTOS/portable/ARM_CM4F
- 來源位置：
  - ./FreeRTOS

## 專案目錄結構

```tree
<project>/
├── Core/
│   ├── Inc/          # 標頭檔 (main.h, HAL config, IT handlers)
│   ├── Src/          # 原始碼 (main.c, HAL MSP, IT handlers, syscalls)
│   └── Startup/      # 啟動組語 (startup_stm32f407vgtx.s)
├── Drivers/
│   ├── CMSIS/        # ARM CMSIS 標頭與裝置定義
│   └── STM32F4xx_HAL_Driver/  # HAL 驅動程式
├── FreeRTOS/
│   ├── include/      # FreeRTOS 標頭與配置
│   ├── portable/     # ARM_CM4F port & MemMang (heap)
│   ├── tasks.c, queue.c, list.c, ...
│   └── FreeRTOSConfig.h # FreeRTOS 配置檔
├── <project>.ioc         # STM32CubeMX 專案檔
├── .cproject         # STM32CubeIDE 專案設定
├── STM32F407VGTX_FLASH.ld  # Flash linker script
└── STM32F407VGTX_RAM.ld    # RAM linker script
```

## 參考連結

- [使用說明書](https://www.st.com/resource/en/user_manual/dm00039084-discovery-kit-with-stm32f407vg-mcu-stmicroelectronics.pdf) (Discovery kit with STM32F407VG MCU - User manual)
- [開發板硬體規格書](https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-and-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf) (STM32F405/415, STM32F407/417, STM32F427/437 and STM32F429/439 advanced Arm®-based 32-bit MCUs - Reference manual)
- [FreeRTOS API 參考書](https://www.freertos.org/a00106.html) (FreeRTOS API Reference)
