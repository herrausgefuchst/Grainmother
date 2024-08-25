#include "menu.hpp"

#define CONSOLE_PRINT

// =======================================================================================
// MARK: - MENU::PAGE
// =======================================================================================


void Menu::Page::up()
{
    // call notify-function if connected
    if (onUp) onUp();
}


void Menu::Page::down()
{
    // call notify-function if connected
    if (onDown) onDown();
}


void Menu::Page::enter()
{
    // call notify-function if connected
    if (onEnter) onEnter();
    
    // default behaviour: return to parent page if connected
    if (parent) menu.setCurrentPage(parent);
}


void Menu::Page::exit()
{
    // call notify-function if connected
    if (onExit) onExit();
    
    // default behaviour: return to parent page if connected
    if (parent) menu.setCurrentPage(parent);
}


// =======================================================================================
// MARK: - MENU::PAGE::PARAMETERPAGE
// =======================================================================================


Menu::ParameterPage::ParameterPage(const String& id_, AudioParameter* param_, Menu& menu_)
    : Page(menu_, id_, param_->getName())
{
    // initialize parameter pointer
    parameter = param_;
}


void Menu::ParameterPage::up()
{
    // tell parameter to nudge upwards
    parameter->nudgeValue(1);
    
    // console print / display
    menu.display();
    
    // call notify-function if connected
    if (onUp) onUp();
}


void Menu::ParameterPage::down()
{
    // tell parameter to nudge downwards
    parameter->nudgeValue(-1);
    
    // console print / display
    menu.display();
    
    // call notify-function if connected
    if (onDown) onDown();
}


// =======================================================================================
// MARK: - MENU::PAGE::NAVIGATIONPAGE
// =======================================================================================


Menu::NavigationPage::NavigationPage(const String& id_, const String& name_, std::initializer_list<Page*> options_, Menu& menu_)
    : Page(menu_, id_, name_)
    , options(options_)
{
    for (uint n = 0; n < options.size(); ++n)
        choiceNames.push_back(options[n]->getName());
}


void Menu::NavigationPage::up()
{
    // decrement the current index (array index is vice versa user control)
    choiceIndex = (choiceIndex == 0) ? options.size() - 1 : choiceIndex - 1;
    
    // console print / display
    menu.display();
    
    // call notify-function if connected
    if (onUp) onUp();
}


void Menu::NavigationPage::down()
{
    // increment the current index (array index is vice versa user control)
    choiceIndex = (choiceIndex >= options.size() - 1) ? 0 : choiceIndex + 1;
    
    // console print / display
    menu.display();
    
    // call notify-function if connected
    if (onDown) onDown();
}


void Menu::NavigationPage::enter()
{
    // set the currently indexed page
    menu.setCurrentPage(options[choiceIndex]);
    
    // call notify-function if connected
    if (onEnter) onEnter();
}


// =======================================================================================
// MARK: - MENU::PAGE::SETTINGPAGE
// =======================================================================================


Menu::SettingPage::SettingPage(const String& id_, const String& name_,
                               String* choiceNames_, const size_t numChoices_,
                               const size_t defaultIndex_, const size_t minIndex_, 
                               Menu& menu_)
    : Page(menu_, id_, name_)
{
    // insert the names of choices into member vector
    // if an array is passed, use it
    if (choiceNames_)
        choiceNames.assign(choiceNames_, choiceNames_+ numChoices_);
    // if not use the minIndex_ integer as first name, and name the rest by incrementing the index
    else
        for(unsigned int n = 0; n < numChoices_; ++n)
            choiceNames.push_back(TOSTRING(minIndex_+n));
    
    // set start index
    choiceIndex = defaultIndex_;
}


Menu::SettingPage::SettingPage(const String& id_, const String& name_,
                               std::initializer_list<String> choiceNames_, const size_t numChoices_,
                               const size_t defaultIndex_, const size_t minIndex_,
                               Menu& menu_)
    : Page(menu_, id_, name_)
{
    // turn initializer list into string array if valid
    String* choiceNamesPtr = choiceNames_.size() > 0 ? const_cast<String*>(&*choiceNames_.begin()) : nullptr;
    
    // insert the names of choices into member vector
    // if an array is passed, use it
    if (choiceNamesPtr)
        choiceNames.assign(choiceNamesPtr, choiceNamesPtr + numChoices_);
    // if not use the minIndex_ integer as first name, and name the rest by incrementing the index
    else
        for(unsigned int n = 0; n < numChoices_; ++n)
            choiceNames.push_back(TOSTRING(minIndex_+n));
    
    // set start index
    choiceIndex = defaultIndex_;
}


void Menu::SettingPage::up()
{
    // decrement the current index
    choiceIndex = (choiceIndex == 0) ? choiceNames.size() - 1 : choiceIndex - 1;
    
    // console print / display
    menu.display();
    
    // call notify-function if connected
    if (onUp) onUp();
}


void Menu::SettingPage::down()
{
    // increment the current index
    choiceIndex = (choiceIndex >= choiceNames.size() - 1) ? 0 : choiceIndex + 1;
    
    // console print / display
    menu.display();
    
    // call notify-function if connected
    if (onDown) onDown();
}


// =======================================================================================
// MARK: - MENU
// =======================================================================================


void Menu::setup(std::array<AudioParameterGroup*, NUM_PARAMETERGROUPS> programParameters_)
{
    // copy all parameters
    programParameters = programParameters_;
    
    // get JSON files for presets and global settings
    initializeJSON();
    
    // create the menu elements
    initializePages();
    
    // build the menu structure
    initializePageHierarchy();
    
    // assign menu actions
    initializePageActions();
    
    // load the last used preset preset
    loadPreset();
    
    // set start page
    setCurrentPage("load_preset");
}


inline void Menu::initializeJSON()
{
    // get the JSON files for presets and global settings
    std::ifstream readfilePresets;
    std::ifstream readfileGlobals;
    
    // console print - version (developing)
    #ifndef BELA_CONNECTED
    readfilePresets.open("/Users/julianfuchs/Dropbox/BelaProjects/GrainMother/GrainMother/presets.json");
    readfileGlobals.open("/Users/julianfuchs/Dropbox/BelaProjects/GrainMother/GrainMother/globals.json");
    // BELA - version (embedded)
    #else
    readfilePresets.open("presets.json");
    readfileGlobals.open("globals.json");
    #endif
    
    // error if files couldnt be found
    engine_error(!readfilePresets.is_open(), "presets.json not found, therefore not able to load presets", __FILE__, __LINE__, true);
    engine_error(!readfileGlobals.is_open(), "globals.json not found, therefore not able to load globals", __FILE__, __LINE__, true);
    
    // parse through the files and save them in JSON variables
    JSONpresets = json::parse(readfilePresets);
    JSONglobals = json::parse(readfileGlobals);
}


void Menu::initializePages()
{
    // Reverb Additional Parameters
    addPage<NavigationPage>("reverb_additionalParameters", "Reverb", std::initializer_list<Page*>{
        getPage("reverb_lowcut"),
        getPage("reverb_multfreq"),
        getPage("reverb_multgain")
    });
    
    // Global Settings
    // pages for the different settings
    addPage<SettingPage>("midi_in_channel", "MIDI Input Channel", nullptr, 16,
                         (size_t)JSONglobals["midiInChannel"] - 1, 1);
    addPage<SettingPage>("midi_out_channel", "MIDI Output Channel", nullptr, 16,
                         (size_t)JSONglobals["midiOutChannel"] - 1, 1);
    addPage<SettingPage>("pot_behaviour", "Pot Behaviour",
                         std::initializer_list<String>{ "Jump", "Catch" },
                         2, (size_t)JSONglobals["potBehaviour"], 0);
    
    // Global Settings
    // page for navigating through the settings
    addPage<NavigationPage>("global_settings", "Global Settings", std::initializer_list<Page*>{
        getPage("midi_in_channel"),
        getPage("midi_out_channel"),
        getPage("pot_behaviour"),
    });
    
    addPage<NavigationPage>("preset_settings", "Preset Settings", std::initializer_list<Page *>{
        getPage("effect_order"),
        getPage("reverb_additionalParameters"),
        getPage("tempo_set")
    });
    
    // Overall Menu
    addPage<NavigationPage>("menu", "Menu", std::initializer_list<Page*>{
        getPage("preset_settings"),
        getPage("global_settings")
    });
    
    // retrieve preset names from JSON
    // the page for saving presets doesn't include the default preset (index 0)
    String presetLoadNames[NUM_PRESETS];
    String presetSaveNames[NUM_PRESETS-1];
    
    for (unsigned int n = 0; n < NUM_PRESETS; ++n)
        presetLoadNames[n] = JSONpresets[n]["name"];
    
    for (unsigned int n = 1; n < NUM_PRESETS; ++n)
        presetSaveNames[n-1] = presetLoadNames[n];
    
    // Home / Load and Show Preset
    addPage<SettingPage>("load_preset", "Home", presetLoadNames,
                         NUM_PRESETS, (size_t)JSONglobals["lastUsedPreset"], 0);
    
    // Save Preset To? (one element smaller than load page)
    addPage<SettingPage>("save_preset", "Save Preset to Slot: ", presetSaveNames,
                         NUM_PRESETS-1, 0, 0);
}


void Menu::initializePageHierarchy()
{
    // add parents to certain pages
    // defines where to jump back on click of exit button
    
    // Reverb - Additional Parameters
    getPage("reverb_lowcut")->addParent(getPage("reverb_additionalParameters"));
    getPage("reverb_multfreq")->addParent(getPage("reverb_additionalParameters"));
    getPage("reverb_multgain")->addParent(getPage("reverb_additionalParameters"));

    // Global Settings
    getPage("midi_in_channel")->addParent(getPage("global_settings"));
    getPage("midi_out_channel")->addParent(getPage("global_settings"));
    getPage("pot_behaviour")->addParent(getPage("global_settings"));
    
    // Preset Settings
    getPage("reverb_additionalParameters")->addParent(getPage("preset_settings"));
    getPage("effect_order")->addParent(getPage("preset_settings"));
    getPage("tempo_set")->addParent(getPage("preset_settings"));
    
    // Overall Menu
    getPage("global_settings")->addParent(getPage("menu"));
    getPage("preset_settings")->addParent(getPage("menu"));

    // Home screen
    getPage("save_preset")->addParent(getPage("load_preset"));
    getPage("menu")->addParent(getPage("load_preset"));
}


void Menu::initializePageActions()
{
    // define special actions for certain pages

    // Load/Home Page
    // - up/down: loading preset
    // - exit: go the menu
    // - enter: go to save-page and copy the current choice index to it
    Page* homePage = getPage("load_preset");
    homePage->onUp = [this] { loadPreset(); };
    homePage->onDown = [this] { loadPreset(); };
    homePage->onExit = [this] { setCurrentPage("menu"); };
    homePage->onEnter = [this] {
        size_t currentLoadIndex = getPage("load_preset")->getCurrentChoice();
        // since save page is one element smaller than load page 
        // (default preset not overwriteable)
        // we need to make the following adjustement to the index
        size_t currentSaveIndex = (currentLoadIndex == 0) ? 0 : currentLoadIndex-1;
        getPage("save_preset")->setCurrentChoice(currentSaveIndex);
        setCurrentPage("save_preset");
    };
    
    // Save Page
    // - enter: save preset
    Page* savePage = getPage("save_preset");
    savePage->onEnter = [this] {
        getPage("load_preset")->setCurrentChoice(getPage("save_preset")->getCurrentChoice()+1);
        savePreset();
    };
    
    // Global Settings
    // - enter: notify listeners
    getPage("midi_in_channel")->onEnter = [this] {
        for (auto i : listeners) i->globalSettingChanged(currentPage);
    };
    getPage("midi_out_channel")->onEnter = [this] {
        for (auto i : listeners) i->globalSettingChanged(currentPage);
    };
    getPage("pot_behaviour")->onEnter = [this] {
        for (auto i : listeners) i->globalSettingChanged(currentPage);
    };
    
    // Menu
    // - exit: reset choice index of menu
    getPage("menu")->onExit = [this] { getPage("menu")->setCurrentChoice(0); };
    
    // Effect Order
    // - notify engine to change algorithm if enter
    getPage("effect_order")->onEnter = [this] { for (auto i : listeners) i->effectOrderChanged(); };
}


Menu::~Menu()
{
    // get the JSON files for presets and global settings
    // console print - version (developing)
    #ifndef BELA_CONNECTED
    std::ofstream writefilePresets("/Users/julianfuchs/Dropbox/BelaProjects/GrainMother/GrainMother/presets.json");
    std::ofstream writefileGlobals("/Users/julianfuchs/Dropbox/BelaProjects/GrainMother/GrainMother/globals.json");
    // BELA - version (embedded)
    #else
    std::ofstream writefilePresets("presets.json");
    std::ofstream writefileGlobals("globals.json");
    #endif
    
    // error if files couldnt be found
    engine_error(!writefilePresets.is_open(), "presets.json not found, not able to save presets", 
                 __FILE__, __LINE__, true);
    engine_error(!writefileGlobals.is_open(), "globals.json not found, not able to save globals", 
                 __FILE__, __LINE__, true);
    
    // get and save the global settings
    JSONglobals["midiInChannel"] = getPage("midi_in_channel")->getCurrentChoice() + 1;
    JSONglobals["midiOutChannel"] = getPage("midi_out_channel")->getCurrentChoice() + 1;
    JSONglobals["potBehaviour"] = getPage("pot_behaviour")->getCurrentChoice();
    JSONglobals["lastUsedPreset"] = lastUsedPresetIndex;
    
    // overwrite the files
    writefilePresets << JSONpresets.dump(4);
    writefileGlobals << JSONglobals.dump(4);
    
    // delete all page pointers
    for (auto i : pages) delete i;
    pages.clear();
}


Menu::Page* Menu::getPage(const String& id_)
{
    Menu::Page* page = nullptr;
    
    // find the corresponding page comparing the given id to all inherited pages
    for (size_t n = 0; n < pages.size(); ++n)
    {
        if (pages[n]->getID() == id_)
        {
            page = pages[n];
            break;
        }
    }
    
    // if this page isn't existing, stop running
    if (!page)
        engine_rt_error("Menu couldn't find Page with ID: " + id_, 
                        __FILE__, __LINE__, true);
    
    return page;
}


void Menu::setCurrentPage(Menu::Page* page_)
{
    // set new page
    currentPage = page_;
    
    // notify display
    display();
}


void Menu::setCurrentPage(const String& id_)
{
    // get and set page with given id
    currentPage = getPage(id_);
    
    // notify display
    display();
}


void Menu::scroll()
{
    // call the current page to move up or downwards
    if (scrollDirection == UP) currentPage->up();
    else currentPage->down();
}


void Menu::buttonClicked (UIElement* _uielement)
{
    if (!onHold)
    {
        Button* button = static_cast<Button*>(_uielement);
             
        switch (button->getIndex())
        {
            case BUTTON_UP:
            {
                currentPage->up();

                break;
            }
            case BUTTON_DOWN:
            {
                currentPage->down();
                
                break;
            }
            case BUTTON_EXIT:
            {
                currentPage->exit();
                break;
            }
            case BUTTON_ENTER:
            {
                currentPage->enter();
                break;
            }
            default:
                break;
        }
    }
    else onHold = false;
}


void Menu::buttonPressed (UIElement* _uielement)
{
    if (!onHold)
    {
        Button* button = static_cast<Button*>(_uielement);

        switch (button->getIndex())
        {
            case BUTTON_UP:
            case BUTTON_DOWN:
            {
                if (isoftype<ParameterPage>(currentPage))
                {
                    isScrolling = true;
                    scrollDirection = (button->getIndex() == BUTTON_UP) ? UP : DOWN;
                }
                break;
            }
            case BUTTON_EXIT:
            {
                if (currentPage->getID() == "load_preset") loadPreset();
                break;
            }
            case BUTTON_ENTER:
            {
                if (isoftype<ParameterPage>(currentPage))
                {
                    ParameterPage* page = static_cast<ParameterPage*>(currentPage);
                    page->getParameter()->setDefaultValue();
                }
            }
            default:
                break;
        }
    }
}


void Menu::buttonReleased (UIElement* _uielement)
{
    if (!onHold)
    {
        Button* button = static_cast<Button*>(_uielement);

        switch (button->getIndex())
        {
            case BUTTON_UP:
            case BUTTON_DOWN:
            {
                isScrolling = false;
            }
            default:
                break;
        }
    }
    else onHold = false;
}


void Menu::loadPreset()
{
    for (auto i : listeners) i->presetChanged();
    
    // extract parametergroups (order is fixed!)
    auto engine = programParameters[0];
    auto effect1= programParameters[1];
    auto effect2 = programParameters[2];
    //TODO: add third effect
//    auto effect3 = programParameters[3];

    // get the index of the currently selected preset
    size_t index = getPage("load_preset")->getCurrentChoice();
    
    // load and set parameters from JSON file (without display notification)
    for (unsigned int n = 0; n < engine->getNumParametersInGroup(); ++n)
        engine->getParameter(n)->setValue((float)JSONpresets[index]["engine"][n], false);
    
    for (unsigned int n = 0; n < effect1->getNumParametersInGroup(); ++n)
        effect1->getParameter(n)->setValue((float)JSONpresets[index]["effect1"][n], false);
    
    for (unsigned int n = 0; n < effect2->getNumParametersInGroup(); ++n)
        effect2->getParameter(n)->setValue((float)JSONpresets[index]["effect2"][n], false);
    
    //TODO: add third effect
//    for (unsigned int n = 0; n < effect3->getNumParametersInGroup(); ++n)
//        effect3->getParameter(n)->setValue((float)JSONpresets[index]["effect3"][n], withPrint);
    
    // last used preset is now the current one
    lastUsedPresetIndex = index;

    #ifdef CONSOLE_PRINT
    consoleprint("Loaded preset with name " + getPage("load_preset")->getCurrentPrintValue() + " from JSON!", __FILE__, __LINE__);
    #endif
    
    // notify listeners
    for (auto i : onLoadMessage) i();
}


void Menu::savePreset()
{
    // get the index of the selected saving slot
    // +1 because JSON file has the default parameter values on index 0
    // and we dont want to overwrite the default preset
    size_t index = getPage("save_preset")->getCurrentChoice() + 1;
    
    // name
    // TODO: add a new name to the preset
    
    // extract parametergroups (order is fixed!)
    auto engine = programParameters[0];
    auto effect1= programParameters[1];
    auto effect2 = programParameters[2];
    //TODO: add third effect
//    auto effect3 = programParameters[3];
    
    // save Data to JSON
    for (unsigned int n = 0; n < engine->getNumParametersInGroup(); ++n)
        JSONpresets[index]["engine"][n] = engine->getParameter(n)->getPrintValueAsFloat();
    
    for (unsigned int n = 0; n < effect1->getNumParametersInGroup(); ++n)
        JSONpresets[index]["effect1"][n] = effect1->getParameter(n)->getPrintValueAsFloat();
    
    for (unsigned int n = 0; n < effect2->getNumParametersInGroup(); ++n)
        JSONpresets[index]["effect2"][n] = effect2->getParameter(n)->getPrintValueAsFloat();
    
    //TODO: add third effect
//    for (unsigned int n = 0; n < effect3->getNumParametersInGroup(); ++n)
//        JSONpresets[index]["effect3"][n] = effect3->getParameter(n)->getPrintValueAsFloat();

    #ifdef CONSOLE_PRINT
    consoleprint("Saved preset with name " + getPage("save_preset")->getCurrentPrintValue() + " to JSON!", __FILE__, __LINE__);
    #endif
    
    // notify listeners
    for (auto i : onSaveMessage) i();
}


inline void Menu::display()
{
    #ifdef CONSOLE_PRINT
    consoleprint("Menu Page: '" + currentPage->getName() + "', Value: '" + currentPage->getCurrentPrintValue() + "'", __FILE__, __LINE__);
    #endif
    
    // notify display
    if(onPageChange) onPageChange();
}
