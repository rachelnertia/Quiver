#include "AudioComponentEditor.h"

#include <ImGui/imgui.h>

#include "Quiver/Entity/AudioComponent/AudioComponent.h"
#include "Quiver/Misc/ImGuiHelpers.h"

namespace qvr
{

struct AudioComponentEditorData
{
	char m_FilenameBuffer[64] = {};
	float m_AttenuationSliderMax = 10;
	float m_MinDistanceSliderMax = 100;
};

AudioComponentEditor::AudioComponentEditor(AudioComponent & audioComponent)
	: m_AudioComponent(audioComponent)
	, m_Data(std::make_unique<AudioComponentEditorData>())
{
}

AudioComponentEditor::~AudioComponentEditor() {}

void AudioComponentEditor::GuiControls()
{
	ImGui::InputText(
		"Sound Buffer Filename",
		m_Data->m_FilenameBuffer);

	if (ImGui::Button("Load Sound Buffer"))
	{
		m_AudioComponent.SetSound(m_Data->m_FilenameBuffer);
	}

	if (m_AudioComponent.m_Sound.getBuffer())
	{
		if (ImGui::Button("Remove Sound"))
		{
			m_AudioComponent.StopSound();
		}

		bool loop = m_AudioComponent.m_Sound.getLoop();
		if (ImGui::Checkbox("Loop", &loop))
		{
			m_AudioComponent.m_Sound.setLoop(loop);
		}

		float attenuation = m_AudioComponent.m_Sound.getAttenuation();
		if (ImGui::SliderFloat("Attenuation", &attenuation, 0.0f, m_Data->m_AttenuationSliderMax))
		{
			m_AudioComponent.m_Sound.setAttenuation(attenuation);
		}

		ImGui::SliderFloat("Attenuation Slider Max", &m_Data->m_AttenuationSliderMax, 1.0f, 100.0f);

		float minDistance = m_AudioComponent.m_Sound.getMinDistance();
		if (ImGui::SliderFloat("Min Distance", &minDistance, 0.1f, m_Data->m_MinDistanceSliderMax))
		{
			m_AudioComponent.m_Sound.setMinDistance(minDistance);
		}

		ImGui::SliderFloat("Min Distance Slider Max", &m_Data->m_MinDistanceSliderMax, 1.0f, 100.0f);
	}
}

}