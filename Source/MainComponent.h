#pragma once

#include <JuceHeader.h>
#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

class LoadHotKeyProcessThread;
struct HotKeyUI;
struct HotKeyProcess;

class MainComponent  : public juce::Viewport, juce::Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    void modifierKeysChanged(const juce::ModifierKeys& modifiers) override;

    void timerCallback() override;

    //==============================================================================

private:
    //==============================================================================
    //hot key functions:

    void addHotKeyPath(juce::String path);

    void setHotKeyProcessState(HotKeyUI* hotKeyUI, bool newState);

    bool loadHotKeyProcess(const juce::String* fileName);
    void loadHotKeyProcessAsync(const juce::String fileName);
    void hotKeyProcessLoadedCallback(HotKeyProcess* processLoaded);

    void exitHotKeyProcess(const juce::String fileName);

    void loadStartUpHotKeyProcess(juce::String fileName);
    void saveStartupHotKeys();

    juce::File* getHotKeyFile(const juce::String& fileName);

    //==============================================================================
    //hot key data:

    juce::Array<juce::File*> m_hotKeyFiles;
    juce::Array<HotKeyProcess*> m_hotKeyProcesses;

    juce::String m_startupHotKeyPath;

    LoadHotKeyProcessThread* m_loadProcessThread;

    //==============================================================================
    //selecting feature members:

    void selectHotKey(int index);
    void deselectAllHotKeys();

    juce::String currentKeyStrokes;
    int selectedHotKeyIndex;

    //==============================================================================
    //UI members:

    HotKeyUI* getHotKeyUI(juce::String fileName);
    void addHotKeyUIToGroup(HotKeyUI* newHotKeyUI);

    juce::Component m_viewportContent;

    juce::Array<HotKeyUI*> m_hotKeyUI;
    juce::Array<juce::Array<HotKeyUI*>> m_hotKeyUIGroups;
    juce::Array<juce::TextEditor*> m_hotKeyGroupHeaders;

    const int UI_HEIGHT = 30;
    const int NAME_WIDTH = 300;
    const int BUTTON_WIDTH = 100;

    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

class LoadHotKeyProcessThread : public juce::Thread
{
public:
    LoadHotKeyProcessThread(HotKeyProcess* newProcess);

    void run() override;

    HotKeyProcess* loadProcess;
    std::function<void()> resultCallback;
};

struct HotKeyProcess
{
    juce::String fileName;
    juce::String fullFilePath;
    STARTUPINFO* startupInfo = nullptr;
    PROCESS_INFORMATION* processInfo = nullptr;
    bool loadSucceeded = false;
};
struct HotKeyUI
{
    HotKeyUI() : currentLoadStatusColor(defaultColor) {}

    void processLoaded(bool succeeded, bool repaint);
    void processExited(bool repaint);
    void applyStatusColor(bool repaint);

    juce::String fileName;
    juce::String parentFolderName;

    juce::TextEditor displayName;
    juce::ToggleButton processToggle{ "Active" };
    juce::ToggleButton startupTogggle{ "Is Startup" };

    juce::Colour currentLoadStatusColor;
    inline static const int textEditorColourId = juce::TextEditor::ColourIds::backgroundColourId;
    inline static const juce::Colour defaultColor = juce::Colour(38, 50, 56);
};