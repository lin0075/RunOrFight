

#ifndef ENEMY_MANAGER_H
#define ENEMY_MANAGER_H

#include <vector>
#include "Model.h"
#include <glm/glm.hpp>
#define _USE_MATH_DEFINES
#include <math.h>
#include <random>
#include <algorithm>
class EnemyManager
{
public:
    EnemyManager(Model &enemyModelTemplate, float spawnRadius);

    void updateEnemies(std::vector<Model> &enemies, const Model &playerModel, float deltaTime);
    void spawnEnemy(std::vector<Model> &enemies, const glm::vec3 &playerPosition);
    float baseSpeed = 3.0f;
    float baseDamage = 1.0f;
    float speedMultiplier = 1.0f;
    int initialEnemyCount = 1;
    int maxEnemyCount = 500;

    void updateDifficulty(int level);

private:
    Model &enemyModelTemplate;
    float spawnRadius;
    unsigned int maxEnemies;
    float movementSpeed = 3.0f;
    void spawnEnemy(const glm::vec3 &playerPosition);
    void EnemyManager::moveTowardsPlayer(Model &enemy, const glm::vec3 &playerPosition, float deltaTime, const std::vector<Model> &allEnemies);
};

EnemyManager::EnemyManager(Model &enemyModelTemplate, float spawnRadius)
    : enemyModelTemplate(enemyModelTemplate), spawnRadius(spawnRadius)
{
}
void EnemyManager::updateDifficulty(int level)
{
    int newEnemyCount = initialEnemyCount + 5 * level;
    if (newEnemyCount > maxEnemyCount)
    {
        newEnemyCount = maxEnemyCount;
    }

    maxEnemies = newEnemyCount;
}
void EnemyManager::updateEnemies(std::vector<Model> &enemies, const Model &playerModel, float deltaTime)
{

    while (enemies.size() < maxEnemies)
    {
        spawnEnemy(enemies, playerModel.getPosition());
    }

    for (auto &enemy : enemies)
    {
        glm::vec3 playerPos = playerModel.getPosition();
        glm::vec3 enemyPos = enemy.getPosition();
        float distanceToPlayer = glm::distance(enemyPos, playerPos);

        float chargeProbability = std::min(50.0f, 5.0f + 45.0f * (spawnRadius - distanceToPlayer) / spawnRadius);

        if (!enemy.getIsCharging() && (rand() % 100) < chargeProbability)
        {
            enemy.startCharge(3.0f);
        }
        moveTowardsPlayer(enemy, playerPos, deltaTime, enemies);
        enemy.updateCharging();
    }
}

void EnemyManager::spawnEnemy(std::vector<Model> &enemies, const glm::vec3 &playerPosition)
{
    static std::random_device rd;
    static std::mt19937 rng(rd());

    std::uniform_real_distribution<float> angleDist(0.0f, 2 * (float)M_PI);
    std::uniform_real_distribution<float> distanceDist(100.0f, spawnRadius);
    std::uniform_real_distribution<float> scaleDist(0.1f, 0.3f);
    Model newEnemy = enemyModelTemplate;

    float angle = angleDist(rng);
    float distance = distanceDist(rng);
    glm::vec3 spawnPosition = playerPosition + glm::vec3(cos(angle) * distance, 0.0f, sin(angle) * distance);
    newEnemy.setPosition(spawnPosition);

    float scale_factor = scaleDist(rng);
    newEnemy.setScale(glm::vec3(scale_factor));

    float rotation_angle = angleDist(rng);
    newEnemy.setRotation(glm::vec3(0.0f, rotation_angle, 0.0f));

    enemies.push_back(newEnemy);
}

void EnemyManager::moveTowardsPlayer(Model &enemy, const glm::vec3 &playerPosition, float deltaTime, const std::vector<Model> &allEnemies)
{
    glm::vec3 enemyPos = enemy.getPosition();
    glm::vec3 direction = glm::normalize(playerPosition - enemyPos);

    direction.y = 0.0f;

    glm::vec3 separationForce(0.0f);
    int closeEnemiesCount = 0;
    float speedMultiplier = enemy.getIsCharging() ? 2.0f : 1.0f;

    for (const auto &otherEnemy : allEnemies)
    {
        if (&enemy != &otherEnemy)
        {
            glm::vec3 otherPos = otherEnemy.getPosition();
            float distance = glm::distance(enemyPos, otherPos);
            float separationThreshold = 10.0f;

            if (distance < separationThreshold)
            {
                closeEnemiesCount++;
                glm::vec3 awayFromEnemy = glm::normalize(enemyPos - otherPos);
                awayFromEnemy.y = 0.0f;
                separationForce += awayFromEnemy / distance;
            }
        }
    }

    if (closeEnemiesCount > 0)
    {
        separationForce /= static_cast<float>(closeEnemiesCount);
    }

    glm::vec3 combinedDirection = glm::normalize(direction + separationForce);

    float currentSpeed = baseSpeed * speedMultiplier;
    glm::vec3 newPosition = enemyPos + combinedDirection * currentSpeed * deltaTime;
    enemy.setPosition(newPosition);

    glm::vec3 toPlayer = glm::normalize(playerPosition - enemy.getPosition());
    toPlayer.y = 0.0f;
    float angleToPlayer = glm::degrees(atan2(toPlayer.x, toPlayer.z));
    enemy.setRotation(glm::vec3(0.0f, angleToPlayer, 0.0f));
}

#endif