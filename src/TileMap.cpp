#include "TileMap.hpp"
#include "Tile.hpp"

#include <fstream>
#include <iostream>
#include "../libs/json.hpp"
#include <filesystem>
#include <algorithm>
#include <set>

using json = nlohmann::json;
namespace fs = std::filesystem;

bool TileMap::loadFromFile(const std::string& filePath){
    std::ifstream file(filePath);
    if (!file.is_open()){
        std::cerr << "Failed to open map file: " << filePath << "\n";
        return false;
    }

    json mapData;
    try {
        file >> mapData;
    } catch (const std::exception& e){
        std::cerr << "JSON parse error: " << e.what() << "\n";
        return false;
    }

    // Get map dimensions
    _width = mapData["width"];
    _height = mapData["height"];
    _tileWidth = mapData["tilewidth"];
    _tileHeight = mapData["tileheight"];

    // Load tilesets and map IDs to tile textures
    std::map<int, std::shared_ptr<sf::Texture>> tileIdMap;
    std::map<std::string, int> tilesetNameToGid;
    std::string mapDirectory = fs::path(filePath).parent_path().string();

    std::cout << "Tilesets array size: " << mapData["tilesets"].size() << "\n";

    for (size_t i = 0; i < mapData["tilesets"].size(); ++i){
        std::cout << "Processing tileset " << i << "\n";
        std::cout.flush();
        
        const auto& tileset = mapData["tilesets"][i];
        int firstGid = tileset["firstgid"];
        std::string tilesetSource = tileset["source"];
        std::string tilesetPath = mapDirectory + "/" + tilesetSource;

        std::cout << "Loading tileset: " << tilesetSource << " at " << tilesetPath << "\n";
        std::cout.flush();
        
        auto tileMap = loadTileset(tilesetPath, firstGid);
        std::cout << "Back from loadTileset, got " << tileMap.size() << " tiles\n";
        std::cout.flush();
        
        if (tileMap.empty()){
            std::cerr << "WARNING: Tileset loaded 0 tiles! Check path.\n";
            continue;
        }
        
        std::cout << "Inserting into tileIdMap...\n";
        std::cout.flush();
        
        for (auto& [id, tex] : tileMap){
            tileIdMap[id] = tex;
        }
        
        std::cout << "Insert complete\n";
        std::cout.flush();
        
        tilesetNameToGid[tilesetSource] = firstGid;
        
        std::cout << "Tileset " << tilesetSource << " has " << tileMap.size() << " tiles\n";
        
        // Check if this is a collision tileset
        if (tilesetSource.find("Village.json") != std::string::npos){
            _collisionTilesetGids.insert(firstGid);
            std::cout << "  -> Marked as collision tileset (gid: " << firstGid << ")\n";
        } else {
            std::cout << "  -> Marked as background tileset\n";
        }
        std::cout.flush();
    }
    
    std::cout << "Finished loading all tilesets\n";
    std::cout << "Total collision tilesets: " << _collisionTilesetGids.size() << "\n";
    std::cout.flush();

    // Load tile layer data
    if (mapData["layers"].empty()){
        std::cerr << "No tile layers found in map\n";
        return false;
    }

    std::cout << "Number of layers: " << mapData["layers"].size() << "\n";

    // Process all layers
    for (size_t layerIdx = 0; layerIdx < mapData["layers"].size(); ++layerIdx){
        const auto& layer = mapData["layers"][layerIdx];
        std::string layerName = layer["name"];
        std::cout << "Processing layer " << layerIdx << ": " << layerName << "\n";

        const std::vector<int>& tileData = layer["data"];
        int layerId = layer["id"];

        // Create tiles from data
        for (int y = 0; y < _height; ++y){
            for (int x = 0; x < _width; ++x){
                int index = y * _width + x;
                int tileId = tileData[index];

                // 0 means empty tile
                if (tileId == 0) continue;

                // Find the texture for this ID
                if (tileIdMap.find(tileId) != tileIdMap.end()){
                    float xPos = x * _tileWidth;
                    float yPos = y * _tileHeight;

                    TileInfo tile = {
                        tileIdMap[tileId],
                        {xPos, yPos},
                        layerId
                    };

                    // Determine which tileset this tile belongs to
                    int tilesetGid = 0;
                    for (int gid : _collisionTilesetGids){
                        if (tileId >= gid) tilesetGid = gid;
                    }

                    // Separate collision and background tiles
                    if (_collisionTilesetGids.count(tilesetGid)){
                        _collisionTiles.push_back(tile);
                    } else {
                        _backgroundTiles.push_back(tile);
                    }
                }
            }
        }
    }

    // Store only the collision layer's tile data for collision queries
    if (!mapData["layers"].empty()){
        const auto& layer = mapData["layers"][0];
        _tileData = layer["data"].get<std::vector<int>>();
    }

    std::cout << "Loaded tilemap: " << _width << "x" << _height 
              << " | Collision: " << _collisionTiles.size() 
              << " | Background: " << _backgroundTiles.size() << "\n";
    return true;
}

void TileMap::drawCollisionTiles(sf::RenderWindow& window) const{
    for (const auto& tileInfo : _collisionTiles){
        sf::Sprite sprite(*tileInfo.texture);
        sprite.setPosition(tileInfo.position);
        window.draw(sprite);
    }
}

void TileMap::drawBackgroundTiles(sf::RenderWindow& window) const{
    for (const auto& tileInfo : _backgroundTiles){
        sf::Sprite sprite(*tileInfo.texture);
        sprite.setPosition(tileInfo.position);
        window.draw(sprite);
    }
}

sf::FloatRect TileMap::getTileCollisionBounds(int mapX, int mapY) const{
    if (mapX < 0 || mapX >= _width || mapY < 0 || mapY >= _height){
        return sf::FloatRect(sf::Vector2f(0, 0), sf::Vector2f(0, 0)); // Out of bounds
    }
    
    int index = mapY * _width + mapX;
    if (index >= (int)_tileData.size() || _tileData[index] == 0){
        return sf::FloatRect(sf::Vector2f(0, 0), sf::Vector2f(0, 0)); // No tile
    }

    int tileId = _tileData[index];
    
    // Find which tileset this tile belongs to
    int tilesetGid = 0;
    for (int gid : _collisionTilesetGids){
        if (tileId >= gid) tilesetGid = gid;
    }
    
    // Only return collision bounds if this is a collision tile
    if (!_collisionTilesetGids.count(tilesetGid)){
        return sf::FloatRect(sf::Vector2f(0, 0), sf::Vector2f(0, 0)); // Background tile, no collision
    }

    float x = mapX * _tileWidth;
    float y = mapY * _tileHeight;
    return sf::FloatRect(sf::Vector2f(x, y), sf::Vector2f(_tileWidth, _tileHeight));
}

std::map<int, std::shared_ptr<sf::Texture>> TileMap::loadTileset(const std::string& tilesetPath, int firstGid){std::map<int, std::shared_ptr<sf::Texture>> tileMap; std::ifstream file(tilesetPath);

    if (!file.is_open()){
        std::cerr << "Failed to open tileset: " << tilesetPath << "\n";
        return tileMap;
    }

    json tilesetData;
    try {
        file >> tilesetData;
    } catch (const std::exception& e){
        std::cerr << "JSON parse error in tileset: " << e.what() << "\n";
        return tileMap;
    }

    // Get tileset info
    std::string imagePath = tilesetData["image"];
    int tileWidth = tilesetData["tilewidth"];
    int tileHeight = tilesetData["tileheight"];
    int columns = tilesetData["columns"];
    std::string tilesetName = tilesetData["name"];

    std::cout << "Tileset JSON loaded: " << tilesetName << "\n";

    // Replace backslashes with forward slashes
    std::replace(imagePath.begin(), imagePath.end(), '\\', '/');

    // Construct full image path
    std::string tilesetDir = fs::path(tilesetPath).parent_path().string();
    std::string fullImagePath = tilesetDir + "/" + imagePath;

    std::cout << "Looking for image at: " << fullImagePath << "\n";

    // Load the tileset image
    auto fullTexture = std::make_shared<sf::Texture>();
    if (!fullTexture->loadFromFile(fullImagePath)){
        std::cerr << "Failed to load tileset image: " << fullImagePath << "\n";
        return tileMap;
    }

    std::cout << "Image loaded successfully\n";

    // Extract individual tiles
    int imageWidth = tilesetData["imagewidth"];
    int imageHeight = tilesetData["imageheight"];
    int tilesPerRow = columns;
    int totalTiles = (imageWidth / tileWidth) * (imageHeight / tileHeight);

    for (int tileId = 0; tileId < totalTiles; ++tileId){
        auto tilTexture = extractTile(*fullTexture, tileId, tilesPerRow, tileWidth, tileHeight);
        tileMap[tileId + firstGid] = tilTexture;
    }

    std::cout << "Loaded tileset: " << tilesetName << " with " << totalTiles << " tiles\n";
    return tileMap;
}

std::shared_ptr<sf::Texture> TileMap::extractTile(const sf::Texture& tilesetTexture, int tileId, int columns, int tileWidth, int tileHeight){
    // Convert the texture into an image
    sf::Image tilesetImage = tilesetTexture.copyToImage();

    int row = tileId / columns;
    int col = tileId % columns;

    int x = col * tileWidth;
    int y = row * tileHeight;

    // Create an empty image for this tile
    sf::Image tileImage;
    tileImage.resize({static_cast<unsigned>(tileWidth), static_cast<unsigned>(tileHeight)}, sf::Color::Transparent);

    // Copy the pixels from the tileset to the tile
    for (int ty = 0; ty < tileHeight; ++ty){
        for (int tx = 0; tx < tileWidth; ++tx){
            sf::Color pixel = tilesetImage.getPixel({static_cast<unsigned>(x + tx), static_cast<unsigned>(y + ty)});
            tileImage.setPixel({static_cast<unsigned>(tx), static_cast<unsigned>(ty)}, pixel);
        }
    }

    // Convert the tile image back into a texture
    auto tileTexture = std::make_shared<sf::Texture>();
    if (!tileTexture->loadFromImage(tileImage)){
        std::cerr << "Failed to load tile texture for tileId " << tileId << "\n";
    }

    return tileTexture;
}

Tile TileMap::getCollidedTile(const sf::FloatRect& bounds) const {
    // Get the tile coordinates the bounds overlap
    int startX = std::max(0, (int)(bounds.position.x / _tileWidth));
    int endX = std::min(_width - 1, (int)((bounds.position.x + bounds.size.x) / _tileWidth));
    int startY = std::max(0, (int)(bounds.position.y / _tileHeight));
    int endY = std::min(_height - 1, (int)((bounds.position.y + bounds.size.y) / _tileHeight));
    
    // Check all tiles in the range
    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            int index = y * _width + x;
            if (index >= (int)_tileData.size()) continue;
            
            int tileId = _tileData[index];
            if (tileId == 0) continue; // Empty tile
            
            sf::FloatRect tileBounds = getTileCollisionBounds(x, y);
            
            // If this tile has collision bounds
            if (tileBounds.size.x > 0 && tileBounds.size.y > 0) {
                // AABB collision
                if (bounds.position.x < tileBounds.position.x + tileBounds.size.x &&
                    bounds.position.x + bounds.size.x > tileBounds.position.x &&
                    bounds.position.y < tileBounds.position.y + tileBounds.size.y &&
                    bounds.position.y + bounds.size.y > tileBounds.position.y) {
                    
                    return Tile(tileId, x, y, tileBounds);
                }
            }
        }
    }
    
    return Tile(); // Returns NONE type
}