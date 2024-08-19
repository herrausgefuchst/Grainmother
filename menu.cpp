#include "menu.hpp"

// MARK: - MENU::PAGE
// ********************************************************************************

Menu::Page::Page (const String _id, const String _name, Menu& _menu, const int _currentChoice)
    : menu(_menu)
    , id(_id)
    , name(_name)
    , currentChoice(_currentChoice)
{}

Menu::Page::~Page()
{
    for (auto i : items) delete i;
    items.clear();
}

void Menu::Page::addItem (const int _id, const String _name)
{
    items.push_back(new Item(_id, _name));
    ++numChoices;
}

void Menu::Page::up()
{
    if (--currentChoice < 0) currentChoice += numChoices;
    
    consoleprint("Menu Page: '" + name + "', Choice: '" + items[currentChoice]->name + "'", __FILE__, __LINE__);
    
    if (onUp) onUp();
}

void Menu::Page::down()
{
    if (++currentChoice >= numChoices) currentChoice -= numChoices;
    
    consoleprint("Menu Page: '" + name + "', Choice: '" + items[currentChoice]->name + "'", __FILE__, __LINE__);
    
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

void Menu::setup(GlobalParameters* _globals)
{
    globals = _globals;
    
    // -- Additional Reverb Parameters
    getPage("reverb_lowcut")->addParent(getPage("reverb_additionalParameters"));
    getPage("reverb_multfreq")->addParent(getPage("reverb_additionalParameters"));
    getPage("reverb_multgain")->addParent(getPage("reverb_additionalParameters"));
    
    // -- Global Settings
    // pages for the different settings
    addPage("midi_in_channel", "MIDI Input Channel", 1, 16, nullptr);
    addPage("midi_out_channel", "MIDI Output Channel", 1, 16, nullptr);
    addPage("pot_behaviour", "Potentiometer Behaviour", 0, 1, { "Jump", "Catch" });
    
    // page for navigating through the settings
    addPage("global_settings", "Global Settings", {
        getPage("midi_in_channel"),
        getPage("midi_out_channel"),
        getPage("pot_behaviour")
    });
    
    // define navigator as parent for the settings pages
    getPage("midi_in_channel")->addParent(getPage("global_settings"));
    getPage("midi_out_channel")->addParent(getPage("global_settings"));
    getPage("pot_behaviour")->addParent(getPage("global_settings"));
    
    // -- ActionMenu
    addPage("menu", "Menu", {
        getPage("global_settings"),
        getPage("reverb_additionalParameters"),
    });
    
    getPage("global_settings")->addParent(getPage("menu"));
    getPage("reverb_additionalParameters")->addParent(getPage("menu"));
    
    // -- Home
    addPage("load_preset", "Home", 0, NUM_PRESETS-1, presetNames);
    
    // -- Save Preset To?
    addPage("save_preset", "Save Preset to Slot: ", 0, NUM_PRESETS-1, presetNames);
    // TODO: add naming of preset
    getPage("save_preset")->addParent(getPage("load_preset"));
    getPage("menu")->addParent(getPage("load_preset"));
    
    // -- set start page
    setCurrentPage("load_preset");
    
    Page* homePage = getPage("load_preset");
    homePage->onUp = [this] { loadPreset(); };
    homePage->onDown = [this] { loadPreset(); };
    homePage->onExit = [this] { setCurrentPage("menu"); };
    homePage->onEnter = [this] { setCurrentPage("save_preset"); };
    
    Page* savePage = getPage("save_preset");
    savePage->onEnter = [this] { savePreset(); };
    
    getPage("midi_in_channel")->onEnter = [this] { saveSetting(); };
    
    
    
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

void Menu::addPage(const String id_, const String name_, const size_t min_, const size_t max_, String* choiceNames_)
{
    pages.push_back(new GlobalSettingPage(id_, name_, min_, max_, *this, choiceNames_));
}

void Menu::addPage(const String id_, const String name_, const size_t min_, const size_t max_, std::initializer_list<String> choiceNames_)
{
    String* choiceNamesPtr = choiceNames_.size() > 0 ? const_cast<String*>(&*choiceNames_.begin()) : nullptr;
    
    pages.push_back(new GlobalSettingPage(id_, name_, min_, max_, *this, choiceNamesPtr));
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
    for (auto i : onLoadMessage) i();
}

void Menu::savePreset()
{
    for (auto i : onSaveMessage) i();
}

void Menu::saveSetting()
{
    if (currentPage->getID() == "midiin") globals->midiInChannel = currentPage->getCurrentChoice()+1;
    else if (currentPage->getID() == "midiout") globals->midiOutChannel = currentPage->getCurrentChoice()+1;
    else if (currentPage->getID() == "potbehaviour") globals->potBehaviour = currentPage->getCurrentChoice();
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
            if (currentPage == pages[Page::HOME]) loadPreset();
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

void Menu::setCurrentPage (const int _index, const bool _withCopyChoice)
{
    if (_withCopyChoice) pages[_index]->setCurrentChoice(currentPage->getCurrentChoice());
    
    currentPage = pages[_index];
    
    for (auto i : listeners) i->menupageSelected(currentPage);
    
    print();
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
    pages[Page::HOME]->setItemName(_name, currentPage->getCurrentChoice());
    pages[Page::SAVE]->setItemName(_name, currentPage->getCurrentChoice());
}

void Menu::addListener (Listener* _listener)
{
    listeners.push_back(_listener);
}

