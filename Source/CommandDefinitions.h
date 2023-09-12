//
//  CommandDefinitions.h
//  Obvious
//
//  Created by Bjørn on 04/09/2023.
//

#ifndef CommandDefinitions_h
#define CommandDefinitions_h

typedef enum : int {
    PositionProportionateX  =    10,
    PositionProportionateY  =    20,
                    ScaleX  =    30,
                    ScaleY  =    40,
              MediaRestart  =    50,
                 MediaStop  =    60,
                 MediaPlay  =    70,
                MediaPause  =    80,
               MediaCursor  =    90,
                       Hue  =   100,
                Saturation  =   110,
                RollSpeedH  =   120,
                RollSpeedV  =   130,
                   Opacity  =   140,
                   CropTop  =   150,
                CropBottom  =   160,
                  CropLeft  =   170,
                 CropRight  =   180,
                SetVisible  =   190,
                  TypeText  =   200,
    TypeSetCursorCharacter  =   210,
      TypeSetCursorVisible  =   220,
           TypeSetNumChars  =   230,
//              Disconnect  =   240, // placeholder... defined below as a string for easier sending
//               Heartbeat  =   250, // placeholder... defined below as a string for easier sending
         HeartbeatResponse  =   260,
} CommandDefinition;

#define CommandDefinitionDefault PositionProportionateX
#define CommandDisconnect "240"
#define CommandHeartbeat "250"

typedef enum : int {
    CommandCategorySource=10,
    CommandCategoryFilter=20,
    CommandCategoryTypeText=30,
} CommandCategory;

#define CommandCategoryDefault CommandCategorySource

struct Command {
    CommandDefinition commandID;
    juce::String triggerParameterID;
    juce::String displayName;
    CommandCategory category;
//    bool requiresFilterField;
    int recommendedRangeLower;
    int recommendedRangeUpper;
    bool requiresToggleButton;
};

#endif /* CommandDefinitions_h */
