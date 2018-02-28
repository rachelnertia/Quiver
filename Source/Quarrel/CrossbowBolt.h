#pragma once

#include <Quiver/Entity/CustomComponent/CustomComponent.h>

class CrossbowBolt : public qvr::CustomComponent
{
public:
	CrossbowBolt(qvr::Entity& entity);

	void OnBeginContact(qvr::Entity& other, b2Fixture& myFixture) override;

	std::string GetTypeName() const override { return "CrossbowBolt"; }
};