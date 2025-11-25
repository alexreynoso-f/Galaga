#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <random>
#include "Player.h"
#include "Bullet.h"
#include "Enemy.h"
#include "Menu.h"
#include "Formation.h"
#include "Shield.h"

static bool rectsIntersect(const sf::FloatRect& a, const sf::FloatRect& b) {
    return !(a.position.x + a.size.x < b.position.x ||
             b.position.x + b.size.x < a.position.x ||
             a.position.y + a.size.y < b.position.y ||
             b.position.y + b.size.y < a.position.y);
}

int main() {
    const int WINDOW_COLS = 24;
    const int WINDOW_ROWS = 25;
    const int CELL_SIZE = 32;
    const float HUD_HEIGHT = 64.f;
    const sf::Vector2f MARGIN{12.f, 12.f};

    unsigned int windowWidth = static_cast<unsigned int>(MARGIN.x * 2 + WINDOW_COLS * CELL_SIZE);
    unsigned int windowHeight = static_cast<unsigned int>(MARGIN.y + HUD_HEIGHT + WINDOW_ROWS * CELL_SIZE + MARGIN.y);

    sf::RenderWindow window(sf::VideoMode({ windowWidth, windowHeight }), "Naves");
    window.setVerticalSyncEnabled(true);

    sf::Texture texPlayer, texBulletPlayer, texBulletEnemy, texBackground;
    sf::Texture texAlienTop, texAlienMid, texAlienBot, texShield;
    sf::Texture texMenuBg; // menu background texture (new)
    sf::Font font;
    bool hasPlayerTex = texPlayer.loadFromFile("assets/textures/player.png");
    bool hasBulletPlayer = texBulletPlayer.loadFromFile("assets/textures/bullet.png");
    bool hasBulletEnemy = texBulletEnemy.loadFromFile("assets/textures/bullet_2.png");
    bool hasAlienTop  = texAlienTop.loadFromFile("assets/textures/alien_top.png");
    bool hasAlienMid  = texAlienMid.loadFromFile("assets/textures/alien_mid.png");
    bool hasAlienBot  = texAlienBot.loadFromFile("assets/textures/alien_bottom.png");
    bool hasBgTex     = texBackground.loadFromFile("assets/textures/background.png");
    bool hasShieldTex = texShield.loadFromFile("assets/textures/shield.png");
    bool hasFont      = font.openFromFile("assets/fonts/font.ttf");

    if (!hasPlayerTex)  std::cerr << "[WARN] No pudo cargar assets/textures/player.png\n";
    if (!hasBulletPlayer) std::cerr << "[WARN] No pudo cargar assets/textures/bullet.png\n";
    if (!hasBulletEnemy) std::cerr << "[WARN] No pudo cargar assets/textures/bullet_2.png\n";
    if (!hasAlienTop)   std::cerr << "[WARN] No pudo cargar assets/textures/alien_top.png\n";
    if (!hasAlienMid)   std::cerr << "[WARN] No pudo cargar assets/textures/alien_mid.png\n";
    if (!hasAlienBot)   std::cerr << "[WARN] No pudo cargar assets/textures/alien_bottom.png\n";
    if (!hasBgTex)      std::cerr << "[WARN] No pudo cargar assets/textures/background.png\n";
    if (!hasShieldTex)  std::cerr << "[WARN] No pudo cargar assets/textures/shield.png\n";
    if (!hasFont)       std::cerr << "[WARN] No pudo cargar assets/fonts/font.ttf\n";

    sf::SoundBuffer laserBuf;
    std::optional<sf::Sound> laserSound;
    if (laserBuf.loadFromFile("assets/sounds/laser_sound.mp3")) {
        laserSound.emplace(laserBuf);
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

    Menu menu(hasFont ? &font : nullptr, 80);
    menu.setOptions({ "NEW GAME", "EXIT" }, { static_cast<float>(windowWidth) / 2.f, static_cast<float>(windowHeight) / 2.f }, 140.f);

    enum class AppState { Menu, Playing };
    AppState state = AppState::Menu;

    sf::Vector2f playerStart{
        MARGIN.x + (WINDOW_COLS * CELL_SIZE) / 2.f,
        MARGIN.y + HUD_HEIGHT + (WINDOW_ROWS * CELL_SIZE) - CELL_SIZE * 1.5f
    };
    Player player(hasPlayerTex ? &texPlayer : nullptr, playerStart);
    float leftMargin = 16.f;
    float rightMargin = static_cast<float>(windowWidth);
    player.setHorizontalLimits(leftMargin, rightMargin);

    const size_t BULLET_POOL_SIZE = 64;
    std::vector<Bullet> bullets;
    bullets.reserve(BULLET_POOL_SIZE);
    for (size_t i = 0; i < BULLET_POOL_SIZE; ++i) bullets.emplace_back(hasBulletPlayer ? &texBulletPlayer : nullptr);

    const size_t ENEMY_BULLET_POOL_SIZE = 32;
    std::vector<Bullet> enemyBullets;
    enemyBullets.reserve(ENEMY_BULLET_POOL_SIZE);
    for (size_t i = 0; i < ENEMY_BULLET_POOL_SIZE; ++i) enemyBullets.emplace_back(hasBulletEnemy ? &texBulletEnemy : nullptr);

    const int ENEMY_COLS = 11;
    const int ENEMY_ROWS = 5;
    const float formationStartX = MARGIN.x + 2.f * CELL_SIZE;
    const float formationStartY = MARGIN.y + HUD_HEIGHT + 1.f * CELL_SIZE;
    const float spacingX = static_cast<float>(CELL_SIZE) * 1.65f;
    const float spacingY = static_cast<float>(CELL_SIZE) * 1.15f;

    Formation formation(
        hasAlienTop ? &texAlienTop : nullptr,
        hasAlienMid ? &texAlienMid : nullptr,
        hasAlienBot ? &texAlienBot : nullptr,
        ENEMY_COLS, ENEMY_ROWS,
        sf::Vector2f{ formationStartX, formationStartY },
        spacingX, spacingY,
        40.f, 18.f
    );

    std::optional<sf::Text> scoreText;
    if (hasFont) {
        scoreText.emplace(font, "Score: 0", 28);
        scoreText->setFillColor(sf::Color::White);
        scoreText->setPosition({ MARGIN.x + 8.f, MARGIN.y + 8.f });
    }
    int score = 0;

    int lives = 3;
    std::optional<sf::Text> livesText;
    if (hasFont) {
        livesText.emplace(font, "Lives: 3", 28);
        livesText->setFillColor(sf::Color::White);
        livesText->setPosition({ MARGIN.x + 200.f, MARGIN.y + 8.f });
    }

    std::vector<Shield> shields;

    sf::Clock clock;
    float dt = 0.f;

    const float SHOOT_COOLDOWN = 0.6f;
    float shootTimer = 0.f;

    auto spawnBulletFromPool = [&](const sf::Vector2f& pos, float speedY) -> bool {
        for (auto &b : bullets) {
            if (!b.isActive()) {
                b.spawn(pos, speedY);
                if (laserSound.has_value()) laserSound->play();
                return true;
            }
        }
        return false;
    };

    auto spawnEnemyBulletFromPool = [&](const sf::Vector2f& pos, float speedY) -> bool {
        for (auto &b : enemyBullets) {
            if (!b.isActive()) {
                b.spawn(pos, speedY);
                return true;
            }
        }
        return false;
    };
    const int SHIELD_HP = 5;
    auto resetGame = [&]() {
        player.setPosition(playerStart);
        formation.reset();
        for (auto &b : bullets) b.deactivate();
        for (auto &b : enemyBullets) b.deactivate();
        shields.clear();
        if (hasShieldTex) {
            float shieldsY = playerStart.y - 120.f;
            float gap = static_cast<float>(windowWidth) / 4.f;
            float centerX = static_cast<float>(windowWidth) / 2.f;
            sf::Vector2f desiredSize1{ 100.f, 50.f };
            sf::Vector2f desiredSize2{ 100.f, 50.f };
            shields.emplace_back(&texShield, sf::Vector2f{ centerX - gap - desiredSize1.x / 2.f, shieldsY }, SHIELD_HP, desiredSize1);
            shields.emplace_back(&texShield, sf::Vector2f{ centerX - desiredSize2.x / 2.f, shieldsY }, SHIELD_HP, desiredSize2);
            shields.emplace_back(&texShield, sf::Vector2f{ centerX + gap - desiredSize1.x / 2.f, shieldsY }, SHIELD_HP, desiredSize1);
        }
    };

    std::mt19937 rng(static_cast<unsigned int>(std::random_device{}()));
    const float ENEMY_SHOOT_INTERVAL_MIN = 0.9f;
    const float ENEMY_SHOOT_INTERVAL_MAX = 2.0f;
    std::uniform_real_distribution<float> shootIntervalDist(ENEMY_SHOOT_INTERVAL_MIN, ENEMY_SHOOT_INTERVAL_MAX);
    std::uniform_int_distribution<int> colDist(0, ENEMY_COLS - 1);
    float enemyShootTimer = shootIntervalDist(rng);

    auto trySpawnFromColumn = [&](int col) -> bool {
        auto &en = formation.enemies();
        for (int r = ENEMY_ROWS - 1; r >= 0; --r) {
            int idx = r * ENEMY_COLS + col;
            if (idx < 0 || idx >= static_cast<int>(en.size())) continue;
            auto &enemy = en[idx];
            if (enemy.isActive()) {
                sf::FloatRect eb = enemy.bounds();
                sf::Vector2f shotPos{ eb.position.x + eb.size.x / 2.f, eb.position.y + eb.size.y + 4.f };
                if (spawnEnemyBulletFromPool(shotPos, 220.f)) return true;
                return false;
            }
        }
        return false;
    };

    resetGame();

    while (window.isOpen()) {
        while (auto evOpt = window.pollEvent()) {
            const sf::Event& ev = *evOpt;
            if (ev.is<sf::Event::Closed>()) {
                window.close();
                break;
            }
            if (ev.is<sf::Event::KeyPressed>()) {
                const auto* k = ev.getIf<sf::Event::KeyPressed>();
                if (k && k->code == sf::Keyboard::Key::Escape) {
                    window.close();
                    break;
                }
            }
            if (state == AppState::Menu) {
                menu.processEvent(ev, window);
            } else {
            }
        }

        dt = clock.restart().asSeconds();

        if (state == AppState::Menu) {
            menu.update(dt);
            if (menu.consumeConfirm()) {
                int sel = menu.getSelectedIndex();
                if (sel == 0) {
                    resetGame();
                    score = 0;
                    lives = 3;
                    if (scoreText) scoreText->setString("Score: 0");
                    if (livesText) livesText->setString("Lives: " + std::to_string(lives));
                    state = AppState::Playing;
                } else if (sel == 1) {
                    window.close();
                    break;
                }
            }
        } else {
            shootTimer -= dt;
            if (shootTimer < 0.f) shootTimer = 0.f;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                player.moveLeft(dt);
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                player.moveRight(dt);
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
                if (shootTimer <= 0.f) {
                    sf::FloatRect pb = player.bounds();
                    sf::Vector2f bulletPos{ pb.position.x + pb.size.x / 2.f, pb.position.y - 6.f };
                    if (spawnBulletFromPool(bulletPos, -480.f)) {
                        shootTimer = SHOOT_COOLDOWN;
                    }
                }
            }

            player.update(dt);
            for (auto &b : bullets) b.update(dt);
            for (auto &b : enemyBullets) b.update(dt);
            formation.update(dt, MARGIN.x, static_cast<float>(windowWidth) - MARGIN.x);

            enemyShootTimer -= dt;
            if (enemyShootTimer <= 0.f) {
                int tries = ENEMY_COLS;
                bool spawned = false;
                while (tries-- > 0 && !spawned) {
                    int col = colDist(rng);
                    spawned = trySpawnFromColumn(col);
                }
                enemyShootTimer = shootIntervalDist(rng);
            }

            for (auto &b : bullets) {
                if (!b.isActive()) continue;
                bool hitShield = false;
                for (auto &s : shields) {
                    if (!s.isActive()) continue;
                    if (rectsIntersect(s.bounds(), b.bounds())) {
                        b.deactivate();
                        hitShield = true;
                        break;
                    }
                }
                if (hitShield) continue;
                for (auto &e : formation.enemies()) {
                    if (!e.isActive()) continue;
                    if (rectsIntersect(b.bounds(), e.bounds())) {
                        b.deactivate();
                        e.setActive(false);
                        score += 10;
                        if (scoreText) scoreText->setString("Score: " + std::to_string(score));
                        break;
                    }
                }
            }

            for (auto &b : enemyBullets) {
                if (!b.isActive()) continue;
                bool hitShield = false;
                for (auto &s : shields) {
                    if (!s.isActive()) continue;
                    if (rectsIntersect(s.bounds(), b.bounds())) {
                        b.deactivate();
                        s.takeDamage(1);
                        hitShield = true;
                        break;
                    }
                }
                if (hitShield) continue;
                if (rectsIntersect(b.bounds(), player.bounds())) {
                    b.deactivate();
                    lives -= 1;
                    if (livesText) livesText->setString("Lives: " + std::to_string(lives));
                    if (lives <= 0) {
                        state = AppState::Menu;
                        resetGame();
                    } else {
                        player.setPosition(playerStart);
                    }
                }
            }

            for (auto &e : formation.enemies()) {
                if (!e.isActive()) continue;
                bool collidedWithShield = false;
                for (auto &s : shields) {
                    if (!s.isActive()) continue;
                    if (rectsIntersect(s.bounds(), e.bounds())) {
                        collidedWithShield = true;
                        break;
                    }
                }
                if (collidedWithShield) continue;
                if (rectsIntersect(e.bounds(), player.bounds())) {
                    e.setActive(false);
                    lives -= 1;
                    if (livesText) livesText->setString("Lives: " + std::to_string(lives));
                    if (lives <= 0) {
                        state = AppState::Menu;
                        resetGame();
                    } else {
                        player.setPosition(playerStart);
                    }
                }
            }
        }

        window.clear(sf::Color(18, 18, 28));

        if (bgSprite) window.draw(*bgSprite);

        if (state == AppState::Menu) {
            menu.draw(window);
        } else {
            for (auto &s : shields) s.draw(window);
            formation.draw(window);
            for (auto &b : bullets) if (b.isActive()) b.draw(window);
            for (auto &b : enemyBullets) if (b.isActive()) b.draw(window);
            player.draw(window);
            if (scoreText) window.draw(*scoreText);
            if (livesText) window.draw(*livesText);
        }

        window.display();
    }

    return 0;
}