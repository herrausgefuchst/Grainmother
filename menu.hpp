#ifndef menu_hpp
#define menu_hpp

#include "functions.h"
#include "uielements.hpp"
#include "parameters.hpp"
#include "globals.h"

class Menu : public UIElement::Listener
{
public:
    
// MARK: - MENU::PAGE
// ********************************************************************************
    
    class Page
    {
    public:
        enum ID { HOME, SAVE, SETTINGS, SETMIDIIN, SETMIDIOUT, SETPOTBEHAVIOUR };
        
        struct Item
        {
            Item (const int _id, const String _name) : id(_id), name(_name) {}
            
            const int id;
            String name;
        };
        
        Page(Menu& menu_) : menu(menu_) {};
        Page (const String _id, const String _name, Menu& _menu, const int _currentChoice = 0);
        virtual ~Page();
        
        std::function<void()> onUp, onDown, onEnter, onExit;
        
        void addItem (const int _id, const String _name);
        
        virtual void up();
        virtual void down();
        virtual void enter();
        virtual void exit();
        
        void setCurrentChoice (const int _index) { currentChoice = _index; }
        void setItemName (const String _name, const int _index) { items[_index]->name = _name; }
        
        String getName() const { return name; }
        String getID() const { return id; }
        virtual String getCurrentPrintValue() const { return items[currentChoice]->name; }
        int getCurrentChoice() const { return currentChoice; }
        int getNumChoices() const { return numChoices;}
        std::vector<Item*> getItems() { return items; }
        
    protected:
        Menu& menu;
        String id, name;
        
        int numChoices = 0;
        int currentChoice = 0;
        
        std::vector<Item*> items;
    };
    
    class ParameterPage : public Page
    {
    public:
        ParameterPage(const String id_, AudioParameter* param_, Menu& menu_) 
            : Page(menu_)
        {
            id = id_;
            name = param_->getName();
            
            parameter = param_;
        }
        
        ~ParameterPage() {}
        
        void up() override
        {
            parameter->nudgeValue(1);
            
            #ifdef CONSOLE_PRINT
            consoleprint("Menu Page: '" + name + "', Value: '" + getCurrentPrintValue(), __FILE__, __LINE__);
            #endif
            
            if (onUp) onUp();
        }
        
        void down() override
        {
            parameter->nudgeValue(-1);
            
            #ifdef CONSOLE_PRINT
            consoleprint("Menu Page: '" + name + "', Value: '" + getCurrentPrintValue(), __FILE__, __LINE__);
            #endif
            
            if (onDown) onDown();
        }
        
        String getCurrentPrintValue() const override
        {
            return TOSTRING(parameter->getPrintValueAsFloat());
        }
        
    private:
        AudioParameter* parameter = nullptr;
    };
    
    class NavigationPage : public Page
    {
    public:
        NavigationPage(const String id_, std::initializer_list<Page*> options_, Menu& menu_) : Page(menu_) {}
        
    private:
        std::vector<Page*> options;
    };
    
    class GlobalSettingPage : public Page
    {
        
    };
    
    class PresetPage : public Page
    {
        
    };

    
// MARK: - MENU
// ********************************************************************************
    
    Menu() {}
    void setup(GlobalParameters* _globals);
    ~Menu ();
        
    void buttonClicked (UIElement* _uielement) override;
    void buttonPressed (UIElement* _uielement) override;
    void buttonReleased (UIElement* _uielement) override;
    
    void scroll (const int _direction);
        
    class Listener
    {
    public:
        virtual ~Listener() {}
        
        virtual void menupageSelected (Page* _page) = 0;
    };

    void addListener (Listener* _listener);
    std::vector<std::function<void()>> onSaveMessage;
    std::vector<std::function<void()>> onLoadMessage;
    
    void addPage(const String id_, AudioParameter* param_);
    
    void setPage (const int _index, const bool _withCopyChoice = false);
    void setNewPresetName (const String _name);
    void setBypass (const bool _bypass) { bypass = _bypass; }
    
    int getCurrentChoice() const{ return currentPage->getCurrentChoice(); }
    String getCurrentPageID() const { return currentPage->getID(); }
    String getCurrentPageName() const { return currentPage->getName(); }
    int getCurrentPreset() const { return pages[Page::HOME]->getCurrentChoice(); }
    
private:
    inline void print();

    void loadPreset();
    void savePreset();
    void saveSetting();

protected:
    Page* currentPage = nullptr;
    
private:
    GlobalParameters* globals;
    
    std::vector<Page*> pages;
    std::vector<Listener*> listeners;
    
    bool isScrolling = false;
    bool bypass = false; // TODO: why is this here?

    //TODO: scrolling function?
};


#endif /* menu_hpp */
