操作路线：固定准备 → 填写任务表 → 初始化调度器 → 循环调度 → 按需添加任务 → 按需取消任务。

@ 固定准备
@ 填写任务表
@ 初始化调度器
@ 循环调度
@ 按需添加任务
@ 按需取消任务

# 固定准备

-> 将 EXTERN 根目录加入搜索路径，将 mag_tick.c、mag_scheduler.c、HW_INIT/TIM/TIM_init.c 加入编译；需要旧版固定周期接口时再加入 mag_task.c。当前仓库 Keil/IAR 已接入基础模块。

-> 在调用文件中引用 `MAG/mag_scheduler.h`，将任务函数和任务表放在自己的业务文件中，任务函数放在任务表之前；无需启用尚未适配的 APP 模块。

-> 目标工程 main.h 引入设备头文件后自动识别芯片；002B 与 F030 均通过 HW_INIT/TIM/TIM_init.c 提供定时中断，002B 的 PWM 映射暂不启用。

-> 系统时钟配置完成后初始化一次；每轮主循环调用 `MAG_SchedulerProcess()`。当前 main.c 已通过 app_main_init/app_main 接通初始化和循环调度。

-> 002B 默认由 TIM_init.c 提供 TIM1_BRK_UP_TRG_COM_IRQHandler；如果目标工程已有该函数，在工程预定义宏中设置 MAG_TICK_EXTERNAL_IRQ=1，并在已有函数内调用 MAG_TickIRQHandler()。

-> TIM1 由 MAG 独占，不同时用于 PWM 或其他计时；TIM_init.c 负责中断分发，mag_tick.c 负责 tick 累加，调度器不依赖串口、日志或低功耗。

-> 旧版 mag_task.c 未提供 LOG_DEBUG 时自动关闭自身日志；提供该宏时继续使用原日志。task_1ms 等回调仍可由业务文件覆盖，默认空实现保留在 mag_task.c 中。

# 填写任务表

需要增加固定任务 → 在现有 `s_app_tasks` 中增加一项 `MAG_TaskTableEntry_t` → 按“执行函数、执行间隔 ms、首次延迟 ms、执行次数”的顺序填写。

指定执行函数 → 使用 `MAG_TaskCallback_t` 要求的函数形式：返回 `void`，接收一个 `void *p_context` 参数 → 任务表注册的任务固定收到 `NULL`，无需参数时忽略它。

设置首次执行 → `first_delay_ms` 从注册时开始计时，0 表示尽快执行 → 实际执行要等主循环调度，不会在初始化时调用。

设置重复间隔 → `period_ms` 从本次任务函数返回后开始计时 → 它不是函数运行时长，也不保证两次开始时间严格等间隔。

设置执行次数 → `run_count` 填 1 表示一次、N 表示 N 次、`MAG_SCHEDULER_RUN_FOREVER` 表示永久循环 → 有限次数执行完后自动移除任务。

检查配置 → 回调不能为空，次数不能为 0，重复任务间隔必须大于 0，单次任务间隔允许为 0 → 首次延迟和间隔均不能超过 `0x7FFFFFFF` ms。

检查数量 → 初始任务表至少一项，固定任务和运行时新增任务共用 `MAG_SCHEDULER_MAX_TASKS` 个名额，默认 8 个 → 需要动态添加时预留空位。

# 初始化调度器

业务模块准备完成 → `MAG_SchedulerInit()`，传入任务表和任务表元素个数 → 仅返回 `MAG_SCHEDULER_STATUS_OK` 后开始正常调度；当前 app_main_init 已接入。

初始化时 → 接口自动装入整张任务表并通过 `mag_tick_init()` 启动 TIM1 的 1ms 时基 → TIM 中断调用 `TIM1_UpdateCallback()` 累加 tick；不要将 TIM1 改作其他用途。

时基 → TIM 根据实际 APB 时钟及定时器倍频计算 PSC/ARR → 产生 1ms 更新事件；4/8/24 MHz 均能精确整分频，参数不支持时返回初始化失败。

中断运行条件 → 保持全局中断开启，避免连续屏蔽中断超过 1ms → 长时间屏蔽会合并更新事件、丢失计时；重新初始化时基保留软件 tick 的连续性。

初始化失败 → 按返回状态处理参数、容量或时基问题 → 修正后重新初始化；再次初始化会清空旧任务并使旧句柄失效，任务回调中调用则返回 `MAG_SCHEDULER_STATUS_BUSY`。

# 循环调度

进入主循环 → 每轮调用 `MAG_SchedulerProcess()` → 到期任务在主循环中依次执行，未到期任务跳过。

编写任务内容 → 每次只做一小步并尽快返回，不使用阻塞延时或等待循环 → 长任务会推迟其他任务，包括按键扫描和喂狗。

任务错过原定时间 → 下一次扫描到它时执行一次 → 不连续补跑遗漏次数，也不要依赖任务表行顺序安排先后关系。

需要中断触发业务 → 中断内记录事件，在主循环或任务回调中处理 → 初始化、注册、取消和调度接口均不从中断调用。

# 按需添加任务

运行中需要新任务、传入上下文或保留取消句柄 → 填写 `MAG_TaskConfig_t` 的 `callback`、`p_context`、`first_delay_ms`、`period_ms`、`run_count` → 在主循环或任务回调中调用 `MAG_SchedulerRegister()`。

注册任务 → 传入配置地址和有效的 `MAG_TaskHandle_t` 输出变量地址 → 返回 `MAG_SCHEDULER_STATUS_OK` 后保存句柄；返回 `MAG_SCHEDULER_STATUS_FULL` 表示没有空位。

传递上下文 → 无需参数时填 `NULL`，否则保证 `p_context` 指向的对象在任务使用期间一直有效 → 配置值会被复制，但上下文对象不会被复制。

在任务回调中注册 → 本轮结束后加入待调度任务 → 最早在下一次 `MAG_SchedulerProcess()` 中执行。

# 按需取消任务

需要停止任务后续执行 → 在主循环或任务回调中调用 `MAG_SchedulerCancel()`，传入注册成功时保存的句柄 → 检查返回状态。

任务正在执行时取消 → 当前回调仍需正常返回 → 后续不再执行；调度期间取消的任务在本轮清理时释放名额。

返回 `MAG_SCHEDULER_STATUS_INVALID_HANDLE` → 该句柄无效或任务已结束、已取消、已被重新初始化清除 → 不再用它操作任务。

任务表中的任务需要单独取消 → 改用运行时注册并保存句柄 → `MAG_SchedulerInit()` 不向业务返回表内任务的句柄，不手工拼接句柄。
