#include "../include/engine.hpp"
// #include ""

// using namespace 
// define static member
GLFWwindow* Ygg::RenderEngine::window = nullptr;

int Ygg::RenderEngine::initGL(const char *vShader, const char *fShader) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Ygg Engine", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // compile shader program (assumes Shader has a ctor taking paths)
    program = Program(vShader, fShader);

    glEnable(GL_DEPTH_TEST);
    return 0;
}

GLFWwindow* Ygg::RenderEngine::getWindow() { return window; }

Ygg::Camera Ygg::RenderEngine::createCamera(glm::vec3 pos) {
    return Camera(pos);
}

Ygg::Mesh Ygg::RenderEngine::createBox(const glm::vec3 &pos, const glm::quat &orientation,
                                       float width, float height, float depth, const glm::vec3 &color) {

    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos)
                    * glm::mat4_cast(orientation)
                    * glm::scale(glm::mat4(1.0f), {width, height, depth});

    // program.setMat4("model", model);

    std::vector<float> vertexData;
    for (int i = 0; i < 8; ++i) {
        glm::vec4 transformed = model * glm::vec4(unit_box[i], 1.0f);
        vertexData.insert(vertexData.end(), {
            unit_box[i].x, unit_box[i].y, unit_box[i].z,
            color.r, color.g, color.b
        });
    }

    Mesh mesh;
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unit_indices), unit_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    mesh.indexCount = 36;
    mesh.model = model;
    return mesh;
}

Ygg::Mesh Ygg::RenderEngine::createSphere(const glm::vec3 &pos, const glm::quat &orientation,
                                         float radius, const glm::vec3 &color,
                                         unsigned int stacks, unsigned int slices) {

    std::vector<float> vertexData;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i <= stacks; ++i) {
        float v = (float)i / stacks;
        float phi = v * glm::pi<float>();

        for (unsigned int j = 0; j <= slices; ++j) {
            float u = (float)j / slices;
            float theta = u * glm::two_pi<float>();

            float x = sin(phi) * cos(theta);
            float y = cos(phi);
            float z = sin(phi) * sin(theta);

            vertexData.insert(vertexData.end(), {
                x * radius, y * radius, z * radius,
                color.r, color.g, color.b
            });
        }
    }

    for (unsigned int i = 0; i < stacks; ++i) {
        for (unsigned int j = 0; j < slices; ++j) {
            unsigned int first = i * (slices + 1) + j;
            unsigned int second = first + slices + 1;
            indices.insert(indices.end(), { first, second, first + 1, second, second + 1, first + 1 });
        }
    }

    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(orientation);
    for (size_t i = 0; i < vertexData.size(); i += 6) {
        glm::vec4 transformed = model * glm::vec4(vertexData[i], vertexData[i + 1], vertexData[i + 2], 1.0f);
        vertexData[i] = transformed.x;
        vertexData[i + 1] = transformed.y;
        vertexData[i + 2] = transformed.z;
    }

    Mesh mesh;
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    mesh.indexCount = static_cast<unsigned int>(indices.size());
    mesh.model = model;
    return mesh;
}

void Ygg::RenderEngine::drawMesh(const Mesh &mesh, const glm::mat4& view, const glm::mat4& projection) {
    program.use();
    program.setMat4("projection", projection);
    program.setMat4("view", view);
    program.setMat4("model", mesh.model);
    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Ygg::RenderEngine::cleanupMesh(Mesh &mesh) {
    if (mesh.VAO) glDeleteVertexArrays(1, &mesh.VAO);
    if (mesh.VBO) glDeleteBuffers(1, &mesh.VBO);
    if (mesh.EBO) glDeleteBuffers(1, &mesh.EBO);
    mesh = {};
}

void Ygg::RenderEngine::terminate() {
    if (window) glfwDestroyWindow(window);
    glfwTerminate();
}

void Ygg::RenderEngine::setCameraUniforms(const Camera &cam) {
    program.use();
    glm::mat4 view = cam.getViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(cam.getFov()), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

    // Assumes Shader has setMat4 API
    program.setMat4("view", view);
    program.setMat4("projection", proj);
}

