class TraderXMainViewController: ViewController
{
    static private TraderXMainViewController m_instance;

    ButtonWidget favBtn, SellBtn, PurchaseBtn, UpgradeBtn, DropBagBtn, PlayerBtn;

    static TraderXMainViewController GetInstance()
    {
        return m_instance;
    }

    ref ObservableCollection<ref ScriptView> main_page_content = new ObservableCollection<ref ScriptView>(this);

    ButtonWidget selectedBtn;

    int navigationId;

    string trader_name, trader_role;

    void TraderXMainViewController()
    {
        m_instance = this;
    }

    void ~TraderXMainViewController()
    {
        TraderXSelectionService.GetInstance().DeSelectAllItems();
    }

    void Setup()
    {
        ChangeSubView(new FavoritePageView(), favBtn);
        UpdateTraderInfo();
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
                NotifyPropertiesChanged({"trader_name", "trader_role"});
            }
        }
    }

    bool OnFavoriteExecute(ButtonCommandArgs args)
	{
        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
        ChangeSubView(new FavoritePageView(), args.Source);
		return true;
	}

    bool OnPurchaseExecute(ButtonCommandArgs args)
	{
        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
        ChangeSubView(new PurchasePageView(), args.Source);
		return true;
	}

    bool OnUpgradeExecute(ButtonCommandArgs args)
	{
        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
        ChangeSubView(new UpgradePageView(), args.Source);
		return true;
	}

    bool OnSellExecute(ButtonCommandArgs args)
	{
        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
        ChangeSubView(new SellPageView(), args.Source);
		return true;
	}

    #ifdef TRADERX_STASH
    bool OnDropBagExecute(ButtonCommandArgs args)
	{
        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
        ChangeSubView(new TemporaryStashView(), args.Source);
		return true;
	}
    #endif

    void ChangeSubView(ScriptView view, ButtonWidget newSelectedBtn)
    {
        TraderXSelectionService.GetInstance().DeSelectAllItems();
        
        if(selectedBtn){
            selectedBtn.SetTextColor(0xffa5a5a5);
            selectedBtn.SetAlpha(0.7);
        }
        selectedBtn = newSelectedBtn;
        selectedBtn.SetTextColor(0xffafbc38);
        selectedBtn.SetAlpha(1.0);

        main_page_content.Clear();
        main_page_content.Insert(view);
    }

    void AddSubPage(ScriptView view)
    {
        if(main_page_content.Count() > 0){
            main_page_content.Get(0).Show(false);
        }
        main_page_content.Insert(view);
    }

    override bool OnMouseEnter(Widget w, int x, int y)
	{
        if(w && w.GetUserID() == EButtonIds.NAVIGATION && w != selectedBtn){
            w.SetAlpha(1.0);
        }
        return false;
	}

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
        if(w && w.GetUserID() == EButtonIds.NAVIGATION && w != selectedBtn){
            w.SetAlpha(0.70);
        }
        return false;
	}
}