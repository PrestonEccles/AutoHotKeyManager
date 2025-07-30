#pragma once

#include <JuceHeader.h>

#define NAME_OF(name) (#name)

typedef juce::String jString;

inline juce::Font getMonoFont(float fontHeight = 14.f) { return juce::Font("Cascadia Mono", fontHeight, 0); }
inline const juce::Colour g_defaultEditorColor = juce::Colour(38, 50, 56);

typedef juce::Rectangle<int> Bounds;
//return the expanded area
inline Bounds expandBottomOfBounds(Bounds& bounds, int expand)
{
	bounds = bounds.withTrimmedBottom(-expand);
	return bounds.withTop(bounds.getBottom() - expand);
}

template<class ButtonType>
void fitButtonInLeftBounds(Bounds& bounds, ButtonType& button)
{
	button.setSize(0, bounds.getHeight());
	button.changeWidthToFitText();
	button.setBounds(bounds.removeFromLeft(button.getWidth()));
}
template<class ButtonType>
void fitButtonInRightBounds(Bounds& bounds, ButtonType& button)
{
	button.setSize(0, bounds.getHeight());
	button.changeWidthToFitText();
	button.setBounds(bounds.removeFromRight(button.getWidth()));
}

inline juce::String getValueTreeID(juce::ValueTree& valueTree) { return valueTree.getType().toString(); }
inline juce::XmlElement::TextFormat getXmlNoWrapFormat()
{
	juce::XmlElement::TextFormat format;
	format.lineWrapLength = 0x7FFFFFFF;
	return format;
}

inline juce::String g_savePath;
inline juce::String g_debugPath;

class DebugLog
{
public:
	inline static void log(juce::String log)
	{
		logFile = g_debugPath + "\\Log.txt";
		if (!logFile.exists())
			return;

		auto newLog = logFile.loadFileAsString();
		if (newLog.isNotEmpty())
			newLog += "\n";
		newLog += log;
		logFile.replaceWithText(newLog);
	}

private:
	inline static juce::File logFile;
};

struct ModalCallback : public juce::ModalComponentManager::Callback
{
	ModalCallback() {}
	ModalCallback(std::function<void(int returnValue)> callback) : callback(callback) {}
	void modalStateFinished(int returnValue) override { if (callback) callback(returnValue); }
	std::function<void(int returnValue)> callback = nullptr;
	JUCE_LEAK_DETECTOR(ModalCallback)
};