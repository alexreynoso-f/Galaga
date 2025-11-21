#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <optional>

// Menu simple: dibuja una lista de opciones, permite navegación por teclado y ratón,
// y ofrece una confirmación (consumeConfirm) que el main puede consultar.
class Menu {
public:
    // font puede ser nullptr — en ese caso se dibujan solo rectángulos de fondo (sin texto).
    Menu(const sf::Font* font, unsigned int charSize = 36);

    // Define las opciones y la posición inicial del primer item.
    // spacing = separación vertical entre items en px.
    void setOptions(const std::vector<std::string>& options,
                    const sf::Vector2f& position,
                    float spacing);

    // Procesa eventos (teclado/ratón). Requiere la ventana para mapear coordenadas del mouse.
    void processEvent(const sf::Event& ev, const sf::RenderWindow& window);

    // Update opcional (por ejemplo para animaciones / blink); dt puede ser 0 si no se usa.
    void update(float dt);

    // Dibujar el menú
    void draw(sf::RenderWindow& window) const;

    // Devuelve índice seleccionado actualmente
    int getSelectedIndex() const { return selectedIndex_; }

    // Si el usuario confirmó (Enter / Space / click), consumeConfirm() devuelve true y
    // reinicia la flag para que no vuelva a disparar hasta la próxima confirmación.
    bool consumeConfirm();

    // Resetea selección/estado
    void reset();

private:
    const sf::Font* font_;
    unsigned int charSize_;

    // texts_ solo se usa si font_ != nullptr; otherwise remain empty.
    std::vector<sf::Text> texts_;
    std::vector<sf::RectangleShape> backRects_; // rectángulos de fondo para resaltar
    std::vector<std::string> options_;

    sf::Vector2f position_;
    float spacing_ = 48.f;

    int selectedIndex_ = 0;
    bool confirmRequested_ = false;

    // Colores
    sf::Color bgColor_{ 40, 40, 48, 200 };
    sf::Color normalColor_{ 220, 220, 220 };
    sf::Color selectedTextColor_{ 24, 24, 24 };
    sf::Color selectedBgColor_{ 240, 200, 60 };

    // Helpers
    sf::FloatRect itemGlobalBounds(size_t i) const;
    bool pointInRect(const sf::Vector2f& p, const sf::FloatRect& r) const;
};