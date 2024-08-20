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
    
// MARK: - MENU::PAGE
// ********************************************************************************
    
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
        
        void setCurrentChoice (const size_t index_) {}
        
        String getName() const { return name; }
        String getID() const { return id; }
        virtual String getCurrentPrintValue() const { return ""; }
        virtual size_t getCurrentChoice() const { return 0; }
        
    protected:
        Menu& menu;
        String id, name;
        Page* parent = nullptr;
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
            return parameter->getPrintValueAsString();
        }
        
    private:
        AudioParameter* parameter = nullptr;
    };
    
    class NavigationPage : public Page
    {
    public:
        NavigationPage(const String id_, const String name_, std::initializer_list<Page*> options_, Menu& menu_)
            : Page(menu_)
            , options(options_)
        {
            id = id_;
            name = name_;
        }
        
        void up() override
        {
            choiceIndex = (choiceIndex >= options.size() - 1) ? 0 : choiceIndex + 1;
            
            #ifdef CONSOLE_PRINT
            consoleprint("Menu Page: '" + name + "', Value: '" + getCurrentPrintValue(), __FILE__, __LINE__);
            #endif
            
            if (onUp) onUp();
        }
        
        void down() override
        {
            choiceIndex = (choiceIndex == 0) ? options.size() - 1 : choiceIndex - 1;
            
            #ifdef CONSOLE_PRINT
            consoleprint("Menu Page: '" + name + "', Value: '" + getCurrentPrintValue(), __FILE__, __LINE__);
            #endif
            
            if (onDown) onDown();
        }
        
        void enter() override
        {
            menu.setCurrentPage(options[choiceIndex]);
            
            if (onEnter) onEnter();
        }
        
        String getCurrentPrintValue() const override
        {
            return options[choiceIndex]->getName();
        }
        
    private:
        std::vector<Page*> options;
        size_t choiceIndex = 0;
    };
    
    class GlobalSettingPage : public Page
    {
    public:
        GlobalSettingPage(const String id_, const String name_, const size_t min_, const size_t max_, Menu& menu_, size_t defaultIndex_ = 0, String* choiceNames_ = nullptr)
            : Page(menu_)
            , min(min_), max(max_)
        {
            id = id_;
            name = name_;
            
            if (choiceNames_) choiceNames.assign(choiceNames_, choiceNames_+ (max-min+1));
            else for(unsigned int n = 0; n < (max-min+1); ++n) choiceNames.push_back(TOSTRING(min+n));
            
            choiceIndex = defaultIndex_;
        }
        
        void up() override
        {
            choiceIndex = (choiceIndex >= choiceNames.size() - 1) ? 0 : choiceIndex + 1;
            
            #ifdef CONSOLE_PRINT
            consoleprint("Menu Page: '" + name + "', Value: '" + getCurrentPrintValue(), __FILE__, __LINE__);
            #endif
            
            if (onUp) onUp();
        }
        
        void down() override
        {
            choiceIndex = (choiceIndex == 0) ? choiceNames.size() - 1 : choiceIndex - 1;
            
            #ifdef CONSOLE_PRINT
            consoleprint("Menu Page: '" + name + "', Value: '" + getCurrentPrintValue(), __FILE__, __LINE__);
            #endif
            
            if (onDown) onDown();
        }
        
        String getCurrentPrintValue() const override
        {
            return choiceNames[choiceIndex];
        }
        
        size_t getCurrentChoice() const override
        {
            return choiceIndex;
        }
    
    private:
        size_t min, max;
        size_t choiceIndex = 0;
        std::vector<String> choiceNames;
    };

    
// MARK: - MENU
// ********************************************************************************
    
    Menu() {}
    void setup(std::array<AudioParameterGroup*, NUM_PARAMETERGROUPS> programParameters_);
    ~Menu ();
        
    void buttonClicked (UIElement* _uielement) override;
    void buttonPressed (UIElement* _uielement) override;
    void buttonReleased (UIElement* _uielement) override;
    
    void scroll();
        
    class Listener
    {
    public:
        virtual ~Listener() {}
        
        virtual void menupageSelected(Page* page_) {}
        
        virtual void globalSettingChanged(Page* page_) {}
    };

    void addListener (Listener* _listener);
    std::vector<std::function<void()>> onSaveMessage;
    std::vector<std::function<void()>> onLoadMessage;
    
    void addPage(const String id_, AudioParameter* param_);
    void addPage(const String id_, const String name_, std::initializer_list<Page*> options_);
    void addPage(const String id_, const String name_, const size_t min_, const size_t max_, const size_t default_, String* choiceNames_);
    void addPage(const String id_, const String name_, const size_t min_, const size_t max_, const size_t default_, std::initializer_list<String> choiceNames_);
    
    Page* getPage(const String id_);
    
    void setCurrentPage(Page* page_);
    void setCurrentPage(const String id_);
    void setNewPresetName (const String _name);
    void setBypass (const bool _bypass) { bypass = _bypass; }
    
    size_t getCurrentChoice() const{ return currentPage->getCurrentChoice(); }
    String getCurrentPageID() const { return currentPage->getID(); }
    String getCurrentPageName() const { return currentPage->getName(); }
    
private:
    inline void print();

    void loadPreset();
    void savePreset();
    
    void initializePages();
    void initializePageHierarchy();
    void initializePageActions();
    void initializeJSON();

protected:
    Page* currentPage = nullptr;
    
private:
    std::vector<Page*> pages;
    std::vector<Listener*> listeners;
    
    bool bypass = false; // TODO: why is this here?

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
