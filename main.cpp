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
#include "Menu.h"
#include "Formation.h"

static bool rectsIntersect(const sf::FloatRect& a, const sf::FloatRect& b) {
    return !(a.position.x + a.size.x < b.position.x ||
             b.position.x + b.size.x < a.position.x ||
             a.position.y + a.size.y < b.position.y ||
             b.position.y + b.size.y < a.position.y);
}

int main() {
    const int WINDOW_COLS = 20;
    const int WINDOW_ROWS = 25;
    const int CELL_SIZE = 32;
    const float HUD_HEIGHT = 80.f;
    const sf::Vector2f MARGIN{12.f, 12.f};

    unsigned int windowWidth = static_cast<unsigned int>(MARGIN.x * 2 + WINDOW_COLS * CELL_SIZE);
    unsigned int windowHeight = static_cast<unsigned int>(MARGIN.y + HUD_HEIGHT + WINDOW_ROWS * CELL_SIZE + MARGIN.y);

    sf::RenderWindow window(sf::VideoMode({ windowWidth, windowHeight }), "Space");
    window.setVerticalSyncEnabled(true);

    sf::Texture texPlayer, texEnemy, texBullet, texBackground;
    sf::Font font;
    sf::Texture texAlienTop, texAlienMid, texAlienBot;
    bool hasAlienTop = texAlienTop.loadFromFile("assets/textures/alien_top.png");
    bool hasAlienMid = texAlienMid.loadFromFile("assets/textures/alien_mid.png");
    bool hasAlienBot = texAlienBot.loadFromFile("assets/textures/alien_bottom.png");

    bool hasPlayerTex = texPlayer.loadFromFile("assets/textures/player.png");
    bool hasEnemyTex  = texEnemy.loadFromFile("assets/textures/alien_bottom.png");
    bool hasBulletTex = texBullet.loadFromFile("assets/textures/bullet.png");
    bool hasBgTex     = texBackground.loadFromFile("assets/textures/background_2.png");
    bool hasFont      = font.openFromFile("assets/fonts/font.ttf");


    sf::SoundBuffer laserBuf;
    std::optional<sf::Sound> laserSound;
    if (laserBuf.loadFromFile("assets/sounds/laser_sound.mp3")) {
        laserSound.emplace(laserBuf);
        std::cout << "Cargado sonido laser: assets/sounds/laser_sound.mp3\n";
    } else {
        std::cerr << "No se pudo cargar assets/sounds/laser_sound.mp3\n";
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

    Menu menu(hasFont ? &font : nullptr, 36);
    menu.setOptions({ "Jugar", "Salir" }, { static_cast<float>(windowWidth) / 2.f, 220.f }, 64.f);

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
    for (size_t i = 0; i < BULLET_POOL_SIZE; ++i) {
        bullets.emplace_back(hasBulletTex ? &texBullet : nullptr);
    }

    const int ENEMY_COLS = 11;
    const int ENEMY_ROWS = 5;
    const float formationStartX = MARGIN.x + 2.f * CELL_SIZE;
    const float formationStartY = MARGIN.y + HUD_HEIGHT + 1.f * CELL_SIZE;
    const float spacingX = static_cast<float>(CELL_SIZE) * 1.5f;
    const float spacingY = static_cast<float>(CELL_SIZE) * 1.15f;

    Formation formation(
        hasAlienTop ? &texAlienTop : nullptr,
        hasAlienMid ? &texAlienMid : nullptr,
        hasAlienBot ? &texAlienBot : nullptr,
        ENEMY_COLS, ENEMY_ROWS,
        sf::Vector2f{ formationStartX, formationStartY },
        spacingX, spacingY,
        /*speed*/ 40.f, /*dropAmount*/ 18.f
    );


    auto resetGame = [&]() {
        player.setPosition(playerStart);
        formation.reset();
        for (auto &b : bullets) b.deactivate();
    };
    resetGame();

    std::optional<sf::Text> scoreText;
    if (hasFont) {
        scoreText.emplace(font, "Score: 0", 35);
        scoreText->setFillColor(sf::Color::White);
        scoreText->setPosition({ MARGIN.x + 8.f, MARGIN.y + 8.f });
    }
    int score = 0;

    sf::Clock clock;
    float dt = 0.f;

    // Helper: spawn bullet usando pool (comprueba laserSound antes de play)
    auto spawnBulletFromPool = [&](const sf::Vector2f& pos, float speedY) {
        for (auto &b : bullets) {
            if (!b.isActive()) {
                b.spawn(pos, speedY);
                if (laserSound.has_value()) laserSound->play();
                return;
            }
        }
    };

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
                if (ev.is<sf::Event::KeyPressed>()) {
                    const auto* k = ev.getIf<sf::Event::KeyPressed>();
                    if (!k) continue;
                    if (k->code == sf::Keyboard::Key::Space) {
                        sf::FloatRect pb = player.bounds();
                        sf::Vector2f bulletPos{ pb.position.x + pb.size.x / 2.f, pb.position.y - 6.f };
                        spawnBulletFromPool(bulletPos, -480.f); // speedY negativo => sube
                    } else if (k->code == sf::Keyboard::Key::R) {
                        resetGame();
                        score = 0;
                        if (scoreText) scoreText->setString("Score: 0");
                    }
                }
            }
        }

        dt = clock.restart().asSeconds();

        if (state == AppState::Menu) {
            menu.update(dt);
            if (menu.consumeConfirm()) {
                int sel = menu.getSelectedIndex();
                if (sel == 0) { // Play
                    resetGame();
                    score = 0;
                    if (scoreText) scoreText->setString("Score: 0");
                    state = AppState::Playing;
                } else if (sel == 1) { // Exit
                    window.close();
                    break;
                }
            }
        } else {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                player.moveLeft(dt);
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                player.moveRight(dt);
            }

            player.update(dt);
            for (auto &b : bullets) b.update(dt);
            formation.update(dt, /*screenLeft=*/MARGIN.x, /*screenRight=*/ static_cast<float>(windowWidth) - MARGIN.x);
            for (auto &b : bullets) {
                if (!b.isActive()) continue;
                for (auto &e : formation.enemies()) {
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

            for (auto &e : formation.enemies()) {
                if (!e.isActive()) continue;
                if (rectsIntersect(e.bounds(), player.bounds())) {
                    e.setActive(false);
                    player.setPosition(playerStart);
                }
            }
        }

        window.clear(sf::Color(18, 18, 28));

        if (bgSprite) {
            window.draw(*bgSprite);
        }

        if (state == AppState::Menu) {
            menu.draw(window);
        } else {
            formation.draw(window);
            for (auto &b : bullets) {
                if (b.isActive()) b.draw(window);
            }
            player.draw(window);
            if (scoreText) window.draw(*scoreText);
        }

        window.display();
    }

    return 0;
}