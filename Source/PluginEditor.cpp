/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterDefinitions.h"

bool allowAutoResize = false;

//==============================================================================
ObviousAudioProcessorEditor::ObviousAudioProcessorEditor (ObviousAudioProcessor& p, juce::AudioProcessorValueTreeState& valueTreeState)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
            
    setResizable(true, true);
    
    addAndMakeVisible(p.ipLabel);
    addAndMakeVisible(p.portLabel);
    addAndMakeVisible(p.sceneLabel);
    addAndMakeVisible(p.sourceLabel);
    addAndMakeVisible(p.valueSlider);
    addAndMakeVisible(p.valueSliderValue);
    addAndMakeVisible(p.commandSelectorSource);
    addAndMakeVisible(p.commandSelectorFilter);
    addAndMakeVisible(p.commandSelectorTypeText);
    addAndMakeVisible(p.commandCategorySelector);
    addAndMakeVisible(p.commandSelectorTitle);
    addAndMakeVisible(p.commandCategorySelectorTitle);
    addAndMakeVisible(p.triggerButton);
    addAndMakeVisible(p.connectionLabel);
    addAndMakeVisible(p.ipTitleLabel);
    addAndMakeVisible(p.portTitleLabel);
    addAndMakeVisible(p.targetLabel);
    addAndMakeVisible(p.sceneTitleLabel);
    addAndMakeVisible(p.sourceTitleLabel);
    addAndMakeVisible(p.filterLabel);
    addAndMakeVisible(p.filterTitleLabel);
    addAndMakeVisible(p.commandLabel);
    addAndMakeVisible(p.rangeLabel);
    addAndMakeVisible(p.rangeLowerTitleLabel);
    addAndMakeVisible(p.rangeUpperTitleLabel);
    addAndMakeVisible(p.rangeLowerLabel);
    addAndMakeVisible(p.rangeUpperLabel);
    addAndMakeVisible(p.typeTextTextLabel);
    addAndMakeVisible(p.typeTextTitleLabel);
    addAndMakeVisible(p.typeTextCursorCharacterLabel);
    addAndMakeVisible(p.typeTextCursorCharacterTitleLabel);
    
    auto settingsStorage = audioProcessor.settings();
    
    p.connectionLabel.setText("Connection", juce::dontSendNotification);
    p.connectionLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
    p.ipTitleLabel.setText("IP Address", juce::dontSendNotification);
    p.portTitleLabel.setText("Port", juce::dontSendNotification);
    p.ipLabel.setText(settingsStorage.getProperty (ParameterIDIP, juce::String("127.0.0.1")), juce::dontSendNotification);
    p.ipLabel.setColour(juce::Label::outlineColourId, juce::Colours::lightgrey);
    p.portLabel.setText(settingsStorage.getProperty (ParameterIDPort, juce::String("11111")), juce::dontSendNotification);
    p.portLabel.setColour(juce::Label::outlineColourId, juce::Colours::lightgrey);
    
    p.targetLabel.setText("Target", juce::dontSendNotification);
    p.targetLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
    p.sceneTitleLabel.setText("Scene", juce::dontSendNotification);
    p.sourceTitleLabel.setText("Source", juce::dontSendNotification);
    p.filterTitleLabel.setText("Filter", juce::dontSendNotification);
    p.sourceLabel.setText(settingsStorage.getProperty (ParameterIDSource, juce::String()), juce::dontSendNotification);
    p.sourceLabel.setColour(juce::Label::outlineColourId, juce::Colours::lightgrey);
    p.sceneLabel.setText(settingsStorage.getProperty (ParameterIDScene, juce::String()), juce::dontSendNotification);
    p.sceneLabel.setColour(juce::Label::outlineColourId, juce::Colours::lightgrey);
    p.filterLabel.setText(settingsStorage.getProperty (ParameterIDFilter, juce::String()), juce::dontSendNotification);
    p.filterLabel.setColour(juce::Label::outlineColourId, juce::Colours::lightgrey);
    
    p.commandLabel.setText("Command", juce::dontSendNotification);
    p.commandLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
    p.commandSelectorTitle.setText("Command", juce::dontSendNotification);
    p.commandCategorySelectorTitle.setText("Category", juce::dontSendNotification);
    p.commandCategorySelector.setSelectedId(settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault));
    p.commandSelectorSource.setSelectedId(settingsStorage.getProperty(ParameterIDCommand, CommandDefinitionDefault));
    p.commandSelectorFilter.setSelectedId(settingsStorage.getProperty(ParameterIDCommand, CommandDefinitionDefault));
    p.commandSelectorTypeText.setSelectedId(settingsStorage.getProperty(ParameterIDCommand, CommandDefinitionDefault));
    
    
    p.valueSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
    p.valueSliderValue.setColour(juce::Label::textColourId, juce::Colours::darkgrey);
    
    p.rangeLabel.setText("Range", juce::dontSendNotification);
    p.rangeLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
    p.rangeLowerTitleLabel.setText("Lower", juce::dontSendNotification);
    p.rangeUpperTitleLabel.setText("Upper", juce::dontSendNotification);
    p.rangeLowerLabel.setColour(juce::Label::outlineColourId, juce::Colours::lightgrey);
    p.rangeUpperLabel.setColour(juce::Label::outlineColourId, juce::Colours::lightgrey);
    p.rangeLowerLabel.setText(settingsStorage.getProperty (ParameterIDRangeLower, juce::String("0.0")), juce::dontSendNotification);
    p.rangeUpperLabel.setText(settingsStorage.getProperty (ParameterIDRangeUpper, juce::String("1.0")), juce::dontSendNotification);
    
    p.typeTextTextLabel.setColour(juce::Label::outlineColourId, juce::Colours::lightgrey);
    p.typeTextTitleLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
    p.typeTextTitleLabel.setText("Text", juce::dontSendNotification);
    p.typeTextCursorCharacterTitleLabel.setText("Cursor character", juce::dontSendNotification);
    p.typeTextCursorCharacterTitleLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
    p.typeTextCursorCharacterLabel.setText(settingsStorage.getProperty(ParameterIDTypeTextCursorCharacter, juce::String()), juce::dontSendNotification);
    p.typeTextCursorCharacterLabel.setColour(juce::Label::outlineColourId, juce::Colours::lightgrey);
    
    p.translateSliderValueAndDisplay(p.valueSlider.getValue());
    p.setCommandComboVisibleState();
    
    p.sendEnabled = true;
//    
    setSize (settingsStorage.getProperty(ParameterIDWindowWidth, 400), settingsStorage.getProperty(ParameterIDWindowHeight, 300));
    
}



/*
 Allow auto resize only after the GUI has been interacted with
 This prevents the window from auto-resizing on startup, which
 causes it to keep growing every time the plugin is activated.
 */
bool ObviousAudioProcessorEditor::hitTest (int x, int y)
{
    bool ret = juce::AudioProcessorEditor::hitTest(x, y);
    if (ret) allowAutoResize = true;
    return ret;
}

ObviousAudioProcessorEditor::~ObviousAudioProcessorEditor()
{
}

//==============================================================================
void ObviousAudioProcessorEditor::paint (juce::Graphics& g)
{
        
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
    if (audioProcessor.valueSlider.isVisible()) {
        
        g.setColour(juce::Colours::lightgrey);
        juce::Rectangle<int> sliderBounds = audioProcessor.valueSlider.getBounds();
        sliderBounds.setWidth(getWidth());
        g.fillRect(sliderBounds);
        
    }

}

int ObviousAudioProcessorEditor::numItems() {
    
    auto settingsStorage = audioProcessor.settings();
    int commandID = settingsStorage.getProperty(ParameterIDCommand, CommandDefinitionDefault);
    int category = settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault);
    
    Command command = audioProcessor.commandWithID(commandID);
    if (category == CommandCategoryTypeText) return 14;
    else if (command.triggerParameterID == ParameterIDValue) return 9;
    else return 7;
}

void ObviousAudioProcessorEditor::resized() {
    
    layout(EditorLayoutPurposeResized);
    
    audioProcessor.setButtonTitle();
    audioProcessor.setButtonColour();
    audioProcessor.setTriggerVisibleState();
    audioProcessor.setTriggerButtonToggleState();
}

int previousNumItems = -1;

void ObviousAudioProcessorEditor::layout(EditorLayoutPurpose purpose)
{
    
    int currentNumItems = numItems();
    
    juce::Rectangle<int> bounds = getBounds();
    int width = bounds.getWidth();
    int height = bounds.getHeight();

    if (purpose == EditorLayoutPurposeUIActivity) {

        if (allowAutoResize && previousNumItems > 0 && currentNumItems != previousNumItems) {

            int numItemsDiff =  currentNumItems - previousNumItems;
            int heightDiff = (height/previousNumItems) * numItemsDiff;
            setSize(width, height+heightDiff);
            return;

        }

    }

    auto settingsStorage = audioProcessor.settings();
    
    settingsStorage.setProperty(ParameterIDWindowWidth, width, nullptr);
    settingsStorage.setProperty(ParameterIDWindowHeight, height, nullptr);
    int halfWidth = width/2;
    int quarterWidth = width/4;
    int threeQuarterWidth = halfWidth+quarterWidth;
    int y = 0;

    previousNumItems = currentNumItems;
    int itemHeight = height/currentNumItems;
    int labelInset = itemHeight/10;
    int labelHeight = itemHeight-(labelInset*2);
    int quarterWidthMinusInset = quarterWidth-(labelInset*2);
    int quarterWidthPlusInset = quarterWidth+labelInset;
    int threeQuarterWidthPlusInset = threeQuarterWidth+labelInset;

    /*
     CONNECTION
     */
    audioProcessor.connectionLabel.setBounds(0, y, width, itemHeight);
    y += itemHeight;

    audioProcessor.ipTitleLabel.setBounds(0, y, quarterWidth, itemHeight);
    audioProcessor.portTitleLabel.setBounds(halfWidth, y, quarterWidth, itemHeight);

    audioProcessor.ipLabel.setBounds(quarterWidthPlusInset, y+labelInset, quarterWidthMinusInset, labelHeight);
    audioProcessor.portLabel.setBounds(threeQuarterWidthPlusInset, y+labelInset, quarterWidthMinusInset, labelHeight);

    y += itemHeight;

    /*
     COMMAND
     */
    audioProcessor.commandLabel.setBounds(0, y, width, itemHeight);
    y+= itemHeight;

    audioProcessor.commandCategorySelectorTitle.setBounds(0, y, quarterWidth, itemHeight);
    audioProcessor.commandCategorySelector.setBounds(quarterWidth, y, quarterWidth, itemHeight);
    audioProcessor.commandSelectorTitle.setBounds(halfWidth, y, quarterWidth, itemHeight);
    juce::Rectangle<int> commandSelectorBounds = juce::Rectangle<int>(threeQuarterWidth, y, quarterWidth, itemHeight);
    audioProcessor.commandSelectorSource.setBounds(commandSelectorBounds);
    audioProcessor.commandSelectorFilter.setBounds(commandSelectorBounds);
    audioProcessor.commandSelectorTypeText.setBounds(commandSelectorBounds);
    audioProcessor.setCommandComboVisibleState();

    y += itemHeight;

    /*
     OBS TARGET
     */
    audioProcessor.targetLabel.setBounds(0, y, width, itemHeight);
    y+= itemHeight;

    int category = settingsStorage.getProperty(ParameterIDCommandCategory, CommandCategoryDefault);

    if (category == CommandCategoryFilter) {

        int thirdWidth = width/3;
        int sixthWidth = width/6;
        int sixthWidthMinusInset = sixthWidth-(labelInset*2);

        audioProcessor.sceneTitleLabel.setBounds(0, y, sixthWidth, itemHeight);
        audioProcessor.sourceTitleLabel.setBounds(thirdWidth, y, sixthWidth, itemHeight);

        audioProcessor.sceneLabel.setBounds(sixthWidth+labelInset, y+labelInset, sixthWidthMinusInset, labelHeight);
        audioProcessor.sourceLabel.setBounds(halfWidth+labelInset, y+labelInset, sixthWidthMinusInset, labelHeight);

        audioProcessor.filterTitleLabel.setBounds(thirdWidth*2, y, sixthWidth, itemHeight);
        audioProcessor.filterLabel.setBounds(sixthWidth*5, y+labelInset, sixthWidthMinusInset, labelHeight);

        audioProcessor.filterLabel.setVisible(true);
        audioProcessor.filterTitleLabel.setVisible(true);
    }
    else {
        audioProcessor.sceneTitleLabel.setBounds(0, y, quarterWidth, itemHeight);
        audioProcessor.sourceTitleLabel.setBounds(halfWidth, y, quarterWidth, itemHeight);

        audioProcessor.sceneLabel.setBounds(quarterWidthPlusInset, y+labelInset, quarterWidthMinusInset, labelHeight);
        audioProcessor.sourceLabel.setBounds(threeQuarterWidthPlusInset, y+labelInset, quarterWidthMinusInset, labelHeight);

        audioProcessor.filterLabel.setVisible(false);
        audioProcessor.filterTitleLabel.setVisible(false);
    }

    y += itemHeight;

    /*
     TYPE TEXT FIELDS
     */

    int commandID = settingsStorage.getProperty(ParameterIDCommand, CommandDefinitionDefault);
    Command command = audioProcessor.commandWithID(commandID);

    if (category == CommandCategoryTypeText) {
        audioProcessor.typeTextTitleLabel.setBounds(0, y, width, itemHeight);
        y += itemHeight;
        
        audioProcessor.typeTextTextLabel.setText(settingsStorage.getProperty(ParameterIDTypeTextText, juce::String()), juce::dontSendNotification);
        audioProcessor.typeTextTextLabel.setBounds(labelInset, y+labelInset, width-(labelInset*2), labelHeight);
        y += itemHeight;
        
        audioProcessor.typeTextCursorCharacterTitleLabel.setBounds(0, y, width, itemHeight);
        y += itemHeight;
        
        audioProcessor.typeTextCursorCharacterLabel.setText(settingsStorage.getProperty(ParameterIDTypeTextCursorCharacter), juce::dontSendNotification);
        audioProcessor.typeTextCursorCharacterLabel.setBounds(labelInset, y+labelInset, width-(labelInset*2), labelHeight);
        y += itemHeight;
        
        audioProcessor.typeTextTitleLabel.setVisible(true);
        audioProcessor.typeTextTextLabel.setVisible(true);
        audioProcessor.typeTextCursorCharacterTitleLabel.setVisible(true);
        audioProcessor.typeTextCursorCharacterLabel.setVisible(true);
    }
    else {
        audioProcessor.typeTextTitleLabel.setVisible(false);
        audioProcessor.typeTextTextLabel.setVisible(false);
        audioProcessor.typeTextCursorCharacterTitleLabel.setVisible(false);
        audioProcessor.typeTextCursorCharacterLabel.setVisible(false);
    }



    /*
     RANGE
     */
    if (command.triggerParameterID == ParameterIDValue || category == CommandCategoryTypeText) {
        audioProcessor.rangeLabel.setVisible(true);
        audioProcessor.rangeLabel.setBounds(0, y, width, itemHeight);
        y += itemHeight;

        audioProcessor.rangeLowerTitleLabel.setVisible(true);
        audioProcessor.rangeLowerTitleLabel.setBounds(0, y, quarterWidth, itemHeight);

        audioProcessor.rangeUpperTitleLabel.setVisible(true);
        audioProcessor.rangeUpperTitleLabel.setBounds(halfWidth, y, quarterWidth, itemHeight);

        audioProcessor.rangeLowerLabel.setVisible(true);
        audioProcessor.rangeLowerLabel.setBounds(quarterWidthPlusInset, y+labelInset, quarterWidthMinusInset, labelHeight);

        audioProcessor.rangeUpperLabel.setVisible(true);
        audioProcessor.rangeUpperLabel.setBounds(threeQuarterWidthPlusInset, y+labelInset, quarterWidthMinusInset, labelHeight);

        y += itemHeight;
    }
    else {
        audioProcessor.rangeLabel.setVisible(false);
        audioProcessor.rangeLowerTitleLabel.setVisible(false);
        audioProcessor.rangeUpperTitleLabel.setVisible(false);
        audioProcessor.rangeLowerLabel.setVisible(false);
        audioProcessor.rangeUpperLabel.setVisible(false);
    }

    /*
     TRIGGERS
     */
    audioProcessor.valueSlider.setBounds(0, y, threeQuarterWidth, itemHeight);
    audioProcessor.valueSliderValue.setBounds(threeQuarterWidth, y, quarterWidth, itemHeight);
    
    if (category == CommandCategoryTypeText) y += itemHeight;
    
    audioProcessor.triggerButton.setBounds(0, y, width, itemHeight);
//    y += itemHeight;
    
}
