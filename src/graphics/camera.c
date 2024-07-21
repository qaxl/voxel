#include "camera.h"

#include <cglm/call/vec3.h>

#include <math.h>
#include <string.h>

static void gic__calculate_camera_vecs(Camera* camera) {
    vec3 front;
    front[0] = cos(camera->yaw) * cos(camera->pitch);
    front[1] = sin(camera->pitch);
    front[2] = sin(camera->yaw) * cos(camera->pitch);

    glm_normalize_to(front, camera->front);

    glm_cross(camera->front, GLM_YUP, camera->right);
    glm_normalize(camera->right);

    glm_cross(camera->right, camera->front, camera->up);
}

Camera gic_create_camera(vec3 position, float yaw, float pitch) {
    Camera camera;

    memcpy(camera.position, position, sizeof(vec3));

    camera.yaw = glm_rad(yaw);
    camera.pitch = glm_rad(pitch);
    gic__calculate_camera_vecs(&camera);

    camera.movement_speed = 12.5f;
    camera.mouse_sensitivity = 0.01f;
    camera.zoom = 45.0f;

    return camera;
}

Camera gic_create_camera_default() {
    return gic_create_camera((vec3){ 0.0f, 0.0f, 0.0f }, -90.0f, 180.0f);
}

void gic_camera_get_view_matrix(Camera* camera, mat4 matrix) {
    vec3 combined_position_and_front;
    glm_vec3_add(camera->position, camera->front, combined_position_and_front);

    glm_lookat(camera->position, combined_position_and_front, camera->up, matrix);
}

void gic_camera_process_keyboard(Camera* camera, CameraMovement direction, float delta) {
    float velo = camera->movement_speed * delta;

    if (direction == CAM_FORWARD) {
        glm_vec3_muladds(camera->front, velo, camera->position);
    }

    if (direction == CAM_BACKWARDS) {
        glm_vec3_muladds(camera->front, -velo, camera->position);
    }

    if (direction == CAM_LEFT) {
        glm_vec3_muladds(camera->right, velo, camera->position);
    }

    if (direction == CAM_RIGHT) {
        glm_vec3_muladds(camera->right, -velo, camera->position);
    }
}

void gic_camera_process_mouse_movement(Camera* camera, float x, float y, bool constrain_pitch) {
    x *= camera->mouse_sensitivity;
    y *= camera->mouse_sensitivity;

    camera->yaw += glm_rad(x);
    camera->pitch += glm_rad(y);

    if (constrain_pitch) {
        if (camera->pitch > glm_rad(89.0f)) {
            camera->pitch = glm_rad(89.0f);
        } else if (camera->pitch < glm_rad(-89.0f)) {
            camera->pitch = glm_rad(-89.0f);
        }
    }

    gic__calculate_camera_vecs(camera);
}

void gic_camera_process_mouse_scroll(Camera* camera, float y) {
    camera->zoom -= y;
    if (camera->zoom < 1.0f) {
        camera->zoom = 1.0f;
    } else if (camera->zoom > 120.0f) {
        camera->zoom = 120.0f;
    }
}
