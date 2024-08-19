#include "menu.hpp"

// MARK: - MENU::PAGE
// *******************************************************************************

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

// MARK: - MENU
// ********************************************************************************

void Menu::setup(std::array<AudioParameterGroup*, NUM_PARAMETERGROUPS> programParameters_)
{
    programParameters = programParameters_;
    
    initializeJSON();
    
    initializePages();
    
    initializePageHierarchy();
    
    initializePageActions();
    
    loadPreset();
    
    // -- set start page
    setCurrentPage("load_preset");
}

inline void Menu::initializeJSON()
{
//#ifdef JSON_USED
    std::ifstream readfilePresets;
    std::ifstream readfileGlobals;
    
#ifndef BELA_CONNECTED
    readfilePresets.open("/Users/julianfuchs/Dropbox/BelaProjects/GrainMother/GrainMother/presets.json");
    readfileGlobals.open("/Users/julianfuchs/Dropbox/BelaProjects/GrainMother/GrainMother/globals.json");
#else
    readfilePresets.open("presets.json");
    readfileGlobals.open("globals.json");
#endif
    
    engine_error(!readfilePresets.is_open(), "presets.json not found, therefore not able to open", __FILE__, __LINE__, true);
    engine_error(!readfileGlobals.is_open(), "globals.json not found, therefore not able to open", __FILE__, __LINE__, true);
    
    JSONpresets = json::parse(readfilePresets);
    JSONglobals = json::parse(readfileGlobals);
//#endif
}

void Menu::initializePages()
{
    // -- Additional Reverb Parameters
    getPage("reverb_lowcut")->addParent(getPage("reverb_additionalParameters"));
    getPage("reverb_multfreq")->addParent(getPage("reverb_additionalParameters"));
    getPage("reverb_multgain")->addParent(getPage("reverb_additionalParameters"));
    
    // -- Global Settings
    // pages for the different settings
    addPage("midi_in_channel", "MIDI Input Channel", 1, 16, (size_t)JSONglobals["midiInChannel"] - 1, nullptr);
    addPage("midi_out_channel", "MIDI Output Channel", 1, 16, (size_t)JSONglobals["midiOutChannel"] - 1, nullptr);
    addPage("pot_behaviour", "Potentiometer Behaviour", 0, 1, (size_t)JSONglobals["potBehaviour"], { "Jump", "Catch" });
    
    // -- Global Settings
    // page for navigating through the settings
    addPage("global_settings", "Global Settings", {
        getPage("midi_in_channel"),
        getPage("midi_out_channel"),
        getPage("pot_behaviour")
    });
    
    // -- Overall Menu
    addPage("menu", "Menu", {
        getPage("global_settings"),
        getPage("reverb_additionalParameters"),
    });
    
    // retrieve preset names from JSON
    String presetName[NUM_PRESETS];
    for (unsigned int n = 0; n < NUM_PRESETS; ++n)
        presetName[n] = JSONpresets[n]["name"];
    
    // -- Home / Load and Show Preset
    addPage("load_preset", "Home", 0, NUM_PRESETS-1, (size_t)JSONglobals["lastUsedPreset"], presetName);
    
    // -- Save Preset To?
    addPage("save_preset", "Save Preset to Slot: ", 0, NUM_PRESETS-1, (size_t)JSONglobals["lastUsedPreset"], presetName);
}

void Menu::initializePageHierarchy()
{
    // -- Global Settings
    getPage("midi_in_channel")->addParent(getPage("global_settings"));
    getPage("midi_out_channel")->addParent(getPage("global_settings"));
    getPage("pot_behaviour")->addParent(getPage("global_settings"));
    
    // -- Overall Menu
    getPage("global_settings")->addParent(getPage("menu"));
    getPage("reverb_additionalParameters")->addParent(getPage("menu"));

    // -- Home screen
    getPage("save_preset")->addParent(getPage("load_preset"));
    getPage("menu")->addParent(getPage("load_preset"));
}

void Menu::initializePageActions()
{
    // Home Page
    Page* homePage = getPage("load_preset");
    homePage->onUp = [this] { loadPreset(); };
    homePage->onDown = [this] { loadPreset(); };
    homePage->onExit = [this] { setCurrentPage("menu"); };
    homePage->onEnter = [this] { setCurrentPage("save_preset"); };
    
    // Save Page
    Page* savePage = getPage("save_preset");
    savePage->onEnter = [this] { savePreset(); };
    
    // Global Settings
    getPage("midi_in_channel")->onUp = [this] { for (auto i : listeners) i->globalSettingChanged(currentPage); };
    getPage("midi_in_channel")->onDown = [this] { for (auto i : listeners) i->globalSettingChanged(currentPage); };
    getPage("midi_out_channel")->onUp = [this] { for (auto i : listeners) i->globalSettingChanged(currentPage); };
    getPage("midi_out_channel")->onDown = [this] { for (auto i : listeners) i->globalSettingChanged(currentPage); };
    getPage("pot_behaviour")->onUp = [this] { for (auto i : listeners) i->globalSettingChanged(currentPage); };
    getPage("pot_behaviour")->onDown = [this] { for (auto i : listeners) i->globalSettingChanged(currentPage); };
}

Menu::~Menu()
{
    for (auto i : pages) delete i;
    pages.clear();
}

void Menu::addPage(const String id_, AudioParameter* param_)
{
    pages.push_back(new ParameterPage(id_, param_, *this));
}

void Menu::addPage(const String id_, const String name_, std::initializer_list<Page*> options_)
{
    pages.push_back(new NavigationPage(id_, name_, options_, *this));
}

void Menu::addPage(const String id_, const String name_, const size_t min_, const size_t max_, const size_t default_, String* choiceNames_)
{
    pages.push_back(new GlobalSettingPage(id_, name_, min_, max_, *this, default_, choiceNames_));
}

void Menu::addPage(const String id_, const String name_, const size_t min_, const size_t max_, const size_t default_, std::initializer_list<String> choiceNames_)
{
    String* choiceNamesPtr = choiceNames_.size() > 0 ? const_cast<String*>(&*choiceNames_.begin()) : nullptr;
    
    pages.push_back(new GlobalSettingPage(id_, name_, min_, max_, *this, default_, choiceNamesPtr));
}

Menu::Page* Menu::getPage(const String id_)
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

inline void Menu::print()
{
    consoleprint("Menu Page: '" + currentPage->getName() + "', Value: '" + currentPage->getCurrentPrintValue() + "'", __FILE__, __LINE__);
}

void Menu::loadPreset()
{
    // parameters
    auto engine = programParameters[0];
    auto effect1= programParameters[1];
    auto effect2 = programParameters[2];
//    auto effect3 = programParameters[3];
    
    // console print yes or no?
    bool withPrint = true;

    // index
    size_t index = getPage("load_preset")->getCurrentChoice();
    
    // load from JSON file
    for (unsigned int n = 0; n < engine->getNumParametersInGroup(); n++)
        engine->getParameter(n)->setValue((float)JSONpresets[index]["engine"][n], withPrint);
    
    for (unsigned int n = 0; n < effect1->getNumParametersInGroup(); n++)
        effect1->getParameter(n)->setValue((float)JSONpresets[index]["effect1"][n], withPrint);
    
    for (unsigned int n = 0; n < effect2->getNumParametersInGroup(); n++)
        effect2->getParameter(n)->setValue((float)JSONpresets[index]["effect2"][n], withPrint);
    
//    for (unsigned int n = 0; n < effect3->getNumParametersInGroup(); n++)
//        effect3->getParameter(n)->setValue((float)JSONpresets[index]["effect3"][n], withPrint);
    
    // last used preset is now the current one
    lastUsedPresetIndex = index;
    
    // update Display Catch
//    display.setPresetCatch(index, JSONpresets[index]["name"]);

    #ifdef CONSOLE_PRINT
    consoleprint("Loaded preset with name " + getPage("load_preset")->getCurrentPrintValue() + " from JSON!", __FILE__, __LINE__);
    #endif
    
    for (auto i : onLoadMessage) i();
}

void Menu::savePreset()
{
    for (auto i : onSaveMessage) i();
}

void Menu::buttonClicked (UIElement* _uielement)
{
    Button* button = static_cast<Button*>(_uielement);
            
    switch (button->getIndex())
    {
        case ButtonID::UP:
        {
            if (!bypass)
            {
                currentPage->up();
                for (auto i : listeners) i->menupageSelected(currentPage);
            }
            else bypass = !bypass;
            break;
        }
        case ButtonID::DOWN:
        {
            if (!bypass)
            {
                currentPage->down();
                for (auto i : listeners) i->menupageSelected(currentPage);
            }
            else bypass = !bypass;
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
            isScrolling = true;
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

void Menu::scroll (const int _direction)
{
    if (_direction >= 0) currentPage->up();
    else currentPage->down();
}

void Menu::setCurrentPage(Menu::Page* page_)
{
    currentPage = page_;
    
    print();
}

void Menu::setCurrentPage(const String id_)
{
    currentPage = getPage(id_);
    
    print();
}

void Menu::setNewPresetName (const String _name)
{
//    pages[Page::HOME]->setItemName(_name, currentPage->getCurrentChoice());
//    pages[Page::SAVE]->setItemName(_name, currentPage->getCurrentChoice());
}

void Menu::addListener (Listener* _listener)
{
    listeners.push_back(_listener);
}

