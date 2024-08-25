#ifndef EngineVariables_h
#define EngineVariables_h

static const unsigned int NUM_POTENTIOMETERS = 8;
static const unsigned int NUM_BUTTONS = 10;
static const unsigned int NUM_LEDS = 6;

/**
 * @enum ButtonID
 * @brief Enumeration for the button IDs.
 */
enum ButtonIndex {
    BUTTON_FX1,
    BUTTON_FX2,
    BUTTON_FX3,
    BUTTON_ACTION,
    BUTTON_BYPASS,
    BUTTON_TEMPO,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_EXIT,
    BUTTON_ENTER
};

enum LedIndex {
    LED_FX1,
    LED_FX2,
    LED_FX3,
    LED_ACTION,
    LED_BYPASS,
    LED_TEMPO
};

enum ParameterIndex {
    UIPARAM_POT1,
    UIPARAM_POT2,
    UIPARAM_POT3,
    UIPARAM_POT4,
    UIPARAM_POT5,
    UIPARAM_POT6,
    UIPARAM_POT7,
    UIPARAM_POT8,
    UIPARAM_BUTTON,
    UIPARAM_SEPCIAL,
    NUM_UIPARAMS,
    MENUPARAMETER
};


static const size_t NUM_PRESETS = 4;

static String presetNames[NUM_PRESETS] = {
    "Default",
    "Preset 1",
    "Preset 2",
    "Preset 3"
};

enum class PotBehaviour {
    JUMP,
    CATCH
};

static const size_t NUM_EFFECTS = 3;

static const size_t NUM_PARAMETERGROUPS = 4;

namespace Engine
{

static const size_t NUM_PARAMETERS = 8;

enum Parameters {
    TEMPO,
    GLOBAL_BYPASS,
    EFFECT1_ENGAGED,
    EFFECT2_ENGAGED,
    EFFECT3_ENGAGED,
    EFFECT_EDIT_FOCUS,
    EFFECT_ORDER,
    TEMPO_SET
};

static const String parameterName[NUM_PARAMETERS] {
    "Tempo",
    "Global Bypass",
    "Effect 1 Engaged",
    "Effect 2 Engaged",
    "Effect 3 Engaged",
    "Effect Edit Focus",
    "Effect Order",
    "Set Tempo To"
};

static const String parameterID[NUM_PARAMETERS] {
    "tempo",
    "global_bypass",
    "effect1_engaged",
    "effect2_engaged",
    "effect3_engaged",
    "effect_edit_focus",
    "effect_order",
    "tempo_set"
};


} // namespace GrainmotherReverb

namespace GrainmotherGranulator 
{

/** @brief an enum to save the parameter Indexes */
enum class Parameters
{
    GRAINLENGTH,
    DENSITY,
    VARIATION,
    PITCH,
    GLIDE,
    FEEDBACK,
    ENVELOPESHAPE,
    WETNESS,
    FREEZE,
    HIGHCUT
};

/** @brief the number of user definable parameters */
static const int NUM_PARAMETERS = 10;

/** @brief names of parameters */
static const std::string parameterName[NUM_PARAMETERS] = {
    "Grain Length",
    "Density",
    "Variation",
    "Pitch",
    "Glide",
    "Feedback",
    "Envelope Shape",
    "Wetness",
    "Freeze",
    "Highcut"
};

/** @brief names of parameters */
static const std::string parameterID[NUM_PARAMETERS] = {
    "gran_grainlength",
    "gran_density",
    "gran_variation",
    "gran_pitch",
    "gran_glide",
    "gran_feedback",
    "gran_envshape",
    "gran_wetness",
    "gran_freeze",
    "gran_highcut"
};


/** @brief minimum values of parameters */
static const float parameterMin[NUM_PARAMETERS] = {
    7.f,
    1.f,
    0.f,
    -12.f,
    -1.f,
    0.f,
    -1.f,
    0.f,
    0.f,
    200.f
};

/** @brief maximum values of parameters */
static const float parameterMax[NUM_PARAMETERS] = {
    70.f,
    85.f,
    100.f,
    12.f,
    1.f,
    0.99f,
    1.f,
    100.f,
    1.f,
    20000.f
};

/** @brief step values of parameters */
static const float parameterStep[NUM_PARAMETERS] = {
    0.5f,
    0.5f,
    1.f,
    0.5f,
    0.01f,
    0.01f,
    0.01f,
    1.f,
    1.f,
    10.f
};

/** @brief units of parameters */
static const std::string parameterSuffix[NUM_PARAMETERS] = {
    " ms",
    " grains/sec",
    " %",
    " semitones",
    " down / up",
    "",
    "",
    " %",
    "",
    " hertz"
};

/** @brief initial values of parameters */
static const float parameterInitialValue[NUM_PARAMETERS] = {
    40.f,
    20.f,
    0.f,
    0.f,
    0.f,
    0.f,
    0.f,
    70.f,
    0.f,
    20000.f
};

} // namespace GrainmotherGranulator

#endif /* EngineVariables_h */
