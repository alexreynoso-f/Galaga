// src/main.cpp
// Versión con soporte de sonido de laser al disparar (usa std::optional<sf::Sound>)
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include "Player.h"
#include "Bullet.h"
#include "Enemy.h"

static bool rectsIntersect(const sf::FloatRect& a, const sf::FloatRect& b) {
    return !(a.position.x + a.size.x < b.position.x ||
             b.position.x + b.size.x < a.position.x ||
             a.position.y + a.size.y < b.position.y ||
             b.position.y + b.size.y < a.position.y);
}

int main() {
    // --- Configuración de ventana y grid ---
    const int WINDOW_COLS = 20;
    const int WINDOW_ROWS = 25;
    const int CELL_SIZE = 32; // px por célula
    const float HUD_HEIGHT = 64.f;
    const sf::Vector2f MARGIN{12.f, 12.f};

    unsigned int windowWidth = static_cast<unsigned int>(MARGIN.x * 2 + WINDOW_COLS * CELL_SIZE);
    unsigned int windowHeight = static_cast<unsigned int>(MARGIN.y + HUD_HEIGHT + WINDOW_ROWS * CELL_SIZE + MARGIN.y);

    sf::RenderWindow window(sf::VideoMode({ windowWidth, windowHeight }), "Naves");
    window.setVerticalSyncEnabled(true);

    // --- Cargar assets (texturas / fuente) ---
    sf::Texture texPlayer, texEnemy, texBullet, texBackground;
    sf::Font font;
    bool hasPlayerTex = texPlayer.loadFromFile("assets/textures/player.png");
    bool hasEnemyTex  = texEnemy.loadFromFile( "assets/textures/enemy.png");
    bool hasBulletTex = texBullet.loadFromFile("assets/textures/bullet.png");
    bool hasBgTex     = texBackground.loadFromFile("assets/textures/background.png");
    bool hasFont      = font.openFromFile("assets/fonts/font.ttf");

    if (!hasPlayerTex)  std::cerr << "[WARN] No pudo cargar assets/textures/player.png\n";
    if (!hasEnemyTex)   std::cerr << "[WARN] No pudo cargar assets/textures/enemy.png\n";
    if (!hasBulletTex)  std::cerr << "[WARN] No pudo cargar assets/textures/bullet.png\n";
    if (!hasBgTex)      std::cerr << "[WARN] No pudo cargar assets/textures/background.png\n";
    if (!hasFont)       std::cerr << "[WARN] No pudo cargar assets/fonts/font.ttf\n";

    // --- Cargar sonido de laser
    sf::SoundBuffer laserBuf;
    std::optional<sf::Sound> laserSound;



    if (laserBuf.loadFromFile("assets/sounds/laser_sound.mp3")) {
        laserSound.emplace(laserBuf);
        std::cout << "[INFO] Cargado sonido laser: assets/sounds/laser_sound.mp3\n";
    } else {
        std::cerr << "[WARN] No se pudo cargar assets/sounds/laser_sound.mp3\n";
    }


    std::unique_ptr<sf::Sprite> bgSprite;
    if (hasBgTex) {
        bgSprite = std::make_unique<sf::Sprite>(texBackground);
        const auto texSize = texBackground.getSize();
        if (texSize.x > 0 && texSize.y > 0) {
            float scaleX = static_cast<float>(windowWidth) / static_cast<float>(texSize.x);
            float scaleY = static_cast<float>(windowHeight) / static_cast<float>(texSize.y);
            float scale = std::max(scaleX, scaleY);
            bgSprite->setScale({ scale, scale });
            auto b = bgSprite->getGlobalBounds();
            bgSprite->setPosition(sf::Vector2f{ (windowWidth - b.size.x) / 2.f, (windowHeight - b.size.y) / 2.f });
        }
    }

    // --- Crear player ---
    sf::Vector2f playerStart{
        MARGIN.x + (WINDOW_COLS * CELL_SIZE) / 2.f,
        MARGIN.y + HUD_HEIGHT + (WINDOW_ROWS * CELL_SIZE) - CELL_SIZE * 1.5f
    };
    Player player(hasPlayerTex ? &texPlayer : nullptr, playerStart);
    float leftMargin = 16.f;
    float rightMargin = static_cast<float>(windowWidth);
    player.setHorizontalLimits(leftMargin, rightMargin);
    // --- Pool de balas  ---
    const size_t BULLET_POOL_SIZE = 64;
    std::vector<Bullet> bullets;
    bullets.reserve(BULLET_POOL_SIZE);
    for (size_t i = 0; i < BULLET_POOL_SIZE; ++i) {
        bullets.emplace_back(hasBulletTex ? &texBullet : nullptr);
    }

    // --- Crear enemigos ---
    std::vector<Enemy> enemies;
    const int ENEMY_COLS = 3;
    const int ENEMY_ROWS = 3;
    const float formationStartX = MARGIN.x + 2.f * CELL_SIZE;
    const float formationStartY = MARGIN.y + HUD_HEIGHT + 1.f * CELL_SIZE;
    const float spacingX = static_cast<float>(CELL_SIZE) * 1.6f;
    const float spacingY = static_cast<float>(CELL_SIZE) * 1.1f;

    for (int r = 0; r < ENEMY_ROWS; ++r) {
        for (int c = 0; c < ENEMY_COLS; ++c) {
            sf::Vector2f pos{
                formationStartX + c * spacingX,
                formationStartY + r * spacingY
            };
            enemies.emplace_back(hasEnemyTex ? &texEnemy : nullptr, pos);
        }
    }

    // Score-
    std::optional<sf::Text> scoreText;
    if (hasFont) {
        scoreText.emplace(font, "Score: 0", 20);
        scoreText->setFillColor(sf::Color::White);
        scoreText->setPosition({ MARGIN.x + 8.f, MARGIN.y + 8.f });
    }
    int score = 0;

    // --- Timing ---
    sf::Clock clock;
    float dt = 0.f;

    // Helper: spawn bullet usando pool (captura laserSound/hasLaser para reproducir)
    auto spawnBulletFromPool = [&](const sf::Vector2f& pos, float speedY) {
        for (auto &b : bullets) {
            if (!b.isActive()) {
                b.spawn(pos, speedY);
                    laserSound->play();
                return;
            }
        }
    };

    // --- Bucle principal ---
    while (window.isOpen()) {
        while (auto evOpt = window.pollEvent()) {
            const sf::Event& ev = *evOpt;
            if (ev.is<sf::Event::Closed>()) {
                window.close();
                break;
            }

            if (ev.is<sf::Event::KeyPressed>()) {
                const auto* k = ev.getIf<sf::Event::KeyPressed>();
                if (!k) continue;
                if (k->code == sf::Keyboard::Key::Space) {
                    sf::FloatRect pb = player.bounds();
                    sf::Vector2f bulletPos{ pb.position.x + pb.size.x / 2.f, pb.position.y - 6.f };
                    spawnBulletFromPool(bulletPos, -480.f); // speedY negativo => sube
                } else if (k->code == sf::Keyboard::Key::R) {

                    player.setPosition(playerStart);
                    enemies.clear();
                    for (int r = 0; r < ENEMY_ROWS; ++r) {
                        for (int c = 0; c < ENEMY_COLS; ++c) {
                            sf::Vector2f pos{
                                formationStartX + c * spacingX,
                                formationStartY + r * spacingY
                            };
                            enemies.emplace_back(hasEnemyTex ? &texEnemy : nullptr, pos);
                        }
                    }
                    for (auto &b : bullets) b.deactivate();
                    score = 0;
                    if (scoreText) scoreText->setString("Score: 0");
                }
            }
        } // fin pollEvent

        // Delta time
        dt = clock.restart().asSeconds();

        // Input continuo: movimiento del player
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
            player.moveLeft(dt);
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
                   sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
            player.moveRight(dt);
        }

        // Actualizaciones
        player.update(dt);
        for (auto &b : bullets) b.update(dt);
        for (auto &e : enemies) e.update(dt);

        // Colisiones: bullets vs enemies
        for (auto &b : bullets) {
            if (!b.isActive()) continue;
            for (auto &e : enemies) {
                if (!e.isActive()) continue;
                if (rectsIntersect(b.bounds(), e.bounds())) {
                    b.deactivate();
                    e.setActive(false);
                    score += 10;
                    if (scoreText) scoreText->setString("Score: " + std::to_string(score));
                    break; // bala consumida -> pasar a la siguiente bala
                }
            }
        }

        // Colisiones: enemy vs player (simple)
        for (auto &e : enemies) {
            if (!e.isActive()) continue;
            if (rectsIntersect(e.bounds(), player.bounds())) {
                e.setActive(false);
                player.setPosition(playerStart);
            }
        }

        // Dibujado
        window.clear(sf::Color(18, 18, 28));

        // Draw background first (si cargado)
        if (bgSprite) {
            window.draw(*bgSprite);
        }

        // Draw enemies
        for (auto &e : enemies) {
            if (e.isActive()) e.draw(window);
        }

        // Draw bullets
        for (auto &b : bullets) {
            if (b.isActive()) b.draw(window);
        }

        // Draw player
        player.draw(window);

        // HUD
        if (scoreText) window.draw(*scoreText);

        window.display();
    }

    return 0;
}