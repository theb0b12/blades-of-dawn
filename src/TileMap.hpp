#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <set>

#include "Tile.hpp"

struct TileInfo {
    std::shared_ptr<sf::Texture> texture;
    sf::Vector2f position;
    int layer;
};

class TileMap {
public:
    TileMap() = default;

    // Load map from Tiled JSON file
    bool loadFromFile(const std::string& filePath);

    // Get collision tiles (solid ground)
    const std::vector<TileInfo>& getCollisionTiles() const { return _collisionTiles; }
    
    // Get background tiles (decorative, no collision)
    const std::vector<TileInfo>& getBackgroundTiles() const { return _backgroundTiles; }

    // Draw collision tiles
    void drawCollisionTiles(sf::RenderWindow& window) const;
    
    // Draw background tiles
    void drawBackgroundTiles(sf::RenderWindow& window) const;

    // Get map dimensions
    int getWidth() const { return _width; }
    int getHeight() const { return _height; }
    int getTileWidth() const { return _tileWidth; }
    int getTileHeight() const { return _tileHeight; }
    
    // Get collision bounds for a tile at map position
    sf::FloatRect getTileCollisionBounds(int mapX, int mapY) const;

    Tile getCollidedTile(const sf::FloatRect& bounds) const;

    

private:
    std::vector<TileInfo> _collisionTiles;  // Ground/solid tiles
    std::vector<TileInfo> _backgroundTiles; // Decorative tiles
    int _width = 0;
    int _height = 0;
    int _tileWidth = 0;
    int _tileHeight = 0;
    
    // Store tile IDs for collision queries
    std::vector<int> _tileData;

    // Cache for loaded tilesets
    std::map<std::string, std::shared_ptr<sf::Texture>> _textureCache;
    
    // Track which tilesets are collision vs background
    std::set<int> _collisionTilesetGids; // firstGid values for collision tilesets

    // Helper to load tileset and extract tiles
    std::map<int, std::shared_ptr<sf::Texture>> loadTileset(const std::string& tilesetPath, int firstGid);
    
    // Extract a single tile from a tileset image
    std::shared_ptr<sf::Texture> extractTile(
        const sf::Texture& tilesetTexture,
        int tileId,
        int columns,
        int tileWidth,
        int tileHeight);
};