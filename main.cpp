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

    sf::Texture texPlayer, texBulletPlayer, texBulletEnemy;
    sf::Texture texAlienTop, texAlienMid, texAlienBot, texShield;
    sf::Font font;
    bool hasPlayerTex = texPlayer.loadFromFile("assets/textures/player.png");
    bool hasBulletPlayer = texBulletPlayer.loadFromFile("assets/textures/bullet.png");
    bool hasBulletEnemy = texBulletEnemy.loadFromFile("assets/textures/bullet_2.png");
    bool hasAlienTop  = texAlienTop.loadFromFile("assets/textures/alien_top.png");
    bool hasAlienMid  = texAlienMid.loadFromFile("assets/textures/alien_mid.png");
    bool hasAlienBot  = texAlienBot.loadFromFile("assets/textures/alien_bottom.png");
    bool hasShieldTex = texShield.loadFromFile("assets/textures/shield.png");
    bool hasFont      = font.openFromFile("assets/fonts/font.ttf");

    if (!hasPlayerTex)  std::cerr << "[WARN] No pudo cargar assets/textures/player.png\n";
    if (!hasBulletPlayer) std::cerr << "[WARN] No pudo cargar assets/textures/bullet.png\n";
    if (!hasBulletEnemy) std::cerr << "[WARN] No pudo cargar assets/textures/bullet_2.png\n";
    if (!hasAlienTop)   std::cerr << "[WARN] No pudo cargar assets/textures/alien_top.png\n";
    if (!hasAlienMid)   std::cerr << "[WARN] No pudo cargar assets/textures/alien_mid.png\n";
    if (!hasAlienBot)   std::cerr << "[WARN] No pudo cargar assets/textures/alien_bottom.png\n";
    if (!hasShieldTex)  std::cerr << "[WARN] No pudo cargar assets/textures/shield.png\n";
    if (!hasFont)       std::cerr << "[WARN] No pudo cargar assets/fonts/font.ttf\n";

    sf::SoundBuffer laserBuf;
    std::optional<sf::Sound> laserSound;
    if (laserBuf.loadFromFile("assets/sounds/laser_sound.mp3")) {
        laserSound.emplace(laserBuf);
    }

    Menu menu(hasFont ? &font : nullptr, 80);
    menu.setOptions({ "NEW GAME", "EXIT" }, { static_cast<float>(windowWidth) / 2.f, static_cast<float>(windowHeight) / 2.f }, 140.f);

    Menu pauseMenu(hasFont ? &font : nullptr, 56);
    pauseMenu.setOptions({ "RESUME", "RESTART", "EXIT TO MENU" }, { static_cast<float>(windowWidth) / 2.f, static_cast<float>(windowHeight) / 2.f }, 96.f);

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

    auto createFormation = [&](void) -> std::unique_ptr<Formation> {
        return std::make_unique<Formation>(
            hasAlienTop ? &texAlienTop : nullptr,
            hasAlienMid ? &texAlienMid : nullptr,
            hasAlienBot ? &texAlienBot : nullptr,
            ENEMY_COLS, ENEMY_ROWS,
            sf::Vector2f{ formationStartX, formationStartY },
            spacingX, spacingY,
            40.f, 18.f
        );
    };

    std::unique_ptr<Formation> formation = createFormation();

    std::optional<sf::Text> scoreText;
    if (hasFont) {
        scoreText.emplace(font, "Score: 0", 28);
        scoreText->setFillColor(sf::Color::White);
    }
    int score = 0;

    std::optional<sf::Text> livesText;
    if (hasFont) {
        livesText.emplace(font, "Lives: 3", 28);
        livesText->setFillColor(sf::Color::White);
    }
    int lives = 3;

    std::vector<Shield> shields;

    sf::Clock clock;
    float dt = 0.f;

    const float SHOOT_COOLDOWN = 0.6f;
    float shootTimer = 0.f;

    const int SHIELD_HP = 5;

    bool pausedForResult = false;
    bool paused = false;
    std::string resultMessage;
    std::optional<sf::Text> overlayTitle;
    std::optional<sf::Text> overlaySub;
    if (hasFont) {
        overlayTitle.emplace(font, "", 64);
        overlayTitle->setFillColor(sf::Color::White);
        overlaySub.emplace(font, "", 28);
        overlaySub->setFillColor(sf::Color(200,200,200));
    }

    auto resetGame = [&]() {
        player.setPosition(playerStart);
        formation = createFormation();
        for (auto &b : bullets) b.deactivate();
        for (auto &b : enemyBullets) b.deactivate();
        shields.clear();
        pausedForResult = false;
        resultMessage.clear();
        paused = false;

        float shieldsY = playerStart.y - 120.f;
        sf::Vector2f desiredSize{ 140.f, 70.f };
        float padding = 48.f;
        float available = static_cast<float>(windowWidth) - 2.f * padding;
        float totalW = 4.f * desiredSize.x;
        float gapBetween = 0.f;
        if (available > totalW) {
            gapBetween = (available - totalW) / 3.f + desiredSize.x;
        } else {
            gapBetween = desiredSize.x + 12.f;
        }
        float firstCenterX = padding + desiredSize.x * 0.5f;
        for (int i = 0; i < 4; ++i) {
            float centerX = firstCenterX + static_cast<float>(i) * gapBetween;
            if (hasShieldTex) {
                shields.emplace_back(&texShield, sf::Vector2f{ centerX - desiredSize.x / 2.f, shieldsY }, SHIELD_HP, desiredSize);
            }
        }

        score = 0;
        lives = 3;
        if (scoreText) scoreText->setString("Score: 0");
        if (livesText) livesText->setString("Lives: 3");
    };

    std::mt19937 rng(static_cast<unsigned int>(std::random_device{}()));
    const float ENEMY_SHOOT_INTERVAL_MIN = 0.8f;
    const float ENEMY_SHOOT_INTERVAL_MAX = 1.8f;
    std::uniform_real_distribution<float> shootIntervalDist(ENEMY_SHOOT_INTERVAL_MIN, ENEMY_SHOOT_INTERVAL_MAX);
    std::uniform_int_distribution<int> colDist(0, ENEMY_COLS - 1);
    float enemyShootTimer = shootIntervalDist(rng);

    auto trySpawnFromColumn = [&](int col) -> bool {
        auto &en = formation->enemies();
        for (int r = ENEMY_ROWS - 1; r >= 0; --r) {
            int idx = r * ENEMY_COLS + col;
            if (idx < 0 || idx >= static_cast<int>(en.size())) continue;
            auto &enemy = en[idx];
            if (enemy.isActive()) {
                sf::FloatRect eb = enemy.bounds();
                sf::Vector2f shotPos{ eb.position.x + eb.size.x / 2.f, eb.position.y + eb.size.y + 4.f };
                for (auto &b : enemyBullets) if (!b.isActive()) { b.spawn(shotPos, 220.f); return true; }
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
                auto k = ev.getIf<sf::Event::KeyPressed>();
                if (k) {
                    if (state == AppState::Playing && pausedForResult) {
                        if (k->code == sf::Keyboard::Key::Enter || k->code == sf::Keyboard::Key::Space) {
                            resetGame();
                        }
                    } else if (state == AppState::Playing && !pausedForResult) {
                        if (k->code == sf::Keyboard::Key::Escape) {
                            paused = !paused;
                        }
                    }
                }
            }
            if (state == AppState::Menu) {
                menu.processEvent(ev, window);
            } else {
                if (paused && !pausedForResult) {
                    pauseMenu.processEvent(ev, window);
                }
            }
        }

        dt = clock.restart().asSeconds();

        if (state == AppState::Menu) {
            menu.update(dt);
            if (menu.consumeConfirm()) {
                int sel = menu.getSelectedIndex();
                if (sel == 0) {
                    resetGame();
                    state = AppState::Playing;
                } else if (sel == 1) {
                    window.close();
                    break;
                }
            }
        } else {
            if (paused && !pausedForResult) {
                pauseMenu.update(dt);
                if (pauseMenu.consumeConfirm()) {
                    int sel = pauseMenu.getSelectedIndex();
                    if (sel == 0) {
                        paused = false;
                    } else if (sel == 1) {
                        resetGame();
                        state = AppState::Playing;
                    } else if (sel == 2) {
                        paused = false;
                        state = AppState::Menu;
                    }
                }
            } else if (!paused && !pausedForResult) {
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
                        for (auto &b : bullets) {
                            if (!b.isActive()) {
                                b.spawn(bulletPos, -480.f);
                                if (laserSound.has_value()) laserSound->play();
                                shootTimer = SHOOT_COOLDOWN;
                                break;
                            }
                        }
                    }
                }

                player.update(dt);
                for (auto &b : bullets) b.update(dt);
                for (auto &b : enemyBullets) b.update(dt);
                formation->update(dt, MARGIN.x, static_cast<float>(windowWidth) - MARGIN.x);

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
                    for (auto &e : formation->enemies()) {
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
                            pausedForResult = true;
                            resultMessage = "GAME OVER";
                            if (hasFont) {
                                overlayTitle->setString(resultMessage);
                                overlayTitle->setFillColor(sf::Color::Red);
                                overlaySub->setString("Press ENTER to restart");
                                sf::FloatRect rt = overlayTitle->getLocalBounds();
                                float ox = rt.position.x + rt.size.x * 0.5f;
                                float oy = rt.position.y + rt.size.y * 0.5f;
                                overlayTitle->setOrigin(sf::Vector2f(ox, oy));
                                overlayTitle->setPosition(sf::Vector2f(static_cast<float>(windowWidth)/2.f, static_cast<float>(windowHeight)/2.f - 24.f));
                                sf::FloatRect rs = overlaySub->getLocalBounds();
                                float sox = rs.position.x + rs.size.x * 0.5f;
                                float soy = rs.position.y + rs.size.y * 0.5f;
                                overlaySub->setOrigin(sf::Vector2f(sox, soy));
                                overlaySub->setPosition(sf::Vector2f(static_cast<float>(windowWidth)/2.f, static_cast<float>(windowHeight)/2.f + 40.f));
                            }
                        } else {
                            player.setPosition(playerStart);
                        }
                    }
                }

                for (auto &e : formation->enemies()) {
                    if (!e.isActive()) continue;
                    sf::FloatRect eb = e.bounds();
                    if (eb.position.y + eb.size.y >= playerStart.y - CELL_SIZE * 0.5f) {
                        pausedForResult = true;
                        resultMessage = "GAME OVER";
                        if (hasFont) {
                            overlayTitle->setString(resultMessage);
                            overlayTitle->setFillColor(sf::Color::Red);
                            overlaySub->setString("Press ENTER to restart");
                            sf::FloatRect rt = overlayTitle->getLocalBounds();
                            float ox = rt.position.x + rt.size.x * 0.5f;
                            float oy = rt.position.y + rt.size.y * 0.5f;
                            overlayTitle->setOrigin(sf::Vector2f(ox, oy));
                            overlayTitle->setPosition(sf::Vector2f(static_cast<float>(windowWidth)/2.f, static_cast<float>(windowHeight)/2.f - 24.f));
                            sf::FloatRect rs = overlaySub->getLocalBounds();
                            float sox = rs.position.x + rs.size.x * 0.5f;
                            float soy = rs.position.y + rs.size.y * 0.5f;
                            overlaySub->setOrigin(sf::Vector2f(sox, soy));
                            overlaySub->setPosition(sf::Vector2f(static_cast<float>(windowWidth)/2.f, static_cast<float>(windowHeight)/2.f + 40.f));
                        }
                        break;
                    }
                }

                {
                    bool anyAlive = false;
                    for (auto &e : formation->enemies()) { if (e.isActive()) { anyAlive = true; break; } }
                    if (!anyAlive) {
                        pausedForResult = true;
                        resultMessage = "YOU WIN";
                        if (hasFont) {
                            overlayTitle->setString(resultMessage);
                            overlayTitle->setFillColor(sf::Color::Yellow);
                            overlaySub->setString("Press ENTER to restart");
                            sf::FloatRect rt = overlayTitle->getLocalBounds();
                            float ox = rt.position.x + rt.size.x * 0.5f;
                            float oy = rt.position.y + rt.size.y * 0.5f;
                            overlayTitle->setOrigin(sf::Vector2f(ox, oy));
                            overlayTitle->setPosition(sf::Vector2f(static_cast<float>(windowWidth)/2.f, static_cast<float>(windowHeight)/2.f - 24.f));
                            sf::FloatRect rs = overlaySub->getLocalBounds();
                            float sox = rs.position.x + rs.size.x * 0.5f;
                            float soy = rs.position.y + rs.size.y * 0.5f;
                            overlaySub->setOrigin(sf::Vector2f(sox, soy));
                            overlaySub->setPosition(sf::Vector2f(static_cast<float>(windowWidth)/2.f, static_cast<float>(windowHeight)/2.f + 40.f));
                        }
                    }
                }
            }
        }

        window.clear(sf::Color(18, 18, 28));

        if (state == AppState::Menu) {
            menu.draw(window);
        } else {
            for (auto &s : shields) s.draw(window);
            formation->draw(window);
            for (auto &b : bullets) if (b.isActive()) b.draw(window);
            for (auto &b : enemyBullets) if (b.isActive()) b.draw(window);
            player.draw(window);

            if (scoreText) {
                scoreText->setPosition(sf::Vector2f(MARGIN.x + 8.f, static_cast<float>(windowHeight) - MARGIN.y - scoreText->getGlobalBounds().size.y - 8.f));
                window.draw(*scoreText);
            }
            if (livesText) {
                float lw = livesText->getGlobalBounds().size.x;
                float lh = livesText->getGlobalBounds().size.y;
                livesText->setPosition(sf::Vector2f(static_cast<float>(windowWidth) - MARGIN.x - 8.f - lw, static_cast<float>(windowHeight) - MARGIN.y - lh - 8.f));
                window.draw(*livesText);
            }

            if (paused && !pausedForResult) {
                sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(windowWidth), static_cast<float>(windowHeight)));
                overlay.setFillColor(sf::Color(0,0,0,160));
                window.draw(overlay);
                pauseMenu.draw(window);
            }

            if (pausedForResult) {
                sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(windowWidth), static_cast<float>(windowHeight)));
                overlay.setFillColor(sf::Color(0,0,0,160));
                window.draw(overlay);
                if (hasFont && overlayTitle && overlaySub) {
                    window.draw(*overlayTitle);
                    window.draw(*overlaySub);
                }
            }
        }

        window.display();
    }

    return 0;
}