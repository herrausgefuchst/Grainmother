#ifndef EngineVariables_h
#define EngineVariables_h

/**
 * @defgroup UserInterfaceParameters
 * @brief a group of static constant variables used in the whole UI structrure
 * @{
 */

// MARK: - NUMS
// =======================================================================================

static const size_t NUM_POTENTIOMETERS = 8;
static const size_t NUM_BUTTONS = 10;
static const size_t NUM_LEDS = 6;
static const size_t NUM_EFFECTS = 3;
static const size_t NUM_PARAMETERGROUPS = 4;

// MARK: - PRESETS
// =======================================================================================

static const size_t NUM_PRESETS = 4;

static String presetNames[NUM_PRESETS] = {
    "Default",
    "Preset 1",
    "Preset 2",
    "Preset 3"
};

// MARK: - UI ELEMENT INDEX
// =======================================================================================

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

// MARK: - EFFECT ORDER
// =======================================================================================

enum class EffectOrder {
    REVERB,
    GRANULATOR,
    RINGMODULATOR
};

static const String effectNames[NUM_EFFECTS] {
    "Reverb",
    "Granulator",
    "Ringmodulator"
};

// MARK: - POT BEHAVIOUR
// =======================================================================================

enum class PotBehaviour {
    JUMP,
    CATCH
};

/** @} */


// MARK: - ENGINE PARAMETERS
// =======================================================================================

namespace Engine
{

static const size_t NUM_PARAMETERS = 9;

enum Parameters {
    TEMPO,
    GLOBAL_BYPASS,
    EFFECT1_ENGAGED,
    EFFECT2_ENGAGED,
    EFFECT3_ENGAGED,
    EFFECT_EDIT_FOCUS,
    EFFECT_ORDER,
    TEMPO_SET,
    GLOBAL_MIX
};

static const String parameterName[NUM_PARAMETERS] {
    "Tempo",
    "Global Bypass",
    "Effect 1 Engaged",
    "Effect 2 Engaged",
    "Effect 3 Engaged",
    "Effect Edit Focus",
    "Effect Order",
    "Set Tempo To",
    "Global Mix"
};

static const String parameterID[NUM_PARAMETERS] {
    "tempo",
    "global_bypass",
    "effect1_engaged",
    "effect2_engaged",
    "effect3_engaged",
    "effect_edit_focus",
    "effect_order",
    "tempo_set",
    "global_mix"
};

} // namespace Engine

#endif /* EngineVariables_h */
