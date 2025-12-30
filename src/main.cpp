/* CSCI 200: Final Project - Blades at Dawn
 *
 * Author: Logan Bordewin-Allen

 * For credits see Credits.txt
 
 * Description:
 *    A 2d platformer, and my first game.
 */

#include "Animation.hpp"
#include "TileMap.hpp"
#include "Tile.hpp"

#include <SFML/Graphics.hpp>

#include <iostream>
#include <optional>
#include <cmath>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

// kind of just used once
// maybe remove at some point
const float PLAYER_WIDTH = 32.f;
const float PLAYER_HEIGHT = 32.f;

// Score constants
const int POINTS_PER_LEVEL = 1000;
const int DEATH_PENALTY = 100;
const int TIME_BONUS_PER_SECOND = 10;

// Leaderboard entry structure
struct LeaderboardEntry {
    std::string name;
    int score;
    
    bool operator>(const LeaderboardEntry& other) const {
        return score > other.score;
    }
};

// Level data structure
struct LevelData {
    std::string mapFile;
    float spawnX, spawnY;
    float winX1, winY1, winX2, winY2; // win zone boundaries
};

// Define all levels here
std::vector<LevelData> levels = {
    {"assets/map/map1.json", 96.f, 928.f, 1664.f, 704.f, 1696.f, 736.f},
    {"assets/map/map2.json", 32.f, 928.f, 256.f, 480.f, 288.f, 512.f},
    {"assets/map/map3.json", 128.f, 896.f, 1152.f, 512.f, 1184.f, 544.f},
    // top left, bottom right for final gate
};


// Leaderboard functions
std::vector<LeaderboardEntry> loadLeaderboard(const std::string& filename){
    std::vector<LeaderboardEntry> leaderboard;
    std::ifstream file(filename);
    
    if (file.is_open()){
        std::string name;
        int score;
        while (file >> name >> score){
            leaderboard.push_back({name, score});
        }
        file.close();
    }
    return leaderboard;
}

void saveLeaderboard(const std::string& filename, const std::vector<LeaderboardEntry>& leaderboard){
    std::ofstream file(filename);
    
    if (file.is_open()){
        for (const auto& entry : leaderboard){
            file << entry.name << " " << entry.score << "\n";
        }
        file.close();
    }
}

void addToLeaderboard(std::vector<LeaderboardEntry>& leaderboard, const std::string& name, int score){
    leaderboard.push_back({name, score});
    std::sort(leaderboard.begin(), leaderboard.end(), std::greater<LeaderboardEntry>());
    
    // Keep only top 10
    if (leaderboard.size() > 10){
        leaderboard.resize(10);
    }
}

// Helper function to check AABB collision
bool checkAABBCollision(sf::FloatRect a, sf::FloatRect b){
    return a.position.x < b.position.x + b.size.x &&
           a.position.x + a.size.x > b.position.x &&
           a.position.y < b.position.y + b.size.y &&
           a.position.y + a.size.y > b.position.y;
}

// Helper function to resolve collision 
void resolveCollision(float& playerX, float& playerY, float& playerVelY, bool& hasJump, bool& hasDash, float& xSpeed, sf::FloatRect playerBounds, sf::FloatRect tileBounds){
    float overlapLeft = (playerBounds.position.x + playerBounds.size.x) - tileBounds.position.x;
    float overlapRight = (tileBounds.position.x + tileBounds.size.x) - playerBounds.position.x;
    float overlapTop = (playerBounds.position.y + playerBounds.size.y) - tileBounds.position.y;
    float overlapBottom = (tileBounds.position.y + tileBounds.size.y) - playerBounds.position.y;

    // Find smallest overlap
    float minOverlap = overlapLeft;
    if (overlapRight < minOverlap) minOverlap = overlapRight;
    if (overlapTop < minOverlap) minOverlap = overlapTop;
    if (overlapBottom < minOverlap) minOverlap = overlapBottom;

    if(minOverlap == overlapTop && playerVelY > 0){
        // Collision from above 
        playerY = tileBounds.position.y - PLAYER_HEIGHT;
        playerVelY = 0.f;
        hasJump = true;
        hasDash = true;
    }else if (minOverlap == overlapBottom && playerVelY < 0){
        // Collision from below 
        playerY = tileBounds.position.y + tileBounds.size.y;
        playerVelY = 0.f;
    }else if (minOverlap == overlapLeft && xSpeed > 0){
        // Collision from left
        playerX = tileBounds.position.x - PLAYER_WIDTH;
        xSpeed = 0.f;
    }else if (minOverlap == overlapRight && xSpeed < 0){
        // Collision from right
        playerX = tileBounds.position.x + tileBounds.size.x;
        xSpeed = 0.f;
    }
}

// Helper function for idle animation
bool isIdle(float xSpeed){
    const float EPS = 0.1f; // small threshold for better idle
    return std::abs(xSpeed) < EPS;
}

// Function to load a level
bool loadLevel(int levelNum, TileMap& tilemap, float& xPos, float& yPos, float& xSpeed, float& ySpeed, bool& hasJump, bool& hasDash, float& mapWidth, float& mapHeight, int& lives){
    if (levelNum < 1 || levelNum > (int)levels.size()){
        std::cerr << "Invalid level number: " << levelNum << "\n";
        return false;
    }
    
    LevelData& level = levels[levelNum - 1];
    
    // Create a new TileMap instance
    TileMap newTilemap;
    if (!newTilemap.loadFromFile(level.mapFile)){
        std::cerr << "Failed to load level " << levelNum << ": " << level.mapFile << "\n";
        return false;
    }
    


    // Replace old tilemap with new one
    tilemap = newTilemap;
    
    // Reset player to spawn position
    xPos = level.spawnX;
    yPos = level.spawnY;
    xSpeed = 0.f;
    ySpeed = 0.f;
    hasJump = true;
    hasDash = true;
    lives = 3;
    
    // Update map dimensions
    mapWidth = tilemap.getWidth() * tilemap.getTileWidth();
    mapHeight = tilemap.getHeight() * tilemap.getTileHeight();
    
    return true;
}

// Check for valid leaderboard initials
bool isValidInitials(const std::string& initials) {
    if (initials.length() != 3) return false;
    
    for (char c : initials) {
        if (c < 'A' || c > 'Z') return false;
    }
    return true;
}

int main(){
    // window making
    unsigned int windowSizeX = 1920;
    unsigned int windowSizeY = 1080;
    sf::RenderWindow window(sf::VideoMode({windowSizeX, windowSizeY}), "Blades at Dawn");

    std::string GAME_STATE = "menu";

    // frame rate set as not to have it hardware limited
    window.setFramerateLimit(60);

    std::string playerInitials = "";
    int finalScore = 0;

    // clock instantiation
    sf::Clock clock;
    sf::Clock levelClock;

    // font and text 
    sf::Font font;
    if (!font.openFromFile("assets/fonts/JAPAN_RAMEN.otf")){
        std::cerr << "Failed to load font\n";
        return -1;
    }

    // Load leaderboard
    std::vector<LeaderboardEntry> leaderboard = loadLeaderboard("leaderboard.txt");
    
    // Menu title
    sf::Text mainMenuTitle(font, "BLADES AT DAWN", 72);
    mainMenuTitle.setFillColor(sf::Color(255, 215, 0));
    mainMenuTitle.setOutlineColor(sf::Color(139, 69, 19));
    mainMenuTitle.setOutlineThickness(3.f);
    sf::FloatRect titleBounds = mainMenuTitle.getLocalBounds();
    mainMenuTitle.setOrigin({titleBounds.size.x / 2.f, titleBounds.size.y / 2.f});
    mainMenuTitle.setPosition({windowSizeX/2.f, windowSizeY/3.f});
    
    // Menu options
    sf::Text startText(font, "Press W to Start", 36);
    startText.setFillColor(sf::Color::White);
    sf::FloatRect startBounds = startText.getLocalBounds();
    startText.setOrigin({startBounds.size.x / 2.f, startBounds.size.y / 2.f});
    startText.setPosition({windowSizeX/2.f, windowSizeY/2.f});
    
    sf::Text leaderboardText(font, "Press L for Leaderboard", 30);
    leaderboardText.setFillColor(sf::Color::White);
    sf::FloatRect leaderboardBounds = leaderboardText.getLocalBounds();
    leaderboardText.setOrigin({leaderboardBounds.size.x / 2.f, leaderboardBounds.size.y / 2.f});
    leaderboardText.setPosition({windowSizeX/2.f, windowSizeY/2.f + 80.f});
    
    sf::Text controlsText(font, "Controls: WASD to move, W to jump, Q/E to dash", 24);
    controlsText.setFillColor(sf::Color(200, 200, 200));
    sf::FloatRect controlsBounds = controlsText.getLocalBounds();
    controlsText.setOrigin({controlsBounds.size.x / 2.f, controlsBounds.size.y / 2.f});
    controlsText.setPosition({windowSizeX/2.f, windowSizeY/2.f + 160.f});
    
    sf::Text exitText(font, "Press ESC to Exit", 24);
    exitText.setFillColor(sf::Color(150, 150, 150));
    sf::FloatRect exitBounds = exitText.getLocalBounds();
    exitText.setOrigin({exitBounds.size.x / 2.f, exitBounds.size.y / 2.f});
    exitText.setPosition({windowSizeX/2.f, windowSizeY - 100.f});

    // win text
    sf::Text winText(font, "You Win!", 100);
    winText.setFillColor(sf::Color::White);
    sf::FloatRect winBounds = winText.getLocalBounds();
    winText.setOrigin({winBounds.size.x / 2.f, winBounds.size.y / 2.f});
    winText.setPosition({windowSizeX/2.f, windowSizeY/2.f});

    // Final score text
    sf::Text finalScoreText(font, "", 40);
    finalScoreText.setFillColor(sf::Color(255, 215, 0));

    // lose text
    sf::Text loseText(font, "You Lose :(", 100);
    loseText.setFillColor(sf::Color::White);
    sf::FloatRect loseBounds = loseText.getLocalBounds();
    loseText.setOrigin({loseBounds.size.x / 2.f, loseBounds.size.y / 2.f});
    loseText.setPosition({windowSizeX/2.f, windowSizeY/2.f});

    // restart text
    sf::Text restartText(font, "Press W to restart", 36);
    restartText.setFillColor(sf::Color::White);
    sf::FloatRect restartBounds = restartText.getLocalBounds();
    restartText.setOrigin({restartBounds.size.x / 2.f, restartBounds.size.y / 2.f});
    restartText.setPosition({windowSizeX/2.f, windowSizeY - 200.f});

    
    // Pulsing effect variables
    float pulseTimer = 0.f;
    float pulseSpeed = 3.f;

    // Game variables
    float xPos = 96.f;
    float yPos = 928.f;
    float xSpeed = 0.f;
    float ySpeed = 0.f;
    float xAccel = 400.f;
    float yAccel = 500.f; // gravity

    int dashLength = 15;
    int dash = 0;
    
    bool hasJump = true;
    bool hasDash = true;
    bool dashDirection = true;
    const float moveSpeed = 200.f;
    int lives = 3;
    int currentLevel = 1;
    int totalScore = 0;

    // map loading
    TileMap tilemap;
    float mapWidth = 0.f;
    float mapHeight = 0.f;

    // Load initial level
    if (!loadLevel(currentLevel, tilemap, xPos, yPos, xSpeed, ySpeed, hasJump, hasDash, mapWidth, mapHeight, lives)){
        return -1;
    }

    float cameraShrinkAmount = 1.7;
    float cameraWidth = windowSizeX / cameraShrinkAmount;
    float cameraHeight = windowSizeY / cameraShrinkAmount;

    // All animations instantiation
    Animation background("assets/images/background", 0);
    background.setScale({4,4});
    background.setPosition({0,0});

    Animation playerAnim("assets/images/player", 10, true);
    playerAnim.setDirection("right", "assets/images/player");
    playerAnim.setPosition({xPos,yPos});

    Animation menuBackground("assets/images/menubackground", 0);
    background.setScale({4,4});
    background.setPosition({0,0});
    
    // virtual camera 
    sf::View camera(sf::FloatRect(sf::Vector2f(0, 0), sf::Vector2f(windowSizeX / cameraShrinkAmount, windowSizeY / cameraShrinkAmount)));
    sf::View mainMenu(sf::FloatRect(sf::Vector2f(0, 0), sf::Vector2f(windowSizeX, windowSizeY)));

    // main outer loop
    while (window.isOpen()){
        
        // Menu State
        if(GAME_STATE == "menu"){
            clock.restart();
            window.setView(mainMenu);
            while (GAME_STATE == "menu" && window.isOpen()){
                float dt = clock.restart().asSeconds();
                pulseTimer += dt;
                
                // Menu event handling
                while (const std::optional event = window.pollEvent()){
                    if (event->is<sf::Event::Closed>()){
                        window.close();
                    }
                    else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
                        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape){
                            window.close();
                        }
                        else if (keyPressed->scancode == sf::Keyboard::Scancode::W){
                            // Reset game state
                            currentLevel = 1;
                            lives = 3;
                            totalScore = 0;
                            loadLevel(currentLevel, tilemap, xPos, yPos, xSpeed, ySpeed, hasJump, hasDash, mapWidth, mapHeight, lives);
                            levelClock.restart();
                            GAME_STATE = "playing";
                        }
                        else if (keyPressed->scancode == sf::Keyboard::Scancode::L){
                            GAME_STATE = "leaderboard";
                        }
                    }
                }
                
                // Pulsing effect
                float alpha = 128.f + 127.f * std::sin(pulseTimer * pulseSpeed);
                startText.setFillColor(sf::Color(255, 255, 255, static_cast<unsigned char>(alpha)));
                
                // Slight bobbing effect for title
                float titleY = windowSizeY/3.f + 10.f * std::sin(pulseTimer * 2.f);
                mainMenuTitle.setPosition({windowSizeX/2.f, titleY});
                
                window.draw(menuBackground.getSprite());
                
                window.draw(mainMenuTitle);
                window.draw(startText);
                window.draw(leaderboardText);
                window.draw(controlsText);
                window.draw(exitText);
                
                window.display();
            }
        }
        // Leaderboard State
        else if(GAME_STATE == "leaderboard"){
            window.setView(mainMenu);
            while (GAME_STATE == "leaderboard" && window.isOpen()){
                // Event handling
                while (const std::optional event = window.pollEvent()){
                    if (event->is<sf::Event::Closed>()){
                        window.close();
                    }
                    else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
                        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape){
                            GAME_STATE = "menu";
                        }
                    }
                }
                
                window.clear();
                window.draw(menuBackground.getSprite());
                
                // Draw leaderboard title
                sf::Text leaderboardTitle(font, "LEADERBOARD", 64);
                leaderboardTitle.setFillColor(sf::Color(255, 215, 0));
                leaderboardTitle.setOutlineColor(sf::Color(139, 69, 19));
                leaderboardTitle.setOutlineThickness(3.f);
                sf::FloatRect lbTitleBounds = leaderboardTitle.getLocalBounds();
                leaderboardTitle.setOrigin({lbTitleBounds.size.x / 2.f, lbTitleBounds.size.y / 2.f});
                leaderboardTitle.setPosition({windowSizeX/2.f, 150.f});
                window.draw(leaderboardTitle);
                
                // Draw leaderboard entries
                float yOffset = 300.f;
                for (size_t i = 0; i < leaderboard.size() && i < 10; ++i){
                    std::string entryText = std::to_string(i + 1) + ". " + 
                                          leaderboard[i].name + " - " + 
                                          std::to_string(leaderboard[i].score);
                    sf::Text entry(font, entryText, 32);
                    entry.setFillColor(sf::Color::White);
                    sf::FloatRect entryBounds = entry.getLocalBounds();
                    entry.setOrigin({entryBounds.size.x / 2.f, 0.f});
                    entry.setPosition({windowSizeX/2.f, yOffset});
                    window.draw(entry);
                    yOffset += 50.f;
                }
                
                // Back instruction
                sf::Text backText(font, "Press ESC to go back", 24);
                backText.setFillColor(sf::Color(150, 150, 150));
                sf::FloatRect backBounds = backText.getLocalBounds();
                backText.setOrigin({backBounds.size.x / 2.f, backBounds.size.y / 2.f});
                backText.setPosition({windowSizeX/2.f, windowSizeY - 100.f});
                window.draw(backText);
                
                window.display();
            }
        }

        // win State
        else if(GAME_STATE == "win"){
            window.setView(mainMenu);

            // Calculate time bonus
            float levelTime = levelClock.getElapsedTime().asSeconds();
            int timeBonus = std::max(0, (int)(TIME_BONUS_PER_SECOND * (120.f - levelTime)));
            totalScore += timeBonus;
            finalScore = totalScore;
            
            // Update final score text
            finalScoreText.setString("Final Score: " + std::to_string(finalScore));
            sf::FloatRect finalScoreBounds = finalScoreText.getLocalBounds();
            finalScoreText.setOrigin({finalScoreBounds.size.x / 2.f, finalScoreBounds.size.y / 2.f});
            finalScoreText.setPosition({windowSizeX/2.f, (windowSizeY /2) + 100.f});
            
            // Reset initials for new entry
            playerInitials = "";
            
            // Transition to initials entry
            GAME_STATE = "enter_initials";
        }

        // Enter Initials State
        else if(GAME_STATE == "enter_initials"){
            window.setView(mainMenu);
            
            while (GAME_STATE == "enter_initials" && window.isOpen()){
                float dt = clock.restart().asSeconds();
                pulseTimer += dt;
                
                // Event handling
                while (const std::optional event = window.pollEvent()){
                    if (event->is<sf::Event::Closed>()){
                        window.close();
                    }
                    else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
                        // Handle backspace
                        if (keyPressed->scancode == sf::Keyboard::Scancode::Backspace) {
                            if (!playerInitials.empty()) {
                                playerInitials.pop_back();
                            }
                        }
                        // Handle enter
                        else if (keyPressed->scancode == sf::Keyboard::Scancode::Enter) {
                            if (isValidInitials(playerInitials)) {
                                // Save to leaderboard
                                addToLeaderboard(leaderboard, playerInitials, finalScore);
                                saveLeaderboard("leaderboard.txt", leaderboard);
                                GAME_STATE = "leaderboard_display";
                            }
                        }
                    }
                    else if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()){
                        if (playerInitials.length() < 3) {
                            char c = static_cast<char>(textEntered->unicode);
                            // Convert to uppercase and check if it's A-Z
                            if (c >= 'a' && c <= 'z') {
                                c = c - 'a' + 'A'; // Convert to uppercase
                            }
                            if (c >= 'A' && c <= 'Z') {
                                playerInitials += c;
                            }
                        }
                    }
                }
                
                window.clear();
                window.draw(menuBackground.getSprite());
                
                // Title
                sf::Text initialsTitle(font, "NEW HIGH SCORE!", 64);
                initialsTitle.setFillColor(sf::Color(255, 215, 0));
                initialsTitle.setOutlineColor(sf::Color(139, 69, 19));
                initialsTitle.setOutlineThickness(3.f);
                sf::FloatRect titleBounds = initialsTitle.getLocalBounds();
                initialsTitle.setOrigin({titleBounds.size.x / 2.f, titleBounds.size.y / 2.f});
                initialsTitle.setPosition({windowSizeX/2.f, 200.f});
                window.draw(initialsTitle);
                
                // Score display
                sf::Text scoreDisplay(font, "Score: " + std::to_string(finalScore), 40);
                scoreDisplay.setFillColor(sf::Color::White);
                sf::FloatRect scoreBounds = scoreDisplay.getLocalBounds();
                scoreDisplay.setOrigin({scoreBounds.size.x / 2.f, scoreBounds.size.y / 2.f});
                scoreDisplay.setPosition({windowSizeX/2.f, 300.f});
                window.draw(scoreDisplay);
                
                // "Enter Your Initials" text
                sf::Text enterText(font, "ENTER YOUR INITIALS", 36);
                enterText.setFillColor(sf::Color::White);
                sf::FloatRect enterBounds = enterText.getLocalBounds();
                enterText.setOrigin({enterBounds.size.x / 2.f, enterBounds.size.y / 2.f});
                enterText.setPosition({windowSizeX/2.f, 400.f});
                window.draw(enterText);
                
                // Display typed initials with underscores for remaining letters
                std::string displayInitials = playerInitials;
                while (displayInitials.length() < 3) {
                    displayInitials += "_";
                }
                
                sf::Text initialsDisplay(font, displayInitials, 100);
                initialsDisplay.setFillColor(sf::Color(255, 215, 0));
                sf::FloatRect initBounds = initialsDisplay.getLocalBounds();
                initialsDisplay.setOrigin({initBounds.size.x / 2.f, initBounds.size.y / 2.f});
                initialsDisplay.setPosition({windowSizeX/2.f, windowSizeY/2.f});
                
                // Pulsing effect
                float alpha = 200.f + 55.f * std::sin(pulseTimer * 3.f);
                initialsDisplay.setFillColor(sf::Color(255, 215, 0, static_cast<unsigned char>(alpha)));
                window.draw(initialsDisplay);
                
                // Instructions
                sf::Text instructions(font, "Type 3 letters, then press ENTER", 28);
                instructions.setFillColor(sf::Color(200, 200, 200));
                sf::FloatRect instrBounds = instructions.getLocalBounds();
                instructions.setOrigin({instrBounds.size.x / 2.f, instrBounds.size.y / 2.f});
                instructions.setPosition({windowSizeX/2.f, windowSizeY - 200.f});
                window.draw(instructions);
                
                // Error message
                if (!isValidInitials(playerInitials) && sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Enter)) {
                    sf::Text errorText(font, "Must be exactly 3 letters!", 24);
                    errorText.setFillColor(sf::Color::Red);
                    sf::FloatRect errorBounds = errorText.getLocalBounds();
                    errorText.setOrigin({errorBounds.size.x / 2.f, errorBounds.size.y / 2.f});
                    errorText.setPosition({windowSizeX/2.f, windowSizeY - 150.f});
                    window.draw(errorText);
                }
                
                window.display();
            }
        }

        // Leaderboard Display State (after entering initials)
        else if(GAME_STATE == "leaderboard_display"){
            window.setView(mainMenu);
            while (GAME_STATE == "leaderboard_display" && window.isOpen()){
                // Event handling
                while (const std::optional event = window.pollEvent()){
                    if (event->is<sf::Event::Closed>()){
                        window.close();
                    }
                    else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
                        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape || keyPressed->scancode == sf::Keyboard::Scancode::Enter || keyPressed->scancode == sf::Keyboard::Scancode::Space) {
                            GAME_STATE = "menu";
                        }
                    }
                }
                
                window.clear();
                window.draw(menuBackground.getSprite());
                
                // Draw leaderboard title
                sf::Text leaderboardTitle(font, "LEADERBOARD", 64);
                leaderboardTitle.setFillColor(sf::Color(255, 215, 0));
                leaderboardTitle.setOutlineColor(sf::Color(139, 69, 19));
                leaderboardTitle.setOutlineThickness(3.f);
                sf::FloatRect lbTitleBounds = leaderboardTitle.getLocalBounds();
                leaderboardTitle.setOrigin({lbTitleBounds.size.x / 2.f, lbTitleBounds.size.y / 2.f});
                leaderboardTitle.setPosition({windowSizeX/2.f, 150.f});
                window.draw(leaderboardTitle);
                
                // Draw leaderboard entries
                float yOffset = 300.f;
                for (size_t i = 0; i < leaderboard.size() && i < 10; ++i){
                    std::string entryText = std::to_string(i + 1) + ".  " + 
                                        leaderboard[i].name + "  -  " + std::to_string(leaderboard[i].score);
                    sf::Text entry(font, entryText, 32);
                    
                    // Highlight the entry that was just added
                    if (leaderboard[i].name == playerInitials && leaderboard[i].score == finalScore) {
                        entry.setFillColor(sf::Color(255, 215, 0));
                    } else {
                        entry.setFillColor(sf::Color::White);
                    }
                    
                    sf::FloatRect entryBounds = entry.getLocalBounds();
                    entry.setOrigin({entryBounds.size.x / 2.f, 0.f});
                    entry.setPosition({windowSizeX/2.f, yOffset});
                    window.draw(entry);
                    yOffset += 50.f;
                }
                
                // to continue instruction
                sf::Text continueText(font, "Press ENTER to continue", 24);
                continueText.setFillColor(sf::Color(150, 150, 150));
                sf::FloatRect contBounds = continueText.getLocalBounds();
                continueText.setOrigin({contBounds.size.x / 2.f, contBounds.size.y / 2.f});
                continueText.setPosition({windowSizeX/2.f, windowSizeY - 100.f});
                window.draw(continueText);
                
                window.display();
            }
        }

        // lose State
        else if(GAME_STATE == "lose"){
            window.setView(mainMenu);
            while (GAME_STATE == "lose" && window.isOpen()){
                // Menu event handling
                while (const std::optional event = window.pollEvent()){
                    if (event->is<sf::Event::Closed>()){
                        window.close();
                    }
                    else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
                        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape){
                            window.close();
                        }
                        else if (keyPressed->scancode == sf::Keyboard::Scancode::W){
                            // Reset to level 1 on restart
                            currentLevel = 1;
                            lives = 3;
                            totalScore = 0;
                            loadLevel(currentLevel, tilemap, xPos, yPos, xSpeed, ySpeed, hasJump, hasDash, mapWidth, mapHeight, lives);
                            levelClock.restart();
                            GAME_STATE = "playing";
                        }
                    }
                }
                
                // bobbing effect for title
                float titleY = windowSizeY/3.f + 10.f * std::sin(pulseTimer * 2.f);
                mainMenuTitle.setPosition({windowSizeX/2.f, titleY});

                window.clear();

                window.draw(menuBackground.getSprite());
                
                window.draw(mainMenuTitle);
                window.draw(loseText);
                window.draw(restartText);
                window.draw(exitText);
                
                window.display();
            }
        }

        // playing state
        else if(GAME_STATE == "playing"){
            clock.restart(); // Reset clock for clean transition
            window.setView(camera); // Set the game camera
            
            while (GAME_STATE == "playing" && window.isOpen()){
                float dt = clock.restart().asSeconds();

                // Game event handling
                while (const std::optional event = window.pollEvent()){
                    if (event->is<sf::Event::Closed>()){
                        window.close();
                    }
                    else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
                        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape){
                            GAME_STATE = "menu";
                        }
                    }
                }

                // WASD controls
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::D )){
                    playerAnim.setDirection("right", "assets/images/player");
                    xSpeed = moveSpeed;
                }else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::A)){
                    playerAnim.setDirection("left", "assets/images/player");
                    xSpeed = -moveSpeed;
                }
                else{
                    if(xSpeed > 0){
                        xSpeed -= xAccel * dt;
                        if(xSpeed < 0) xSpeed = 0.f;
                    }else if(xSpeed < 0){
                        xSpeed += xAccel * dt;
                        if(xSpeed > 0) xSpeed = 0.f;
                    }
                }

                if ((sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::W )|| sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::K))){
                    if(hasJump){
                        ySpeed = -400.f;
                        hasJump = false;
                    }
                }

                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::S)){
                    xSpeed /=2;
                }

                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::A) && sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::D)){
                    xSpeed = 0;
                }

                if((sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::E )|| sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::L )) && hasDash){
                    dash = dashLength;
                    dashDirection = true;
                    playerAnim.setDirection("attack_right", "assets/images/player");
                    hasDash = false;
                }

                if((sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Q) || sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::J)) && hasDash){
                    dash = dashLength;
                    dashDirection = false;
                    playerAnim.setDirection("attack_left", "assets/images/player");
                    hasDash = false;
                }

                if(dash > 0 && dashDirection){
                    xSpeed = moveSpeed*2;
                }
                else if(dash > 0 && !dashDirection){
                    xSpeed = -moveSpeed*2;
                }

                // Idle animation handling
                if(isIdle(xSpeed)){
                    if(playerAnim.getCurrentDirection() == "left" || playerAnim.getCurrentDirection() == "attack_left"){
                        playerAnim.setDirection("idle_left", "assets/images/player");
                    }
                    else if(playerAnim.getCurrentDirection() == "right" || playerAnim.getCurrentDirection() == "attack_right"){
                        playerAnim.setDirection("idle_right", "assets/images/player");
                    }
                }

                // Physics engine
                ySpeed += yAccel * dt;
                xPos += xSpeed * dt;
                yPos += ySpeed * dt;

                if(dash > 0){
                    dash--;
                }
                if(dash == 0 && xSpeed != 0){
                    if(playerAnim.getCurrentDirection() == "attack_right"){
                        playerAnim.setDirection("right", "assets/images/player");
                    }
                    else if(playerAnim.getCurrentDirection() == "attack_left"){
                        playerAnim.setDirection("left", "assets/images/player");
                    }
                }

                // Death/ respawn
                if (yPos >= windowSizeY -32.f){
                    xPos = levels[currentLevel - 1].spawnX;
                    yPos = levels[currentLevel - 1].spawnY;
                    ySpeed = 0.f;
                    xSpeed = 0.f;
                    hasJump = true;
                    lives--;
                    totalScore = std::max(0, totalScore - DEATH_PENALTY); // Lose points on death
                }

                // Screen boundaries
                if(xPos <= 0){
                    xPos = 0;
                    xSpeed = 0;
                }
                if(xPos >= windowSizeX -32.f){
                    xPos = windowSizeX -32.f;
                    xSpeed = 0.f;
                }

                // Win detection
                if(xPos > levels[currentLevel - 1].winX1 && xPos < levels[currentLevel - 1].winX2 && 
                   yPos > levels[currentLevel - 1].winY1 && yPos < levels[currentLevel - 1].winY2){

                    // Calculate level bonus
                    float levelTime = levelClock.getElapsedTime().asSeconds();
                    int timeBonus = std::max(0, (int)(TIME_BONUS_PER_SECOND * (60.f - levelTime))); // 1 min max
                    totalScore += POINTS_PER_LEVEL + timeBonus;

                    currentLevel++;
                    // win state change to that screen
                    if(currentLevel > (int)levels.size()){
                        GAME_STATE = "win";
                        currentLevel = 1; // Reset for replay
                        lives = 3;
                    } else {
                        // Load next level
                        loadLevel(currentLevel, tilemap, xPos, yPos, xSpeed, ySpeed, hasJump, hasDash, mapWidth, mapHeight, lives);
                        std::cout << "Loaded Level " << currentLevel << " - Spawn: (" << xPos << ", " << yPos << ")" << std::endl;
                        levelClock.restart(); // Reset timer for new level
                        // Reset dash state
                        dash = 0;
                        dashDirection = true;
                        // Reset animation
                        playerAnim.setDirection("right", "assets/images/player");

                        window.clear(sf::Color(54, 69, 79));
                        window.display();
                        clock.restart();
                        continue; // Skip rest of this frame, idk why but it works

                    }
                }

                // lose detection
                if(lives <= 0){
                    GAME_STATE = "lose";
                    lives = 3;
                }

                //spike collision detection
                sf::FloatRect playerBounds2(sf::Vector2f(xPos, yPos), sf::Vector2f(PLAYER_WIDTH, PLAYER_HEIGHT));
                Tile collidedTile = tilemap.getCollidedTile(playerBounds2);

                if (collidedTile.isType(TileType::SPIKE)) {
                    xPos = levels[currentLevel - 1].spawnX;
                    yPos = levels[currentLevel - 1].spawnY;
                    ySpeed = 0.f;
                    xSpeed = 0.f;
                    hasJump = true;
                    lives--;
                    totalScore = std::max(0, totalScore - DEATH_PENALTY);
                }

                // Tile collision detection
                int playerTileX = (int)(xPos / tilemap.getTileWidth());
                int playerTileY = (int)(yPos / tilemap.getTileHeight());

                sf::FloatRect playerBounds(sf::Vector2f(xPos, yPos), sf::Vector2f(PLAYER_WIDTH, PLAYER_HEIGHT));

                // Check surrounding tiles (3x3 grid)
                for (int dy = -1; dy <= 1; ++dy){
                    for (int dx = -1; dx <= 1; ++dx){
                        sf::FloatRect tileBounds = tilemap.getTileCollisionBounds(playerTileX + dx, playerTileY + dy);
                        
                        if (tileBounds.size.x > 0 && tileBounds.size.y > 0){
                            if (checkAABBCollision(playerBounds, tileBounds)){
                                resolveCollision(xPos, yPos, ySpeed, hasJump, hasDash, xSpeed, playerBounds, tileBounds);
                                playerBounds.position.x = xPos;
                                playerBounds.position.y = yPos;
                            }
                        }
                    }
                }

                // Camera update
                float clampedCameraX = xPos + PLAYER_WIDTH / 2.f;
                float clampedCameraY = yPos + PLAYER_HEIGHT / 2.f;

                if(clampedCameraX - cameraWidth / 2.f < 0){
                    clampedCameraX = cameraWidth / 2.f;
                }
                else if(clampedCameraX + cameraWidth / 2.f > mapWidth){
                    clampedCameraX = mapWidth - cameraWidth / 2.f;
                }

                if(clampedCameraY - cameraHeight / 2.f < 0){
                    clampedCameraY = cameraHeight / 2.f;
                }
                else if(clampedCameraY + cameraHeight / 2.f > mapHeight){
                    clampedCameraY = mapHeight - cameraHeight / 2.f;
                }

                camera.setCenter(sf::Vector2f(clampedCameraX, clampedCameraY));
                window.setView(camera);
                
                
                // Rendering
                sf::Color color(1, 2, 3);
                window.clear(color);

                window.draw(background.getSprite());
                tilemap.drawBackgroundTiles(window);
                tilemap.drawCollisionTiles(window);

                if(dash > 0 && !dashDirection){
                    playerAnim.setPosition({xPos-28, yPos});
                }
                else{
                    playerAnim.setPosition({xPos, yPos});
                }
                playerAnim.update(dt);
                window.draw(playerAnim.getSprite());

                
                window.display();
            }
        }
    }
    return 0;
}