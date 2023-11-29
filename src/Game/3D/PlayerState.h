#include <math.h>
class PlayerState
{
public:
    PlayerState() : health(100), score(0), level(1), uniqueSkillChance(5) {}
    const float skillCooldown = 5.0f;
    const float rebellionCooldown = 8.0f;
    float skillCooldownEndTime = 0.0f;
    float rebellionCooldownEndTime = 0.0f;
    bool hasSkillAvailable()
    {
        return uniqueSkillChance > 0;
    }
    void update()
    {
        float currentTime = (float)glfwGetTime();
        if (isSkill && currentTime > skillEndTime)
        {

            isSkill = false;
            movementSpeed /= 1.5f;
            jumpForce -= 10;
        }
        if (isRebellion && currentTime > rebellionEndTime)
        {

            movementSpeed *= 2;
            isRebellion = false;
        }
    }
    void useSkill(float duration)
    {
        float currentTime = (float)glfwGetTime();
        if (!isSkill && uniqueSkillChance > 0 && currentTime > skillCooldownEndTime)
        {

            skillCooldownEndTime = currentTime + skillCooldown;
            movementSpeed *= 1.5f;
            jumpForce += 10;
            uniqueSkillChance--;
            skillEndTime = (float)glfwGetTime() + duration;
            isSkill = true;
        }
    }
    void useRebellion(float duration)
    {
        float currentTime = (float)glfwGetTime();
        if (!isRebellion && uniqueSkillChance > 0 && currentTime > rebellionCooldownEndTime)
        {
            rebellionCooldownEndTime = currentTime + rebellionCooldown;
            movementSpeed /= 2;
            isRebellion = true;
            rebellionEndTime = (float)glfwGetTime() + duration;
            uniqueSkillChance--;
        }
    }
    bool isSkillOnCooldown() const
    {
        return (float)glfwGetTime() < skillCooldownEndTime;
    }

    bool isRebellionOnCooldown() const
    {
        return (float)glfwGetTime() < rebellionCooldownEndTime;
    }
    bool isRebellionActive() const
    {
        return isRebellion;
    }
    bool isSkillAvailable() const
    {
        return isSkill;
    }
    void levelUp()
    {
        if (level < 1000)
        {
            level++;
            movementSpeed += 0.1f;
            health += 50;
            uniqueSkillChance++;
        }
    }

    void increaseScore(int amount)
    {
        score += amount;

        if (score >= level * 600 - health / 2 + getLevel() * 50)
        {
            levelUp();
        }
    }
    int getNextLevelUpScore()
    {

        return level * 600 - health / 2 + getLevel() * 50;
    }
    int getScore()
    {
        return score;
    }
    int getLevel()
    {
        return level;
    }
    float getMovementSpeed()
    {
        return movementSpeed;
    }
    float getJumpForce()
    {
        return jumpForce;
    }
    int getHealth()
    {
        return health;
    }
    int getUniqueSkillChance()
    {
        return uniqueSkillChance;
    }
    void takeDamage(int damage) { health -= damage; }
    float movementSpeed = 5;

private:
    int score;
    int health;
    int level;
    float jumpForce = 5;
    int uniqueSkillChance;
    bool isSkill = false;
    float skillEndTime;
    bool isRebellion = false;
    float rebellionEndTime;
};
