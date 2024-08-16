#ifndef parameters_hpp
#define parameters_hpp

#include "uielements.hpp"
#include "helpers.hpp"

// MARK: - AUDIO PARAMETER
// *******************************************************************************

/**
 * @class AudioParameter
 * @brief A class representing an audio parameter that can be observed by listeners.
 *
 * Print Value = value that is going to be printed on display
 * (Current) Value = mostly same as Print Value, but can vary in case of Ramps (Slide Parameter)
 * Normalized Value = value between 0..1 as received from GUI or Potentiometers
 *
 */
class AudioParameter : public UIElement::Listener
{
public:
    /**
     * @brief Deleted default constructor to enforce parameterized construction.
     */
    AudioParameter() = delete;

    /**
     * @brief Constructs an AudioParameter with a specified ID and name.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     */
    AudioParameter(const String id_, const String name_) : id(id_), name(name_) {}

    /**
     * @brief Virtual destructor for AudioParameter.
     */
    virtual ~AudioParameter() {}

    /**
     * @class Listener
     * @brief A listener class to observe changes in AudioParameter.
     */
    class Listener
    {
    public:
        /**
         * @brief Virtual destructor for Listener.
         */
        virtual ~Listener() {}

        /**
         * @brief Called when the parameter value changes.
         * @param param_ Pointer to the changed parameter.
         */
        virtual void parameterChanged(AudioParameter* param_) {}
    };

    /**
     * @brief Adds a listener to the parameter.
     * @param listener_ Pointer to the listener to add.
     */
    void addListener(AudioParameter::Listener* listener_)
    {
        listeners.push_back(listener_);
    }

    /**
     * @brief Notifies all listeners of a parameter change.
     * @param withPrint_ Whether to print the change or not.
     */
    virtual void notifyListeners(const bool withPrint_) = 0;

    std::vector<std::function<void()>> onChange;  /**< List of functions to call on value change */
    std::vector<std::function<void()>> onClick;   /**< List of functions to call on click */
    std::vector<std::function<void()>> onPress;   /**< List of functions to call on press */
    std::vector<std::function<void()>> onRelease; /**< List of functions to call on release */

    /**
     * @brief Processes the parameter (can be overridden).
     */
    virtual void process() {}

    /**
     * @brief Sets the parameter value (float version).
     * @param value_ The new value.
     * @param withNotification_ Whether to notify listeners.
     */
    virtual void setValue(const float value_, const bool withNotification_ = true) {}

    /**
     * @brief Sets the parameter value (int version).
     * @param value_ The new value.
     * @param withNotification_ Whether to notify listeners.
     */
    virtual void setValue(const int value_, const bool withNotification_ = true) {}

    /**
     * @brief Nudges the parameter value by a given direction.
     * @param direction_ The direction to nudge the value.
     */
    virtual void nudgeValue(const int direction_) {}

    /**
     * @brief Gets the name of the parameter.
     * @return The name of the parameter.
     */
    String getName() const { return name; }

    /**
     * @brief Gets the ID of the parameter.
     * @return The ID of the parameter.
     */
    String getParameterID() const { return id; }

    /**
     * @brief Gets the current value of the parameter as a float.
     * @return The float value.
     */
    virtual float getValueAsFloat() const = 0;

    /**
     * @brief Gets the current value of the parameter as an int.
     * @return The int value.
     */
    virtual int getValueAsInt() const = 0;
    
    /**
     * @brief Gets the printable value of the parameter as a float.
     * @return The printable float value.
     */
    virtual float getPrintValueAsFloat() const = 0;

    /**
     * @brief Gets the printable value of the parameter as an int.
     * @return The printable int value.
     */
    virtual int getPrintValueAsInt() const = 0;

    /**
     * @brief Gets the printable value of the parameter as a string.
     * @return The printable string value.
     */
    virtual String getPrintValueAsString() const = 0;

    /**
     * @brief Gets the normalized value of the parameter.
     * @return The normalized value.
     */
    virtual float getNormalizedValue() const { return 0.f; }

    /**
     * @brief Gets the minimum value of the parameter.
     * @return The minimum value.
     */
    virtual float getMin() const { return -1.f; }

    /**
     * @brief Gets the maximum value of the parameter.
     * @return The maximum value.
     */
    virtual float getMax() const { return -1.f; }

    /**
     * @brief Gets the step size of the parameter.
     * @return The step size.
     */
    virtual float getStep() const { return -1.f; }

    /**
     * @brief Gets the range of the parameter.
     * @return The range.
     */
    virtual float getRange() const { return -1.f; }

protected:
    std::vector<Listener*> listeners; /**< List of listeners observing this parameter */
    const String id, name; /**< The ID and name of the parameter */
};


// MARK: - CHOICE PARAMETER
// *******************************************************************************

/**
 * @class ChoiceParameter
 * @brief A class representing a parameter with multiple choice options.
 */
class ChoiceParameter : public AudioParameter
{
public:
    /**
     * @brief Constructs a ChoiceParameter with a specified ID, name, number of choices, and choice names.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param numChoices_ The number of choices available.
     * @param choices_ Pointer to an array of choice names.
     */
    ChoiceParameter(const String id_, const String name_, const int numChoices_, const String* choices_);

    /**
     * @brief Destructor for ChoiceParameter.
     */
    ~ChoiceParameter();
        
    /**
     * @brief Sets the value of the parameter (int version).
     * @param value_ The new value.
     * @param withPrint_ Whether to notify listeners.
     */
    void setValue(const int value_, const bool withPrint_ = true) override;

    /**
     * @brief Sets the value of the parameter (float version).
     * @param value_ The new value.
     * @param withPrint_ Whether to notify listeners.
     */
    void setValue(const float value_, const bool withPrint_ = true) override;

    /**
     * @brief Handles potentiometer changes from a UI element.
     * @param uielement_ Pointer to the UI element that changed.
     */
    void potChanged(UIElement* uielement_) override;

    /**
     * @brief Notifies all listeners of a parameter change.
     * @param withPrint_ Whether to print the change or not.
     */
    void notifyListeners(const bool withPrint_) override;

    float getValueAsFloat() const override { return static_cast<float>(getValueAsInt()); }
    int getValueAsInt() const override { return choice; }
    String getPrintValueAsString() const override { return choice_names[choice]; }
    float getPrintValueAsFloat() const override { return getValueAsFloat(); }
    int getPrintValueAsInt() const override { return getValueAsInt(); }

    /**
     * @brief Gets the number of choices available.
     * @return The number of choices.
     */
    int getNumChoices() const { return numChoices; }

    /**
     * @brief Gets the names of the available choices.
     * @return Pointer to an array of choice names.
     */
    String* getChoiceNames() const { return choice_names; }
    
private:
    const int numChoices = 0; /**< Number of choices available */
    int choice = 0; /**< The current choice */
    String* choice_names; /**< Array of choice names */
};


// MARK: - SLIDE PARAMETER
// *******************************************************************************



/**
 * @class SlideParameter
 * @brief A class representing a parameter with a sliding scale, supporting linear and frequency scaling.
 */
class SlideParameter : public AudioParameter
{
public:
    /**
     * @enum Scaling
     * @brief Enumeration for the type of scaling applied to the parameter.
     */
    enum Scaling { LIN, FREQ };
    
    /**
     * @brief Constructs a SlideParameter with specified attributes.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param unit_ The unit of measurement for the parameter.
     * @param min_ The minimum value of the parameter.
     * @param max_ The maximum value of the parameter.
     * @param step_ The step size for the parameter.
     * @param default_ The default value of the parameter.
     * @param scaling_ The scaling type (linear or frequency).
     * @param ramptime_ms_ The ramp time in milliseconds for changes.
     */
    SlideParameter(const String id_, const String name_, const String unit_, const float min_, const float max_, const float step_, const float default_, const Scaling scaling_ = LIN, const float ramptime_ms_ = 0.f);

    /**
     * @brief Destructor for SlideParameter.
     */
    ~SlideParameter() {}
    
    void process() override;
    void potChanged(UIElement* uielement_) override;
    void notifyListeners(const bool withPrint_) override;

    void setValue(float value_, const bool withPrint_ = true) override;
    void setValue(const int value_, const bool withPrint_ = true) override;
    
    /**
     * @brief Sets the normalized value of the parameter.
     * @param value_ The normalized value to set.
     * @param withPrint_ Whether to notify listeners.
     */
    void setNormalizedValue(float value_, const bool withPrint_ = true);

    void nudgeValue(const int direction_) override;

    float getValueAsFloat() const override { return value.getCurrent(); }
    float getPrintValueAsFloat() const override { return value.getGoal(); }
    float getNormalizedValue() const override { return normalizedValue; }
    int getValueAsInt() const override { return static_cast<int>(getValueAsFloat()); }
    int getPrintValueAsInt() const override { return static_cast<int>(getPrintValueAsFloat()); }

    /**
     * @brief Gets the unit of measurement for the parameter.
     * @return The unit as a string.
     */
    String getUnit() const { return unit; }

    String getPrintValueAsString() const override { return TOSTRING(value.getGoal()); }
    
    float getMin() const override { return min; }
    float getMax() const override { return max; }
    float getStep() const override { return step; }
    float getRange() const override { return range; }
    
private:
    const String unit;
    const float min;
    const float max;
    const float step;
    const float range;
    const float ramptime_ms;
    const Scaling scaling;
    Ramp value;
    float normalizedValue;
};


// MARK: - BUTTON PARAMETER
// ********************************************************************************

/**
 * @class ButtonParameter
 * @brief A class representing a button-based parameter with different interaction types.
 *
 * This class defines the interaction types for various buttons. The interaction types
 * include TOGGLE, MOMENTARY, and combinations of these. The configuration specifies
 * whether the button supports short press (TOGGLE), long press (MOMENTARY), or both.
 *
 * ### Button Interaction Summary:
 * | Button       | Short Press  | Long Press                          |
 * |--------------|--------------|-------------------------------------|
 * | FX 1..3      | TOGGLE       | No                                  |
 * | ACTION       | TOGGLE       | MOMENTARY                           |
 * | TEMPO        | No           | MOMENTARY (no ButtonParameter connected) |
 * | BYPASS       | TOGGLE       | MOMENTARY                           |
 * | MENUs        | No           | MOMENTARY (no ButtonParameter connected) |
 *
 * "No" indicates that the parameter does not need to save a value.
 * Therefore, three interaction types are defined:
 * 1. **TOGGLE only**: Button toggles its state with short presses.
 * 2. **MOMENTARY only**: Button activates only while pressed (long press).
 * 3. **Coupled**: Button supports both TOGGLE (short press) and MOMENTARY (long press) interactions.
 * 
 */
class ButtonParameter : public AudioParameter
{
public:
    /**
     * @enum Type
     * @brief Enumeration for the type of button interaction (e.g., TOGGLE, MOMENTARY, COUPLED).
     */
    enum Type { TOGGLE, MOMENTARY, COUPLED };

    /**
     * @enum Toggle
     * @brief Enumeration for the toggle state (UP, DOWN).
     */
    enum Toggle { UP, DOWN };
        
    /**
     * @brief Constructs a ButtonParameter with a specified ID, name, and interaction type.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param type_ The type of button interaction.
     */
    ButtonParameter(const String id_, const String name_, const Type type_);

    /**
     * @brief Destructor for ButtonParameter.
     */
    ~ButtonParameter() {}
        
    void buttonClicked(UIElement* uielement_) override;
    void buttonPressed(UIElement* uielement_) override;
    void buttonReleased(UIElement* uielement_) override;
    
    void notifyListeners(const bool withPrint_) override;

    void setValue(const float value_, const bool withPrint_ = true) override;
    void setValue(const int value_, const bool withPrint_ = true) override;

    float getValueAsFloat() const override { return static_cast<float>(getValueAsInt()); }
    float getPrintValueAsFloat() const override { return getValueAsFloat(); }
    int getValueAsInt() const override { return value; }
    int getPrintValueAsInt() const override { return getValueAsInt(); }
    String getPrintValueAsString() const override { return TOSTRING(value); } // TODO: enum to string?
    
private:
    const Type type;
    int value = UP;
};


// MARK: - AUDIO PARAMETER GROUP
// *******************************************************************************

/**
 * @class AudioParameterGroup
 * @brief A class representing a group of audio parameters, organized by type.
 *
 * The `AudioParameterGroup` class allows you to manage and organize audio parameters
 * by grouping them together. This class supports different types of parameters,
 * such as sliders, buttons, and choices, and enables easy retrieval and management.
 */
class AudioParameterGroup
{
public:
    /**
     * @enum ID
     * @brief Identifiers for different parameter groups.
     */
    enum ID { ENGINE, BEATREPEAT, GRANULATOR, DELAY };

    /**
     * @enum Type
     * @brief Types of parameter groups, indicating their purpose.
     */
    enum class Type { ENGINE, EFFECT };
    
    /**
     * @brief Deleted default constructor to enforce parameterized construction.
     */
    AudioParameterGroup() = delete;

    /**
     * @brief Constructs an AudioParameterGroup with a specified name and type.
     * @param name_ The name of the parameter group.
     * @param type_ The type of the parameter group (ENGINE or EFFECT).
     */
    AudioParameterGroup(const String name_, const Type type_);

    /**
     * @brief Destructor for AudioParameterGroup.
     */
    ~AudioParameterGroup();
        
    /**
     * @brief Adds a slider parameter to the group.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param unit_ The unit of measurement for the parameter.
     * @param min_ The minimum value of the parameter.
     * @param max_ The maximum value of the parameter.
     * @param step_ The step size for the parameter.
     * @param default_ The default value of the parameter.
     * @param scaling_ The scaling type for the parameter (linear or frequency).
     * @param ramptime_ms_ The ramp time in milliseconds for value changes.
     */
    void addParameter(const String id_, const String name_, const String unit_, const float min_, const float max_, const float step_, const float default_, const SlideParameter::Scaling scaling_ = SlideParameter::LIN, const float ramptime_ms_ = 0.f);

    /**
     * @brief Adds a button parameter to the group.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param type_ The button type (e.g., TOGGLE, MOMENTARY).
     */
    void addParameter(const String id_, const String name_, const ButtonParameter::Type type_);

    /**
     * @brief Adds a choice parameter to the group.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param numChoices_ The number of choices available.
     * @param array_ A pointer to an array of choice names.
     */
    void addParameter(const String id_, const String name_, const int numChoices_, const String* array_);

    /**
     * @brief Adds a choice parameter to the group using an initializer list.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param choices A list of choices for the parameter.
     */
    void addParameter(const String id_, const String name_, std::initializer_list<String> choices);
    
    /**
     * @brief Retrieves a parameter by its index within the group.
     * @param index_ The index of the parameter in the group.
     * @return A pointer to the requested AudioParameter.
     */
    AudioParameter* getParameter(const int index_);
    
    /**
     * @brief Retrieves a parameter by its ID.
     * @param id_ The ID of the parameter.
     * @param withErrorMessage_ Whether to print an error message if the parameter is not found.
     * @return A pointer to the requested AudioParameter.
     */
    AudioParameter* getParameter(const String id_, const bool withErrorMessage_ = true);
    
    /**
     * @brief Gets the name of the parameter group.
     * @return The name of the parameter group.
     */
    String getName() const { return name; }

    /**
     * @brief Gets the number of parameters in the group.
     * @return The number of parameters in the group.
     */
    int getNumParametersInGroup() const { return static_cast<int>(parametergroup.size()); }
    
private:
    std::vector<AudioParameter*> parametergroup; /**< Vector containing the parameters in the group */
    const String name; /**< Name of the parameter group */
    const Type type; /**< Type of the parameter group (ENGINE or EFFECT) */
};

#endif /* parameters_hpp */
