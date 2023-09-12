/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

Command ObviousAudioProcessor::commandWithID(int commandID) {
    
    std::vector<Command>::iterator it = std::find_if(
        commands.begin(), commands.end(),
        [&commandID](const Command& command) { return command.commandID == commandID;});
    
    return *it;
    
}

//==============================================================================
ObviousAudioProcessor::ObviousAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), parameters(*this, nullptr, juce::Identifier("Obvious"),
                                     {
                           
                           std::make_unique<juce::AudioParameterFloat>(ParameterIDValue, "Value", juce::NormalisableRange<float>(0.0f, 1.0f, 0.000001f), 0.5f),
                           std::make_unique<juce::AudioParameterBool>(ParameterIDTrigger, "Trigger", false),
                           
                           
                       }),
                        clientThread(ObviousThread(*this)),
                        heartbeatThread(HeartbeatThread(*this))
#endif
{
    
    auto settingsStorage = settings();
    
    parameters.addParameterListener(ParameterIDValue, this);
    parameters.addParameterListener(ParameterIDTrigger, this);
    
    
    
    ipLabel.setEditable(true);
    ipLabel.onTextChange = [this] {
        auto settingsStorage = settings();
        settingsStorage.setProperty(ParameterIDIP, ipLabel.getText(), nullptr);
        closeSocket();
        connectSocket();
    };
    
    portLabel.setEditable(true);
    portLabel.onTextChange = [this] {
        auto settingsStorage = settings();
        settingsStorage.setProperty(ParameterIDPort, portLabel.getText(), nullptr);
        closeSocket();
        connectSocket();
    };
    
    sourceLabel.setEditable(true);
    sourceLabel.onTextChange = [this] {
        auto settingsStorage = settings();
        settingsStorage.setProperty(ParameterIDSource, sourceLabel.getText(), nullptr);
    };
    
    sceneLabel.setEditable(true);
    sceneLabel.onTextChange = [this] {
        auto settingsStorage = settings();
        settingsStorage.setProperty(ParameterIDScene, sceneLabel.getText(), nullptr);
    };
    
    filterLabel.setEditable(true);
    filterLabel.onTextChange = [this] {
        auto settingsStorage = settings();
        settingsStorage.setProperty(ParameterIDFilter, filterLabel.getText(), nullptr);
    };
    
    rangeLowerLabel.setEditable(true);
    rangeLowerLabel.onTextChange = [this] {
        auto settingsStorage = settings();
        settingsStorage.setProperty(ParameterIDRangeLower, rangeLowerLabel.getText().getDoubleValue(), nullptr);
    };
    
    rangeUpperLabel.setEditable(true);
    rangeUpperLabel.onTextChange = [this] {
        auto settingsStorage = settings();
        settingsStorage.setProperty(ParameterIDRangeUpper, rangeUpperLabel.getText().getDoubleValue(), nullptr);
    };
    
    sliderAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (parameters, ParameterIDValue, valueSlider));
    buttonAttachment.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (parameters, ParameterIDTrigger, triggerButton));
    
    valueSliderValue.setEditable(true);
    valueSliderValue.onTextChange = [this] {
        auto settingsStorage = settings();
        double rawValue = std::stod(valueSliderValue.getText().toStdString());
        float rangeLower = settingsStorage.getProperty(ParameterIDRangeLower, 0.0f);
        float rangeUpper = settingsStorage.getProperty(ParameterIDRangeUpper, 1.0f);
        float range = rangeUpper-rangeLower;
        double value = (rawValue-rangeLower) / range;
        
        valueSlider.setValue(value);
    };
    
    typeTextTextLabel.setEditable(true);
    typeTextTextLabel.onTextChange = [this] {
        
        auto settingsStorage = settings();
        settingsStorage.setProperty(ParameterIDTypeTextText, typeTextTextLabel.getText(), nullptr);
        send(TypeText, 0.0f);
        
    };
    
    typeTextCursorCharacterLabel.setEditable(true);
    typeTextCursorCharacterLabel.onTextChange = [this] {
        
        auto settingsStorage = settings();
        settingsStorage.setProperty(ParameterIDTypeTextCursorCharacter, typeTextCursorCharacterLabel.getText(), nullptr);
        send(TypeSetCursorCharacter, 0.0f);
        
    };
    
    triggerButton.onStateChange = [this] {
        
        auto settingsStorage = settings();
        int commandID = settingsStorage.getProperty(ParameterIDCommand, CommandDefinitionDefault);
        int category = settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault);
        Command command = commandWithID(commandID);
//        if ((int)settingsStorage.getProperty(ParameterIDCommandCategory) == CommandCategoryTypeText) {
//            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Error", "No command type selected" );
//
//            return;
//        }
        
        /*
         We don't need to do anything if the button is a toggle
         Toggling it causes the parameter to change, and sending
         is therefore handled in parameterChanged()
         */
        if (command.requiresToggleButton || category == CommandCategoryTypeText) {
            return;
        }
        
        float value;
        juce::Button::ButtonState state = triggerButton.getState();
        if (state == juce::Button::buttonDown) {
            buttonPreviouslyDown = true;
            
//            if (command.requiresToggleButton || category == CommandCategoryTypeText) {
//                return;
//            }
            
            value = 1.0f;
        }
        else if (state == juce::Button::buttonOver) {
            if (buttonPreviouslyDown == false) {
                return;
            }
            buttonPreviouslyDown = false;
            
            /*if (command.requiresToggleButton || category == CommandCategoryTypeText) {
                if (triggerButton.getToggleState()) {
                    value = 0.0f;
                }
                else {
                    value = 1.0f;
                }
            }
            else*/ value = 0.0f;
        }
        else {
            return;
        }

//        if (category == CommandCategoryTypeText) {
//            send(TypeSetCursorVisible, value);
//        }
//        else {
            send(ParameterIDTrigger, value);
//        }
    };
    
    commandCategorySelector.addItem("Source", CommandCategorySource);
    commandCategorySelector.addItem("Filter", CommandCategoryFilter);
    commandCategorySelector.addItem("Type text", CommandCategoryTypeText);
    commandCategorySelector.onChange = [this] {
        
        handleCommandCategoryChange();
        
    };
    
    
    commands.push_back({PositionProportionateX, ParameterIDValue, "Position (x, proportionate)", CommandCategorySource, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), false});
    commands.push_back({PositionProportionateY, ParameterIDValue, "Position (y, proportionate)", CommandCategorySource, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), false});
    commands.push_back({ScaleX, ParameterIDValue, "Scale (x)", CommandCategorySource, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), false});
    commands.push_back({ScaleY, ParameterIDValue, "Scale (y)", CommandCategorySource, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), false});
    commands.push_back({MediaRestart, ParameterIDTrigger, "Media restart", CommandCategorySource, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), false});
    commands.push_back({MediaStop, ParameterIDTrigger, "Media stop", CommandCategorySource, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), false});
    commands.push_back({MediaPlay, ParameterIDTrigger, "Media play", CommandCategorySource, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), false});
    commands.push_back({MediaPause, ParameterIDTrigger, "Media pause", CommandCategorySource, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), false});
    commands.push_back({MediaCursor, ParameterIDValue, "Media cursor", CommandCategorySource, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), false});
    commands.push_back({Hue, ParameterIDValue, "Filter hue", CommandCategoryFilter, -180, 180, false});
    commands.push_back({Saturation, ParameterIDValue, "Filter saturation", CommandCategoryFilter, -1, 5, false});
    commands.push_back({RollSpeedH, ParameterIDValue, "Roll speed (x)", CommandCategoryFilter, -500, 500, false});
    commands.push_back({RollSpeedV, ParameterIDValue, "Roll speed (y)", CommandCategoryFilter, -500, 500, false});
    commands.push_back({Opacity, ParameterIDValue, "Opacity", CommandCategoryFilter, 0, 1, false});
    commands.push_back({CropTop, ParameterIDValue, "Crop (top)", CommandCategorySource, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), false});
    commands.push_back({CropBottom, ParameterIDValue, "Crop (bottom)", CommandCategorySource, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), false});
    commands.push_back({CropLeft, ParameterIDValue, "Crop (left)", CommandCategorySource, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), false});
    commands.push_back({CropRight, ParameterIDValue, "Crop (right)", CommandCategorySource, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), false});
    commands.push_back({SetVisible, ParameterIDTrigger, "Visible", CommandCategorySource, std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), true});
    
    commandSelectorTypeText.setTextWhenNoChoicesAvailable("chill we got this");
    
    /*
     TypeText needs slider, TypeSetCursorCharacter needs toggle button
     Add command selector for this category and remember to show/hide it as necessary along with the others
     Only add TypeText and TypeSetCursorCharacter to the command selector (SetTypeNumChars is hidden)
     */
    
    /*
     If I can get populateCommandsList() working then this will go here, instead of the following for_each function
     which populates two separate combo boxes, one for each category
     */
    std::for_each(commands.begin(), commands.end(), [this](Command command){
        
        switch (command.category) {
            case CommandCategorySource:
                commandSelectorSource.addItem(command.displayName, command.commandID);
                break;
                
            case CommandCategoryFilter:
                commandSelectorFilter.addItem(command.displayName, command.commandID);
                break;
                
            case CommandCategoryTypeText:
                commandSelectorTypeText.addItem(command.displayName, command.commandID);
                break;
                
            default:
                break;
        }
        
    });
        
    commandSelectorSource.onChange = [this] {

        handleCommandChange(commandSelectorSource.getSelectedId());

    };
    
    commandSelectorFilter.onChange = [this] {
        handleCommandChange(commandSelectorFilter.getSelectedId());
    };
    
    commandSelectorTypeText.onChange = [this] {
        handleCommandChange(commandSelectorTypeText.getSelectedId());
    };
    
    int commandID = settingsStorage.getProperty(ParameterIDCommand, CommandDefinitionDefault);
//    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Error", std::to_string(commandID) );
//    Command command = commandWithID(commandID);
//    int category = settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault);
    
    handleCommandCategoryChange();
    handleCommandChange(commandID);
    setCommandComboVisibleState();
        
}

void ObviousAudioProcessor::handleCommandCategoryChange() {
    
    if (!ignoreCommandSelectorChanges) {
        
        int categoryID = commandCategorySelector.getSelectedId();
        if (categoryID > 0) {
            auto settingsStorage = settings();
            settingsStorage.setProperty(ParameterIDCommandCategory, categoryID, nullptr);
            ((ObviousAudioProcessorEditor*)getActiveEditor())->layout(EditorLayoutPurposeUIActivity);
//            populateCommandsList();
        }
        
        setButtonTitle();
        setButtonColour();
        setTriggerVisibleState();
        setTriggerButtonToggleState();
        
    }
    
}

void ObviousAudioProcessor::setButtonColour() {
    
    auto settingsStorage = settings();
    int commandID = settingsStorage.getProperty(ParameterIDCommand, CommandDefinitionDefault);
    Command command = commandWithID(commandID);
    int category = settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault);
    
    if (command.requiresToggleButton || category == CommandCategoryTypeText) {
        triggerButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
        triggerButton.setColour(juce::TextButton::buttonColourId, juce::Colours::orange);
    }
    else {
        triggerButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        triggerButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    }
    
}

void ObviousAudioProcessor::setButtonTitle() {
    
    auto settingsStorage = settings();
    
    int category = settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault);
    if (category == CommandCategoryTypeText) {
        triggerButton.setButtonText("Show cursor");
    }
    else {
        int commandID = settingsStorage.getProperty(ParameterIDCommand, CommandDefinitionDefault);
        switch (commandID) {
            case MediaRestart:
                triggerButton.setButtonText("Restart");
                break;
                
            case MediaStop:
                triggerButton.setButtonText("Stop");
                break;
                
            case MediaPlay:
                triggerButton.setButtonText("Play");
                break;
                
            case MediaPause:
                triggerButton.setButtonText("Pause");
                break;
                
            case SetVisible:
                triggerButton.setButtonText("Visible");
                break;
                
            default:
                triggerButton.setButtonText("Send");
                break;
        }
    }
    
}

void ObviousAudioProcessor::setTriggerButtonToggleState() {
    
    auto settingsStorage = settings();
    int category = settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault);
    int commandID = settingsStorage.getProperty(ParameterIDCommand, CommandDefinitionDefault);
    Command selectedCommand = commandWithID(commandID);
    triggerButton.setClickingTogglesState(selectedCommand.requiresToggleButton || category == CommandCategoryTypeText);
    
    if (category == CommandCategoryTypeText) {
        triggerButton.setToggleState(settingsStorage.getProperty(ParameterIDTypeTextCursorVisible, false), juce::dontSendNotification);
    }
    
}

void ObviousAudioProcessor::handleCommandChange(int commandID) {
    
//            if (!ignoreCommandSelectorChanges) {
//
//                ignoreCommandSelectorChanges = true;
                
    

//                int commandID = selector.getSelectedId();

                if (commandID > 0) {

                    auto settingsStorage = settings();
                    
                    settingsStorage.setProperty(ParameterIDCommand, commandID, nullptr);
                    Command selectedCommand = commandWithID(commandID);
                    selectedCommand = commandWithID(commandID);

                    triggerButton.setToggleState(false, juce::dontSendNotification);

                    int category = settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault);

                    if (selectedCommand.triggerParameterID == ParameterIDTrigger || category == CommandCategoryTypeText) {

                        setTriggerButtonToggleState();
//                        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Info", std::to_string(category) );
                    }

                    switch (selectedCommand.category) {
                        case CommandCategorySource:
                            commandSelectorFilter.setSelectedId(0);
                            commandSelectorTypeText.setSelectedId(0);
                            break;

                        case CommandCategoryFilter:
                            commandSelectorSource.setSelectedId(0);
                            commandSelectorTypeText.setSelectedId(0);
                            break;

                        case CommandCategoryTypeText:
                            commandSelectorSource.setSelectedId(0);
                            commandSelectorFilter.setSelectedId(0);
                            break;

                        default:
                            break;
                    }

                    if (!ignoreCommandSelectorChanges) {
                        if (selectedCommand.recommendedRangeLower != std::numeric_limits<int>::max() || selectedCommand.recommendedRangeUpper != std::numeric_limits<int>::max()) {


                            float rangeLower = settingsStorage.getProperty (ParameterIDRangeLower, 0);
                            float rangeUpper = settingsStorage.getProperty (ParameterIDRangeUpper, 1);

                            if (selectedCommand.recommendedRangeLower != rangeLower || selectedCommand.recommendedRangeUpper != rangeUpper) {

                                std::string rangeWarning = "The recommended range for this command is " + std::to_string(selectedCommand.recommendedRangeLower) + " to " + std::to_string(selectedCommand.recommendedRangeUpper) + " (current range: " + std::to_string(rangeLower) + " to " + std::to_string(rangeUpper) + ")";
                                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Info", rangeWarning );

                            }

                        }
                        setTriggerVisibleState();
                        ObviousAudioProcessorEditor *editor = (ObviousAudioProcessorEditor*)getActiveEditor();
                        if (editor != nullptr) {
                            editor->layout(EditorLayoutPurposeUIActivity);
                        }
                        setButtonTitle();
                        setButtonColour();
//                        ((ObviousAudioProcessorEditor*)getActiveEditor())->layout(EditorLayoutPurposeUIActivity);
                    }
                    
                }
                
//                ignoreCommandSelectorChanges = false;
//
//            }
    
}

/*
 
 NOT WORKING. :(
 
 I had hoped to use the selected command category to determine which items should be in the command selector combo box.
 Unfortunately the command selector will not switch to the selected command on startup when this is the case.
 This function is unused, and there are instead two combo boxes, one for each command category.
 If I can get the below working then I can switch back to a single combo and reload its contents each time the category is changed.
 
 */
#warning fix?
void ObviousAudioProcessor::populateCommandsList() {
    
    auto settingsStorage = settings();
    int category = settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault);
    
    if (category == 0) {
        return;
    }
    
    commandSelectorSource.clear();

    std::for_each(commands.begin(), commands.end(), [this, &category](Command command){
        if (command.category == category) {
            commandSelectorSource.addItem(command.displayName, command.commandID);
        }
    });
        
}

void ObviousAudioProcessor::setTriggerVisibleState() {
    
    auto settingsStorage = settings();
    int category = settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault);
    int commandID = settingsStorage.getProperty(ParameterIDCommand, CommandDefinitionDefault);
    Command selectedCommand = commandWithID(commandID);
    
    valueSlider.setVisible(selectedCommand.triggerParameterID == ParameterIDValue || category == CommandCategoryTypeText);
    triggerButton.setVisible(selectedCommand.triggerParameterID == ParameterIDTrigger || category == CommandCategoryTypeText);
    
}

void ObviousAudioProcessor::setCommandComboVisibleState() {
    
    auto settingsStorage = settings();
    int category = settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault);
    commandSelectorSource.setVisible(category == CommandCategorySource);
    commandSelectorFilter.setVisible(category == CommandCategoryFilter);
    commandSelectorTypeText.setVisible(category == CommandCategoryTypeText);
    
}

juce::ValueTree ObviousAudioProcessor::settings() {
    return parameters.state.getOrCreateChildWithName (ParameterIDSettingsStorage, nullptr);
}

ObviousAudioProcessor::~ObviousAudioProcessor()
{
    
    closeSocket();
    
}

float ObviousAudioProcessor::translateSliderValue(float rawValue) {
    auto settingsStorage = settings();
    float rangeLower = settingsStorage.getProperty (ParameterIDRangeLower, 0.0f);
    float rangeUpper = settingsStorage.getProperty (ParameterIDRangeUpper, 1.0f);
    float value = ((rangeUpper-rangeLower)*rawValue)+rangeLower;
    return value;
}

float ObviousAudioProcessor::translateSliderValueAndDisplay(float rawValue) {
    float translated = translateSliderValue(rawValue);
    valueSliderValue.setText(juce::String(translated, 6), juce::dontSendNotification);
    return translated;
}

void ObviousAudioProcessor::parameterChanged(const juce::String &parameterID, float newValue) {
    
    if (parameterID == ParameterIDTrigger) {
        juce::ValueTree settingsStorage = settings();
        int category = settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault);
        if (category == CommandCategoryTypeText) {
            settingsStorage.setProperty(ParameterIDTypeTextCursorVisible, (newValue == 1), nullptr);
        }
        
    }
    
    if (sendOnParameterChange) {
//            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Error", juce::String(newValue) );
        send(parameterID, newValue);
        
    }
    
}

void ObviousAudioProcessor::send(const juce::String &parameterID, float value) {
    
    if (!sendEnabled) {
        return;
    }
        
    auto settingsStorage = settings();
    int commandID = settingsStorage.getProperty(ParameterIDCommand, CommandDefinitionDefault);
    int category = settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault);
    Command selectedCommand = commandWithID(commandID);
    
    if (category == CommandCategoryTypeText) {
        
        if (parameterID == ParameterIDValue) commandID = TypeSetNumChars;
        else if (parameterID == ParameterIDTrigger) commandID = TypeSetCursorVisible;

    }
    else if ((commandID == 0 || selectedCommand.category != (int)settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault))) {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Error", "No command type selected" );
        return;
    }
    
    
    
    if (parameterID != selectedCommand.triggerParameterID && category != CommandCategoryTypeText) {
        return;
    }
    
    if (parameterID == ParameterIDValue) {
        value = translateSliderValueAndDisplay(value);
    }
    
    send(commandID, value);
    
}

void ObviousAudioProcessor::send(int command, float value) {
    
    if (!sendEnabled) {
        return;
    }
        
    auto settingsStorage = settings();
    
    juce::String scene = settingsStorage.getProperty (ParameterIDScene, juce::String());
    if (scene.length() < 1) {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Error", "No scene specified" );
        return;
    }
    
    juce::String source = settingsStorage.getProperty (ParameterIDSource, juce::String());
    if (source.length() < 1) {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Error", "No source specified" );
        return;
    }
    
    std::string sendString = std::to_string(command) + CommandChunkDelimiter + scene.toStdString() + CommandChunkDelimiter + source.toStdString() + CommandChunkDelimiter;
    
    int category = settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault);
    
    if (category == CommandCategoryFilter) {
        
        juce::String filter = settingsStorage.getProperty (ParameterIDFilter, juce::String());
        if (filter.length() < 1) {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Error", "No filter specified" );
            return;
        }
        
        sendString = sendString + filter.toStdString() + CommandChunkDelimiter + std::to_string(value);
        
    }
    
    else if (category == CommandCategoryTypeText && command != TypeSetNumChars && command != TypeSetCursorVisible) {
        
        if (command == TypeText) {
            sendString = sendString + settingsStorage.getProperty(ParameterIDTypeTextText, juce::String()).toString().toStdString();
        }
        else if (command == TypeSetCursorCharacter) {
            sendString = sendString + settingsStorage.getProperty(ParameterIDTypeTextCursorCharacter, juce::String()).toString().toStdString();
        }
        
    }
    else {
        sendString = sendString + std::to_string(value);
    }
    
    sendString = sendString + CommandDelimiter;
        
    if (!socket.isConnected() || forceConnect) {
        connectSocket();
    }
    
//    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Send", sendString );
    socket.write(sendString.c_str(), static_cast<int>(sendString.length()));
    
}

void ObviousAudioProcessor::connectSocket() {
    
//    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Info", "Connecting" );

    
    auto settingsStorage = settings();
    
    juce::String ip = settingsStorage.getProperty (ParameterIDIP, juce::String("127.0.0.1"));
    int port = settingsStorage.getProperty (ParameterIDPort, juce::String("11111"));
    
    socket.connect(ip, port);
    
    clientThread.shouldRun = true;
    clientThread.startThread();
    
    heartbeatThread.shouldRun = true;
    heartbeatThread.startThread();
    
    forceConnect = false;
    
}

void ObviousAudioProcessor::closeSocket() {
    
    clientThread.shouldRun = false;
    clientThread.stopThread(0);
    
    heartbeatThread.shouldRun = false;
    heartbeatThread.stopThread(0);
    
    if (socket.isConnected()) {
        
        forceConnect = true;
        
//        if (forceConnect) juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "yes", "yes" );
//        else juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "NO", "NO" );
        
        std::string s = std::string(CommandDisconnect) + CommandDelimiter;
        const char * ss = s.c_str();
        socket.write(ss, (int)strlen(ss));
        socket.close();
        
//        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "close", "close" );
    }
    
    
    
    
    
}

void ObviousAudioProcessor::clientMessageReceived(std::string message) {
    
    std::stringstream stringStream(message);
    std::string segment;
    std::vector<std::string> seglist;

    while(std::getline(stringStream, segment, CommandChunkDelimiter))
    {
       seglist.push_back(segment);
    }
    
    if (seglist.size() == 2) {
        
        int responseCode = std::stoi(seglist[0]);
        if (responseCode == HeartbeatResponse) {
            hasReceivedHeartbeatResponse = true;
        }
        else if (responseCode == ResponseCodeRequestTypeText) {
            send(TypeText, 0.0f);
        }
        else if (responseCode == ResponseCodeRequestTypeNumChars) {
            send(ParameterIDValue, valueSlider.getValue());
        }
        else if (responseCode == ResponseCodeRequestTypeCursorChar) {
            send(TypeSetCursorCharacter, 0.0f);
        }
        else if (responseCode == ResponseCodeRequestTypeCursorVisible) {
            float visibleValue = 0.0f;
            juce::ValueTree settingsStorage = settings();
            if (settingsStorage.getProperty(ParameterIDTypeTextCursorVisible, false)) visibleValue = 1.0f;
            send(TypeSetCursorVisible, visibleValue);
        }
        else {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Error", message );
        }
        
    }
    else {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Error", message );
    }
    
}

void ObviousAudioProcessor::heartbeatLost() {
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Info", "OBS has disconnected" );
    closeSocket();
}

//==============================================================================
const juce::String ObviousAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ObviousAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ObviousAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ObviousAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ObviousAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ObviousAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ObviousAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ObviousAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ObviousAudioProcessor::getProgramName (int index)
{
    return {};
}

void ObviousAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ObviousAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void ObviousAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ObviousAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ObviousAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
//    for (int channel = 0; channel < totalNumInputChannels; ++channel)
//    {
//        auto* channelData = buffer.getWritePointer (channel);
//
//        // ..do something to the data...
//    }
  
    

    
    
    
//    if (socket.isConnected()) {
        
////        if (n > 0) {
////            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Info", s );
////        }
//    }
    
}

//void processBlock (AudioBuffer<float>& audio,  MidiBuffer& midi) override { process (audio, midi); }
//void processBlock (AudioBuffer<double>& audio, MidiBuffer& midi) override { process (audio, midi); }

//==============================================================================
bool ObviousAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ObviousAudioProcessor::createEditor()
{
    return new ObviousAudioProcessorEditor (*this, parameters);
}

//==============================================================================
void ObviousAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ObviousAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
     
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ObviousAudioProcessor();
}
