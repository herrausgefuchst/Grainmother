#ifndef parameters_hpp
#define parameters_hpp

#include "uielements.hpp"

// =======================================================================================
// MARK: - AUDIO PARAMETER
// =======================================================================================

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
    /** Deleted default constructor to enforce parameterized construction. */
    AudioParameter() = delete;

    /**
     * Constructs an AudioParameter with a specified index, ID, and name.
     * @param index_ The index of the parameter.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     */
    AudioParameter(const uint index_, const String& id_, const String& name_)
        : index(index_), id(id_), name(name_) {}

    /** Virtual destructor for AudioParameter. */
    virtual ~AudioParameter() {}

    /**
     * @class Listener
     * @brief A listener class to observe changes in AudioParameter.
     */
    class Listener
    {
    public:
        /** Virtual destructor for Listener. */
        virtual ~Listener() {}

        /**
         * Called when the parameter value changes.
         * @param param_ Pointer to the changed parameter.
         */
        virtual void parameterChanged(AudioParameter* param_) {}
        
        /** 
         * Called when the display is requested.
         * @param param_ Pointer to the changed parameter.
         */
        virtual void parameterCalledDisplay(AudioParameter* param_) {}
    };

    /**
     * Adds a listener to the parameter.
     * @param listener_ Pointer to the listener to add.
     */
    void addListener(AudioParameter::Listener* listener_)
    {
        listeners.push_back(listener_);
    }

    /**
     * Notifies all listeners of a parameter change.
     * @param withPrint_ Whether to print the change or not.
     */
    virtual void notifyListeners(const bool withPrint_);

    std::vector<std::function<void()>> onChange;  /**< List of functions to call on value change */
    std::vector<std::function<void()>> onClick;   /**< List of functions to call on click */
    std::vector<std::function<void()>> onPress;   /**< List of functions to call on press */
    std::vector<std::function<void()>> onRelease; /**< List of functions to call on release */

    /** Processes the parameter ramp (can be overridden). */
    virtual void processRamp() {}

    /**
     * Sets the parameter value (float version).
     * @param value_ The new value.
     * @param withPrint_ Whether to print the new value to display.
     */
    virtual void setValue(const float value_, const bool withPrint_ = true) {}

    /**
     * Sets the parameter value (int version).
     * @param value_ The new value.
     * @param withPrint_ Whether to print the new value to display.
     */
    virtual void setValue(const int value_, const bool withPrint_ = true) {}

    /** Sets the parameter to its default value. */
    virtual void setDefaultValue() {}
    
    /**
     * Nudges the parameter value by a given direction.
     * @param direction_ The direction to nudge the value (negativ values = down, 0 and positiv values = up)
     */
    virtual void nudgeValue(const int direction_) {}
    
    /** Gets the index of the parameter. */
    uint getIndex() const { return index; }
    
    /** Gets the ID of the parameter. @return The ID of the parameter. */
    String getID() const { return id; }

    /** Gets the name of the parameter. @return The name of the parameter. */
    String getName() const { return name; }

    /** Gets the current value of the parameter as a float. @return The float value. */
    virtual float getValueAsFloat() const = 0;

    /** Gets the current value of the parameter as an int. @return The int value. */
    virtual int getValueAsInt() const = 0;

    /** Gets the printable value of the parameter as a float. @return The printable float value. */
    virtual float getPrintValueAsFloat() const = 0;

    /** Gets the printable value of the parameter as a string. @return The printable string value. */
    virtual String getPrintValueAsString() const = 0;

    /** Gets the normalized value of the parameter. @return The normalized value. */
    virtual float getNormalizedValue() const { return 0.f; }

    /** Gets the minimum value of the parameter. @return The minimum value. */
    virtual float getMin() const { return -1.f; }

    /** Gets the maximum value of the parameter. @return The maximum value. */
    virtual float getMax() const { return -1.f; }

    /** Gets the nudgeStep size of the parameter. @return The nudge step size. */
    virtual float getNudgeStep() const { return -1.f; }

    /** Gets the range of the parameter. @return The range. */
    virtual float getRange() const { return -1.f; }
    
protected:
    std::vector<Listener*> listeners; /**< List of listeners observing this parameter */
    const uint index; /**< The index of the parameter */
    const String id; /**< The ID of the parameter */
    const String name; /**< The name of the parameter */
};


// =======================================================================================
// MARK: - CHOICE PARAMETER
// =======================================================================================

/**
 * @class ChoiceParameter
 * @brief Represents a parameter with multiple choice options.
 */
class ChoiceParameter : public AudioParameter
{
public:
    /**
     * @brief Constructs a ChoiceParameter with a specified index, ID, name, choice names, and number of choices.
     * @param index_ The index of the parameter.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param choiceNames_ Pointer to an array of choice names.
     * @param numChoices_ The number of choices available.
     */
    ChoiceParameter(const uint index_, const String& id_, const String& name_, 
                    const String* choiceNames_, const unsigned int numChoices_);
    
    /**
     * @brief Constructs a ChoiceParameter with a specified index, ID, name, and an initializer list of choice names.
     * @param index_ The index of the parameter.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param toggleStateNames_ An initializer list of choice names.
     */
    ChoiceParameter(const uint index_, const String& id_, const String& name_, 
                    std::initializer_list<String> toggleStateNames_);

    /** Destructor for ChoiceParameter. */
    ~ChoiceParameter() {}

    /**
     * @brief Sets the value of the parameter (int version).
     * @param value_ The new value.
     * @param withPrint_ Whether to print the new value to display.
     */
    void setValue(const int value_, const bool withPrint_ = true) override;

    /**
     * @brief Sets the value of the parameter (float version).
     * @param value_ The new value.
     * @param withPrint_ Whether to print the new value to display.
     */
    void setValue(const float value_, const bool withPrint_ = true) override;

    /**
     * @brief Handles potentiometer changes from a UI element.
     * @param uielement_ Pointer to the UI element that changed.
     */
    void potChanged(UIElement* uielement_) override;

    /**
     * @brief Handles button clicks from a UI element.
     * @param uielement_ Pointer to the UI element that changed.
     */
    void buttonClicked(UIElement* uielement_) override;
    
    /** Nudges the parameter value in the specified direction. */
    void nudgeValue(const int direction_) override;

    /** @brief Gets the current value as a float. @return The float value of the current choice. */
    float getValueAsFloat() const override { return static_cast<float>(getValueAsInt()); }
    
    /** @brief Gets the printable value as a float. @return The printable float value of the current choice. */
    float getPrintValueAsFloat() const override { return getValueAsFloat(); }
    
    /** @brief Gets the current value as an int. @return The int value of the current choice. */
    int getValueAsInt() const override { return choice; }
    
    /** @brief Gets the printable value as a string. @return The printable string value of the current choice. */
    String getPrintValueAsString() const override { return choiceNames[choice]; }

    /** @brief Gets the number of choices available. @return The number of choices. */
    size_t getNumChoices() const { return numChoices; }

    /** @brief Gets the names of the available choices. @return Pointer to an array of choice names. */
    String* getChoiceNames() const { return choiceNames.get(); }

private:
    const size_t numChoices = 0; /**< The number of choices available. */
    unsigned int choice = 0; /**< The current choice. */
    std::unique_ptr<String[]> choiceNames; /**< Array of choice names. */
};



// =======================================================================================
// MARK: - SLIDE PARAMETER
// =======================================================================================

/**
 * @class SlideParameter
 * @brief Represents a parameter with a sliding scale, supporting linear and frequency scaling.
 *
 * The SlideParameter includes a ramp for the print value.
 */
class SlideParameter : public AudioParameter
{
public:
    /**
     * @enum Scaling
     * @brief Defines the type of scaling applied to the parameter.
     */
    enum Scaling { LIN, FREQ };
    
    /**
     * @brief Constructs a SlideParameter with specified attributes.
     * @param index_ The index of the parameter.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param suffix_ The unit of measurement for the parameter.
     * @param min_ The minimum value of the parameter.
     * @param max_ The maximum value of the parameter.
     * @param nudgeStep_ The step size for the parameter.
     * @param default_ The default value of the parameter.
     * @param sampleRate_ The sample rate for the parameter.
     * @param scaling_ The scaling type (linear or frequency).
     * @param ramptimeMs_ The ramp time in milliseconds for changes.
     */
    SlideParameter(const uint index_, const String& id_, const String& name_, 
                   const String& suffix_, const float min_, const float max_,
                   const float nudgeStep_,
                   const float default_,
                   const float sampleRate_,
                   const Scaling scaling_ = LIN,
                   const float ramptimeMs_ = 0.f);

    /** Destructor for SlideParameter. */
    ~SlideParameter() {}
    
    /** Processes the parameter ramp. */
    void processRamp() override;
    
    /** Handles potentiometer changes from a UI element. */
    void potChanged(UIElement* uielement_) override;
    
    /**
     * @brief Sets the value of the parameter (float version).
     * @param value_ The new value.
     * @param withPrint_ Whether to print the new value to display.
     */
    void setValue(float value_, const bool withPrint_ = true) override;
    
    /**
     * @brief Sets the value of the parameter (int version).
     * @param value_ The new value.
     * @param withPrint_ Whether to print the new value to display.
     */
    void setValue(const int value_, const bool withPrint_ = true) override;
    
    /** Sets the scaling type of the parameter. */
    void setScaling(const Scaling scaling_) { scaling = scaling_; }
    
    /** Sets the ramp time in milliseconds. */
    void setRampTimeMs(const float rampTimeMs_) { ramptimeMs = rampTimeMs_; }
    
    /**
     * @brief Sets the normalized value of the parameter (0..1)
     * @param value_ The normalized value to set.
     * @param withPrint_ Whether to notify listeners.
     */
    void setNormalizedValue(const float value_, const bool withPrint_ = true);
    
    /** Sets the parameter to its default value. */
    void setDefaultValue() override;

    /** Nudges the parameter value in the specified direction. */
    void nudgeValue(const int direction_) override;

    /** @brief Gets the current value as a float. @return The float value of the parameter. */
    float getValueAsFloat() const override { return value(); }
    
    /** @brief Gets the printable value as a float. @return The printable float value of the parameter. */
    float getPrintValueAsFloat() const override { return value.getTarget(); }
    
    /** @brief Gets the normalized value of the parameter. (0...1) @return The normalized value. */
    float getNormalizedValue() const override { return normalizedValue; }
    
    /** @brief Gets the current value as an int. @return The int value of the parameter. */
    int getValueAsInt() const override { return static_cast<int>(getValueAsFloat()); }

    /** @brief Gets the suffix (unit of measurement) for the parameter. @return The unit as a string. */
    String getSuffix() const { return suffix; }

    /** @brief Gets the printable value as a string. @return The printable string value of the parameter. */
    String getPrintValueAsString() const override { return TOSTRING(value.getTarget()); }
    
    /** @brief Gets the minimum value of the parameter. @return The minimum value. */
    float getMin() const override { return min; }
    
    /** @brief Gets the maximum value of the parameter. @return The maximum value. */
    float getMax() const override { return max; }
    
    /** @brief Gets the nudge step size of the parameter. @return The nudge step size. */
    float getNudgeStep() const override { return nudgeStep; }
    
    /** @brief Gets the range of the parameter. @return The range. */
    float getRange() const override { return range; }
    
private:
    /**
     * @brief Sets the ramp value with an option to use the ramp.
     * @param value_ The value to set.
     * @param withRamp_ Whether to use the ramp.
     */
    void setRampValue(const float value_, const bool withRamp_ = true);
    
    const String suffix;             /**< The unit of measurement for the parameter. */
    const float min;                 /**< The minimum value of the parameter. */
    const float max;                 /**< The maximum value of the parameter. */
    float nudgeStep;                 /**< The nudge step size of the parameter. */
    const float defaultValue;        /**< The default value of the parameter. */
    const float range;               /**< The range of the parameter. */
    float ramptimeMs;                /**< The ramp time in milliseconds. */
    Scaling scaling;                 /**< The scaling type (linear or frequency). */
    RampLinear value;                /**< The current value, managed with a ramp. */
    float normalizedValue;           /**< The normalized value of the parameter. */
};


// =======================================================================================
// MARK: - BUTTON PARAMETER
// =======================================================================================

/**
 * @class ButtonParameter
 * @brief Represents a parameter that can toggle between states, typically used for buttons.
 *
 * reacts on Clicks, Presses and Releases of a Button
 *
 */
class ButtonParameter : public AudioParameter
{
public:
    /**
     * @enum ToggleState
     * @brief Represents the toggle state of the button (INACTIVE, ACTIVE).
     */
    enum ToggleState { INACTIVE, ACTIVE };

    /**
     * @brief Constructs a ButtonParameter with a specified index, ID, name, and optional toggle state names.
     * @param index_ The index of the parameter.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param toggleStateNames_ Pointer to an array of toggle state names (optional).
     */
    ButtonParameter(const uint index_, const String& id_, const String& name_, 
                    const String* toggleStateNames_ = nullptr);
    
    /**
     * @brief Constructs a ButtonParameter with a specified index, ID, name, and an initializer list of toggle state names.
     * @param index_ The index of the parameter.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param toggleStateNames_ An initializer list of toggle state names.
     */
    ButtonParameter(const uint index_, const String& id_, const String& name_, 
                    std::initializer_list<String> toggleStateNames_);

    /** Destructor for ButtonParameter. */
    ~ButtonParameter() {}
        
    /** Handles button clicks from a UI element. */
    void buttonClicked(UIElement* uielement_) override;
    
    /** Handles button presses from a UI element. */
    void buttonPressed(UIElement* uielement_) override;
    
    /** Handles button releases from a UI element. */
    void buttonReleased(UIElement* uielement_) override;
        
    /**
     * @brief Sets the value of the button parameter (float version).
     * @param value_ The new value.
     * @param withPrint_ Whether to print the new value to display.
     */
    void setValue(const float value_, const bool withPrint_ = true) override;
    
    /**
     * @brief Sets the value of the button parameter (int version).
     * @param value_ The new value.
     * @param withPrint_ Whether to print the new value to display.
     */
    void setValue(const int value_, const bool withPrint_ = true) override;

    /** @brief Gets the current value as a float. @return The float value of the parameter. */
    float getValueAsFloat() const override { return static_cast<float>(getValueAsInt()); }
    
    /** @brief Gets the printable value as a float. @return The printable float value of the parameter. */
    float getPrintValueAsFloat() const override { return getValueAsFloat(); }
    
    /** @brief Gets the current value as an int. @return The int value of the parameter. */
    int getValueAsInt() const override { return value; }
    
    /** @brief Gets the printable value as a string. @return The printable string value of the parameter. */
    String getPrintValueAsString() const override;
    
private:
    /** Toggles the button state between INACTIVE and ACTIVE. */
    void toggle();
    
    ToggleState value = INACTIVE;                 /**< The current toggle state of the button. */
    std::unique_ptr<String[]> toggleStateNames;   /**< Array of toggle state names. */
};


// =======================================================================================
// MARK: - TOGGLE PARAMETER
// =======================================================================================


/**
 * @class ToggleParameter
 * @brief Represents a parameter that toggles between states, typically used for Buttons
 *
 * pretty much the same as ButtonParameter, but this type can only toggle on button clicks
 */
class ToggleParameter : public AudioParameter
{
public:
    /**
     * @enum ToggleState
     * @brief Represents the toggle state of the parameter (INACTIVE, ACTIVE).
     */
    enum ToggleState { INACTIVE, ACTIVE };
    
    /**
     * @brief Constructs a ToggleParameter with a specified index, ID, name, and optional toggle state names.
     * @param index_ The index of the parameter.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param toggleStateNames_ Pointer to an array of toggle state names (optional).
     */
    ToggleParameter(const uint index_, const String& id_, const String& name_, 
                    const String* toggleStateNames_ = nullptr);
    
    /**
     * @brief Constructs a ToggleParameter with a specified index, ID, name, and an initializer list of toggle state names.
     * @param index_ The index of the parameter.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param toggleStateNames_ An initializer list of toggle state names.
     */
    ToggleParameter(const uint index_, const String& id_, const String& name_, 
                    std::initializer_list<String> toggleStateNames_);
    
    /** Destructor for ToggleParameter. */
    ~ToggleParameter() {}
        
    /** Handles button clicks from a UI element. */
    void buttonClicked(UIElement* uielement_) override;
    
    /** Handles button presses from a UI element. */
    void buttonPressed(UIElement* uielement_) override;
        
    /**
     * @brief Sets the value of the toggle parameter (float version).
     * @param value_ The new value.
     * @param withPrint_ Whether to print the new value to display.
     */
    void setValue(const float value_, const bool withPrint_ = true) override;
    
    /**
     * @brief Sets the value of the toggle parameter (int version).
     * @param value_ The new value.
     * @param withPrint_ Whether to print the new value to display.
     */
    void setValue(const int value_, const bool withPrint_ = true) override;

    /** @brief Gets the current value as a float. @return The float value of the parameter. */
    float getValueAsFloat() const override { return static_cast<float>(getValueAsInt()); }
    
    /** @brief Gets the printable value as a float. @return The printable float value of the parameter. */
    float getPrintValueAsFloat() const override { return getValueAsFloat(); }
    
    /** @brief Gets the current value as an int. @return The int value of the parameter. */
    int getValueAsInt() const override { return value; }
    
    /** @brief Gets the printable value as a string. @return The printable string value of the parameter. */
    String getPrintValueAsString() const override;
    
private:
    ToggleState value = INACTIVE;                 /**< The current toggle state of the parameter. */
    std::unique_ptr<String[]> toggleStateNames;   /**< Array of toggle state names. */
};


// =======================================================================================
// MARK: - AUDIO PARAMETER GROUP
// =======================================================================================

/**
 * @class AudioParameterGroup
 * @brief Manages and organizes a group of audio parameters by type.
 *
 * The `AudioParameterGroup` class allows for the efficient management and organization
 * of audio parameters by grouping them together. This class supports various types of
 * parameters, such as sliders, buttons, and choices, and provides easy methods for
 * adding, retrieving, and managing these parameters within the group.
 */
class AudioParameterGroup
{
public:
    /** Deleted default constructor to enforce parameterized construction. */
    AudioParameterGroup() = delete;

    /**
     * @brief Constructs an AudioParameterGroup with a specified ID and size.
     * @param id_ The ID of the parameter group.
     * @param size_ The number of parameters that the group can hold.
     */
    AudioParameterGroup(const String id_, const size_t size_);

    /** Destructor for AudioParameterGroup. */
    ~AudioParameterGroup();
        
    /**
     * @brief Adds a parameter to the group.
     *
     * This template function allows you to add different types of parameters to the group.
     * @tparam ParameterType The type of the parameter to add.
     * @tparam Args The types of arguments to forward to the parameter's constructor.
     * @param index_ The index of the parameter within the group.
     * @param id_ The ID of the parameter.
     * @param name_ The name of the parameter.
     * @param args_ The arguments to forward to the parameter's constructor.
     */
    template<typename ParameterType, typename... Args>
    void addParameter(const uint index_, const String& id_, const String& name_, Args&&... args_)
    {
        int nextFreeIndex = 0;

        // search for the next free slot in the vector
        while (parameterGroup[nextFreeIndex] != nullptr)
        {
            ++nextFreeIndex;
            
            // return if no free slot is found
            if (nextFreeIndex >= parameterGroup.size())
            {
                engine_rt_error("This AudioParameterGroup (" + id + ") is already full!",
                                __FILE__, __LINE__, false);
                return;
            }
        }

        // if a free slot is found, create a new parameter with specified type and arguments
        parameterGroup[nextFreeIndex] = new ParameterType(index_, id_, name_, std::forward<Args>(args_)...);
    }
    
    /**
     * @brief Retrieves a parameter by its index within the group.
     * @param index_ The index of the parameter in the group.
     * @return A pointer to the requested AudioParameter.
     */
    AudioParameter* getParameter(const unsigned int index_);
    
    /**
     * @brief Retrieves a parameter by its ID.
     * @param id_ The ID of the parameter.
     * @return A pointer to the requested AudioParameter.
     */
    AudioParameter* getParameter(const String id_);
    
    /** @brief Gets the ID of the parameter group. @return The ID of the parameter group. */
    String getID() const { return id; }

    /** @brief Gets the number of parameters in the group. @return The number of parameters in the group. */
    size_t getNumParametersInGroup() const { return parameterGroup.size(); }
    
private:
    const String id; /**< The ID of the parameter group. */
    std::vector<AudioParameter*> parameterGroup; /**< A vector containing the parameters in the group. */
};

#endif /* parameters_hpp */
