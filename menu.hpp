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
        Page(Menu& menu_, const String& id_, const String& name_)
            : menu(menu_), id(id_), name(name_) {};
        
        virtual ~Page() {}
        
        void addParent(Page* parent_) { parent = parent_; }
        
        virtual void update(String presetName_, uint index_ = 0) {}
        
        std::function<void()> onUp;
        std::function<void()> onDown;
        std::function<void()> onEnter;
        std::function<void()> onExit;
        
        virtual void up();
        virtual void down();
        virtual void enter();
        virtual void exit();
        
        virtual void setCurrentChoice(const uint index_) {}
        
        String getName() const { return name; }
        
        String getID() const { return id; }
        
        virtual String getCurrentPrintValue() const { return ""; }
        
        virtual uint getCurrentChoiceIndex() const { return 0; }
        
        virtual size_t getNumChoices() const { return 0; }
        
        virtual String* getChoiceNames() { return nullptr; }
        
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
        
        String getCurrentPrintValue() const override { return parameter->getPrintValueAsString(); }
        
        AudioParameter* getParameter() { return parameter; }
        
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
                
        void setCurrentChoice(const uint index_) override { choiceIndex = index_; }
        
        uint getCurrentChoiceIndex() const override { return choiceIndex; }
        
        String getCurrentPrintValue() const override { return options[choiceIndex]->getName(); }
        
        size_t getNumChoices() const override { return options.size(); }
        
        String* getChoiceNames() override { return choiceNames.data(); }
        
    private:
        std::vector<Page*> options;
        std::vector<String> choiceNames;
        uint choiceIndex = 0;
    };
    
    // =======================================================================================
    // MARK: - MENU::PAGE::NAMINGPAGE
    // =======================================================================================
    
    class NamingPage : public Page
    {
    public:
        NamingPage(const String& id_, const String& name_, Menu& menu_);
        
        void update(String presetName_, uint index_ = 0) override;
        
        void up() override;
        void down() override;
        void enter() override;
        
        String getCurrentPrintValue() const override { return editedPresetName; }
        
        uint getCurrentChoiceIndex() const override { return 0; }
        
        size_t getNumChoices() const override { return 1; }
        
        String* getChoiceNames() override { return &editedPresetName; }
                                
    private:
        char getCharFromIndex(uint index_) const;
        uint getIndexFromChar(char char_) const;
        
        String editedPresetName;
        uint charIndex = 0;
        uint charPosition = 0;
        
        const size_t numChars = 63; // Number of characters: space, 26 uppercase, 26 lowercase, 10 digits
        const size_t nameLength = 10;
    };
    
    // =======================================================================================
    // MARK: - MENU::PAGE::SETTINGPAGE
    // =======================================================================================
    
    class SettingPage : public Page
    {
    public:
        SettingPage(const String& id_, const String& name_,
                    String* choiceNames_, const size_t numChoices_,
                    const uint defaultIndex_, const uint minIndex_, Menu& menu_);
        
        SettingPage(const String& id_, const String& name_,
                    std::initializer_list<String> choiceNames_, const size_t numChoices_,
                    const uint defaultIndex_, const uint minIndex_, Menu& menu);
        
        void update(String presetName_, uint index_ = 0) override;

        void up() override;
        void down() override;
        
        void setCurrentChoice(const uint index_) override { choiceIndex = index_; }

        uint getCurrentChoiceIndex() const override { return choiceIndex; }

        String getCurrentPrintValue() const override { return choiceNames[choiceIndex]; }
                    
        size_t getNumChoices() const override { return choiceNames.size(); }
        
        String* getChoiceNames() override { return choiceNames.data(); }
        
    private:
        uint choiceIndex = 0;
        std::vector<String> choiceNames;
    };

    // =======================================================================================
    // MARK: - MENU
    // =======================================================================================
    
    Menu() {}
    ~Menu ();
        
    void setup(std::array<AudioParameterGroup*, NUM_PARAMETERGROUPS> programParameters_);
    
    template<typename PageType, typename... Args>
    void addPage(const String& id_, Args&&... args_)
    {
        pages.push_back(new PageType(id_, std::forward<Args>(args_)..., *this));
    }
    
    Page* getPage(const String& id_);
    
    Page* getCurrentPage() { return currentPage; }
    
    void setCurrentPage(Page* page_);
    void setCurrentPage(const String& id_);
    
    void scroll();
    
    void buttonClicked(UIElement* uielement_) override;
    void buttonPressed(UIElement* uielement_) override;
    void buttonReleased(UIElement* uielement_) override;
    
    std::function<void()> onPresetSave;
    std::function<void()> onPresetLoad;
    std::function<void()> onPageChange;
    std::function<void()> onEffectOrderChange;
    std::function<void(Page* page_)> onGlobalSettingChange;
    
    void loadPreset(uint index_ = 0);
    
private:
    void initializePages();
    void initializePageHierarchy();
    void initializePageActions();
    void initializeJSON();
    
    void display();

    void savePreset();

protected:
    Page* currentPage = nullptr;
    
private:
    std::vector<Page*> pages;

    json JSONpresets;
    json JSONglobals;
    
    std::array<AudioParameterGroup*, NUM_PARAMETERGROUPS> programParameters;
    uint lastUsedPresetIndex = 0;
    
    enum ScrollDirection { DOWN, UP };
    ScrollDirection scrollDirection;
    
public:
    bool isScrolling = false;
    bool onHold = false;
};


#endif /* menu_hpp */
