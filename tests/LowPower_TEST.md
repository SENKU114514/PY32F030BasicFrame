长按状态机可在本机测试，在项目目录执行：

```sh
gcc -std=c99 -Wall -Wextra -Werror -I EXTERN/HW_INIT/LOWPOWER tests/lowpower_key_test.c -o tests/lowpower_key_test.exe
./tests/lowpower_key_test.exe
```

覆盖 2990ms 不唤醒、3000ms 确认、松开重置、多个短按不能累计、进入时已按住必须先松手。

硬件流程仍须上板验证：

- 配置空闲唤醒引脚；短按后保持休眠，长按约三秒后返回 OK。
- 在 4/8/16/24/48MHz 的框架时钟配置下检查恢复后的串口、PWM 和 MAG 时基。
- 确认其他普通外设中断不会恢复业务；唤醒按键同组其他 EXTI 不作为唤醒源。
- 传输忙时应返回 BUSY；GPIO 读失败时应恢复外设并返回错误。
- Stop 和长按确认期间分别测量电流，不能用手册典型值替代整板实测。

本模式暂停框架支持的普通外设，按键确认期间使用 1MHz 和无中断的 SysTick 轮询。没有改 GPIO 输出电平；板外负载通过回调管理。不可关闭的独立看门狗需在启用本模式前另行规划，不能依靠原来的周期喂狗任务维持无限休眠。
