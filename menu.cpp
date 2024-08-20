#include "menu.hpp"

// =======================================================================================
// MARK: - MENU::PAGE
// =======================================================================================


void Menu::Page::up()
{
    if (onUp) onUp();
}


void Menu::Page::down()
{
    if (onDown) onDown();
}


void Menu::Page::enter()
{
    if (parent) menu.setCurrentPage(parent);
    
    if (onEnter) onEnter();
}


void Menu::Page::exit()
{
    if (parent) menu.setCurrentPage(parent);
    
    if (onExit) onExit();
}


// =======================================================================================
// MARK: - MENU::PAGE::PARAMETERPAGE
// =======================================================================================


Menu::ParameterPage::ParameterPage(const String& id_, AudioParameter* param_, Menu& menu_)
    : Page(menu_)
{
    id = id_;
    name = param_->getName();
    
    parameter = param_;
}


void Menu::ParameterPage::up()
{
    parameter->nudgeValue(1);
    
    #ifdef CONSOLE_PRINT
    consoleprint("Menu Page: '" + name + "', Value: '" + getCurrentPrintValue(), __FILE__, __LINE__);
    #endif
    
    if (onUp) onUp();
}


void Menu::ParameterPage::down()
{
    parameter->nudgeValue(-1);
    
    #ifdef CONSOLE_PRINT
    consoleprint("Menu Page: '" + name + "', Value: '" + getCurrentPrintValue(), __FILE__, __LINE__);
    #endif
    
    if (onDown) onDown();
}


String Menu::ParameterPage::getCurrentPrintValue() const
{
    return parameter->getPrintValueAsString();
}


// =======================================================================================
// MARK: - MENU::PAGE::NAVIGATIONPAGE
// =======================================================================================


Menu::NavigationPage::NavigationPage(const String& id_, const String& name_, std::initializer_list<Page*> options_, Menu& menu_)
    : Page(menu_)
    , options(options_)
{
    id = id_;
    name = name_;
}


void Menu::NavigationPage::up()
{
    choiceIndex = (choiceIndex == 0) ? options.size() - 1 : choiceIndex - 1;
    
    #ifdef CONSOLE_PRINT
    consoleprint("Menu Page: '" + name + "', Value: '" + getCurrentPrintValue(), __FILE__, __LINE__);
    #endif
    
    if (onUp) onUp();
}


void Menu::NavigationPage::down()
{
    choiceIndex = (choiceIndex >= options.size() - 1) ? 0 : choiceIndex + 1;
    
    #ifdef CONSOLE_PRINT
    consoleprint("Menu Page: '" + name + "', Value: '" + getCurrentPrintValue(), __FILE__, __LINE__);
    #endif
    
    if (onDown) onDown();
}


void Menu::NavigationPage::enter()
{
    menu.setCurrentPage(options[choiceIndex]);
    
    if (onEnter) onEnter();
}


String Menu::NavigationPage::getCurrentPrintValue() const
{
    return options[choiceIndex]->getName();
}


void Menu::NavigationPage::setCurrentChoice(const size_t index_)
{
    choiceIndex = index_;
}


// =======================================================================================
// MARK: - MENU::PAGE::SETTINGPAGE
// =======================================================================================


Menu::SettingPage::SettingPage(const String& id_, const String& name_, size_t min_, size_t max_, size_t defaultIndex_, String* choiceNames_, Menu& menu_)
    : Page(menu_)
    , min(min_), max(max_)
{
    id = id_;
    name = name_;
    
    if (choiceNames_) choiceNames.assign(choiceNames_, choiceNames_+ (max-min+1));
    else for(unsigned int n = 0; n < (max-min+1); ++n) choiceNames.push_back(TOSTRING(min+n));
    
    choiceIndex = defaultIndex_;
}


Menu::SettingPage::SettingPage(const String& id_, const String& name_, size_t min_, size_t max_, size_t defaultIndex_, std::initializer_list<String> choiceNames_, Menu& menu)
    : Page(menu)
    , min(min_), max(max_)
{
    String* choiceNamesPtr = choiceNames_.size() > 0 ? const_cast<String*>(&*choiceNames_.begin()) : nullptr;
    
    id = id_;
    name = name_;
    
    if (choiceNamesPtr) choiceNames.assign(choiceNamesPtr, choiceNamesPtr + (max-min+1));
    else for(unsigned int n = 0; n < (max-min+1); ++n) choiceNames.push_back(TOSTRING(min+n));
    
    choiceIndex = defaultIndex_;
}


void Menu::SettingPage::up()
{
    choiceIndex = (choiceIndex >= choiceNames.size() - 1) ? 0 : choiceIndex + 1;
    
    #ifdef CONSOLE_PRINT
    consoleprint("Menu Page: '" + name + "', Value: '" + getCurrentPrintValue(), __FILE__, __LINE__);
    #endif
    
    if (onUp) onUp();
}


void Menu::SettingPage::down()
{
    choiceIndex = (choiceIndex == 0) ? choiceNames.size() - 1 : choiceIndex - 1;
    
    #ifdef CONSOLE_PRINT
    consoleprint("Menu Page: '" + name + "', Value: '" + getCurrentPrintValue(), __FILE__, __LINE__);
    #endif
    
    if (onDown) onDown();
}


String Menu::SettingPage::getCurrentPrintValue() const
{
    return choiceNames[choiceIndex];
}


size_t Menu::SettingPage::getCurrentChoice() const
{
    return choiceIndex;
}


void Menu::SettingPage::setCurrentChoice(const size_t index_)
{
    choiceIndex = index_;
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
    // -- Reverb Additional Parameters
    addPage<NavigationPage>("reverb_additionalParameters", "Reverb - Additional Parameters", std::initializer_list<Page*>{
        getPage("reverb_lowcut"),
        getPage("reverb_multfreq"),
        getPage("reverb_multgain")
    });
    
    // -- Global Settings
    // pages for the different settings
    addPage<SettingPage>("midi_in_channel", "MIDI Input Channel", 1, 16, (size_t)JSONglobals["midiInChannel"] - 1, nullptr);
    addPage<SettingPage>("midi_out_channel", "MIDI Output Channel", 1, 16, (size_t)JSONglobals["midiOutChannel"] - 1, nullptr);
    addPage<SettingPage>("pot_behaviour", "Potentiometer Behaviour", 0, 1, (size_t)JSONglobals["potBehaviour"], std::initializer_list<String>{ "Jump", "Catch" });
    
    // -- Global Settings
    // page for navigating through the settings
    addPage<NavigationPage>("global_settings", "Global Settings", std::initializer_list<Page*>{
        getPage("midi_in_channel"),
        getPage("midi_out_channel"),
        getPage("pot_behaviour")
    });
    
    // -- Overall Menu
    addPage<NavigationPage>("menu", "Menu", std::initializer_list<Page*>{
        getPage("effect_order"),
        getPage("reverb_additionalParameters"),
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
    
    // -- Home / Load and Show Preset
    addPage<SettingPage>("load_preset", "Home", 0, NUM_PRESETS-1, (size_t)JSONglobals["lastUsedPreset"], presetLoadNames);
    
    // -- Save Preset To? (one element smaller than load page)
    addPage<SettingPage>("save_preset", "Save Preset to Slot: ", 0, NUM_PRESETS-2, 0, presetSaveNames);
}


void Menu::initializePageHierarchy()
{
    // add parents to certain pages
    // defines where to jump back on click of exit button
    
    // -- Reverb - Additional Parameters
    getPage("reverb_lowcut")->addParent(getPage("reverb_additionalParameters"));
    getPage("reverb_multfreq")->addParent(getPage("reverb_additionalParameters"));
    getPage("reverb_multgain")->addParent(getPage("reverb_additionalParameters"));
    
    // -- Global Settings
    getPage("midi_in_channel")->addParent(getPage("global_settings"));
    getPage("midi_out_channel")->addParent(getPage("global_settings"));
    getPage("pot_behaviour")->addParent(getPage("global_settings"));
    
    // -- Overall Menu
    getPage("global_settings")->addParent(getPage("menu"));
    getPage("reverb_additionalParameters")->addParent(getPage("menu"));
    getPage("effect_order")->addParent(getPage("menu"));

    // -- Home screen
    getPage("save_preset")->addParent(getPage("load_preset"));
    getPage("menu")->addParent(getPage("load_preset"));
}


void Menu::initializePageActions()
{
    // define special actions for certain pages

    // Load/Home Page
    // - loading preset if up/down
    // - go the menu if exit
    // - go to save-page and copy the current choice index to it if enter
    Page* homePage = getPage("load_preset");
    homePage->onUp = [this] { loadPreset(); };
    homePage->onDown = [this] { loadPreset(); };
    homePage->onExit = [this] { setCurrentPage("menu"); };
    homePage->onEnter = [this] {
        size_t currentLoadIndex = getPage("load_preset")->getCurrentChoice();
        // since save page is one element smaller than load page 
        // (default preset not overwriteable)
        // we need to make the following change to the index
        size_t currentSaveIndex = (currentLoadIndex == 0) ? 0 : currentLoadIndex-1;
        getPage("save_preset")->setCurrentChoice(currentSaveIndex);
        setCurrentPage("save_preset");
    };
    
    // Save Page
    // - save preset if enter
    Page* savePage = getPage("save_preset");
    savePage->onEnter = [this] { savePreset(); };
    
    // Global Settings
    // - notify listeners if enter
    getPage("midi_in_channel")->onEnter = [this] { for (auto i : listeners) i->globalSettingChanged(currentPage); };
    getPage("midi_out_channel")->onEnter = [this] { for (auto i : listeners) i->globalSettingChanged(currentPage); };
    getPage("pot_behaviour")->onEnter = [this] { for (auto i : listeners) i->globalSettingChanged(currentPage); };
    
    // Menu
    // - reset choice index of menu if exit
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
    engine_error(!writefilePresets.is_open(), "presets.json not found, not able to save presets", __FILE__, __LINE__, true);
    engine_error(!writefileGlobals.is_open(), "globals.json not found, not able to save globals", __FILE__, __LINE__, true);
    
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
    
    for (size_t n = 0; n < pages.size(); ++n)
    {
        if (pages[n]->getID() == id_)
        {
            page = pages[n];
            break;
        }
    }
    
    if (!page)
        engine_rt_error("Menu couldn't find Page with ID: " + id_, 
                        __FILE__, __LINE__, true);
    
    return page;
}


void Menu::setCurrentPage(Menu::Page* page_)
{
    currentPage = page_;
    
    #ifdef CONSOLE_PRINT
    print();
    #endif
}


void Menu::setCurrentPage(const String& id_)
{
    currentPage = getPage(id_);
    
    #ifdef CONSOLE_PRINT
    print();
    #endif
}


void Menu::scroll()
{
    if (scrollDirection == UP) currentPage->up();
    else currentPage->down();
}


void Menu::buttonClicked (UIElement* _uielement)
{
    Button* button = static_cast<Button*>(_uielement);
         
    // TODO: index/id could be more understandable and clrea
    switch (button->getIndex())
    {
        case ButtonID::UP:
        {
            currentPage->up();
            
            for (auto i : listeners) i->menuPageChanged(currentPage);

            break;
        }
        case ButtonID::DOWN:
        {
            currentPage->down();
            
            for (auto i : listeners) i->menuPageChanged(currentPage);
            
            break;
        }
        case ButtonID::EXIT:
        {
            currentPage->exit();
            break;
        }
        case ButtonID::ENTER:
        {
            currentPage->enter();
            break;
        }
        default:
            break;
    }
}


void Menu::buttonPressed (UIElement* _uielement)
{
    Button* button = static_cast<Button*>(_uielement);

    switch (button->getIndex())
    {
        case ButtonID::UP:
        case ButtonID::DOWN:
        {
            if (isoftype<ParameterPage>(currentPage))
            {
                isScrolling = true;
                scrollDirection = (button->getIndex() == ButtonID::UP) ? UP : DOWN;
            }
            break;
        }
        case ButtonID::EXIT:
        {
            if (currentPage->getID() == "load_preset") loadPreset();
            break;
        }
        case ButtonID::ENTER:
        default:
            break;
    }
}


void Menu::buttonReleased (UIElement* _uielement)
{
    Button* button = static_cast<Button*>(_uielement);

    switch (button->getIndex())
    {
        case ButtonID::UP:
        case ButtonID::DOWN:
        {
            isScrolling = false;
        }
        default:
            break;
    }
}


void Menu::addListener (Listener* _listener)
{
    listeners.push_back(_listener);
}


void Menu::loadPreset()
{
    // extract parametergroups (order is fixed!)
    auto engine = programParameters[0];
    auto effect1= programParameters[1];
    auto effect2 = programParameters[2];
//    auto effect3 = programParameters[3];
    
    // console print yes or no? (developping)
    bool withPrint = true;

    // get the index of the currently selected preset
    size_t index = getPage("load_preset")->getCurrentChoice();
    
    // load and set parameters from JSON file
    for (unsigned int n = 0; n < engine->getNumParametersInGroup(); ++n)
        engine->getParameter(n)->setValue((float)JSONpresets[index]["engine"][n], withPrint);
    
    for (unsigned int n = 0; n < effect1->getNumParametersInGroup(); ++n)
        effect1->getParameter(n)->setValue((float)JSONpresets[index]["effect1"][n], withPrint);
    
    for (unsigned int n = 0; n < effect2->getNumParametersInGroup(); ++n)
        effect2->getParameter(n)->setValue((float)JSONpresets[index]["effect2"][n], withPrint);
    
//    for (unsigned int n = 0; n < effect3->getNumParametersInGroup(); ++n)
//        effect3->getParameter(n)->setValue((float)JSONpresets[index]["effect3"][n], withPrint);
    
    // last used preset is now the current one
    lastUsedPresetIndex = index;
    
    // update Display Catch
//    display.setPresetCatch(index, JSONpresets[index]["name"]);

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
//    auto effect3 = programParameters[3];
    
    // save Data to JSON
    for (unsigned int n = 0; n < engine->getNumParametersInGroup(); ++n)
        JSONpresets[index]["engine"][n] = engine->getParameter(n)->getPrintValueAsFloat();
    
    for (unsigned int n = 0; n < effect1->getNumParametersInGroup(); ++n)
        JSONpresets[index]["effect1"][n] = effect1->getParameter(n)->getPrintValueAsFloat();
    
    for (unsigned int n = 0; n < effect2->getNumParametersInGroup(); ++n)
        JSONpresets[index]["effect2"][n] = effect2->getParameter(n)->getPrintValueAsFloat();
    
//    for (unsigned int n = 0; n < effect3->getNumParametersInGroup(); ++n)
//        JSONpresets[index]["effect3"][n] = effect3->getParameter(n)->getPrintValueAsFloat();

    #ifdef CONSOLE_PRINT
    consoleprint("Saved preset with name " + getPage("save_preset")->getCurrentPrintValue() + " to JSON!", __FILE__, __LINE__);
    #endif
    
    // notify listeners
    for (auto i : onSaveMessage) i();
}


inline void Menu::print()
{
    consoleprint("Menu Page: '" + currentPage->getName() + "', Value: '" + currentPage->getCurrentPrintValue() + "'", __FILE__, __LINE__);
}
