#ifndef GALAGA_PLAYER_H
#define GALAGA_PLAYER_H


// src/Player.h
#pragma once
#include <SFML/Graphics.hpp>

class Player {
public:
    Player(const sf::Texture* texture, const sf::Vector2f& startPos);

    void update(float dt);
    void draw(sf::RenderWindow& window) const;

    // Movement API used from main
    void moveLeft(float dt);
    void moveRight(float dt);
    void setPosition(const sf::Vector2f& pos);
    sf::FloatRect bounds() const;

private:
    std::unique_ptr<sf::Sprite> sprite_;
    sf::RectangleShape fallbackRect_; // usado si no hay textura
    sf::Vector2f position_;
    float speed_ = 360.f; // px/s
};


#endif //GALAGA_PLAYER_H