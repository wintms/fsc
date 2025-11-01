# 环境基准曲线算法 (Ambient Base Curve Algorithm)

## 概述

本算法实现了基于环境温度的风扇控制策略，通过进风口传感器校准获得准确的环境温度，并根据环境基准曲线计算相应的PWM值。

## 主要特性

1. **进风口传感器校准**: 将进风口传感器读数转换为准确的环境温度
2. **环境基准曲线**: 支持多项式和分段线性映射
3. **速率限制**: 防止PWM值变化过快
4. **滞后控制**: 避免频繁的PWM调整
5. **负载场景支持**: 可根据不同负载情况调整曲线

## 数据结构

### FSCAmbientCalibration
用于进风口传感器校准的数据结构：
```c
typedef struct {
    INT8U CalType;                              // 校准类型 (0=多项式, 1=分段线性)
    INT8U CoeffCount;                           // 多项式系数个数
    float Coefficients[MAX_POLYNOMIAL_COEFFS];  // 多项式系数
    INT8U PointCount;                           // 分段线性点数
    FSCAmbientCalPoint PiecewisePoints[MAX_PIECEWISE_POINTS]; // 分段线性点
} FSCAmbientCalibration;
```

### FSCAmbientBase
用于环境基准曲线的数据结构：
```c
typedef struct {
    INT8U CurveType;                            // 曲线类型 (0=多项式, 1=分段线性)
    INT8U LoadScenario;                         // 负载场景
    INT8U CoeffCount;                           // 多项式系数个数
    float Coefficients[MAX_POLYNOMIAL_COEFFS];  // 多项式系数
    INT8U PointCount;                           // 分段线性点数
    FSCAmbientBasePoint PiecewisePoints[MAX_PIECEWISE_POINTS]; // 分段线性点
    float FallingHyst;                          // 下降滞后温度
    float MaxRisingRate;                        // 最大上升速率 (%/cycle)
    float MaxFallingRate;                       // 最大下降速率 (%/cycle)
} FSCAmbientBase;
```

## 核心函数

### FSCGetAmbientTemperature
```c
float FSCGetAmbientTemperature(float inlet_temp, float current_pwm);
```
将进风口传感器读数转换为环境温度。

**参数:**
- `inlet_temp`: 进风口传感器温度读数
- `current_pwm`: 当前风扇PWM值

**返回值:** 校准后的环境温度

### FSCGetPWMValue_AmbientBase
```c
float FSCGetPWMValue_AmbientBase(float ambient_temp, FSCAmbientBase* pAmbientBase, 
                                float current_pwm, INT8U enable_rate_limit);
```
根据环境基准曲线计算PWM值。

**参数:**
- `ambient_temp`: 环境温度
- `pAmbientBase`: 环境基准曲线参数
- `current_pwm`: 当前PWM值
- `enable_rate_limit`: 是否启用速率限制

**返回值:** 计算得到的PWM值

## JSON配置示例

### 环境校准配置
```json
{
    "ambient_calibration": {
        "cal_type": 1,
        "point_count": 6,
        "piecewise_points": [
            {"pwm": 0, "delta_temp": 8.5},
            {"pwm": 20, "delta_temp": 6.2},
            {"pwm": 40, "delta_temp": 4.8},
            {"pwm": 60, "delta_temp": 3.5},
            {"pwm": 80, "delta_temp": 2.8},
            {"pwm": 100, "delta_temp": 2.2}
        ]
    }
}
```

### 环境基准曲线配置
```json
{
    "profile_info": {
        "profile_list": [
            {
                "label": "System inlet sensor - Ambient Base",
                "profile_index": 3,
                "sensor_num": 3,
                "sensor_name": "SW_U15",
                "type": "ambient_base",
                "curve_type": 1,
                "load_scenario": 0,
                "point_count": 8,
                "piecewise_points": [
                    {"temp": 25, "pwm": 30},
                    {"temp": 30, "pwm": 30},
                    {"temp": 35, "pwm": 32},
                    {"temp": 40, "pwm": 38},
                    {"temp": 45, "pwm": 50},
                    {"temp": 50, "pwm": 70},
                    {"temp": 55, "pwm": 85},
                    {"temp": 60, "pwm": 100}
                ],
                "falling_hyst": 2,
                "max_rising_rate": 10,
                "max_falling_rate": 5
            }
        ]
    }
}
```

## 算法流程

1. **传感器读取**: 获取进风口传感器温度读数
2. **环境温度校准**: 使用`FSCGetAmbientTemperature`函数校准环境温度
3. **PWM计算**: 使用`FSCGetPWMValue_AmbientBase`函数计算目标PWM值
4. **速率限制**: 应用速率限制防止PWM变化过快
5. **滞后处理**: 应用下降滞后避免频繁调整
6. **边界限制**: 确保PWM值在有效范围内

## 测试验证

使用提供的测试程序验证算法正确性：

```bash
# 编译测试程序
make -f Makefile.test

# 运行测试
make -f Makefile.test test
```

测试程序将验证：
- 进风口传感器校准功能
- 环境基准曲线映射
- 速率限制功能

## 注意事项

1. 确保进风口传感器校准数据准确
2. 根据实际硬件调整环境基准曲线参数
3. 合理设置速率限制参数避免系统震荡
4. 定期验证算法在不同环境条件下的表现

## 文件列表

- `fsc_core.h`: 核心数据结构和函数声明
- `fsc_core.c`: 核心算法实现
- `fsc_parser.h`: JSON解析器头文件
- `fsc_parser.c`: JSON解析器实现
- `fsc_loop.c`: 主循环集成
- `fsc_z9964f_b2f_ambient.json`: B2F配置示例
- `test_ambient_algorithm.c`: 测试程序
- `Makefile.test`: 测试编译脚本