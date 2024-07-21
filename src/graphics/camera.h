#pragma once

#include <cglm/mat4.h>
#include <cglm/vec3.h>

typedef struct GicWindow GicWindow;
typedef struct GicGlRenderer GicGlRenderer;

typedef struct Camera {
    vec3 position;
    vec3 front;
    vec3 right;
    vec3 up;
    
    float yaw;
    float pitch;

    float movement_speed;
    float mouse_sensitivity;
    float zoom;
} Camera;

typedef enum CameraMovement {
    CAM_FORWARD,
    CAM_BACKWARDS,
    CAM_LEFT,
    CAM_RIGHT,
} CameraMovement;

Camera gic_create_camera(vec3 position, float yaw, float pitch);
Camera gic_create_camera_default();

void gic_camera_get_view_matrix(Camera* camera, mat4 matrix);
void gic_camera_process_keyboard(Camera* camera, CameraMovement direction, float delta);
void gic_camera_process_mouse_movement(Camera* camera, float x, float y, bool constrain_pitch);
void gic_camera_process_mouse_scroll(Camera* camera, float y);
