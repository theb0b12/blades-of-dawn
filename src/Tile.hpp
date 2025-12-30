#pragma once
#include <string>
#include <SFML/Graphics.hpp>

enum class TileType {
    NONE,
    GROUND,
    SPIKE,
    LAVA,
    ICE,
    BOUNCE_PAD,
    CHECKPOINT
    // Add more types as needed
};

class Tile {
public:
    // Hardcoded tile ID constants
    static const int SPIKE_ID = 1330;
    static const int SPIKE_ID2 = 1329;

    
    Tile() : _type(TileType::NONE), _id(0), _gridX(0), _gridY(0) {}
    
    Tile(int id, int gridX, int gridY, sf::FloatRect bounds)
        : _id(id), _gridX(gridX), _gridY(gridY), _bounds(bounds) {
        _type = idToType(id);
    }
    
    // Getters
    TileType getType() const { return _type; }
    int getId() const { return _id; }
    int getGridX() const { return _gridX; }
    int getGridY() const { return _gridY; }
    sf::FloatRect getBounds() const { return _bounds; }
    
    // Check tile type
    bool isType(TileType type) const { return _type == type; }
    bool isNone() const { return _type == TileType::NONE; }
    
private:
    TileType _type;
    int _id;
    int _gridX;
    int _gridY;
    sf::FloatRect _bounds;
    
    // Convert tile ID to type
    static TileType idToType(int id) {
        if (id == SPIKE_ID || id ==SPIKE_ID2) return TileType::SPIKE;
        return TileType::GROUND; // Default to ground for any collision tile
    }
};
