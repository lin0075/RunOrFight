#include <iostream>
#include <algorithm>
#include "Model.h"
#include <glm/glm.hpp>

class CollisionSystem
{
public:
    static bool checkCollision(const Model &model1, const Model &model2);
    static void handleCollision(Model &model1, Model &model2);
};

bool CollisionSystem::checkCollision(const Model &model1, const Model &model2)
{

    auto &box1 = model1.getBoundingBox();
    auto &box2 = model2.getBoundingBox();

    return (box1.min.x <= box2.max.x && box1.max.x >= box2.min.x) &&
           (box1.min.y <= box2.max.y && box1.max.y >= box2.min.y) &&
           (box1.min.z <= box2.max.z && box1.max.z >= box2.min.z);
}

void CollisionSystem::handleCollision(Model &model1, Model &model2)
{

    float overlapX = std::min(model1.getBoundingBox().max.x, model2.getBoundingBox().max.x) -
                     std::max(model1.getBoundingBox().min.x, model2.getBoundingBox().min.x);
    float overlapY = std::min(model1.getBoundingBox().max.y, model2.getBoundingBox().max.y) -
                     std::max(model1.getBoundingBox().min.y, model2.getBoundingBox().min.y);
    float overlapZ = std::min(model1.getBoundingBox().max.z, model2.getBoundingBox().max.z) -
                     std::max(model1.getBoundingBox().min.z, model2.getBoundingBox().min.z);

    float minOverlap = std::min({overlapX, overlapY, overlapZ});

    glm::vec3 responseVector(0.0f);
    if (minOverlap == overlapX)
    {
        responseVector.x = (model1.position.x < model2.position.x) ? -overlapX : overlapX;
    }
    else if (minOverlap == overlapY)
    {
        responseVector.y = (model1.position.y < model2.position.y) ? -overlapY : overlapY;
    }
    else
    {
        responseVector.z = (model1.position.z < model2.position.z) ? -overlapZ : overlapZ;
    }

    model1.position += responseVector * 1.0f;
    model2.position -= responseVector * 1.0f;
}
