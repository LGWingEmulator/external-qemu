/* Copyright (C) 2009 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#pragma once

#include "android/physics/Physics.h"
#include "android/utils/compiler.h"

#include <stdint.h>
#include <math.h>

/* forward declaration */
struct QAndroidPhysicalStateAgent;
typedef struct PhysicalModel PhysicalModel;

ANDROID_BEGIN_HEADER

/* initialize sensor emulation */
extern void  android_hw_sensors_init( void );

/* initialize remote controller for HW sensors. This must be called after
 * init_clocks(), i.e. later than android_hw_sensors_init(). */
extern void android_hw_sensors_init_remote_controller( void );

/* struct for use in sensor values */
struct vec3 {
    float x;
    float y;
    float z;

#ifdef __cplusplus
    bool operator==(const vec3& rhs) const {
        const float kEpsilon = 0.00001f;

        const double diffX = fabs(x - rhs.x);
        const double diffY = fabs(y - rhs.y);
        const double diffZ = fabs(z - rhs.z);
        return (diffX < kEpsilon && diffY < kEpsilon && diffZ < kEpsilon);
    }

    bool operator!=(const vec3& rhs) const {
        return !(*this == rhs);
    }
#endif  // __cplusplus
};

typedef struct vec3 vec3;


/* NOTE: Sensor status Error definition, It will be used in the Sensors Command
 *       part in android-qemu1-glue/console.c. Details:
 *       SENSOR_STATUS_NO_SERVICE: "sensors" qemud service is not available/initiated.
 *       SENSOR_STATUS_DISABLED: sensor is disabled.
 *       SENSOR_STATUS_UNKNOWN: wrong sensor name.
 *       SENSOR_STATUS_OK: Everything is OK to the current sensor.
 * */
typedef enum{
    SENSOR_STATUS_NO_SERVICE = -3,
    SENSOR_STATUS_DISABLED   = -2,
    SENSOR_STATUS_UNKNOWN    = -1,
    SENSOR_STATUS_OK         = 0,
} SensorStatus;

/* NOTE: this list must be the same that the one defined in
 *       the sensors_qemu.c source of the libsensors.goldfish.so
 *       library.
 *
 *       DO NOT CHANGE THE ORDER IN THIS LIST, UNLESS YOU INTEND
 *       TO BREAK SNAPSHOTS!
 */
#define  SENSORS_LIST  \
    SENSOR_(ACCELERATION,"acceleration",Accelerometer,vec3,"acceleration:%g:%g:%g") \
    SENSOR_(GYROSCOPE,"gyroscope",Gyroscope,vec3,"gyroscope:%g:%g:%g") \
    SENSOR_(MAGNETIC_FIELD,"magnetic-field",Magnetometer,vec3,"magnetic:%g:%g:%g") \
    SENSOR_(ORIENTATION,"orientation",Orientation,vec3,"orientation:%g:%g:%g") \
    SENSOR_(TEMPERATURE,"temperature",Temperature,float,"temperature:%g") \
    SENSOR_(PROXIMITY,"proximity",Proximity,float,"proximity:%g") \
    SENSOR_(LIGHT,"light",Light,float,"light:%g") \
    SENSOR_(PRESSURE,"pressure",Pressure,float,"pressure:%g") \
    SENSOR_(HUMIDITY,"humidity",Humidity,float,"humidity:%g") \
    SENSOR_(MAGNETIC_FIELD_UNCALIBRATED,"magnetic-field-uncalibrated",MagnetometerUncalibrated,vec3,"magnetic-uncalibrated:%g:%g:%g") \
    SENSOR_(GYROSCOPE_UNCALIBRATED,"gyroscope-uncalibrated",GyroscopeUncalibrated,vec3,"gyroscope-uncalibrated:%g:%g:%g") \

typedef enum {
#define  SENSOR_(x,y,z,v,w)  ANDROID_SENSOR_##x,
    SENSORS_LIST
#undef   SENSOR_
    MAX_SENSORS  /* do not remove */
} AndroidSensor;

/* NOTE: Physical parameters Error definition, It will be used in the Physics
 *       Command part in console.c. Details:
 *       PHYSICAL_PARAMETER_STATUS_NO_SERVICE: no physical model present.
 *       PHYSICAL_PARAMETER_STATUS_UNKNOWN: wrong physical parameter name.
 *       PHYSICAL_PARAMETER_STATUS_OK: Everything is OK to the current physical
 *                                     param.
 * */
typedef enum{
    PHYSICAL_PARAMETER_STATUS_NO_SERVICE = -2,
    PHYSICAL_PARAMETER_STATUS_UNKNOWN    = -1,
    PHYSICAL_PARAMETER_STATUS_OK         = 0,
} PhysicalParameterStatus;

/*
 * Note: DO NOT CHANGE THE ORDER IN THIS LIST, UNLESS YOU INTEND
 *       TO BREAK SNAPSHOTS!
 */
#define  PHYSICAL_PARAMETERS_LIST  \
    PHYSICAL_PARAMETER_(POSITION,"position",Position,vec3) \
    PHYSICAL_PARAMETER_(ROTATION,"rotation",Rotation,vec3) \
    PHYSICAL_PARAMETER_(MAGNETIC_FIELD,"magnetic-field",MagneticField,vec3) \
    PHYSICAL_PARAMETER_(TEMPERATURE,"temperature",Temperature,float) \
    PHYSICAL_PARAMETER_(PROXIMITY,"proximity",Proximity,float) \
    PHYSICAL_PARAMETER_(LIGHT,"light",Light,float) \
    PHYSICAL_PARAMETER_(PRESSURE,"pressure",Pressure,float) \
    PHYSICAL_PARAMETER_(HUMIDITY,"humidity",Humidity,float) \
    PHYSICAL_PARAMETER_(VELOCITY,"velocity",Velocity,vec3) \
    PHYSICAL_PARAMETER_(AMBIENT_MOTION,"ambientMotion",AmbientMotion,float) \

typedef enum {
#define PHYSICAL_PARAMETER_(x,y,z,w)  PHYSICAL_PARAMETER_##x,
    PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
    MAX_PHYSICAL_PARAMETERS
} PhysicalParameter;

extern void  android_hw_sensor_enable( AndroidSensor  sensor );

/* COARSE ORIENTATION VALUES */
typedef enum {
    ANDROID_COARSE_PORTRAIT,            // 0 degrees
    ANDROID_COARSE_REVERSE_LANDSCAPE,   // 90 degrees
    ANDROID_COARSE_REVERSE_PORTRAIT,    // 180 degrees
    ANDROID_COARSE_LANDSCAPE,           // 270 degrees
} AndroidCoarseOrientation;

/* change the coarse orientation value */
extern int android_sensors_set_coarse_orientation(
        AndroidCoarseOrientation  orient, float tilt_degrees);

/* get sensor values */
extern int android_sensors_get( int sensor_id,
        float* out_a, float* out_b, float* out_c );

/* set sensor override values */
extern int android_sensors_override_set(
        int sensor_id, float a, float b, float c );

/* Get sensor id from sensor name */
extern int android_sensors_get_id_from_name( const char* sensorname );

/* Get sensor name from sensor id */
extern const char* android_sensors_get_name_from_id( int sensor_id );

/* Get sensor from sensor id */
extern uint8_t android_sensors_get_sensor_status( int sensor_id );

/* Get the host-to-guest time offset */
extern int64_t android_sensors_get_time_offset();

/* Get the current sensor delay (determines update rate) */
extern int32_t android_sensors_get_delay_ms();

/* Get the physical model instance. */
extern PhysicalModel* android_physical_model_instance();

/* Get physical model values */
extern int android_physical_model_get(
    int physical_parameter, float* out_a, float* out_b, float* out_c,
    ParameterValueType parameter_value_type);

/* Set physical model target values */
extern int android_physical_model_set(
    int physical_parameter, float a, float b, float c,
    int interpolation_method);

/* Set agent to receive physical state change callbacks */
extern int android_physical_agent_set(
    const struct QAndroidPhysicalStateAgent* agent);

/* Get physical parameter id from name */
extern int android_physical_model_get_parameter_id_from_name(
    const char* physical_parameter_name );

/* Get physical parameter name from id */
extern const char* android_physical_model_get_parameter_name_from_id(
    int physical_parameter_id );

// Start recording ground truth to the specified file.
extern int android_physical_model_record_ground_truth(const char* file_name);

// Stop recording ground truth.
extern int android_physical_model_stop_recording();

/* Foldable state */

#define ANDROID_FOLDABLE_MAX_HINGES 16

enum FoldableDisplayType {
    // Horizontal split means something like a laptop, i.e.
    // |-----| Camera is here
    // | top |
    // |-----| hinge 0
    // |     |
    // |-----| hinge 1
    // |     |
    // |-----|
    ANDROID_FOLDABLE_HORIZONTAL_SPLIT = 0,

    // Vertical split is left to right, rotated version of horizontal split:
    // |-camera|-------|------|-------|
    // |       |       |      |       |
    // |       |       |      |       |
    // |-------|-------|------|-------|
    // hinge:  0       1      2
    // |-----|
    // | top |
    // |-----| hinge 0
    // |     |
    // |-----| hinge 1
    // |     |
    // |-----|
    ANDROID_FOLDABLE_VERTICAL_SPLIT = 1,

    // Roll configurations (essentially the # hinges are infinite,
    // representable via separate parameters)
    ANDROID_FOLDABLE_HORIZONTAL_ROLL = 2,
    ANDROID_FOLDABLE_VERTICAL_ROLL = 3,
};

struct FoldableHingeParameters {
    float percentAlongDisplay; // % of display height (horiz. hinge) or display width (vertical hinge) where the hinge occurrs
    float minDegrees;
    float maxDegrees;
    float defaultDegrees;
};

struct RollableParameters {
    float rollRadiusAsDisplayPercent; // % of display height (horiz. roll) or display width (vertical roll) that determines the radius of the rollable
    float minRolledPercent;
    float maxRolledPercent;
    float defaultRolledPercent;
};

struct FoldableConfig {
    enum FoldableDisplayType hingesType;

    // Display id where the folding occurs.  TODO: account for multiple
    // displays that are related by a hinge, perhaps via folding (heh)
    // displayId into hingeParams
    int displayId;

    // For hinges only
    int numHinges;
    struct FoldableHingeParameters hingeParams[ANDROID_FOLDABLE_MAX_HINGES];
    // For rollables only
    struct RollableParameters rollableParams;
};

struct FoldableState {
    struct FoldableConfig config;
    float currentHingeDegrees[ANDROID_FOLDABLE_MAX_HINGES];
    float percentRolled;
};

struct FoldableState* android_foldable_initialize(const struct FoldableConfig* config);
void android_foldable_set_hinge_degrees(unsigned int hinge_index, float degrees);
float android_foldable_get_hinge_degrees(unsigned int hinge_index);
void android_foldable_set_with_1d_parameter(float t);
struct FoldableState* android_foldable_get_state_ptr();

ANDROID_END_HEADER
