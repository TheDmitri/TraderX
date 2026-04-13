class TraderXMainComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-main", TraderXMainComponent);
    private static ref TraderXMainComponent s_Instance;

    // Navigation buttons (auto-loaded by name)
    ButtonWidget favBtn, SellBtn, PurchaseBtn, UpgradeBtn, DropBagBtn, PlayerBtn, CatalogBtn, GarageBtn;

    // Sub-page container widget
    Widget main_page_content;

    // Active sub-page component
    ref DZlrComponentBase m_ActivePage;
    ref DZlrComponentBase m_SubPage;

    // Currently selected nav button
    ButtonWidget selectedBtn;

    // Data-bound properties
    string trader_name;
    string trader_role;

    // Refresh timer
    float deltaT = 0.0;
    float REFRESH_PERIOD = 1.0;

    static TraderXMainComponent GetInstance()
    {
        return s_Instance;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-main";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXMainView.layout";
    }

    override void OnInit()
    {
        s_Instance = this;
        Setup();
    }

    override void OnDestroy()
    {
        TraderXSelectionService.GetInstance().DeSelectAllItems();

        if (m_ActivePage)
            delete m_ActivePage;
        if (m_SubPage)
            delete m_SubPage;

        s_Instance = null;
    }

    void Setup()
    {
        ChangeSubView(new FavoritePageComponent(), favBtn);
        UpdateTraderInfo();
        UpdateGarageButtonVisibility();
    }

    /**
     * Show/hide the garage button based on whether a garage provider is registered
     * and available at the current trader NPC.
     */
    void UpdateGarageButtonVisibility()
    {
        if (!GarageBtn)
            return;

        if (!TraderXProviderRegistry.GetInstance().HasGarageProvider())
        {
            GarageBtn.Show(false);
            return;
        }

        int npcId = TraderXTradingService.GetInstance().GetNpcId();
        bool available = TraderXProviderRegistry.GetInstance().GetGarageProvider().IsAvailableAtNpc(npcId);
        GarageBtn.Show(available);
    }

    void UpdateTraderInfo()
    {
        int npcId = TraderXTradingService.GetInstance().GetNpcId();
        if (npcId != -1)
        {
            TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(npcId);
            if (npc)
            {
                trader_name = npc.GetGivenName();
                trader_role = npc.GetRole();
                UpdateTraderInfoWidgets();
            }
        }
    }

    private void UpdateTraderInfoWidgets()
    {
        Widget traderNameWidget = m_Root.FindAnyWidget("trader_name");
        Widget traderRoleWidget = m_Root.FindAnyWidget("trader_role");

        if (traderNameWidget)
            TextWidget.Cast(traderNameWidget).SetText(trader_name);
        if (traderRoleWidget)
            TextWidget.Cast(traderRoleWidget).SetText(trader_role);
    }

    void ChangeSubView(DZlrComponentBase pageComponent, ButtonWidget newSelectedBtn)
    {
        ItemCardComponent.DestroyAllTooltips();
        TraderXSelectionService.GetInstance().DeSelectAllItems();

        // Update nav button visuals
        if (selectedBtn)
        {
            selectedBtn.SetTextColor(0xffa5a5a5);
            selectedBtn.SetAlpha(0.7);
        }
        selectedBtn = newSelectedBtn;
        if (selectedBtn)
        {
            selectedBtn.SetTextColor(0xffafbc38);
            selectedBtn.SetAlpha(1.0);
        }

        // Remove old sub-page
        if (m_SubPage)
        {
            if (m_SubPage.GetRoot())
                m_SubPage.GetRoot().Unlink();
            delete m_SubPage;
            m_SubPage = null;
        }

        // Remove old active page
        if (m_ActivePage)
        {
            if (m_ActivePage.GetRoot())
                m_ActivePage.GetRoot().Unlink();
            delete m_ActivePage;
        }

        // Set new page
        m_ActivePage = pageComponent;
        if (m_ActivePage && main_page_content)
        {
            // Re-parent the component's root widget into our container
            if (m_ActivePage.GetRoot())
            {
                main_page_content.AddChild(m_ActivePage.GetRoot());
                m_ActivePage.GetRoot().Show(true);
            }
        }
    }

    void AddSubPage(DZlrComponentBase subPageComponent)
    {
        // Hide current active page
        if (m_ActivePage && m_ActivePage.GetRoot())
        {
            m_ActivePage.GetRoot().Show(false);
        }

        // Remove old sub-page if exists
        if (m_SubPage)
        {
            if (m_SubPage.GetRoot())
                m_SubPage.GetRoot().Unlink();
            delete m_SubPage;
        }

        m_SubPage = subPageComponent;
        if (m_SubPage && main_page_content)
        {
            main_page_content.AddChild(m_SubPage.GetRoot());
            m_SubPage.GetRoot().Show(true);
        }
    }

    void RemoveSubPage()
    {
        if (m_SubPage)
        {
            if (m_SubPage.GetRoot())
                m_SubPage.GetRoot().Unlink();
            delete m_SubPage;
            m_SubPage = null;
        }

        // Show active page again
        if (m_ActivePage && m_ActivePage.GetRoot())
        {
            m_ActivePage.GetRoot().Show(true);
        }
    }

    // Button click handling
    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT)
            return false;

        if (w == favBtn)
        {
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
            ChangeSubView(new FavoritePageComponent(), favBtn);
            return true;
        }
        if (w == PurchaseBtn)
        {
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
            ChangeSubView(new PurchasePageComponent(), PurchaseBtn);
            return true;
        }
        if (w == UpgradeBtn)
        {
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
            ChangeSubView(new UpgradePageComponent(), UpgradeBtn);
            return true;
        }
        if (w == SellBtn)
        {
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
            ChangeSubView(new SellPageComponent(), SellBtn);
            return true;
        }
        if (w == CatalogBtn)
        {
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
            ChangeSubView(new CatalogPageComponent(), CatalogBtn);
            return true;
        }
        #ifdef TRADERX_STASH
        if (w == DropBagBtn)
        {
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
            ChangeSubView(new DropBagPageComponent(), DropBagBtn);
            return true;
        }
        #endif

        if (w == GarageBtn)
        {
            if (!TraderXProviderRegistry.GetInstance().HasGarageProvider())
                return false;

            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
            GaragePageComponent garagePage = new GaragePageComponent();
            garagePage.InitComponent(null);
            AddSubPage(garagePage);
            return true;
        }

        return false;
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (w && w.GetUserID() == EButtonIds.NAVIGATION && w != selectedBtn)
        {
            w.SetAlpha(1.0);
        }
        return false;
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        if (w && w.GetUserID() == EButtonIds.NAVIGATION && w != selectedBtn)
        {
            w.SetAlpha(0.70);
        }
        return false;
    }
}
