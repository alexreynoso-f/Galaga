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

    // VIRTUAL size (logical/game coordinate space). We keep the game content layout tied to this.
    const unsigned int VIRTUAL_WIDTH = windowWidth;
    const unsigned int VIRTUAL_HEIGHT = windowHeight;

    // Maximum content width the game will expand to. If the window becomes wider than this,
    // the game content will not stretch further; instead we'll center it with side bars.
    const unsigned int MAX_CONTENT_WIDTH = 1280u;

    sf::RenderWindow window(sf::VideoMode({ windowWidth, windowHeight }), "Naves");
    window.setVerticalSyncEnabled(true);

    // Create a view for rendering the game world in VIRTUAL coordinates and a letterbox/centered viewport
    // SFML 3: sf::FloatRect uses a (position, size) constructor (two vectors).
    sf::View gameView(sf::FloatRect({0.f, 0.f}, {static_cast<float>(VIRTUAL_WIDTH), static_cast<float>(VIRTUAL_HEIGHT)}));

    auto updateGameViewForWindow = [&](unsigned int winW, unsigned int winH) {
        // Compute scaleX but clamp based on MAX_CONTENT_WIDTH to avoid overly wide stretching
        float scaleX = static_cast<float>(winW) / static_cast<float>(VIRTUAL_WIDTH);
        if (winW > MAX_CONTENT_WIDTH) {
            scaleX = static_cast<float>(MAX_CONTENT_WIDTH) / static_cast<float>(VIRTUAL_WIDTH);
        }
        float scaleY = static_cast<float>(winH) / static_cast<float>(VIRTUAL_HEIGHT);

        // choose the scale that fits entirely (letterbox)
        float scale = std::min(scaleX, scaleY);

        float viewWpx = static_cast<float>(VIRTUAL_WIDTH) * scale;
        float viewHpx = static_cast<float>(VIRTUAL_HEIGHT) * scale;

        float vpW = viewWpx / static_cast<float>(winW);
        float vpH = viewHpx / static_cast<float>(winH);
        float vpL = (1.f - vpW) * 0.5f;
        float vpT = (1.f - vpH) * 0.5f;

        // SFML 3: construct FloatRect with two vectors
        gameView.setViewport(sf::FloatRect({vpL, vpT}, {vpW, vpH}));
    };

    updateGameViewForWindow(window.getSize().x, window.getSize().y);

    // --- assets ---
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

    // --- music ---
    sf::Music bgMusic;
    bool hasMusic = false;
    bool musicOn = false;
    if (bgMusic.openFromFile("assets/music/bg_music.ogg")) {
        // Use the SFML API available in your build: setLooping is present there
        bgMusic.setLooping(true);
        bgMusic.play();
        hasMusic = true;
        musicOn = true;
    } else {
        std::cerr << "[WARN] Could not load assets/music/bg_music.ogg. Music disabled.\n";
    }

    // This flag remembers if music was playing when menu appeared (so we can resume it)
    bool musicWasPlayingBeforeMenu = false;

    // UI: music toggle button (drawn in default view, upper-left)
    sf::RectangleShape musicBtn;
    musicBtn.setSize({48.f, 48.f});
    // SFML setPosition takes a single Vector2f in this build
    musicBtn.setPosition(MARGIN);
    musicBtn.setFillColor(sf::Color(40, 40, 50));
    musicBtn.setOutlineColor(sf::Color(200, 200, 200));
    musicBtn.setOutlineThickness(-2.f);

    // music icon text — sf::Text does not have a default ctor in this SFML build
    std::optional<sf::Text> musicIcon;
    if (hasFont) {
        musicIcon.emplace(font, musicOn ? "Off" : "On", 22);
        musicIcon->setFillColor(sf::Color::White);
        auto rt = musicIcon->getLocalBounds();
        float ox = rt.position.x + rt.size.x * 0.5f;
        float oy = rt.position.y + rt.size.y * 0.5f;
        musicIcon->setOrigin(sf::Vector2f{ox, oy});
        sf::Vector2f bpos = musicBtn.getPosition();
        sf::Vector2f bsize = musicBtn.getSize();
        musicIcon->setPosition(sf::Vector2f{ bpos.x + bsize.x * 0.5f, bpos.y + bsize.y * 0.5f });
    }

    sf::SoundBuffer laserBuf;
    std::optional<sf::Sound> laserSound;
    if (laserBuf.loadFromFile("assets/sounds/laser_sound.mp3")) {
        laserSound.emplace(laserBuf);
    }

    Menu menu(hasFont ? &font : nullptr, 80);
    menu.setOptions({ "NEW GAME", "EXIT" }, { static_cast<float>(VIRTUAL_WIDTH) / 2.f, static_cast<float>(VIRTUAL_HEIGHT) / 2.f }, 140.f);

    Menu pauseMenu(hasFont ? &font : nullptr, 56);
    pauseMenu.setOptions({ "RESUME", "RESTART", "EXIT TO MENU" }, { static_cast<float>(VIRTUAL_WIDTH) / 2.f, static_cast<float>(VIRTUAL_HEIGHT) / 2.f }, 96.f);

    enum class AppState { Menu, Playing };
    AppState state = AppState::Menu;

    sf::Vector2f playerStart{
        MARGIN.x + (WINDOW_COLS * CELL_SIZE) / 2.f,
        MARGIN.y + HUD_HEIGHT + (WINDOW_ROWS * CELL_SIZE) - CELL_SIZE * 1.5f
    };
    Player player(hasPlayerTex ? &texPlayer : nullptr, playerStart);
    float leftMargin = 16.f;
    float rightMargin = static_cast<float>(VIRTUAL_WIDTH);
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
        float available = static_cast<float>(VIRTUAL_WIDTH) - 2.f * padding;
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

            // handle resize: update view viewport
            if (ev.is<sf::Event::Resized>()) {
                auto r = ev.getIf<sf::Event::Resized>();
                if (r) {
                    // SFML 3: Resized event stores .size (vector)
                    updateGameViewForWindow(static_cast<unsigned int>(r->size.x), static_cast<unsigned int>(r->size.y));
                }
            }

            // handle mouse clicks for music button and menus
            if (ev.is<sf::Event::MouseButtonPressed>()) {
                auto mb = ev.getIf<sf::Event::MouseButtonPressed>();
                if (mb && mb->button == sf::Mouse::Button::Left) {
                    // only allow toggling music while playing (no menus visible)
                    if (state == AppState::Playing && !paused && !pausedForResult) {
                        sf::Vector2i pix = sf::Mouse::getPosition(window);
                        sf::Vector2f mp = window.mapPixelToCoords(pix);
                        if (musicBtn.getGlobalBounds().contains(mp)) {
                            if (hasMusic) {
                                if (musicOn) {
                                    bgMusic.pause();
                                    musicOn = false;
                                } else {
                                    bgMusic.play();
                                    musicOn = true;
                                }
                                if (musicIcon) {
                                    musicIcon->setString(musicOn ? "Off" : "On");
                                    auto rt = musicIcon->getLocalBounds();
                                    float ox = rt.position.x + rt.size.x * 0.5f;
                                    float oy = rt.position.y + rt.size.y * 0.5f;
                                    musicIcon->setOrigin(sf::Vector2f{ox, oy});
                                    sf::Vector2f bpos = musicBtn.getPosition();
                                    sf::Vector2f bsize = musicBtn.getSize();
                                    musicIcon->setPosition(sf::Vector2f{ bpos.x + bsize.x * 0.5f, bpos.y + bsize.y * 0.5f });
                                }
                            } else {
                                // no music loaded: toggle visual state only
                                musicOn = !musicOn;
                                if (musicIcon) musicIcon->setString(musicOn ? "Off" : "On");
                            }
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
                formation->update(dt, MARGIN.x, static_cast<float>(VIRTUAL_WIDTH) - MARGIN.x);

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
                        }
                    }
                }
            }
        }

        // Decide whether a menu is visible (main menu, pause menu, or end-game overlay).
        bool menuVisible = (state == AppState::Menu) || (state == AppState::Playing && (paused || pausedForResult));

        // Music handling: pause music while menu visible, resume when returning to gameplay
        if (hasMusic) {
            auto status = bgMusic.getStatus();
            if (menuVisible) {
                // if it's playing now, remember and pause it
                if (status == sf::SoundSource::Status::Playing) {
                    musicWasPlayingBeforeMenu = true;
                    bgMusic.pause();
                }
            } else {
                // not menuVisible -> gameplay active; resume if it was playing before menu and user hasn't turned music off
                if (musicWasPlayingBeforeMenu && musicOn) {
                    bgMusic.play();
                }
                // reset the remembering flag
                musicWasPlayingBeforeMenu = false;
            }
        }

        window.clear(sf::Color(18, 18, 28));

        // Render logic
        if (state == AppState::Menu) {
            // Main menu: draw only the menu (default view)
            window.setView(window.getDefaultView());
            // Draw a neutral background behind the menu so nothing below shows
            sf::RectangleShape fullBg(sf::Vector2f(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)));
            fullBg.setFillColor(sf::Color(8,8,12));
            window.draw(fullBg);
            menu.draw(window);
        } else { // Playing
            if (paused || pausedForResult) {
                // Menu visible during gameplay: draw only the pause menu / overlay (no game world)
                window.setView(window.getDefaultView());
                // draw dark background so game objects aren't visible
                sf::RectangleShape fullBg(sf::Vector2f(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)));
                fullBg.setFillColor(sf::Color(0,0,0,200));
                window.draw(fullBg);

                if (paused && !pausedForResult) {
                    // draw pause menu (only)
                    pauseMenu.draw(window);
                }

                if (pausedForResult) {
                    // draw end-game centered overlay (only)
                    if (hasFont && overlayTitle && overlaySub) {
                        sf::Vector2u cur = window.getSize();
                        sf::FloatRect rt = overlayTitle->getLocalBounds();
                        float ox = rt.position.x + rt.size.x * 0.5f;
                        float oy = rt.position.y + rt.size.y * 0.5f;
                        overlayTitle->setOrigin(sf::Vector2f(ox, oy));
                        overlayTitle->setPosition(sf::Vector2f(static_cast<float>(cur.x)/2.f, static_cast<float>(cur.y)/2.f - 24.f));

                        sf::FloatRect rs = overlaySub->getLocalBounds();
                        float sox = rs.position.x + rs.size.x * 0.5f;
                        float soy = rs.position.y + rs.size.y * 0.5f;
                        overlaySub->setOrigin(sf::Vector2f(sox, soy));
                        overlaySub->setPosition(sf::Vector2f(static_cast<float>(cur.x)/2.f, static_cast<float>(cur.y)/2.f + 40.f));

                        window.draw(*overlayTitle);
                        window.draw(*overlaySub);
                    }
                }
            } else {
                // Normal gameplay: draw the game world (letterboxed) then HUD and music button
                window.setView(gameView);

                for (auto &s : shields) s.draw(window);
                formation->draw(window);
                for (auto &b : bullets) if (b.isActive()) b.draw(window);
                for (auto &b : enemyBullets) if (b.isActive()) b.draw(window);
                player.draw(window);

                // After drawing the world, switch back to default view to draw HUD and menus anchored to the window
                window.setView(window.getDefaultView());

                // HUD (score / lives) - position based on music button position (right side of the button)
                // Draw music toggle button (upper-left) first so counters can be placed relative to it
                window.draw(musicBtn);
                if (musicIcon) window.draw(*musicIcon);

                // place counters to the right of the music button, vertically centered to it
                {
                    // music button position/size
                    sf::Vector2f btnPos = musicBtn.getPosition();
                    sf::Vector2f btnSize = musicBtn.getSize();

                    const float paddingX = 12.f; // gap between button and first counter
                    const float spacing = 12.f;  // gap between score and lives
                    const float startX = btnPos.x + btnSize.x + paddingX;
                    const float centerY = btnPos.y + btnSize.y * 0.5f;

                    // Score text: left-aligned at startX, vertically centered to the music button
                    if (scoreText) {
                        sf::FloatRect tb = scoreText->getLocalBounds();
                        scoreText->setOrigin(sf::Vector2f{0.f, tb.position.y + tb.size.y * 0.5f});
                        scoreText->setPosition(sf::Vector2f{startX, centerY});
                        window.draw(*scoreText);
                    }

                    // Lives text: placed to the right of score text, keeping them side-by-side
                    float nextX = startX;
                    if (scoreText) {
                        nextX += scoreText->getGlobalBounds().size.x + spacing;
                    }
                    if (livesText) {
                        sf::FloatRect tb2 = livesText->getLocalBounds();
                        livesText->setOrigin(sf::Vector2f{0.f, tb2.position.y + tb2.size.y * 0.5f});
                        livesText->setPosition(sf::Vector2f{nextX, centerY});
                        window.draw(*livesText);
                    }
                }
            }
        }

        window.display();
    }

    return 0;
}