#include "Menu.h"
#include <algorithm>

Menu::Menu(const sf::Font* font, unsigned int charSize)
: font_(font), charSize_(charSize)
{}

void Menu::setOptions(const std::vector<std::string>& options,
                      const sf::Vector2f& position,
                      float spacing)
{
    options_ = options;
    position_ = position;
    spacing_ = spacing;
    texts_.clear();
    backRects_.clear();

    for (size_t i = 0; i < options.size(); ++i) {
        // background rect (siempre)
        sf::RectangleShape rect;
        rect.setSize({160.f, 40.f});
        rect.setOrigin(rect.getSize() / 2.f);
        rect.setPosition(sf::Vector2f{ position_.x, position_.y + static_cast<float>(i) * spacing_ });
        rect.setFillColor(bgColor_);
        backRects_.push_back(std::move(rect));

        // texto solo si hay fuente disponible
        if (font_) {
            sf::Text t(*font_, options[i], charSize_);
            // Ajuste de origen para centrar el texto (local bounds)
            auto lb = t.getLocalBounds();
            t.setOrigin(sf::Vector2f{ lb.position.x + lb.size.x / 2.f, lb.position.y + lb.size.y / 2.f });
            t.setPosition(sf::Vector2f{ position_.x, position_.y + static_cast<float>(i) * spacing_ });
            t.setFillColor(normalColor_);
            texts_.push_back(std::move(t));
        }
    }

    // inicializar selección
    selectedIndex_ = 0;
    confirmRequested_ = false;
}

void Menu::processEvent(const sf::Event& ev, const sf::RenderWindow& window) {
    // Navegación por teclado
    if (ev.is<sf::Event::KeyPressed>()) {
        const auto* k = ev.getIf<sf::Event::KeyPressed>();
        if (!k) return;
        if (k->code == sf::Keyboard::Key::Up) {
            if (!options_.empty()) {
                selectedIndex_ = std::max(0, selectedIndex_ - 1);
            }
        } else if (k->code == sf::Keyboard::Key::Down) {
            if (!options_.empty()) {
                selectedIndex_ = std::min<int>(static_cast<int>(options_.size()) - 1, selectedIndex_ + 1);
            }
        } else if (k->code == sf::Keyboard::Key::Enter || k->code == sf::Keyboard::Key::Space) {
            confirmRequested_ = true;
        }
    }

    // Interacción con ratón: hover + click
    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
    sf::Vector2f mouseWorld = window.mapPixelToCoords(mousePixel);

    // hover: buscar item que contenga mouse
    for (size_t i = 0; i < backRects_.size(); ++i) {
        sf::FloatRect r = itemGlobalBounds(i);
        if (pointInRect(mouseWorld, r)) {
            selectedIndex_ = static_cast<int>(i);
        }
    }

    // click (left)
    if (ev.is<sf::Event::MouseButtonPressed>()) {
        const auto* mb = ev.getIf<sf::Event::MouseButtonPressed>();
        if (!mb) return;
        if (mb->button == sf::Mouse::Button::Left) {
            if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(backRects_.size())) {
                sf::FloatRect r = itemGlobalBounds(selectedIndex_);
                if (pointInRect(mouseWorld, r)) confirmRequested_ = true;
            }
        }
    }
}

void Menu::update(float /*dt*/) {
    // actualizar colores según selección
    for (size_t i = 0; i < backRects_.size(); ++i) {
        if (static_cast<int>(i) == selectedIndex_) {
            backRects_[i].setFillColor(selectedBgColor_);
            if (font_ && i < texts_.size()) {
                texts_[i].setFillColor(selectedTextColor_);
            }
        } else {
            backRects_[i].setFillColor(bgColor_);
            if (font_ && i < texts_.size()) {
                texts_[i].setFillColor(normalColor_);
            }
        }
    }
}

void Menu::draw(sf::RenderWindow& window) const {
    // Dibuja los elementos (background + texto)
    for (size_t i = 0; i < backRects_.size(); ++i) {
        window.draw(backRects_[i]);
        if (font_ && i < texts_.size()) {
            window.draw(texts_[i]);
        }
    }
}

bool Menu::consumeConfirm() {
    if (confirmRequested_) {
        confirmRequested_ = false;
        return true;
    }
    return false;
}

void Menu::reset() {
    selectedIndex_ = 0;
    confirmRequested_ = false;
}

sf::FloatRect Menu::itemGlobalBounds(size_t i) const {
    if (i < backRects_.size()) {
        return backRects_[i].getGlobalBounds();
    }
    return sf::FloatRect{};
}

bool Menu::pointInRect(const sf::Vector2f& p, const sf::FloatRect& r) const {
    return !(p.x < r.position.x || p.x > r.position.x + r.size.x ||
             p.y < r.position.y || p.y > r.position.y + r.size.y);
}