#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <optional>

class Menu {
public:
    Menu(const sf::Font* font, unsigned int charSize = 36);

    void setOptions(const std::vector<std::string>& options,
                    const sf::Vector2f& position,
                    float spacing);

    void processEvent(const sf::Event& ev, const sf::RenderWindow& window);

    void update(float dt);

    void draw(sf::RenderWindow& window) const;

    int getSelectedIndex() const { return selectedIndex_; }

    bool consumeConfirm();

    void reset();

private:
    const sf::Font* font_;
    unsigned int charSize_;


    std::vector<sf::Text> texts_;
    std::vector<sf::RectangleShape> backRects_;
    std::vector<std::string> options_;

    sf::Vector2f position_;
    float spacing_ = 48.f;

    int selectedIndex_ = 0;
    bool confirmRequested_ = false;

    sf::Color bgColor_{ 40, 40, 48, 200 };
    sf::Color normalColor_{ 220, 220, 220 };
    sf::Color selectedTextColor_{ 24, 24, 24 };
    sf::Color selectedBgColor_{ 240, 200, 60 };

    sf::FloatRect itemGlobalBounds(size_t i) const;
    bool pointInRect(const sf::Vector2f& p, const sf::FloatRect& r) const;
};