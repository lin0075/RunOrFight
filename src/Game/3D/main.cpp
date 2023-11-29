#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "base/filesystem.h"
#include "base/shader.h"
#include "base/camera.h"
#include "Model.h"
#include "Render.h"
#include "PlayerState.h"
#include "EnemyManager.h"
#include "CollisionSystem.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 900;

Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

const float gravity = 9.8f;
float verticalVelocity = 0.0f;
bool isOnGround = true;
const float groundLevel = 0.0f;

float lastScoreUpdateTime = 0.0f;
const float scoreUpdateInterval = 1.0f;
bool isPaused = false;
static bool backspacePressedLastFrame = false;
bool backspacePressedThisFrame;
int main()
{

    string option;
    option = FileSystem::getPath("resources/textures/hdr/resting_place_8k.hdr");

    cout << "Loading..." << endl;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Run or Fight!", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    int xPos = (mode->width - SCR_WIDTH) / 2;
    int yPos = (mode->height - SCR_HEIGHT) / 2;

    glfwSetWindowPos(window, xPos, yPos);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    glDepthFunc(GL_LEQUAL);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    Shader shader(FileSystem::getPath("shaders/pbr.vs").c_str(), FileSystem::getPath("shaders/pbr.fs").c_str());

    Shader textShader(FileSystem::getPath("shaders/text.vs").c_str(), FileSystem::getPath("shaders/text.fs").c_str());

    Shader pbrShader(FileSystem::getPath("shaders/pbr.vs").c_str(), FileSystem::getPath("shaders/pbr.fs").c_str());
    Shader equirectangularToCubemapShader(FileSystem::getPath("shaders/cubemap.vs").c_str(), FileSystem::getPath("shaders/equirectangular_to_cubemap.fs").c_str());
    Shader irradianceShader(FileSystem::getPath("shaders/cubemap.vs").c_str(), FileSystem::getPath("shaders/irradiance_convolution.fs").c_str());
    Shader prefilterShader(FileSystem::getPath("shaders/cubemap.vs").c_str(), FileSystem::getPath("shaders/prefilter.fs").c_str());
    Shader brdfShader(FileSystem::getPath("shaders/brdf.vs").c_str(), FileSystem::getPath("shaders/brdf.fs").c_str());
    Shader backgroundShader(FileSystem::getPath("shaders/background.vs").c_str(), FileSystem::getPath("shaders/background.fs").c_str());

    pbrShader.use();
    pbrShader.setInt("irradianceMap", 0);
    pbrShader.setInt("prefilterMap", 1);
    pbrShader.setInt("brdfLUT", 2);
    pbrShader.setInt("albedoMap", 3);
    pbrShader.setInt("normalMap", 4);
    pbrShader.setInt("metallicMap", 5);
    pbrShader.setInt("roughnessMap", 6);
    pbrShader.setInt("aoMap", 7);

    backgroundShader.use();
    backgroundShader.setInt("environmentMap", 0);

    Model playerModel(FileSystem::getPath("resources/objects/basicCharacter/basicCharacter.obj"));
    Model enemyModel(FileSystem::getPath("resources/objects/basicCharacter/basicCharacter.obj"));

    unsigned int plasticAlbedoMap = loadTexture(FileSystem::getPath("resources/textures/pbr/plastic/albedo.png").c_str());
    unsigned int plasticNormalMap = loadTexture(FileSystem::getPath("resources/textures/pbr/plastic/normal.png").c_str());
    unsigned int plasticMetallicMap = loadTexture(FileSystem::getPath("resources/textures/pbr/plastic/metallic.png").c_str());
    unsigned int plasticRoughnessMap = loadTexture(FileSystem::getPath("resources/textures/pbr/plastic/roughness.png").c_str());
    unsigned int plasticAOMap = loadTexture(FileSystem::getPath("resources/textures/pbr/plastic/ao.png").c_str());

    unsigned int goldAlbedoMap = loadTexture(FileSystem::getPath("resources/textures/pbr/gold/albedo.png").c_str());
    unsigned int goldNormalMap = loadTexture(FileSystem::getPath("resources/textures/pbr/gold/normal.png").c_str());
    unsigned int goldMetallicMap = loadTexture(FileSystem::getPath("resources/textures/pbr/gold/metallic.png").c_str());
    unsigned int goldRoughnessMap = loadTexture(FileSystem::getPath("resources/textures/pbr/gold/roughness.png").c_str());
    unsigned int goldAOMap = loadTexture(FileSystem::getPath("resources/textures/pbr/gold/ao.png").c_str());

    unsigned int playerAlbedoMap = loadTexture(FileSystem::getPath("resources/objects/basicCharacter/skin_man.png").c_str());
    unsigned int playerAlbedoMap2 = loadTexture(FileSystem::getPath("resources/objects/basicCharacter/skin_adventurer.png").c_str());
    unsigned int playerAlbedoMap3 = loadTexture(FileSystem::getPath("resources/objects/basicCharacter/skin_soldier.png").c_str());

    unsigned int playerNormalMap = loadTexture(FileSystem::getPath("resources/textures/pbr/plastic/normal.png").c_str());
    unsigned int playerMetallicMap = loadTexture(FileSystem::getPath("resources/textures/pbr/plastic/metallic.png").c_str());
    unsigned int playerRoughnessMap = loadTexture(FileSystem::getPath("resources/textures/pbr/plastic/roughness.png").c_str());
    unsigned int playerAOMap = loadTexture(FileSystem::getPath("resources/textures/pbr/plastic/ao.png").c_str());

    unsigned int enemyAlbedoMap = loadTexture(FileSystem::getPath("resources/objects/basicCharacter/skin_orc.png").c_str());
    unsigned int enemyNormalMap = loadTexture(FileSystem::getPath("resources/textures/pbr/plastic/normal.png").c_str());
    unsigned int enemyMetallicMap = loadTexture(FileSystem::getPath("resources/textures/pbr/paintedplaster/metallic.png").c_str());
    unsigned int enemyRoughnessMap = loadTexture(FileSystem::getPath("resources/textures/pbr/paintedplaster/roughness.png").c_str());
    unsigned int enemyAOMap = loadTexture(FileSystem::getPath("resources/textures/pbr/paintedplaster/ao.png").c_str());

    glm::vec3 lightPositions[] = {
        glm::vec3(500.0f, 20.0f, 500.0f),
        glm::vec3(-500.0f, 20.0f, 500.0f),
        glm::vec3(500.0f, 20.0f, -500.0f),
        glm::vec3(-500.0f, 20.0f, -500.0f),
    };
    glm::vec3 lightColors[] = {
        glm::vec3(300000.0f, 300000.0f, 300000.0f),
        glm::vec3(300000.0f, 300000.0f, 300000.0f),
        glm::vec3(300000.0f, 300000.0f, 300000.0f),
        glm::vec3(300000.0f, 300000.0f, 300000.0f)};

    // 从HDR图片加载环境贴图,转换为立方体贴图格式
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;

    float *data = stbi_loadf(option.c_str(), &width, &height, &nrComponents, 0);
    unsigned int hdrTexture;
    if (data)
    {
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load HDR image." << std::endl;
    }

    unsigned int envCubemap;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    equirectangularToCubemapShader.use();
    equirectangularToCubemapShader.setInt("equirectangularMap", 0);
    equirectangularToCubemapShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        equirectangularToCubemapShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    // 从环境贴图生成辐照度贴图(irradiance map), 用于approximation全局光照
    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    irradianceShader.use();
    irradianceShader.setInt("environmentMap", 0);
    irradianceShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glViewport(0, 0, 32, 32);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        irradianceShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // 从环境贴图生成预过滤贴图(prefilter map),用于计算漫反射和镜面反射的粗糙度
    unsigned int prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    prefilterShader.use();
    prefilterShader.setInt("environmentMap", 0);
    prefilterShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {

        unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        prefilterShader.setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            prefilterShader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderCube();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // 生成BRDF积分查找表(LUT),用于实现基于物理的渲染方程近似
    unsigned int brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);

    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    glViewport(0, 0, 512, 512);
    brdfShader.use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    pbrShader.use();
    pbrShader.setMat4("projection", projection);
    backgroundShader.use();
    backgroundShader.setMat4("projection", projection);

    std::string gameOverText1 = "GAME OVER";
    std::string gameOverText2 = "Press Enter to Restart";
    glm::mat4 projection1 = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));

    std::string fpsText;
    std::string scoreText;
    std::string healthText;
    std::string levelText;
    std::string skillChanceText;
    std::string skillStatusText;
    std::string rebellionStatusText;
    std::string tipText;
    std::string levelUpText;

    glm::mat4 view;

    glm::vec3 forwardDirection;
    glm::vec3 rightDirection;

    unsigned int currentPlayerTexture;

    glm::mat4 playerModelMatrix;

    std::vector<Model>::iterator it;

    glm::mat4 model = glm::mat4(1.0f);

    int scrWidth, scrHeight;
    glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
    glViewport(0, 0, scrWidth, scrHeight);
    initTextRendering();
    std::cout << "Game is Starting... " << std::endl;
    glm::vec3 initialPlayerPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraPosition = glm::vec3(0.0f, 20.0f, -20.0f);
    camera.Position = cameraPosition;

    std::cout << "Press Enter to Pause(or Start)" << std::endl;
loop:
    playerModel.setPosition(initialPlayerPosition);
    PlayerState player;
    EnemyManager enemyManager(enemyModel, 300.0f);
    std::vector<Model> enemies;
    glm::vec3 directionToPlayer = glm::normalize(playerModel.getPosition() - camera.Position);
    camera.Front = directionToPlayer;

    while (!glfwWindowShouldClose(window))
    {

        processInput(window);

        if (!isPaused)
        {
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            {
                glm::vec3 directionToPlayer = glm::normalize(playerModel.getPosition() - camera.Position);
                camera.Front = directionToPlayer;
            }
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (player.getHealth() < 0)
            {
                isPaused = true;
                glEnable(GL_CULL_FACE);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                float centerX = SCR_WIDTH / 2.0f - (gameOverText1.size() * 10);
                float centerY = SCR_HEIGHT / 2.0f;
                renderText(textShader, gameOverText1, centerX, centerY, 1.0f, glm::vec3(1.0, 0.0, 0.0));
                renderText(textShader, gameOverText2, centerX - 5.0f, centerY - 50.0f, 0.5f, glm::vec3(1.0, 0.0, 0.0));
                glDisable(GL_CULL_FACE);
                glDisable(GL_BLEND);
                std::cout << "Game is Restarting... " << std::endl;
                enemies.clear();
                goto loop;
            }

            float currentFrame = (float)glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            enemyManager.updateDifficulty(player.getLevel());

            if (currentFrame - lastScoreUpdateTime >= scoreUpdateInterval)
            {
                if (player.isSkillAvailable())
                {
                    player.increaseScore(10 * player.getLevel());
                }
                player.increaseScore(10);
                lastScoreUpdateTime = currentFrame;
            }

            glEnable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            textShader.use();

            textShader.setMat4("projection", projection1);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            tipText = "Press Q to Look at Player";
            renderText(textShader, tipText, 25.0f, SCR_HEIGHT - 100.0f, 0.4f, glm::vec3(1.0, 1.0, 1.0));
            float fps = 1.0f / deltaTime;
            fpsText = "FPS: " + std::to_string(static_cast<int>(fps));
            renderText(textShader, fpsText, 25.0f, SCR_HEIGHT - 75.0f, 0.5f, glm::vec3(1.0, 0.5, 0.0));

            scoreText = "Score: " + std::to_string(player.getScore());
            renderText(textShader, scoreText, 25.0f, SCR_HEIGHT - 50.0f, 1.0f, glm::vec3(0.5, 0.8, 1.0));

            healthText = "Health: " + std::to_string(player.getHealth());
            renderText(textShader, healthText, 25.0f, 25.0f, 1.0f, glm::vec3(1.0, 0.9, 0));

            levelText = "Level: " + std::to_string(player.getLevel());
            renderText(textShader, levelText, SCR_WIDTH - 200.0f, SCR_HEIGHT - 50.0f, 0.8f, glm::vec3(1.0, 0.5, 0));

            skillChanceText = "Skill Chance: " + std::to_string(player.getUniqueSkillChance());
            renderText(textShader, skillChanceText, SCR_WIDTH - 300.0f, 110.0f, 0.6f, glm::vec3(0.5, 0.8, 1.0));
            skillStatusText = player.isSkillOnCooldown() ? "Run: Coolingdown" : (player.isSkillAvailable() ? "Run: Active" : "Press E to Run!");
            renderText(textShader, skillStatusText, SCR_WIDTH - 300.0f, 75.0f, 0.5f, glm::vec3(0.5, 0.8, 1.0));

            rebellionStatusText = player.isRebellionOnCooldown() ? "Fight: Coolingdown" : (player.isRebellionActive() ? "Fight: Active" : "Press R to Fight!");
            renderText(textShader, rebellionStatusText, SCR_WIDTH - 300.0f, 50.0f, 0.5f, glm::vec3(0.5, 0.8, 1.0));
            levelUpText = "Next Level Up: " + std::to_string(player.getNextLevelUpScore()) + " points";
            renderText(textShader, levelUpText, SCR_WIDTH - 300.0f, 25.0f, 0.4f, glm::vec3(1.0, 0.5, 0.0));

            glDisable(GL_CULL_FACE);
            glDisable(GL_BLEND);

            pbrShader.use();
            view = camera.GetViewMatrix();
            pbrShader.setMat4("view", view);
            pbrShader.setVec3("camPos", camera.Position);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

            if (!isOnGround)
            {
                verticalVelocity -= gravity * deltaTime;
            }

            playerModel.position.y += verticalVelocity * deltaTime;

            if (playerModel.position.y <= groundLevel)
            {
                playerModel.position.y = groundLevel;
                verticalVelocity = 0.0f;
                isOnGround = true;
            }

            if (isOnGround && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            {
                verticalVelocity = player.getJumpForce();
                isOnGround = false;
            }
            float velocity = player.getMovementSpeed() * deltaTime;
            forwardDirection = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
            rightDirection = glm::normalize(glm::cross(forwardDirection, glm::vec3(0.0f, 1.0f, 0.0f)));

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                playerModel.position += forwardDirection * velocity;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                playerModel.position -= forwardDirection * velocity;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                playerModel.position -= rightDirection * velocity;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                playerModel.position += rightDirection * velocity;

            player.update();
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            {
                player.useSkill(3.0f);
            }
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            {
                player.useRebellion(3.0f);
            }

            currentPlayerTexture = playerAlbedoMap;
            if (player.isSkillAvailable())
            {
                currentPlayerTexture = playerAlbedoMap2;
            }
            if (player.isRebellionActive())
            {
                currentPlayerTexture = playerAlbedoMap3;
            }
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, currentPlayerTexture);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, playerNormalMap);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, playerMetallicMap);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, playerRoughnessMap);
            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, playerAOMap);

            playerModel.setScale(glm::vec3(0.2f, 0.2f, 0.2f));

            float playerAngle = atan2(forwardDirection.x, forwardDirection.z);

            playerModelMatrix = glm::mat4(1.0f);
            playerModelMatrix = glm::translate(playerModelMatrix, playerModel.getPosition());
            playerModelMatrix = glm::rotate(playerModelMatrix, playerAngle * 5, glm::vec3(0.0, 1.0, 0.0));
            playerModelMatrix = glm::scale(playerModelMatrix, playerModel.getScale());
            pbrShader.setMat4("model", playerModelMatrix);
            pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(playerModelMatrix))));
            playerModel.Draw(pbrShader);

            enemyManager.updateEnemies(enemies, playerModel, deltaTime);

            playerModel.updateBoundingBox();

            for (it = enemies.begin(); it != enemies.end();)
            {
                it->updateBoundingBox();
                if (CollisionSystem::checkCollision(playerModel, *it))
                {
                    CollisionSystem::handleCollision(playerModel, *it);
                    if (player.isRebellionActive())
                    {
                        it = enemies.erase(it);
                        player.increaseScore(10 * (int)enemyManager.speedMultiplier);
                    }
                    else
                    {
                        glActiveTexture(GL_TEXTURE3);
                        glBindTexture(GL_TEXTURE_2D, enemyAlbedoMap);
                        glActiveTexture(GL_TEXTURE4);
                        glBindTexture(GL_TEXTURE_2D, enemyNormalMap);
                        glActiveTexture(GL_TEXTURE5);
                        glBindTexture(GL_TEXTURE_2D, enemyMetallicMap);
                        glActiveTexture(GL_TEXTURE6);
                        glBindTexture(GL_TEXTURE_2D, enemyRoughnessMap);
                        glActiveTexture(GL_TEXTURE7);
                        glBindTexture(GL_TEXTURE_2D, enemyAOMap);

                        pbrShader.use();
                        model = glm::mat4(1.0f);
                        model = glm::translate(model, it->getPosition());
                        model = glm::rotate(model, glm::radians(it->getRotation().y), glm::vec3(0.0, 1.0, 0.0));
                        model = glm::scale(model, it->getScale());
                        pbrShader.setMat4("model", model);
                        pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));

                        it->Draw(pbrShader);

                        player.takeDamage((int)enemyManager.baseDamage);
                        ++it;
                    }
                }
                else
                {

                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, enemyAlbedoMap);
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_2D, enemyNormalMap);
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, enemyMetallicMap);
                    glActiveTexture(GL_TEXTURE6);
                    glBindTexture(GL_TEXTURE_2D, enemyRoughnessMap);
                    glActiveTexture(GL_TEXTURE7);
                    glBindTexture(GL_TEXTURE_2D, enemyAOMap);

                    pbrShader.use();
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, it->getPosition());
                    model = glm::rotate(model, glm::radians(it->getRotation().y), glm::vec3(0.0, 1.0, 0.0));
                    model = glm::scale(model, it->getScale());
                    pbrShader.setMat4("model", model);
                    pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));

                    it->Draw(pbrShader);
                    ++it;
                }
            }
            for (unsigned int i = 0; i < sizeof(lightPositions) / sizeof(lightPositions[0]); ++i)
            {
                glm::vec3 newPos = lightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
                newPos = lightPositions[i];
                pbrShader.setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
                pbrShader.setVec3("lightColors[" + std::to_string(i) + "]", lightColors[i]);

                model = glm::mat4(1.0f);
                model = glm::translate(model, newPos);
                model = glm::scale(model, glm::vec3(0.0f));
                pbrShader.setMat4("model", model);
                pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
                renderSphere();
            }

            backgroundShader.use();

            backgroundShader.setMat4("view", view);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
            renderCube();
        }
        else
        {

            lastFrame = (float)glfwGetTime();
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    deleteTextRenderingResources();
    deleteSphereRenderingResources();
    deleteShaderProgram(shader.ID);
    deleteFramebuffer(captureFBO);
    deleteRenderbuffer(captureRBO);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    backspacePressedThisFrame = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
    if (backspacePressedThisFrame && !backspacePressedLastFrame)
    {
        isPaused = !isPaused;
        if (!isPaused)
        {

            lastFrame = (float)glfwGetTime();
        }
    }

    backspacePressedLastFrame = backspacePressedThisFrame;
}
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
unsigned int loadTexture(char const *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
