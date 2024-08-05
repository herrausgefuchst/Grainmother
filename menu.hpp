#ifndef menu_hpp
#define menu_hpp

#include "functions.h"
#include "uielements.hpp"
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
        
        Page (const String _id, const String _name, Menu& _menu, const int _currentChoice = 0);
        ~Page();
        
        std::function<void()> onUp, onDown, onEnter, onExit;
        
        void addItem (const int _id, const String _name);
        
        void up();
        void down();
        void enter();
        void exit();
        
        void setCurrentChoice (const int _index) { currentChoice = _index; }
        void setItemName (const String _name, const int _index) { items[_index]->name = _name; }
        
        String getName() const { return name; }
        String getID() const { return id; }
        String getCurrentItemName() const { return items[currentChoice]->name; }
        int getCurrentChoice() const { return currentChoice; }
        int getNumChoices() const { return numChoices;}
        std::vector<Item*> getItems() { return items; }
        
    private:
        Menu& menu;
        const String id, name;
        
        int numChoices = 0;
        int currentChoice = 0;
        
        std::vector<Item*> items;
    };

    
// MARK: - MENU
// ********************************************************************************
    
    Menu() = delete;
    Menu (GlobalParameters* _globals);
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
