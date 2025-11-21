// src/Player.cpp
#include "Player.h"
#include <memory>

static constexpr float TARGET_PLAYER_W = 80.f; // ancho objetivo en px (ajusta si quieres)
static constexpr float TARGET_PLAYER_H = 80.f; // alto objetivo en px

Player::Player(const sf::Texture* texture, const sf::Vector2f& startPos)
    : position_(startPos)
{
    if (texture) {
        sprite_ = std::make_unique<sf::Sprite>(*texture);
        auto local = sprite_->getLocalBounds();

        // calcular escala para que el sprite quepa en la caja objetivo (manteniendo aspect ratio)
        float scaleX = TARGET_PLAYER_W / local.size.x;
        float scaleY = TARGET_PLAYER_H / local.size.y;
        float scale = std::min(scaleX, scaleY);
        sprite_->setScale({ scale, scale });

        // centrar origen en el sprite (ahora en coordenadas locales escaladas)
        auto newLocal = sprite_->getLocalBounds();
        sprite_->setOrigin({ newLocal.size.x / 2.f, newLocal.size.y / 2.f });
        sprite_->setPosition(position_);
    } else {
        // fallback visual si no hay textura, usar tamaño objetivo
        fallbackRect_.setSize({ TARGET_PLAYER_W, TARGET_PLAYER_H });
        fallbackRect_.setOrigin(fallbackRect_.getSize() / 2.f);
        fallbackRect_.setFillColor(sf::Color(120, 180, 240));
        fallbackRect_.setPosition(position_);
    }
}

void Player::update(float /*dt*/) {
    if (sprite_) sprite_->setPosition(position_);
    else fallbackRect_.setPosition(position_);
}

void Player::draw(sf::RenderWindow& window) const {
    if (sprite_) window.draw(*sprite_);
    else window.draw(fallbackRect_);
}

void Player::moveLeft(float dt) {
    position_.x -= speed_ * dt;
    // clamp left boundary (margen de 16 px)
    if (position_.x < 16.f) position_.x = 16.f;
}

void Player::moveRight(float dt) {
    position_.x += speed_ * dt;
    // Nota: si quieres restringir al ancho de ventana, es mejor pasar límites desde main
}

void Player::setPosition(const sf::Vector2f& pos) {
    position_ = pos;
    if (sprite_) sprite_->setPosition(position_);
    else fallbackRect_.setPosition(position_);
}

sf::FloatRect Player::bounds() const {
    if (sprite_) return sprite_->getGlobalBounds();
    return fallbackRect_.getGlobalBounds();
}