IR 接收测试使用模拟下降沿，不依赖开发板。

在项目目录执行：

```sh
gcc -std=c99 -Wall -Wextra -Werror -I tests/ir_receive_support -I EXTERN/SERVER/IR tests/ir_receive_test.c EXTERN/SERVER/IR/IR_Receive.c -o tests/ir_receive_test.exe
./tests/ir_receive_test.exe
```

覆盖正常帧、引导段比例缩放、位间隔容差、32 位时间回绕、反码错误、非法位间隔、残帧恢复、标称重复帧忽略、待取帧保护和中断状态恢复。

`ir_receive_support/main.h` 仅供本机测试模拟中断开关，不加入 Keil 工程。实际 GPIO、中断延迟、连续微秒计时源及遥控器接收效果仍需上板验证。
