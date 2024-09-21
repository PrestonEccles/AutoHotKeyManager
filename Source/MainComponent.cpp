#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : m_loadProcessThread(nullptr), selectedHotKeyIndex(-1)
{
    juce::String exePath = juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentExecutableFile).getFullPathName();
    juce::String hotKeyScriptsDirectory = exePath.substring(0, exePath.indexOf("AutoHotKey") + 10);
    addHotKeyPath(hotKeyScriptsDirectory);
    addHotKeyPath(hotKeyScriptsDirectory + "\\Mpq");
    addHotKeyPath(hotKeyScriptsDirectory + "\\Reaper");
    addHotKeyPath(hotKeyScriptsDirectory + "\\Programming");

    for (int i = 0; i < m_hotKeyFiles.size(); i++)
    {
        juce::String fileName = m_hotKeyFiles[i]->getFileName();

        juce::String folderName = m_hotKeyFiles[i]->getParentDirectory().getFileName();
        juce::String displayName = folderName != "AutoHotKey" ? folderName + '_' + fileName : fileName;

        HotKeyUI* newHotKey = new HotKeyUI();
        m_hotKeyUI.add(newHotKey);

        newHotKey->fileName = fileName;
        newHotKey->parentFolderName = m_hotKeyFiles[i]->getParentDirectory().getFileName();
        
        newHotKey->displayName.setText(displayName);
        newHotKey->displayName.setReadOnly(true);
        m_viewComponent.addAndMakeVisible(newHotKey->displayName);

        newHotKey->processToggle.setButtonText("Active");
        newHotKey->processToggle.setName(displayName);
        newHotKey->processToggle.onClick = [=]() 
        {
            setHotKeyProcessState(newHotKey, newHotKey->processToggle.getToggleState());
        };
        m_viewComponent.addAndMakeVisible(newHotKey->processToggle);

        newHotKey->startupTogggle.setButtonText("Is Startup");
        newHotKey->startupTogggle.onClick = [this]() { saveStartupHotKeys(); };
        m_viewComponent.addAndMakeVisible(newHotKey->startupTogggle);
    }

    //scripts to run on start
    juce::File::SpecialLocationType exePathType = juce::File::SpecialLocationType::currentExecutableFile;
    juce::String exeFolderPath = juce::File::getSpecialLocation(exePathType).getParentDirectory().getFullPathName();
    m_startupHotKeyPath = exeFolderPath + "\\SaveData\\StartupHotKeys.txt";

    juce::File hokKeyStartData(m_startupHotKeyPath);
    if (hokKeyStartData.exists())
    {
        juce::FileInputStream input(hokKeyStartData);
        if (input.openedOk())
        {
            while (!input.isExhausted())
            {
                loadStartUpHotKeyProcess(input.readString());
            }
        }
    }

    int parentHeight = m_hotKeyFiles.size() * UI_HEIGHT;
    const int MAX_PARENT_HEIGHT = 785;
    parentHeight = parentHeight > MAX_PARENT_HEIGHT ? MAX_PARENT_HEIGHT : parentHeight;

    this->setScrollBarsShown(false, false, true, false);
    setSize(NAME_WIDTH + BUTTON_WIDTH * 2, parentHeight);

    addAndMakeVisible(&m_viewComponent);
    setViewedComponent(&m_viewComponent);
}

MainComponent::~MainComponent()
{
    for (int i = 0; i < m_hotKeyUI.size(); i++)
    {
        delete m_hotKeyUI[i];
    }

    for (int i = 0; i < m_hotKeyFiles.size(); i++)
    {
        delete m_hotKeyFiles[i];
    }

    for (int i = 0; i < m_hotKeyProcesses.size(); i++)
    {
        TerminateProcess(m_hotKeyProcesses[i]->processInfo->hProcess, 1);
        delete m_hotKeyProcesses[i];
    }

    delete m_loadProcessThread;
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    if (!hasKeyboardFocus(true))
        grabKeyboardFocus();
}

void MainComponent::resized()
{
    for (int i = 0; i < m_hotKeyUI.size(); i++)
    {
        m_hotKeyUI[i]->displayName.setSize(NAME_WIDTH, UI_HEIGHT);
        m_hotKeyUI[i]->displayName.setTopLeftPosition(0, UI_HEIGHT * i);

        m_hotKeyUI[i]->processToggle.setSize(BUTTON_WIDTH, UI_HEIGHT);
        m_hotKeyUI[i]->processToggle.setTopLeftPosition(NAME_WIDTH, UI_HEIGHT * i);

        m_hotKeyUI[i]->startupTogggle.setSize(BUTTON_WIDTH, UI_HEIGHT);
        m_hotKeyUI[i]->startupTogggle.setTopLeftPosition(NAME_WIDTH + BUTTON_WIDTH, UI_HEIGHT * i);
    }
    m_viewComponent.setSize(getWidth(), getHeight());
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    if (key.getKeyCode() >= 'A' && key.getKeyCode() <= 'Z')
    {
        currentKeyStrokes += (char) key.getKeyCode();

        for (int i = 0; i < m_hotKeyUI.size(); i++)
        {
            if (m_hotKeyUI[i]->fileName.substring(0, currentKeyStrokes.length()).equalsIgnoreCase(currentKeyStrokes))
            {
                selectHotKey(i);
                return true;
            }
        }
        for (int i = 0; i < m_hotKeyUI.size(); i++)
        {
            if (m_hotKeyUI[i]->fileName.indexOfIgnoreCase(currentKeyStrokes) != -1)
            {
                selectHotKey(i);
                return true;
            }
        }

        deselectAllHotKeys();
        return false;
    }
    if (key.getKeyCode() == key.returnKey)
    {
        currentKeyStrokes = "";

        if (selectedHotKeyIndex == -1)
            return false;

        for (auto hotKey : m_hotKeyUI)
        {
            hotKey->applyStatusColor(true);
        }

        setHotKeyProcessState(m_hotKeyUI[selectedHotKeyIndex], !m_hotKeyUI[selectedHotKeyIndex]->processToggle.getToggleState());
        selectedHotKeyIndex = -1;

        return true;
    }
    if (key.getKeyCode() == key.backspaceKey)
    {
        deselectAllHotKeys();
        return true;
    }

    return false;
}

void MainComponent::timerCallback()
{
    stopTimer();
    repaint();
}

void MainComponent::addHotKeyPath(juce::String path)
{
    for (juce::DirectoryEntry entry : juce::RangedDirectoryIterator(juce::File(path), false))
    {
        juce::String fileName = entry.getFile().getFileName();
        //find hotkey executables
        if (fileName.contains(".exe"))
        {
            m_hotKeyFiles.add(new juce::File(entry.getFile().getFullPathName()));
        }
    }
}

void MainComponent::setHotKeyProcessState(HotKeyUI* hotKeyUI, bool newState)
{
    hotKeyUI->processToggle.setToggleState(newState, juce::NotificationType::dontSendNotification);
    
    if (newState)
    {
        loadHotKeyProcessAsync(hotKeyUI->fileName);
    }
    else
    {
        exitHotKeyProcess(hotKeyUI->fileName);

        hotKeyUI->processExited(true);
    }
}
void MainComponent::loadStartUpHotKeyProcess(juce::String fileName)
{
    HotKeyUI* hotKeyUI = getHotKeyUI(fileName);
    if (hotKeyUI == nullptr)
        return;

    hotKeyUI->processToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    hotKeyUI->startupTogggle.setToggleState(true, juce::NotificationType::dontSendNotification);

    hotKeyUI->processLoaded(loadHotKeyProcess(new juce::String(fileName)), false);
}

void MainComponent::saveStartupHotKeys()
{
    juce::File hokKeyStartData(m_startupHotKeyPath);
    hokKeyStartData.deleteFile();
    hokKeyStartData.create();

    juce::FileOutputStream output(hokKeyStartData);
    if (output.failedToOpen())
    {
        DBG("hokKeyStartData failed to open");
        return;
    }

    bool isStartUp;
    for each (HotKeyUI* hotKey in m_hotKeyUI)
    {
        isStartUp = hotKey->startupTogggle.getToggleState();
        if (isStartUp)
        {
            output.writeString(hotKey->fileName);
        }
    }
}


bool MainComponent::loadHotKeyProcess(const juce::String* fileName)
{
    HotKeyProcess* newProcess = new HotKeyProcess();
    m_hotKeyProcesses.add(newProcess);

    newProcess->fileName = *fileName;
    newProcess->startupInfo = new STARTUPINFO();
    newProcess->processInfo = new PROCESS_INFORMATION();

    ZeroMemory(newProcess->startupInfo, sizeof(*newProcess->startupInfo));
    newProcess->startupInfo->cb = sizeof(*newProcess->startupInfo);
    ZeroMemory(newProcess->processInfo, sizeof(*newProcess->processInfo));

    return CreateProcessA
    (
        getHotKeyFile(*fileName)->getFullPathName().toRawUTF8(),
        NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        newProcess->startupInfo,
        newProcess->processInfo
    );
}

void MainComponent::loadHotKeyProcessAsync(const juce::String fileName)
{
    HotKeyProcess* newProcess = new HotKeyProcess();
    m_hotKeyProcesses.add(newProcess);

    newProcess->fileName = fileName;
    newProcess->fullFilePath = getHotKeyFile(fileName)->getFullPathName();


    if (m_loadProcessThread != nullptr)
        delete m_loadProcessThread;

    m_loadProcessThread = new LoadHotKeyProcessThread(newProcess);
    m_loadProcessThread->resultCallback = [=]() { hotKeyProcessLoadedCallback(newProcess); };
    m_loadProcessThread->startThread();
}

void MainComponent::hotKeyProcessLoadedCallback(HotKeyProcess* processLoaded)
{
    getHotKeyUI(processLoaded->fileName)->processLoaded(processLoaded->loadSucceeded, false);
    startTimer(1);
}

void MainComponent::exitHotKeyProcess(const juce::String fileName)
{
    for (int i = 0; i < m_hotKeyProcesses.size(); i++)
    {
        if (m_hotKeyProcesses[i]->fileName == fileName)
        {
            TerminateProcess(m_hotKeyProcesses[i]->processInfo->hProcess, 1);
            m_hotKeyProcesses.remove(i);
            break;
        }
    }
}

juce::File* MainComponent::getHotKeyFile(const juce::String& fileName)
{
    for (int i = 0; i < m_hotKeyFiles.size(); i++)
    {
        if (m_hotKeyFiles[i]->getFileName() == fileName)
        {
            return m_hotKeyFiles[i];
        }
    }
    return nullptr;
}

void MainComponent::selectHotKey(int index)
{
    selectedHotKeyIndex = index;

    for (auto hotKey : m_hotKeyUI)
    {
        hotKey->applyStatusColor(false);
    }

    m_hotKeyUI[index]->displayName.setColour(HotKeyUI::textEditorColourId, juce::Colours::lightgrey);

    repaint();
}

void MainComponent::deselectAllHotKeys()
{
    currentKeyStrokes = "";
    selectedHotKeyIndex = -1;

    for (auto hotKey : m_hotKeyUI)
    {
        hotKey->applyStatusColor(true);
    }
}

HotKeyUI* MainComponent::getHotKeyUI(juce::String fileName)
{
    for (int i = 0; i < m_hotKeyUI.size(); i++)
    {
        if (m_hotKeyUI[i]->fileName == fileName)
        {
            return m_hotKeyUI[i];
        }
    }
    return nullptr;
}

LoadHotKeyProcessThread::LoadHotKeyProcessThread(HotKeyProcess* newProcess) 
    : juce::Thread("LoadHotKeyProcess"), loadProcess(newProcess)
{
}

void LoadHotKeyProcessThread::run()
{
    loadProcess->startupInfo = new STARTUPINFO();
    loadProcess->processInfo = new PROCESS_INFORMATION();

    ZeroMemory(loadProcess->startupInfo, sizeof(*loadProcess->startupInfo));
    loadProcess->startupInfo->cb = sizeof(*loadProcess->startupInfo);
    ZeroMemory(loadProcess->processInfo, sizeof(*loadProcess->processInfo));

    loadProcess->loadSucceeded = CreateProcessA
    (
        loadProcess->fullFilePath.toRawUTF8(),
        NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        loadProcess->startupInfo,
        loadProcess->processInfo
    );

    resultCallback();
}

void HotKeyUI::processLoaded(bool succeeded, bool repaint)
{
    currentLoadStatusColor = succeeded ? juce::Colours::green : juce::Colours::red;
    applyStatusColor(repaint);
}

void HotKeyUI::processExited(bool repaint)
{
    currentLoadStatusColor = defaultColor;
    applyStatusColor(repaint);
}

void HotKeyUI::applyStatusColor(bool repaint)
{
    displayName.setColour(textEditorColourId, currentLoadStatusColor);

    if (repaint)
        displayName.repaint();
}
