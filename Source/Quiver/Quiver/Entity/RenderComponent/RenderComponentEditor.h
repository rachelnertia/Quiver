#pragma once

#include <memory>
#include <string>

namespace qvr
{

class RenderComponent;

class RenderComponentEditor
{
public:
	RenderComponentEditor(RenderComponent& renderComponent);
	~RenderComponentEditor();
	void GuiControls();
	bool IsTargeting(const RenderComponent& renderComponent) const
	{
		return &renderComponent == &m_RenderComponent;
	}
private:
	RenderComponent& m_RenderComponent;
	std::string m_TextureFilename;
};

}