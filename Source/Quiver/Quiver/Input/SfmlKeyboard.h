#pragma once

#include <array>

#include "Keyboard.h"

namespace qvr {

inline int ToInt(const KeyboardKey key) {
	return (int)key;
}

class SfmlKeyboard : public Keyboard
{
public:
	void Update();

	bool IsDown(const KeyboardKey key) const override {
		return m_Keys[ToInt(key)].isDown;
	}
	bool JustDown(const KeyboardKey key) const override {
		return m_Keys[ToInt(key)].isDown && !m_Keys[ToInt(key)].wasDown;
	}
	bool JustUp(const KeyboardKey key) const override {
		return m_Keys[ToInt(key)].wasDown && !m_Keys[ToInt(key)].isDown;
	}
private:
	struct Key {
		bool isDown = false, wasDown = false;
	};

	using KeyArray = std::array<Key, (size_t)KeyboardKey::KeyCount>;

	KeyArray m_Keys;
};

}