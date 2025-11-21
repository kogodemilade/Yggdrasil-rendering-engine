#include "ygg/engine.hpp"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
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

Ygg::Line Ygg::RenderEngine::createLine()
{
    Line line;

    glGenVertexArrays(1, &line.VAO);
    glGenBuffers(1, &line.VBO);

    glBindVertexArray(line.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, line.VBO);

    // 2 vertices, each: pos(3) + normal(3) + color(3) = 9 floats per vertex
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 18, nullptr, GL_DYNAMIC_DRAW);

    // Position (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal (location = 1) — unused, but shader needs it
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Color (location = 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    return line;
}


void Ygg::RenderEngine::updateLine(const Line& line, 
                              glm::vec3 p1, glm::vec3 p2,
                              glm::vec3 color)
{
    float normal[3] = {0.0f, 0.0f, 0.0f}; // dummy normal

    float data[18] = {
        // vertex 1
        p1.x, p1.y, p1.z,
        normal[0], normal[1], normal[2],
        color.x, color.y, color.z,

        // vertex 2
        p2.x, p2.y, p2.z,
        normal[0], normal[1], normal[2],
        color.x, color.y, color.z
    };

    glBindBuffer(GL_ARRAY_BUFFER, line.VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(data), data);
}


Ygg::Mesh Ygg::RenderEngine::createBox(const glm::vec3 &pos, const glm::quat &orientation,
                                       float width, float height, float depth, const glm::vec3 &color) {

    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos)
                    * glm::mat4_cast(orientation)
                    * glm::scale(glm::mat4(1.0f), {width, height, depth});

    // program.setMat4("model", model);

    std::vector<Vertex> vertices(8);

    for (int i = 0; i < 8; i++) {
        glm::vec4 transformed = model * glm::vec4(unit_box[i], 1.0f);
        vertices[i].pos = glm::vec3(transformed);
        vertices[i].color = color;
        vertices[i].normal = glm::vec3(0.0f); // temporarily zero
    }

    std::vector<unsigned> indices(unit_indices, unit_indices + 36);

    // ---------------------------------------
    // 3. Compute normals per triangle
    // ---------------------------------------
std::vector<glm::vec3> normals(vertices.size(), glm::vec3(0.0f));

for (int i = 0; i < 36; i += 3) {
    unsigned int i0 = indices[i];
    unsigned int i1 = indices[i+1];
    unsigned int i2 = indices[i+2];

    glm::vec3 v0 = vertices[i0].pos;
    glm::vec3 v1 = vertices[i1].pos;
    glm::vec3 v2 = vertices[i2].pos;

    glm::vec3 N = glm::normalize(glm::cross(v1 - v0, v2 - v0));

    normals[i0] += N;
    normals[i1] += N;
    normals[i2] += N;
}

for (size_t i = 0; i < vertices.size(); i++)
    vertices[i].normal = glm::normalize(normals[i]);

    // ---------------------------------------
    // 4. Upload to OpenGL
    // ---------------------------------------
    Mesh mesh;

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(Vertex),   // FIXED
                 vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned),
                 indices.data(),
                 GL_STATIC_DRAW);

    // layout: pos(3), normal(3), color(3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    mesh.indexCount = 36;
    mesh.model = model;
    return mesh;
}







Ygg::Mesh Ygg::RenderEngine::createSphere(const glm::vec3 &pos,
const glm::quat &orientation,
float radius,
const glm::vec3 &color,
unsigned int stacks,
unsigned int slices)
{
std::vector<Vertex> vertices;
std::vector<unsigned int> indices;


// generate vertices
for (unsigned int i = 0; i <= stacks; ++i) {
float v = (float)i / (float)stacks;
float phi = v * glm::pi<float>(); // 0..pi


for (unsigned int j = 0; j <= slices; ++j) {
float u = (float)j / (float)slices;
float theta = u * glm::two_pi<float>(); // 0..2pi


float x = sin(phi) * cos(theta);
float y = cos(phi);
float z = sin(phi) * sin(theta);


glm::vec3 position = glm::vec3(x, y, z) * radius;
glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));


Vertex vert;
vert.pos = position;
vert.normal = normal;
vert.color = color;
vertices.push_back(vert);
}
}


// generate indices (triangles)
for (unsigned int i = 0; i < stacks; ++i) {
for (unsigned int j = 0; j < slices; ++j) {
unsigned int first = i * (slices + 1) + j;
unsigned int second = first + slices + 1;


// two triangles per quad
indices.push_back(first);
indices.push_back(second);
indices.push_back(first + 1);


indices.push_back(second);
indices.push_back(second + 1);
indices.push_back(first + 1);
}
}


// transform positions by model (do NOT transform normals here with non-uniform scale; we will store model)
glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(orientation);


// upload to GPU
Mesh mesh;
glGenVertexArrays(1, &mesh.VAO);
glGenBuffers(1, &mesh.VBO);
glGenBuffers(1, &mesh.EBO);


glBindVertexArray(mesh.VAO);


glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);


glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);


// vertex layout: pos(0), normal(1), color(2)
glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));


glEnableVertexAttribArray(1);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));


glEnableVertexAttribArray(2);
glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));


glBindVertexArray(0);


mesh.indexCount = static_cast<unsigned int>(indices.size());
mesh.model = model;
return mesh;
}






void Ygg::RenderEngine::drawMesh(const Mesh &mesh, const glm::mat4& view, const glm::mat4& projection, const glm::vec3 &cameraPos, const glm::mat4 &rotAndPos) {

    glm::mat4 updated = rotAndPos;
    program.use();    
    program.setMat4("projection", projection);
    program.setMat4("view", view);
    program.setMat4("model", updated);
    program.setVec3("lightPos", glm::vec3(0.0f, 10.0f, 3.0f));   
    program.setVec3("lightColor", glm::vec3(1.0f));             
    program.setVec3("cameraPos", cameraPos);           
    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Ygg::RenderEngine::drawLine(const Line& line,
                            const glm::mat4& view,
                            const glm::mat4& proj, 
                            const glm::vec3 &cameraPos,
                            glm::vec3 color)
{
    program.use();
    glm::mat4 model = glm::mat4(1.0f);
    program.setMat4("model", model);
    program.setMat4("view", view);
    program.setMat4("projection", proj);
    program.setVec3("lightPos", glm::vec3(0.0f, 10.0f, 3.0f));   
    program.setVec3("lightColor", glm::vec3(1.0f));   
    // program.setVec3("color", color);
    program.setVec3("cameraPos", cameraPos);            // ADD


    glBindVertexArray(line.VAO);
    glDrawArrays(GL_LINES, 0, 2);
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

