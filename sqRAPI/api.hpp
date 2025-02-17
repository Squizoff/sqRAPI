#pragma once
#include <GL/glew.h>
#include <Windows.h>
#include <iostream>
#include <cmath>
#include <memory>

class Point3D {
public:
    float x, y, z;

    Point3D(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f) : x(_x), y(_y), z(_z) {}

    Point3D operator+(const Point3D& other) const {
        return Point3D(x + other.x, y + other.y, z + other.z);
    }

    Point3D operator-(const Point3D& other) const {
        return Point3D(x - other.x, y - other.y, z - other.z);
    }

    Point3D operator*(float scalar) const {
        return Point3D(x * scalar, y * scalar, z * scalar);
    }

    Point3D operator/(float scalar) const {
        return (scalar != 0.0f) ? Point3D(x / scalar, y / scalar, z / scalar) : Point3D(0.0f, 0.0f, 0.0f);
    }

    bool operator==(const Point3D& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const Point3D& other) const {
        return !(*this == other);
    }

    float DistanceTo(const Point3D& other) const {
        return std::sqrt(std::pow(x - other.x, 2) + std::pow(y - other.y, 2) + std::pow(z - other.z, 2));
    }

    friend std::ostream& operator<<(std::ostream& os, const Point3D& p) {
        os << "(" << p.x << ", " << p.y << ", " << p.z << ")";
        return os;
    }
};

class SQRAPI {
public:
    static HINSTANCE hInstance;
    static HWND window;
    static HDC hDC;
    static HGLRC hRC;

    static bool Init(const char* name = "Window", int w = 640, int h = 480) {
        WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_OWNDC, WindowProc, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "sqRAPI", NULL };
        if (!RegisterClassEx(&wc)) {
            return false;
        }

        window = CreateWindowEx(0, "sqRAPI", name, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, w, h, NULL, NULL, wc.hInstance, NULL);
        if (!window) {
            return false;
        }

        hDC = GetDC(window);
        PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0 };
        int pf = ChoosePixelFormat(hDC, &pfd);
        SetPixelFormat(hDC, pf, &pfd);
        hRC = wglCreateContext(hDC);
        wglMakeCurrent(hDC, hRC);

        GLenum err = glewInit();
        if (err != GLEW_OK) {
            return false;
        }

        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

        //glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glViewport(0, 0, w, h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, 1);

        glMatrixMode(GL_MODELVIEW);

        ShowWindow(window, SW_SHOW);
        UpdateWindow(window);

        return true;
    }

    static void Exit() {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hRC);
        ReleaseDC(window, hDC);
        DestroyWindow(window);
    }

    static void ClearScreen() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    static void End() {
        SwapBuffers(hDC);
    }

    static void Begin() {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                exit(0);
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    static void SetRenderColor(int r, int g, int b) {
        glColor3f(r / 255.0f, g / 255.0f, b / 255.0f);
    }

    static void SetBGColor(int r, int g, int b, float a) {
        glClearColor(r / 255.0f, g / 255.0f, b / 255.0f, a);
    }

    static uint32_t GetTicks() {
        return static_cast<uint32_t>(GetTickCount64());
    }

    static float getDeltaTime() {
        static float lastTime = 0.0f;
        float currentTime = static_cast<float>(GetTickCount64()) / 1000.0f;
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        return deltaTime;
    }

    static void DisableCursor() {
        ShowCursor(FALSE);
        SetCapture(window);
        RECT rect;
        GetWindowRect(window, &rect);
        SetCursorPos((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2);
    }

    static void RenderDrawLine(const Point3D& p1, const Point3D& p2) {
        GLfloat vertices[] = {
            p1.x, p1.y, p1.z,
            p2.x, p2.y, p2.z
        };

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
        glEnableVertexAttribArray(0);

        glDrawArrays(GL_LINES, 0, 2);

        glDisableVertexAttribArray(0);
    }

    static void RenderTriangle(const Point3D& p1, const Point3D& p2, const Point3D& p3, int c1, int c2, int c3, float c4) {
        GLfloat vertices[] = {
            p1.x, p1.y, p1.z,
            p2.x, p2.y, p2.z,
            p3.x, p3.y, p3.z
        };

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
        glEnableVertexAttribArray(0);

        glColor3f((c1 * c4) / 255.f, (c2 * c4) / 255.f, (c3 * c4) / 255.f);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glDisableVertexAttribArray(0);
    }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;

        case WM_SIZE:
            glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_MOUSEMOVE:
            DisableCursor();

            static bool firstMouse = true;
            static int lastX = 0, lastY = 0;

            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            if (firstMouse) {
                lastX = x;
                lastY = y;
                firstMouse = false;
            }

            int offsetX = x - lastX;
            int offsetY = y - lastY;

            lastX = x;
            lastY = y;

            RECT rect;
            GetWindowRect(hwnd, &rect);
            int centerX = (rect.left + rect.right) / 2;
            int centerY = (rect.top + rect.bottom) / 2;
            SetCursorPos(centerX, centerY);

            return 0;
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
};

HINSTANCE SQRAPI::hInstance = GetModuleHandle(NULL);
HWND SQRAPI::window;
HDC SQRAPI::hDC;
HGLRC SQRAPI::hRC;
