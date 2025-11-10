# Fan Speed Control (FSC) JSON Configuration Guide

This document explains how to configure the JSON file for Fan Speed Control (FSC). The FSC system dynamically adjusts the fan's PWM (Pulse Width Modulation) duty cycle based on readings from one or more sensors (such as temperature sensors) to control fan speed.

## JSON File Structure

The JSON configuration file is primarily composed of the following sections:

1.  **Top-level Configuration**
2.  **System Information (`system_info`)**
3.  **Ambient Calibration (`ambient_calibration`)**
4.  **Profile Information (`profile_info`)**

Below is a configuration example:

```json
{
    "description": "Z9964F F2B Fan Speed Control Configuration",
    "debug_verbose": 3,
    "system_info": {
        "fsc_mode": "auto",
        "fsc_version": "0.1",
        "fan_max_pwm": 100,
        "fan_min_pwm": 20,
        "fan_initial_pwm": 50
    },
    "ambient_calibration": {
        "cal_type": 0,
        "coeff_count": 4,
        "coefficients": [9.2, -0.18, 0.0025, -0.000015]
    },
    "profile_info": {
        "profile_list": [
            {
                "label": "CPU sensor",
                "profile_index": 0,
                "sensor_num": 1,
                "type": "pid",
                "setpoint": 95,
                "setpoint_type": 0,
                "kp": 1,
                "ki": 0.1,
                "kd": 0.1
            },
            {
                "label": "Inlet sensor",
                "profile_index": 1,
                "sensor_nums": [17, 18],
                "aggregation": "average",
                "type": "polynomial",
                "curve_type": 0,
                "load_scenario": 0,
                "coeff_count": 4,
                "coefficients": [30.566, -0.2357, 0.003, 0.0007],
                "falling_hyst": 2,
                "max_rising_rate": 10,
                "max_falling_rate": 5
            }
        ]
    }
}
```

---

### 1. Top-level Configuration

These are the basic configuration items located at the root level of the JSON file.

-   `description` (string): A brief description of this configuration file.
-   `debug_verbose` (integer): The verbosity level for debugging information. A higher level results in more detailed log output.

---

### 2. System Information (`system_info`)

This section defines the global parameters for fan control.

-   `fsc_mode` (string): The operating mode of FSC. `"auto"` enables automatic fan control.
-   `fsc_version` (string): The version number of the configuration file.
-   `fan_max_pwm` (integer): The maximum PWM duty cycle for the fans (percentage), typically `100`.
-   `fan_min_pwm` (integer): The minimum PWM duty cycle for the fans (percentage), e.g., `20`. The fan speed will not drop below this value.
-   `fan_initial_pwm` (integer): The initial PWM duty cycle for the fans on system startup (percentage).

---

### 3. Ambient Calibration (`ambient_calibration`)

This section is used to calibrate sensor readings based on environmental conditions (such as altitude).

-   `cal_type` (integer): Calibration type. `0` indicates a polynomial curve-based calibration.
-   `coeff_count` (integer): The number of coefficients in the `coefficients` array.
-   `coefficients` (array of numbers): The polynomial coefficients used for the calibration calculation.

---

### 4. Profile Information (`profile_info`)

This section is the core of FSC, defining how to calculate the fan PWM value based on sensor inputs.

-   `profile_list` (array of objects): A list containing multiple control policies. The system calculates the PWM output for each profile and typically uses the maximum of these values as the final fan PWM setting.

#### Profile Object Structure

Each profile object contains the following fields:

-   `label` (string): A human-readable name for the profile, e.g., `"CPU sensor"`.
-   `profile_index` (integer): A unique index for the profile.
-   `sensor_num` (integer): The number of a single sensor associated with this profile.
-   `sensor_nums` (array of integers): A list of sensor numbers associated with this profile. If `sensor_nums` is defined, `sensor_num` is typically not used.
-   `aggregation` (string): When using `sensor_nums`, specifies how to aggregate the readings from multiple sensors. Options include `"average"` and `"max"`.
-   `type` (string): The type of control algorithm.
    -   `"pid"`: Uses a PID (Proportional-Integral-Derivative) controller.
    -   `"polynomial"`: Uses a polynomial curve to calculate PWM.

#### PID Controller (`type: "pid"`)

-   `setpoint` (number): The target temperature setpoint. The PID controller will attempt to maintain the sensor temperature at this value.
-   `setpoint_type` (integer): Setpoint type. `0` indicates a static setpoint.
-   `kp` (number): Proportional gain.
-   `ki` (number): Integral gain.
-   `kd` (number): Derivative gain.

#### Polynomial Controller (`type: "polynomial"`)

-   `curve_type` (integer): Curve type. `0` indicates a standard temperature-to-PWM mapping curve.
-   `load_scenario` (integer): Load scenario identifier. `0` indicates the default or normal load.
-   `coeff_count` (integer): The number of coefficients in the `coefficients` array.
-   `coefficients` (array of numbers): The polynomial coefficients used to calculate PWM. The formula is typically `PWM = c[0] + c[1]*T + c[2]*T^2 + ...`, where `T` is the sensor temperature.
-   `falling_hyst` (number): Hysteresis value for falling temperature (in degrees Celsius). When the temperature drops, it must fall below the rising threshold by this value before the fan speed is reduced, preventing frequent speed changes near the threshold.
-   `max_rising_rate` (integer): The maximum allowed rate of increase for the PWM duty cycle per second (percentage). Used for smooth fan acceleration.
-   `max_falling_rate` (integer): The maximum allowed rate of decrease for the PWM duty cycle per second (percentage). Used for smooth fan deceleration.

## Configuration Workflow

1.  **Define Global Fan Behavior**: Set the minimum, maximum, and initial fan PWM values in `system_info`.
2.  **Create Control Profiles**:
    -   Create a profile for each critical component (e.g., CPU, switch chip, PSU) or area (e.g., air inlet) that needs monitoring.
    -   Select the appropriate sensor(s) (`sensor_num` or `sensor_nums`).
    -   Choose the control `type` (`pid` or `polynomial`) based on requirements.
    -   **For PID**: Adjust `setpoint`, `kp`, `ki`, and `kd` parameters to achieve stable temperature control.
    -   **For Polynomial**: Define one or more temperature-to-PWM curves. Adjust `coefficients` to shape the curve. Use `falling_hyst`, `max_rising_rate`, and `max_falling_rate` to optimize for smooth and stable fan behavior.
3.  **Final PWM Determination**: The FSC daemon runs all profiles in parallel, calculates their respective PWM demands, and typically selects the maximum of all demands to set the final system fan speed. This ensures that even if only one component is overheating, the fans will provide sufficient cooling.
