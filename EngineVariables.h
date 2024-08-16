#ifndef EngineVariables_h
#define EngineVariables_h

static const unsigned int NUM_POTENTIOMETERS = 8;
static const unsigned int NUM_BUTTONS = 10;
static const unsigned int NUM_LEDS = 6;

/**
 * @enum ButtonID
 * @brief Enumeration for the button IDs.
 */
enum ButtonID {
    FX1,
    FX2,
    FX3,
    ACTION,
    BYPASS,
    TEMPO,
    UP,
    DOWN,
    EXIT,
    ENTER
};

static const size_t NUM_EFFECTS = 3;

static const size_t NUM_ENGINEPARAMETERS = 6;

enum class EngineParameters {
    TEMPO,
    GLOBALBYPASS,
    EFFECT1BYPASS,
    EFFECT2BYPASS,
    EFFECT3BYPASS,
    EFFECTEDITFOCUS,
};

static const String engineParameterName[NUM_ENGINEPARAMETERS] {
    "Tempo",
    "Global Bypass",
    "Effect 1 Bypass",
    "Effect 2 Bypass",
    "Effect 3 Bypass",
    "Effect Edit Focus"
};

static const String engineParameterID[NUM_ENGINEPARAMETERS] {
    "tempo",
    "global_bypass",
    "effect1_bypass",
    "effect2_bypass",
    "effect3_bypass",
    "effect_edit_focus"
};


#endif /* EngineVariables_h */
