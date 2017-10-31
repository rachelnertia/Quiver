#pragma once

#include <memory>

namespace qvr
{

class AudioComponent;
struct AudioComponentEditorData;

class AudioComponentEditor
{
public:
	AudioComponentEditor(AudioComponent& audioComponent);
	~AudioComponentEditor();
	void GuiControls();
	bool IsTargeting(const AudioComponent& audioComponent) const
	{
		return &audioComponent == &m_AudioComponent;
	}
private:
	AudioComponent& m_AudioComponent;
	std::unique_ptr<AudioComponentEditorData> m_Data;
};

}