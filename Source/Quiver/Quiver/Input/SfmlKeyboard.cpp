#include "SfmlKeyboard.h"

#include <SFML/Window/Keyboard.hpp>

namespace qvr {

sf::Keyboard::Key ToKey(const int i) {
	return (sf::Keyboard::Key)i;
}

void SfmlKeyboard::Update() {
	for (Key& key : m_Keys) {
		key.wasDown = key.isDown;
	}

	for (int i = 0; i < (int)m_Keys.size(); ++i) {
		m_Keys[i].isDown = sf::Keyboard::isKeyPressed(ToKey(i));
	}
}

}