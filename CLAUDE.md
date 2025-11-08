# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

This is the Mt.Jefferson air cooling project, implementing Fan Speed Control (FSC) for Dell BMC systems. The codebase provides thermal management with support for multiple sensors, ambient temperature calibration, and intelligent fan control algorithms.

## Architecture

### Core Components

**Fan Control Library** (`libthermalmgr_dell-src/`)
- `fsc_loop.c`: Main fan control loop implementation with sensor aggregation
- `fsc_core.c`/`fsc_core.h`: Core fan control algorithms (PID, Linear, Polynomial)
- `fsc_parser.c`/`fsc_parser.h`: JSON configuration file parser
- `fsc_utils.h`: Utility macros and debug definitions

**IPMI PDK Library** (`libipmipdk-ARM-AST2600-AST2600EVB-AMI-src/`)
- Platform-specific IPMI stack implementation
- OEM-specific handlers for fan, sensor, and hardware management
- Integrates with BMC hardware layer

**Key Features**
- Multi-sensor aggregation (average/max with configurable group size up to 64 sensors)
- PID control with power-based parameter switching
- Ambient temperature calibration using inlet sensor
- Polynomial-based ambient base curves for environment temperature mapping
- OSFP optical module thermal management

### Build System

The project uses Make with AMI-specific build infrastructure:

```bash
# Build libthermalmgr_dell
cd libthermalmgr_dell-src/data && make

# Build libipmipdk
cd libipmipdk-ARM-AST2600-AST2600EVB-AMI-src/data && make
```

Build environment variables required:
- `SPXINC`: Path to AMI SDK headers
- `SPXLIB`: Path to AMI SDK libraries
- `TOOLDIR`: Path to AMI build tools
- `TARGETDIR`: Target system directory

**Debug Build**: Uncomment `DEBUG = y` in Makefiles to enable verbose logging

### Configuration

**Configuration Files** (`libthermalmgr_dell-src/data/configs/`):
- `fsc_z9964f_b2f.json`: Back-to-Front airflow configuration
- `fsc_z9964f_f2b.json`: Front-to-Back airflow configuration
- Configuration selected at runtime based on system airflow direction

**Key Configuration Structure**:
- `system_info`: FSC mode, fan max PWM, initial PWM
- `profile_info`: Per-sensor control profiles with PID/polynomial parameters
- `ambient_calibration`: Inlet sensor correction (B2F projects use polynomial coefficients: [9.2, -0.18, 0.0025, -0.000015])
- `ambient_base_curve`: Ambient temp to PWM mapping for environment control

### Control Algorithms

**PID Control**: Standard PID with optional power-based parameter switching (up to 8 buckets)
- `SetPoint`: Target temperature/pwm
- `Kp`, `Ki`, `Kd`: PID coefficients
- `SetPointType`: 0=temperature, 1=PWM

**Linear Control**: Simple hysteresis-based control with min/max temperature and PWM ranges

**Polynomial Control**: Ambient-based curve for environment temperature to PWM mapping
- Coefficients: PWM = a0 + a1*T + a2*T² + a3*T³
- Supports multiple load scenarios (idle/low_power/full_load)
- Configurable rising/falling rates and hysteresis

**Ambient Calibration**: Inlet sensor compensation
- Polynomial: ΔT = a0 + a1×PWM + a2×PWM² + a3×PWM³
- Piecewise: Linear interpolation between PWM/delta_temp points

### Sensor Aggregation

**Multi-Sensor Support**: Each profile can aggregate multiple sensors
- `SensorNums[]`: Array of sensor numbers (max 64 sensors for optical modules)
- `AggregationMode`: 0=average, 1=maximum
- OSFP modules benefit from averaging 64 temperature readings

**Power Sensor Mapping**: Optional power-based parameter switching
- `PowerSensorNums[]`: Power sensors tied to temperature sensor group
- Used to dynamically select PID parameters based on system load

## Build and Development Commands

### Building Libraries
```bash
# Build thermal manager library
cd libthermalmgr_dell-src/data && make

# Build IPMI library
cd libipmipdk-ARM-AST2600-AST2600EVB-AMI-src/data && make

# Clean builds
cd libthermalmgr_dell-src/data && make clean
cd libipmipdk-ARM-AST2600-AST2600EVB-AMI-src/data && make clean
```

### Working with Config Files
```bash
# Edit JSON configs (B2F and F2B variants)
vi libthermalmgr_dell-src/data/configs/fsc_z9964f_b2f.json
vi libthermalmgr_dell-src/data/configs/fsc_z9964f_f2b.json
```

### Git Workflow
```bash
# Check current status (Note: currently on 'avg_sensor' branch)
git status

# View recent commits focusing on FSC changes
git log --oneline -10 -- libthermalmgr_dell-src/data/

# View changes before committing
git diff libthermalmgr_dell-src/data/

# Commit changes (user explicitly requested this
git add libthermalmgr_dell-src/data/
git commit -m "commit message here"
```

## Recent Development Focus

Based on recent commits, the codebase has been enhanced with:
- **Average sensor value support**: Multi-sensor averaging with configurable group size (max 64 sensors)
- **OSFP thermal control**: Optical module thermal management requiring up to 64 sensor averages
- **Ambient calibration**: Inlet sensor correction using polynomial coefficients specific to B2F airflow
- **Code organization**: Recent changes to make `verbose` variable static in main loop

## Important Notes

- This is **production BMC firmware code** that runs on embedded ARM platforms
- Thermal control directly impacts system reliability and component lifespan
- Configurations are loaded from `/conf/fsc/` path on target system at runtime
- B2F configurations use specific calibration polynomial coefficients derived from physical testing
- All changes should maintain backward compatibility with existing JSON configurations
- The code follows AMI SDK conventions and integrates with their build system
