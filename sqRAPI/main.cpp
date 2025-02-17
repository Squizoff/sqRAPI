#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include "api.hpp"
#include <dinput.h>

LPDIRECTINPUT8 dinput; 
LPDIRECTINPUTDEVICE8 keyboard;
LPDIRECTINPUTDEVICE8 mouse;

#undef min
#undef max
#define w 800
#define h 600

struct Vertex {
    float x, y, z;
};

struct Face {
    int v1, v2, v3;
};

std::vector<Vertex> vertices;
std::vector<Face> faces;

struct Camera {
    float x, y, z;
    float yaw, pitch;
    float speed;

    Camera() : x(5), y(5), z(-5), yaw(0.75f), pitch(-5.7f), speed(10.0f) {}

    void Move(float dx, float dy, float dz) {
        x += dx;
        y += dy;
        z += dz;
    }

    void Rotate(float dYaw, float dPitch) {
        yaw += dYaw;
        pitch += dPitch;
    }
};

Camera camera;

struct LightSource {
    float x, y, z;
    LightSource() : x(5.0f), y(5.0f), z(5.0f) {}
} lightSource;

bool loadObj(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "v") {
            Vertex vertex;
            ss >> vertex.x >> vertex.y >> vertex.z;
            vertices.push_back(vertex);
        }
        else if (token == "f") {
            Face face;
            ss >> face.v1 >> face.v2 >> face.v3;
            face.v1--; face.v2--; face.v3--;
            faces.push_back(face);
        }
    }
    return true;
}

Point3D projectVertex(const Vertex& vertex, float width, float height) {
    const float fov = 90.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    float aspectRatio = width / height;
    float projectionMatrix[4][4] = {
        {aspectRatio * tan(fov * 0.5f), 0, 0, 0},
        {0, tan(fov * 0.5f), 0, 0},
        {0, 0, (farPlane + nearPlane) / (nearPlane - farPlane), (2 * farPlane * nearPlane) / (nearPlane - farPlane)},
        {0, 0, -1, 0}
    };

    float x = vertex.x - camera.x;
    float y = vertex.y - camera.y;
    float z = -(vertex.z - camera.z);

    float cosYaw = cos(camera.yaw);
    float sinYaw = sin(camera.yaw);
    float cosPitch = cos(camera.pitch);
    float sinPitch = sin(camera.pitch);

    float rotatedX = x * cosYaw - z * sinYaw;
    float rotatedZ = x * sinYaw + z * cosYaw;

    float rotatedY = y * cosPitch - rotatedZ * sinPitch;
    rotatedZ = y * sinPitch + rotatedZ * cosPitch;

    float projectedX = (projectionMatrix[0][0] * rotatedX + projectionMatrix[0][1] * rotatedY + projectionMatrix[0][2] * rotatedZ + projectionMatrix[0][3]) /
        (projectionMatrix[3][0] * rotatedX + projectionMatrix[3][1] * rotatedY + projectionMatrix[3][2] * rotatedZ + projectionMatrix[3][3]);
    float projectedY = (projectionMatrix[1][0] * rotatedX + projectionMatrix[1][1] * rotatedY + projectionMatrix[1][2] * rotatedZ + projectionMatrix[1][3]) /
        (projectionMatrix[3][0] * rotatedX + projectionMatrix[3][1] * rotatedY + projectionMatrix[3][2] * rotatedZ + projectionMatrix[3][3]);

    projectedX = (projectedX + 1) * 0.5f * width;
    projectedY = (1 - projectedY) * 0.5f * height;

    return { static_cast<float>(projectedX), static_cast<float>(projectedY) };
}

Vertex calculateNormal(const Vertex& v1, const Vertex& v2, const Vertex& v3) {
    Vertex edge1 = { v2.x - v1.x, v2.y - v1.y, v2.z - v1.z };
    Vertex edge2 = { v3.x - v1.x, v3.y - v1.y, v3.z - v1.z };
    Vertex normal = {
        edge1.y * edge2.z - edge1.z * edge2.y,
        edge1.z * edge2.x - edge1.x * edge2.z,
        edge1.x * edge2.y - edge1.y * edge2.x
    };
    return normal;
}

float calculateLighting(Vertex& normal, const LightSource& light) {
    Vertex lightDirection = { light.x - normal.x, light.y - normal.y, light.z - normal.z };
    float normalLength = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    float lightLength = sqrt(lightDirection.x * lightDirection.x + lightDirection.y * lightDirection.y + lightDirection.z * lightDirection.z);
    normal.x /= normalLength;
    normal.y /= normalLength;
    normal.z /= normalLength;
    lightDirection.x /= lightLength;
    lightDirection.y /= lightLength;
    lightDirection.z /= lightLength;

    float dotProduct = normal.x * lightDirection.x + normal.y * lightDirection.y + normal.z * lightDirection.z;
    return std::max(dotProduct, 0.0f);
}

bool isTriangleView(const Point3D& p1, const Point3D& p2, const Point3D& p3, float width, float height) {
    return (p1.x >= 0 && p1.x < width && p1.y >= 0 && p1.y < height) ||
        (p2.x >= 0 && p2.x < width && p2.y >= 0 && p2.y < height) ||
        (p3.x >= 0 && p3.x < width && p3.y >= 0 && p3.y < height);
}

void renderModel(float width, float height) {
    for (const auto& face : faces) {
        Point3D p1 = projectVertex(vertices[face.v1], width, height);
        Point3D p2 = projectVertex(vertices[face.v2], width, height);
        Point3D p3 = projectVertex(vertices[face.v3], width, height);

        if (isTriangleView(p1, p2, p3, width, height)) {
            Vertex normal = calculateNormal(vertices[face.v1], vertices[face.v2], vertices[face.v3]);
            float lighting = calculateLighting(normal, lightSource);

            int c1 = 255, c2 = 255, c3 = 255;

            SQRAPI::RenderTriangle(p1, p2, p3, c1, c2, c3, lighting);

            //polygon lines
            /*SQRAPI::SetRenderColor(255, 0, 0);
            SQRAPI::RenderDrawLine({ p1.x, p1.y }, { p2.x, p2.y });
            SQRAPI::RenderDrawLine({ p2.x, p2.y }, { p3.x, p3.y });
            SQRAPI::RenderDrawLine({ p3.x, p3.y }, { p1.x, p1.y });*/
        }
    }
}

bool initDirectInput(HINSTANCE hInstance) {
    HRESULT hr;
    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&dinput, NULL);
    if (FAILED(hr)) {
        return false;
    }

    hr = dinput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
    if (FAILED(hr)) {
        return false;
    }

    hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
    if (FAILED(hr)) {
        return false;
    }

    hr = keyboard->SetCooperativeLevel(SQRAPI::window, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
    if (FAILED(hr)) {
        return false;
    }

    hr = keyboard->Acquire();
    if (FAILED(hr)) {
        return false;
    }

    hr = dinput->CreateDevice(GUID_SysMouse, &mouse, NULL);
    if (FAILED(hr)) {
        return false;
    }

    hr = mouse->SetDataFormat(&c_dfDIMouse);
    if (FAILED(hr)) {
        return false;
    }

    hr = mouse->SetCooperativeLevel(SQRAPI::window, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    if (FAILED(hr)) {
        return false;
    }

    hr = mouse->Acquire();
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

void handleKeyboard() {
    BYTE keyState[256];
    keyboard->GetDeviceState(sizeof(keyState), (LPVOID)&keyState);

    float dx = 0, dy = 0, dz = 0;
    const float moveSpeed = camera.speed;

    float deltaTime = SQRAPI::getDeltaTime();

    if (keyState[DIK_W] & 0x80) dz = moveSpeed * deltaTime;
    if (keyState[DIK_S] & 0x80) dz = -moveSpeed * deltaTime;
    if (keyState[DIK_A] & 0x80) dx = -moveSpeed * deltaTime;
    if (keyState[DIK_D] & 0x80) dx = moveSpeed * deltaTime;
    if (keyState[DIK_SPACE] & 0x80) dy = moveSpeed * deltaTime;
    if (keyState[DIK_LSHIFT] & 0x80) dy = -moveSpeed * deltaTime;

    if (keyState[DIK_UP] & 0x80) camera.Rotate(0, -0.05f * deltaTime);
    if (keyState[DIK_DOWN] & 0x80) camera.Rotate(0, 0.05f * deltaTime);
    if (keyState[DIK_LEFT] & 0x80) camera.Rotate(0.05f * deltaTime, 0);
    if (keyState[DIK_RIGHT] & 0x80) camera.Rotate(-0.05f * deltaTime, 0);

    camera.Move(dx, dy, dz);
}

void handleMouse() {
    DIMOUSESTATE mouseState;
    mouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState);

    const float sensitivity = 0.002f;

    camera.Rotate(-mouseState.lX * sensitivity, mouseState.lY * sensitivity);
}

int main(int argc, char* argv[]) {
    SQRAPI::Init("SQRAPI", w, h);
    if (!initDirectInput(SQRAPI::hInstance)) {
        return -1;
    }

    SQRAPI::SetBGColor(30, 0, 0, 1);

    if (!loadObj("model.obj")) {
        return 1;
    }

    uint32_t lastTime = SQRAPI::GetTicks();
    int frameCount = 0;
    char title[256];

    while (true) {
        SQRAPI::ClearScreen();

        handleKeyboard();
        handleMouse();

        SQRAPI::Begin();

        renderModel(w, h);

        SQRAPI::End();

        frameCount++;
        uint32_t currentTime = SQRAPI::GetTicks();
        if (currentTime - lastTime >= 1000) {
            int fps = frameCount;
            frameCount = 0;
            lastTime = currentTime;

            snprintf(title, sizeof(title), "FPS: %d", fps);
            SetWindowText(SQRAPI::window, title);
        }
    }

    SQRAPI::Exit();

    return 0;
}