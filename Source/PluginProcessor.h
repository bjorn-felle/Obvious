/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CommandDefinitions.h"
#include "ParameterDefinitions.h"

//==============================================================================
/**
*/
class ObviousAudioProcessor  : public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    ObviousAudioProcessor();
    ~ObviousAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    juce::AudioProcessorValueTreeState parameters;
    
    juce::Slider valueSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
    juce::Label valueSliderValue;
    
    juce::Label targetLabel;
    juce::Label sceneTitleLabel;
    juce::Label sourceTitleLabel;
    juce::Label sceneLabel;
    juce::Label sourceLabel;
    juce::Label filterLabel;
    juce::Label filterTitleLabel;
    
    juce::Label commandSelectorTitle;
    juce::Label commandCategorySelectorTitle;
    juce::ComboBox commandCategorySelector;
    juce::ComboBox commandSelectorSource;
    juce::ComboBox commandSelectorFilter;
    juce::ComboBox commandSelectorTypeText;
    
    juce::Label connectionLabel;
    juce::Label ipTitleLabel;
    juce::Label portTitleLabel;
    juce::Label ipLabel;
    juce::Label portLabel;
    
    juce::Label commandLabel;
    
    juce::Label rangeLabel;
    juce::Label rangeLowerTitleLabel;
    juce::Label rangeUpperTitleLabel;
    juce::Label rangeLowerLabel;
    juce::Label rangeUpperLabel;
    
    juce::Label typeTextTitleLabel;
    juce::Label typeTextTextLabel;
    juce::Label typeTextCursorCharacterTitleLabel;
    juce::Label typeTextCursorCharacterLabel;
    
    
    juce::TextButton triggerButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachment;
    bool buttonPreviouslyDown = false;
    
//    Command selectedCommand;
    
    float translateSliderValueAndDisplay(float rawValue);
    float translateSliderValue(float rawValue);
    
    bool ignoreCommandSelectorChanges = false;
    
    void setCommandComboVisibleState();
    
    bool sendEnabled = false;
    bool sendOnParameterChange = true;
    bool typeTextHasSentText = false;
    
    Command commandWithID(int commandID);
    
    void clientMessageReceived(std::string message);
    
    void setButtonTitle();
    void setButtonColour();
    void setTriggerVisibleState();
    void setTriggerButtonToggleState();
    
    void connectSocket();
    void closeSocket();
    
    bool hasReceivedHeartbeatResponse = false;
    void heartbeatLost();
    
    juce::ValueTree settings();
            
private:
    
    typedef enum : int {
        ResponseCodeError=10,
        ResponseCodeRequestTypeText=20,
        ResponseCodeRequestTypeCursorChar=30,
        ResponseCodeRequestTypeCursorVisible=40,
        ResponseCodeRequestTypeNumChars=50,
    } ResponseCode;

    #define CommandDelimiter char(30)
    #define CommandChunkDelimiter char(31)
    
    void send(const juce::String &parameterID, float value);
    void send(int command, float value);
    juce::StreamingSocket socket = juce::StreamingSocket();
    
    std::vector<Command> commands;
    
    
    
    
    void parameterChanged(const juce::String &parameterID, float newValue) override;
    
    void populateCommandsList();
    void handleCommandChange(int commandID);
    void handleCommandCategoryChange();
    
    bool forceConnect = false;
    
//    void sendTextType(float numChars);
    
    class ObviousThread : public juce::Thread
        {
        public:
            ObviousThread(ObviousAudioProcessor &p) : juce::Thread ("ObviousThread"), audioProcessor(p) {}
            
            ObviousAudioProcessor& audioProcessor;
            bool shouldRun = true;
            
            void run() override
            {
                while (shouldRun) {
                                            
                    if (audioProcessor.socket.isConnected()) {
                        
                        char buffer[1024];
                        int n = audioProcessor.socket.read(buffer, 1024, false);
                        if (n > 0) {
                            std::string messages = std::string(buffer).substr(0, n);
                            
                            std::stringstream stringStream(messages);
                            std::string segment;
                            std::vector<std::string> seglist;

                            while(std::getline(stringStream, segment, CommandDelimiter))
                            {
                               seglist.push_back(segment);
                            }
                            
                            std::for_each(seglist.begin(), seglist.end(), [this](std::string message){
                                audioProcessor.clientMessageReceived(message);
                            });
                            
                            // audioProcessor.clientMessageReceived(s);
                        }
                        
                    }
                                            
                    sleep(10);
                }
            }
        };

    ObviousThread clientThread;
    
    class HeartbeatThread : public juce::Thread
        {
        public:
            HeartbeatThread(ObviousAudioProcessor &p) : juce::Thread ("HeartbeatThread"), audioProcessor(p) {}
            
            ObviousAudioProcessor& audioProcessor;
            bool shouldRun = true;
            
            void run() override
            {
                while (shouldRun) {
                                            
                    if (audioProcessor.socket.isConnected()) {
                        
                        audioProcessor.hasReceivedHeartbeatResponse = false;
                        
                        std::string s = std::string(CommandHeartbeat) + CommandDelimiter;
                        const char * ss = s.c_str();
                        audioProcessor.socket.write(ss, (int)strlen(ss));
                        
//                        audioProcessor.socket.write(CommandHeartbeat, strlen(CommandHeartbeat));
                        sleep(1000);
                        
                        if (!audioProcessor.hasReceivedHeartbeatResponse) {
                            audioProcessor.heartbeatLost();
                            shouldRun = false;
                        }
                        
                    }
                                            
//                    sleep(1000);
                }
            }
        };
    
    HeartbeatThread heartbeatThread;
    
        
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ObviousAudioProcessor)
};
