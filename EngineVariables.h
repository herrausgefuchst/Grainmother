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

enum LedIndex {
    LED_FX1,
    LED_FX2,
    LED_FX3,
    LED_ACTION,
    LED_BYPASS,
    LED_TEMPO
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

static const size_t NUM_ENGINEPARAMETERS = 8;

enum class EngineParameters {
    TEMPO,
    GLOBALBYPASS,
    EFFECT1BYPASS,
    EFFECT2BYPASS,
    EFFECT3BYPASS,
    EFFECTEDITFOCUS,
    EFFECTORDER,
    TEMPOSET
};

static const String engineParameterName[NUM_ENGINEPARAMETERS] {
    "Tempo",
    "Global Bypass",
    "Effect 1 Engaged",
    "Effect 2 Engaged",
    "Effect 3 Engaged",
    "Effect Edit Focus",
    "Effect Order",
    "Set Tempo To"
};

static const String engineParameterID[NUM_ENGINEPARAMETERS] {
    "tempo",
    "global_bypass",
    "effect1_engaged",
    "effect2_engaged",
    "effect3_engaged",
    "effect_edit_focus",
    "effect_order",
    "tempo_set"
};

enum class ParameterGroupID {
    ENGINE,
    REVERB,
    GRANULATOR,
    RESONATOR
};

namespace GrainmotherReverb
{

/** @brief number of reverb types */
static const unsigned int NUM_TYPES = 4;

/** @brief the reverb type-enum */
enum class ReverbTypes {
    CHURCH,
    DIGITALVINTAGE,
    SEASICK,
    ROOM
};

static const std::string reverbTypeNames[NUM_TYPES] = {
    "Church",
    "Digital Vintage",
    "Seasick",
    "Room"
};

/** @brief the number of user definable parameters */
static const unsigned int NUM_PARAMETERS = 12;

/** @brief an enum to save the parameter Indexes */
enum class Parameters
{
    DECAY,
    PREDELAY,
    MODRATE,
    MODDEPTH,
    SIZE,
    FEEDBACK,
    HIGHCUT,
    WETNESS,
    TYPE,
    LOWCUT,
    MULTFREQ,
    MULTGAIN
};

/** @brief names of parameters */
static const std::string parameterID[NUM_PARAMETERS] = {
    "reverb_decay",
    "reverb_predelay",
    "reverb_modrate",
    "reverb_moddepth",
    "reverb_size",
    "reverb_feedback",
    "reverb_highcut",
    "reverb_wetness",
    "reverb_type",
    "reverb_lowcut",
    "reverb_multfreq",
    "reverb_multgain"
};

/** @brief names of parameters */
static const std::string parameterName[NUM_PARAMETERS] = {
    "Decay",
    "Predelay",
    "Modulation Rate",
    "Modulation Depth",
    "Size",
    "Feedback",
    "Highcut",
    "Wetness",
    "Reverb Type",
    "Lowcut",
    "Multiplier Freq",
    "Multiplier Gain"
};

/** @brief minimum values of parameters */
static const float parameterMin[NUM_PARAMETERS] = {
    0.3f,
    0.f,
    0.01f,
    0.f,
    10.f,
    0.f,
    200.f,
    0.f,
    0.f,
    20.f,
    80.f,
    -12.f
};

/** @brief maximum values of parameters */
static const float parameterMax[NUM_PARAMETERS] = {
    20.f,
    150.f,
    30.f,
    100.f,
    300.f,
    0.99f,
    20000.f,
    100.f,
    (float)(NUM_TYPES-1),
    1500.f,
    3000.f,
    12.f
};

/** @brief step values of parameters */
static const float parameterStep[NUM_PARAMETERS] = {
    0.1f,
    1.f,
    0.5f,
    0.5f,
    1.f,
    0.01f,
    10.f,
    1.f,
    1.f,
    10.f,
    10.f,
    0.5f
};

/** @brief units of parameters */
static const std::string parameterSuffix[NUM_PARAMETERS] = {
    " sec",
    " msec",
    " hertz",
    " %",
    " %",
    "",
    " hertz",
    " %",
    "",
    " hertz",
    " hertz",
    " dB"
};

/** @brief initial values of parameters */
static const float parameterInitialValue[NUM_PARAMETERS] = {
    1.7f,
    25.f,
    5.f,
    0.f,
    100.f,
    0.f,
    20000.f,
    65.f,
    3.f,
    20.f,
    120.f,
    0.f
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
