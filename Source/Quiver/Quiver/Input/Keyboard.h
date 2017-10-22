#pragma once

#include "KeyboardKey.h"

namespace qvr {

class Keyboard
{
public:
	// TODO: Write our own key enum. Ugh. So dull.
	virtual bool IsDown  (const KeyboardKey key) const = 0;
	virtual bool JustDown(const KeyboardKey key) const = 0;
	virtual bool JustUp  (const KeyboardKey key) const = 0;
};

}
