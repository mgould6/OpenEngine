#include "Camera.h"
#include "../common_utils/Logger.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    ProjectionMatrix = glm::perspective(glm::radians(Zoom), 4.0f / 3.0f, 0.1f, 100.0f);

    updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
    Position = glm::vec3(posX, posY, posZ);
    WorldUp = glm::vec3(upX, upY, upZ);
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}



void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)
        Position += Front * velocity;
    if (direction == BACKWARD)
        Position -= Front * velocity;
    if (direction == LEFT)
        Position -= Right * velocity;
    if (direction == RIGHT)
        Position += Right * velocity;
    if (direction == UP)
        Position += Up * velocity;  // Use the dynamic camera Up vector
    if (direction == DOWN)
        Position -= Up * velocity;  // Use the dynamic camera Up vector
}


void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (constrainPitch) {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset) {
    if (Zoom >= 1.0f && Zoom <= 45.0f)
        Zoom -= yoffset;
    if (Zoom <= 1.0f)
        Zoom = 1.0f;
    if (Zoom >= 45.0f)
        Zoom = 45.0f;
    ProjectionMatrix = glm::perspective(glm::radians(Zoom), 4.0f / 3.0f, 0.1f, 100.0f);
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

void Camera::setCameraToFitModel(const Model& model) {
    glm::vec3 center = model.getBoundingBoxCenter();
    float radius = model.getBoundingBoxRadius();

    // Adjust the distance based on the bounding box size
    float distance = (radius * 2.5f) / glm::tan(glm::radians(this->Zoom / 2.0f));

    this->Position = center + glm::vec3(0.0f, 0.0f, distance);
    this->Front = glm::normalize(center - this->Position);

    Logger::log("DEBUG: Adjusted Camera Position: (" +
        std::to_string(Position.x) + ", " +
        std::to_string(Position.y) + ", " +
        std::to_string(Position.z) + ")", Logger::INFO);

    updateCameraVectors();
}


void Camera::setMovementSpeed(float speed) {
    MovementSpeed = speed;
}

void Camera::SetPerspective(float fov, float aspectRatio, float nearPlane, float farPlane) {
    ProjectionMatrix = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
}