#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class Animation{
public:
    //basic constructor
    Animation() = default;
    //constructor for sprites that dont need to change direction with movement
    Animation(const std::string& folderPath, float speed);
    //constructor for sprites that need to change their direction of animation for moving
    Animation(const std::string& folderPath, float speed, bool doesMove);

    //loads sprite animation sheets from a specific folder and adds them to a vetor
    void loadFromFolder(const std::string& folderPath);
    //sets the speed of the animation
    void setSpeed(float speed);
    //updates the animations to the current frame
    void update(float deltaTime);
    //changes / sets the vector to be the files from the direction directory inside of the base folder
    void setDirection(const std::string& newDirection, const std::string& baseFolderPath);

    // a getter to display the current animation frame for the sprite
    const sf::Sprite& getSprite() const;
    const std::string& getCurrentDirection() const;

    
    void setPosition(const sf::Vector2f& pos);
    void setScale(const sf::Vector2f& scale);
    void setRotation(float angle);

    

private:
    std::vector<sf::Texture> _frames;
    std::unique_ptr<sf::Sprite> _sprite;

    std::string _currentDirection = "right";
    

    float _frameTime = 0.1f;
    float _elapsedTime = 0.f;
    std::size_t _currentFrame = 0;
};