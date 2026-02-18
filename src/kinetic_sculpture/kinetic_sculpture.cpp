// ============================================================================
// 3D Kinetic Sculpture — Inspired by BMW Museum Kinetic Installation
// ============================================================================
// A grid of metallic pillars that oscillate vertically, driven by layered
// sine-wave patterns.  The user can orbit the camera freely and cycle through
// several animation modes with the number keys.
//
// Controls
//   WASD / mouse   — fly camera
//   Scroll         — zoom
//   1-6            — switch wave pattern
//   Space          — pause / resume animation
//   +/-            — change animation speed
//   R              — reset camera
//   ESC            — quit
// ============================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>

#include <iostream>
#include <cmath>
#include <vector>
#include <string>

// ── callbacks & helpers ─────────────────────────────────────────────────────
void framebuffer_size_callback(GLFWwindow* window, int w, int h);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow* window);

// ── constants ───────────────────────────────────────────────────────────────
const unsigned int SCR_WIDTH  = 1280;
const unsigned int SCR_HEIGHT = 720;

// Grid
const int   GRID_SIZE    = 25;          // 25 × 25 pillars
const float GRID_SPACING = 0.45f;       // distance between pillar centres
const float PILLAR_RADIUS = 0.08f;      // visual half-width of a pillar
const float BASE_HEIGHT   = 0.05f;      // minimum pillar height
const float MAX_AMPLITUDE = 2.0f;       // max wave amplitude

// ── global state ────────────────────────────────────────────────────────────
Camera camera(glm::vec3(0.0f, 8.0f, 14.0f),
              glm::vec3(0.0f, 1.0f,  0.0f),
              -90.0f, -30.0f);
float lastX      = SCR_WIDTH  / 2.0f;
float lastY      = SCR_HEIGHT / 2.0f;
bool  firstMouse = true;
float deltaTime  = 0.0f;
float lastFrame  = 0.0f;

int   currentPattern = 1;   // 1..6
bool  paused         = false;
float animSpeed      = 1.0f;
float animTime       = 0.0f;

// ── wave-height function ────────────────────────────────────────────────────
// Returns a height in [0, MAX_AMPLITUDE] for the pillar at grid (ix,iz).
float pillarHeight(int ix, int iz, float t)
{
    float cx = (ix - GRID_SIZE / 2.0f) * GRID_SPACING;
    float cz = (iz - GRID_SIZE / 2.0f) * GRID_SPACING;
    float d  = sqrtf(cx * cx + cz * cz);          // distance from centre
    float h  = 0.0f;

    switch (currentPattern)
    {
    case 1: // concentric ripple
        h = sinf(d * 2.0f - t * 3.0f);
        break;
    case 2: // diagonal wave
        h = sinf((cx + cz) * 1.5f - t * 2.5f);
        break;
    case 3: // double spiral
        {
            float angle = atan2f(cz, cx);
            h = sinf(angle * 3.0f + d * 1.2f - t * 2.0f);
        }
        break;
    case 4: // cross wave
        h = sinf(cx * 2.0f - t * 2.5f) * sinf(cz * 2.0f - t * 2.5f);
        break;
    case 5: // breathing (radial pulse)
        h = sinf(d * 1.5f - t * 2.0f) * cosf(t * 0.5f);
        break;
    case 6: // random-ish chaos (layered)
        h = 0.5f * sinf(cx * 3.1f - t * 1.7f)
          + 0.3f * sinf(cz * 2.7f + t * 2.3f)
          + 0.2f * cosf((cx + cz) * 1.9f - t * 3.1f);
        break;
    default:
        h = sinf(d * 2.0f - t * 3.0f);
        break;
    }

    // remap [-1,1] → [BASE_HEIGHT, MAX_AMPLITUDE]
    return BASE_HEIGHT + (h * 0.5f + 0.5f) * MAX_AMPLITUDE;
}

// ── geometry helpers ────────────────────────────────────────────────────────
// Build a unit box [−0.5,0.5]³ with normals.  We'll scale it per-pillar.
struct Vertex { glm::vec3 pos; glm::vec3 norm; };

static std::vector<Vertex> makeCubeVertices()
{
    // 6 faces × 2 tris × 3 verts
    std::vector<Vertex> v;
    v.reserve(36);

    // helper: push a face quad (two tris) given 4 corners + normal
    auto face = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 n){
        v.push_back({a,n}); v.push_back({b,n}); v.push_back({c,n});
        v.push_back({a,n}); v.push_back({c,n}); v.push_back({d,n});
    };

    // front  (+Z)
    face({-0.5f,-0.5f, 0.5f},{ 0.5f,-0.5f, 0.5f},{ 0.5f, 0.5f, 0.5f},{-0.5f, 0.5f, 0.5f},{0,0,1});
    // back   (−Z)
    face({ 0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,-0.5f},{-0.5f, 0.5f,-0.5f},{ 0.5f, 0.5f,-0.5f},{0,0,-1});
    // left   (−X)
    face({-0.5f,-0.5f,-0.5f},{-0.5f,-0.5f, 0.5f},{-0.5f, 0.5f, 0.5f},{-0.5f, 0.5f,-0.5f},{-1,0,0});
    // right  (+X)
    face({ 0.5f,-0.5f, 0.5f},{ 0.5f,-0.5f,-0.5f},{ 0.5f, 0.5f,-0.5f},{ 0.5f, 0.5f, 0.5f},{1,0,0});
    // top    (+Y)
    face({-0.5f, 0.5f, 0.5f},{ 0.5f, 0.5f, 0.5f},{ 0.5f, 0.5f,-0.5f},{-0.5f, 0.5f,-0.5f},{0,1,0});
    // bottom (−Y)
    face({-0.5f,-0.5f,-0.5f},{ 0.5f,-0.5f,-0.5f},{ 0.5f,-0.5f, 0.5f},{-0.5f,-0.5f, 0.5f},{0,-1,0});

    return v;
}

// Build a flat base-plate quad (XZ plane, y = 0)
static std::vector<Vertex> makePlaneVertices(float halfSize)
{
    glm::vec3 n(0, 1, 0);
    float s = halfSize;
    return {
        {{-s, 0,  s}, n}, {{ s, 0,  s}, n}, {{ s, 0, -s}, n},
        {{-s, 0,  s}, n}, {{ s, 0, -s}, n}, {{-s, 0, -s}, n},
    };
}

// Build a sphere for the floating light orbs
static std::vector<Vertex> makeSphereVertices(int stacks, int slices)
{
    std::vector<Vertex> verts;
    for (int i = 0; i < stacks; ++i)
    {
        float phi0 = glm::pi<float>() * float(i)     / stacks - glm::pi<float>() / 2.0f;
        float phi1 = glm::pi<float>() * float(i + 1) / stacks - glm::pi<float>() / 2.0f;
        for (int j = 0; j < slices; ++j)
        {
            float th0 = 2.0f * glm::pi<float>() * float(j)     / slices;
            float th1 = 2.0f * glm::pi<float>() * float(j + 1) / slices;

            auto P = [](float ph, float th) -> glm::vec3 {
                return {cosf(ph)*cosf(th), sinf(ph), cosf(ph)*sinf(th)};
            };
            glm::vec3 a = P(phi0, th0), b = P(phi0, th1);
            glm::vec3 c = P(phi1, th1), d = P(phi1, th0);

            verts.push_back({a, a}); verts.push_back({d, d}); verts.push_back({c, c});
            verts.push_back({a, a}); verts.push_back({c, c}); verts.push_back({b, b});
        }
    }
    return verts;
}

// ── create VAO/VBO from vertex vector ───────────────────────────────────────
static unsigned int uploadMesh(const std::vector<Vertex>& verts, unsigned int& vbo)
{
    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, norm));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return vao;
}

// ── pattern names for HUD ───────────────────────────────────────────────────
const char* patternName(int p)
{
    switch(p) {
        case 1: return "Concentric Ripple";
        case 2: return "Diagonal Wave";
        case 3: return "Double Spiral";
        case 4: return "Cross Wave";
        case 5: return "Breathing Pulse";
        case 6: return "Layered Chaos";
        default: return "Unknown";
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// MAIN
// ═════════════════════════════════════════════════════════════════════════════
int main()
{
    // ── GLFW init ───────────────────────────────────────────────────────────
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "3D Kinetic Sculpture", NULL, NULL);
    if (!window) { std::cout << "GLFW window creation failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // ── GLAD init ───────────────────────────────────────────────────────────
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "GLAD init failed\n"; return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // ── shaders ─────────────────────────────────────────────────────────────
    Shader pillarShader("kinetic_pillar.vs", "kinetic_pillar.fs");
    Shader lightShader ("kinetic_light.vs",  "kinetic_light.fs");

    // ── geometry ────────────────────────────────────────────────────────────
    auto cubeVerts  = makeCubeVertices();
    auto planeVerts = makePlaneVertices(8.0f);
    auto sphereVerts = makeSphereVertices(16, 24);

    unsigned int cubeVBO, planeVBO, sphereVBO;
    unsigned int cubeVAO  = uploadMesh(cubeVerts,  cubeVBO);
    unsigned int planeVAO = uploadMesh(planeVerts, planeVBO);
    unsigned int sphereVAO = uploadMesh(sphereVerts, sphereVBO);
    int sphereVertCount = (int)sphereVerts.size();

    // ── light positions (floating orbs) ─────────────────────────────────────
    const int NUM_LIGHTS = 4;
    glm::vec3 baseLightPos[NUM_LIGHTS] = {
        { 3.0f, 4.0f,  3.0f},
        {-3.0f, 3.5f, -3.0f},
        { 4.0f, 5.0f, -2.0f},
        {-2.0f, 4.5f,  4.0f},
    };
    glm::vec3 lightColors[NUM_LIGHTS] = {
        {1.0f, 0.95f, 0.8f},
        {0.8f, 0.9f,  1.0f},
        {1.0f, 0.85f, 0.7f},
        {0.85f, 1.0f, 0.95f},
    };

    // Print controls
    std::cout << "===== 3D Kinetic Sculpture =====\n";
    std::cout << "  WASD / Mouse : Move camera\n";
    std::cout << "  Scroll       : Zoom\n";
    std::cout << "  1-6          : Switch wave pattern\n";
    std::cout << "  Space        : Pause / Resume\n";
    std::cout << "  +/-          : Speed up / slow down\n";
    std::cout << "  R            : Reset camera\n";
    std::cout << "  ESC          : Quit\n";
    std::cout << "================================\n\n";
    std::cout << "Pattern: " << currentPattern << " - " << patternName(currentPattern) << "\n";

    // ═════════════════════════════════════════════════════════════════════════
    // Render loop
    // ═════════════════════════════════════════════════════════════════════════
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (!paused)
            animTime += deltaTime * animSpeed;

        processInput(window);

        // ── clear ───────────────────────────────────────────────────────────
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);   // very dark blue-black
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ── matrices ────────────────────────────────────────────────────────
        glm::mat4 view       = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        // ── animate light positions ─────────────────────────────────────────
        glm::vec3 lightPos[NUM_LIGHTS];
        for (int i = 0; i < NUM_LIGHTS; i++)
        {
            float phase = i * 1.57f;
            lightPos[i] = baseLightPos[i]
                + glm::vec3(sinf(animTime * 0.4f + phase) * 1.5f,
                            sinf(animTime * 0.6f + phase) * 0.8f,
                            cosf(animTime * 0.5f + phase) * 1.5f);
        }

        // ════════════════════════════════════════════════════════════════════
        // 1.  Draw pillars + base  (pillarShader)
        // ════════════════════════════════════════════════════════════════════
        pillarShader.use();
        pillarShader.setMat4("view", view);
        pillarShader.setMat4("projection", projection);
        pillarShader.setVec3("viewPos", camera.Position);

        // lights
        for (int i = 0; i < NUM_LIGHTS; i++)
        {
            std::string idx = "lights[" + std::to_string(i) + "]";
            pillarShader.setVec3(idx + ".position",  lightPos[i]);
            pillarShader.setVec3(idx + ".color",     lightColors[i]);
            pillarShader.setFloat(idx + ".constant",  1.0f);
            pillarShader.setFloat(idx + ".linear",    0.09f);
            pillarShader.setFloat(idx + ".quadratic", 0.032f);
        }
        pillarShader.setInt("numLights", NUM_LIGHTS);

        // ambient / directional fill
        pillarShader.setVec3("dirLight.direction", glm::vec3(-0.3f, -1.0f, -0.4f));
        pillarShader.setVec3("dirLight.ambient",   glm::vec3(0.08f));
        pillarShader.setVec3("dirLight.diffuse",   glm::vec3(0.25f));
        pillarShader.setVec3("dirLight.specular",  glm::vec3(0.3f));

        // ── draw base plate ─────────────────────────────────────────────────
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.02f, 0.0f));
            pillarShader.setMat4("model", model);
            // dark surface
            pillarShader.setVec3("material.color",    glm::vec3(0.12f, 0.12f, 0.14f));
            pillarShader.setFloat("material.specular", 0.2f);
            pillarShader.setFloat("material.shininess", 16.0f);
            glBindVertexArray(planeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // ── draw pillars ────────────────────────────────────────────────────
        glBindVertexArray(cubeVAO);
        for (int ix = 0; ix < GRID_SIZE; ix++)
        {
            for (int iz = 0; iz < GRID_SIZE; iz++)
            {
                float h  = pillarHeight(ix, iz, animTime);
                float cx = (ix - GRID_SIZE / 2.0f) * GRID_SPACING;
                float cz = (iz - GRID_SIZE / 2.0f) * GRID_SPACING;

                glm::mat4 model(1.0f);
                model = glm::translate(model, glm::vec3(cx, h * 0.5f, cz));
                model = glm::scale(model, glm::vec3(PILLAR_RADIUS * 2.0f, h, PILLAR_RADIUS * 2.0f));
                pillarShader.setMat4("model", model);

                // colour: map height to a cool gradient
                float t = h / (BASE_HEIGHT + MAX_AMPLITUDE);  // 0..1
                glm::vec3 colLow (0.10f, 0.20f, 0.45f);   // deep blue
                glm::vec3 colMid (0.15f, 0.70f, 0.85f);   // cyan
                glm::vec3 colHigh(0.95f, 0.95f, 1.00f);   // near-white
                glm::vec3 col;
                if (t < 0.5f)
                    col = glm::mix(colLow, colMid, t * 2.0f);
                else
                    col = glm::mix(colMid, colHigh, (t - 0.5f) * 2.0f);

                pillarShader.setVec3("material.color",    col);
                pillarShader.setFloat("material.specular", 0.6f);
                pillarShader.setFloat("material.shininess", 64.0f);

                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // ════════════════════════════════════════════════════════════════════
        // 2.  Draw light orbs (lightShader — unlit emissive spheres)
        // ════════════════════════════════════════════════════════════════════
        lightShader.use();
        lightShader.setMat4("view", view);
        lightShader.setMat4("projection", projection);

        glBindVertexArray(sphereVAO);
        for (int i = 0; i < NUM_LIGHTS; i++)
        {
            glm::mat4 model(1.0f);
            model = glm::translate(model, lightPos[i]);
            model = glm::scale(model, glm::vec3(0.15f));
            lightShader.setMat4("model", model);
            lightShader.setVec3("lightColor", lightColors[i]);
            glDrawArrays(GL_TRIANGLES, 0, sphereVertCount);
        }

        // ── swap & poll ─────────────────────────────────────────────────────
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteBuffers(1, &sphereVBO);
    glfwTerminate();
    return 0;
}

// ═════════════════════════════════════════════════════════════════════════════
// Input
// ═════════════════════════════════════════════════════════════════════════════
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action != GLFW_PRESS) return;

    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_6)
    {
        currentPattern = key - GLFW_KEY_1 + 1;
        std::cout << "Pattern: " << currentPattern << " - " << patternName(currentPattern) << "\n";
    }
    if (key == GLFW_KEY_SPACE)
    {
        paused = !paused;
        std::cout << (paused ? "PAUSED\n" : "RESUMED\n");
    }
    if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)    // '+'
    {
        animSpeed = std::min(animSpeed + 0.25f, 5.0f);
        std::cout << "Speed: " << animSpeed << "x\n";
    }
    if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) // '-'
    {
        animSpeed = std::max(animSpeed - 0.25f, 0.25f);
        std::cout << "Speed: " << animSpeed << "x\n";
    }
    if (key == GLFW_KEY_R)
    {
        camera.Position = glm::vec3(0.0f, 8.0f, 14.0f);
        camera.Yaw   = -90.0f;
        camera.Pitch = -30.0f;
        std::cout << "Camera reset\n";
    }
}

void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

void mouse_callback(GLFWwindow*, double xposIn, double yposIn)
{
    float xpos = (float)xposIn, ypos = (float)yposIn;
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    camera.ProcessMouseMovement(xpos - lastX, lastY - ypos);
    lastX = xpos; lastY = ypos;
}

void scroll_callback(GLFWwindow*, double, double yoffset)
{
    camera.ProcessMouseScroll((float)yoffset);
}
