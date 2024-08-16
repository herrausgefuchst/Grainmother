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
    
    consoleprint("Menu Page: '" + name + "', Choice: '" + items[currentChoice]->name, __FILE__, __LINE__);
    
    if (onUp) onUp();
}

void Menu::Page::down()
{
    if (++currentChoice >= numChoices) currentChoice -= numChoices;
    
    consoleprint("Menu Page: '" + name + "', Choice: '" + items[currentChoice]->name, __FILE__, __LINE__);
    
    if (onDown) onDown();
}

void Menu::Page::enter()
{
    if (onEnter) onEnter();
}

void Menu::Page::exit()
{
    if (onExit) onExit();
}

// MARK: - MENU
// ********************************************************************************

Menu::Menu (GlobalParameters* _globals)
    : globals(_globals)
{
    // Home Page
    pages.push_back(new Page("home", "Home", *this));
    for (unsigned int n = 0; n < NUM_PRESETS; n++) pages[Page::HOME]->addItem(n, globals->presetNames[n]);
    pages[Page::HOME]->onUp = [this] { loadPreset(); };
    pages[Page::HOME]->onDown = [this] { loadPreset(); };
    pages[Page::HOME]->onExit = [this] { setPage(Page::SETTINGS); };
    pages[Page::HOME]->onEnter = [this] { setPage(Page::SAVE, true); };
    
    // Save Page
    pages.push_back(new Page("save", "Save Preset To?", *this));
    for (unsigned int n = 0; n < NUM_PRESETS; n++) pages[Page::SAVE]->addItem(n, globals->presetNames[n]);
    pages[Page::SAVE]->onExit = [this] { setPage(Page::HOME); };
    pages[Page::SAVE]->onEnter = [this] { savePreset(); };
    
    // Settings Page
    pages.push_back(new Page("settings", "Global Settings", *this));
    pages[Page::SETTINGS]->addItem(0, "Midi In Channel");
    pages[Page::SETTINGS]->addItem(1, "Midi Out Channel");
    pages[Page::SETTINGS]->addItem(2, "Pot Behaviour");
    pages[Page::SETTINGS]->onExit = [this] { setPage(Page::HOME); };
    pages[Page::SETTINGS]->onEnter = [this] { setPage(Page::SETTINGS+1+pages[Page::SETTINGS]->getCurrentChoice()); };
    
    // Setting - Midi In Channel
    pages.push_back(new Page("midiin", "Midi In Channel", *this, globals->midiInChannel-1));
    for (unsigned int n = 1; n < 17; n++) pages[Page::SETMIDIIN]->addItem(n, std::to_string(n));
    pages[Page::SETMIDIIN]->onExit = [this] { setPage(Page::SETTINGS); };
    pages[Page::SETMIDIIN]->onEnter = [this] { saveSetting(); };

    // Setting - Midi Out Channel
    pages.push_back(new Page("midiout", "Midi Out Channel", *this, globals->midiOutChannel-1));
    for (unsigned int n = 1; n < 17; n++) pages[Page::SETMIDIOUT]->addItem(n, std::to_string(n));
    pages[Page::SETMIDIOUT]->onExit = [this] { setPage(Page::SETTINGS); };
    pages[Page::SETMIDIOUT]->onEnter = [this] { saveSetting(); };
    
    // Setting - PotBehaviour
    pages.push_back(new Page("potbehaviour", "Pot Behaviour", *this, globals->potBehaviour));
    pages[Page::SETPOTBEHAVIOUR]->addItem(0, "Jump");
    pages[Page::SETPOTBEHAVIOUR]->addItem(1, "Catch");
    pages[Page::SETPOTBEHAVIOUR]->onExit = [this] { setPage(Page::SETTINGS); };
    pages[Page::SETPOTBEHAVIOUR]->onEnter = [this] { saveSetting(); };
    
    currentPage = pages[Page::HOME];
    
    currentPage->setCurrentChoice(globals->lastUsedPreset);
    
    print();
}

Menu::~Menu()
{
    for (auto i : pages) delete i;
    pages.clear();
}

inline void Menu::print()
{
    consoleprint("Menu Page: '" + currentPage->getName() + "', Choice: '" + currentPage->getCurrentItemName() + "'", __FILE__, __LINE__);
}

void Menu::loadPreset()
{
    for (auto i : onLoadMessage) i();
}

void Menu::savePreset()
{
    for (auto i : onSaveMessage) i();
    
    setPage(Page::HOME, true);
}

void Menu::saveSetting()
{
    if (currentPage->getID() == "midiin") globals->midiInChannel = currentPage->getCurrentChoice()+1;
    else if (currentPage->getID() == "midiout") globals->midiOutChannel = currentPage->getCurrentChoice()+1;
    else if (currentPage->getID() == "potbehaviour") globals->potBehaviour = currentPage->getCurrentChoice();
        
    setPage(Page::SETTINGS);
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

void Menu::setPage (const int _index, const bool _withCopyChoice)
{
    if (_withCopyChoice) pages[_index]->setCurrentChoice(currentPage->getCurrentChoice());
    
    currentPage = pages[_index];
    
    for (auto i : listeners) i->menupageSelected(currentPage);
    
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

