#ifndef menu_hpp
#define menu_hpp

#include <fstream>

#include "functions.h"
#include "uielements.hpp"
#include "parameters.hpp"

#include "json.h"
using json = nlohmann::json;

class Menu : public UIElement::Listener
{
public:

    // =======================================================================================
    // MARK: - MENU::PAGE
    // =======================================================================================
    
    class Page
    {
    public:
        Page(Menu& menu_) : menu(menu_) {};
        virtual ~Page() {}
        
        void addParent(Page* parent_) { parent = parent_; }
        
        std::function<void()> onUp, onDown, onEnter, onExit;
        
        virtual void up();
        virtual void down();
        virtual void enter();
        virtual void exit();
        
        virtual void setCurrentChoice (const size_t index_) {}
        
        String getName() const { return name; }
        String getID() const { return id; }
        virtual String getCurrentPrintValue() const { return ""; }
        virtual size_t getCurrentChoice() const { return 0; }
        
    protected:
        Menu& menu;
        String id, name;
        Page* parent = nullptr;
    };
    
    // =======================================================================================
    // MARK: - MENU::PAGE::PARAMETERPAGE
    // =======================================================================================
    
    class ParameterPage : public Page
    {
    public:
        ParameterPage(const String& id_, AudioParameter* param_, Menu& menu_);
        
        void up() override;
        void down() override;
        
        String getCurrentPrintValue() const override;
        
    private:
        AudioParameter* parameter = nullptr;
    };
    
    // =======================================================================================
    // MARK: - MENU::PAGE::NAVIGATIONPAGE
    // =======================================================================================
    
    class NavigationPage : public Page
    {
    public:
        NavigationPage(const String& id_, const String& name_, std::initializer_list<Page*> options_, Menu& menu_);
        
        void up() override;
        void down() override;
        void enter() override;
        
        String getCurrentPrintValue() const override;
        
        void setCurrentChoice(const size_t index_) override;
        
    private:
        std::vector<Page*> options;
        size_t choiceIndex = 0;
    };
    
    // =======================================================================================
    // MARK: - MENU::PAGE::SETTINGPAGE
    // =======================================================================================
    
    class SettingPage : public Page
    {
    public:
        SettingPage(const String& id_, const String& name_, size_t min_, size_t max_, size_t defaultIndex_, String* choiceNames_, Menu& menu_);
        SettingPage(const String& id_, const String& name_, size_t min_, size_t max_, size_t defaultIndex_, std::initializer_list<String> choiceNames_, Menu& menu_);
        
        void up() override;
        void down() override;
        
        String getCurrentPrintValue() const override;
        size_t getCurrentChoice() const override;
    
        void setCurrentChoice(const size_t index_) override;
        
    private:
        size_t min, max;
        size_t choiceIndex = 0;
        std::vector<String> choiceNames;
    };

    // =======================================================================================
    // MARK: - MENU
    // =======================================================================================
    
    Menu() {}
    ~Menu ();
        
    void setup(std::array<AudioParameterGroup*, NUM_PARAMETERGROUPS> programParameters_);
    
    template<typename PageType, typename... Args>
    void addPage(const String& id_, Args&&... args)
    {
        pages.push_back(new PageType(id_, std::forward<Args>(args)..., *this));
    }
    
    Page* getPage(const String& id_);
    
    void setCurrentPage(Page* page_);
    void setCurrentPage(const String& id_);
    
    void scroll();
    
    void buttonClicked(UIElement* uielement_) override;
    void buttonPressed(UIElement* uielement_) override;
    void buttonReleased(UIElement* uielement_) override;
        
    class Listener
    {
    public:
        virtual ~Listener() {}
        
        virtual void menuPageChanged(Page* page_) {}
        
        virtual void globalSettingChanged(Page* page_) {}
        
        virtual void effectOrderChanged() {}
    };

    void addListener(Listener* listener_);
    
    std::vector<std::function<void()>> onSaveMessage;
    std::vector<std::function<void()>> onLoadMessage;
    
private:
    void initializePages();
    void initializePageHierarchy();
    void initializePageActions();
    void initializeJSON();
    
    void print();

    void loadPreset();
    void savePreset();

protected:
    Page* currentPage = nullptr;
    
private:
    std::vector<Page*> pages;
    std::vector<Listener*> listeners;

    json JSONpresets;
    json JSONglobals;
    
    std::array<AudioParameterGroup*, NUM_PARAMETERGROUPS> programParameters;
    size_t lastUsedPresetIndex = 0;
    
    enum ScrollDirection { DOWN, UP };
    ScrollDirection scrollDirection;
    
public:
    bool isScrolling = false;
};


#endif /* menu_hpp */
