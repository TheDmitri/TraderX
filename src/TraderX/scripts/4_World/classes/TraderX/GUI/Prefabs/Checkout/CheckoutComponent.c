class CheckoutComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-checkout", CheckoutComponent);

    ref array<ref CheckoutCardComponent> m_CheckoutCards = new array<ref CheckoutCardComponent>();

    static ref ScriptInvoker Event_OnTotalPriceChange = new ScriptInvoker();

    int totalPrice;
    string total_amount;
    string bank_amount;
    string money_amount;

    private static ref CheckoutComponent m_Instance;

    // Widgets
    Widget checkout_card_list;
    TextWidget totalAmountText, bankAmountText, moneyAmountText;
    ButtonWidget clearCheckoutBtn, checkoutBtn;

    static CheckoutComponent GetInstance()
    {
        return m_Instance;
    }

    array<ref CheckoutCardComponent> GetCheckoutItems()
    {
        return m_CheckoutCards;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-checkout";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Checkout/CheckoutView.layout";
    }

    override void OnInit()
    {
        m_Instance = this;
        TraderXSelectionService.Event_OnItemSelectionChanged.Insert(OnItemSelectionChanged);
        SellPageComponent.Event_OnSliderQuantityChangedCallBack.Insert(OnSliderQuantityChanged);
        TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTraderXResponseReceived);
        CheckoutCardComponent.Event_OnPriceChanged.Insert(OnPriceChanged);
        UpdateBankMoney();
        UpdatePlayerMoney();
    }

    override void OnDestroy()
    {
        TraderXSelectionService.Event_OnItemSelectionChanged.Remove(OnItemSelectionChanged);
        SellPageComponent.Event_OnSliderQuantityChangedCallBack.Remove(OnSliderQuantityChanged);
        TraderXTradingService.Event_OnTraderXResponseReceived.Remove(OnTraderXResponseReceived);
        CheckoutCardComponent.Event_OnPriceChanged.Remove(OnPriceChanged);
        ClearCheckoutCards();
        m_Instance = null;
    }

    void ClearCheckoutCards()
    {
        foreach (CheckoutCardComponent card : m_CheckoutCards)
        {
            if (card && card.GetRoot())
                card.GetRoot().Unlink();
        }
        m_CheckoutCards.Clear();
    }

    void AddCheckoutItem(TraderXProduct item)
    {
        CheckoutCardComponent existingCard = GetCheckoutCardForItem(item);
        if (existingCard)
        {
            existingCard.RefreshQuantityAndPrice();
        }
        else
        {
            if (item.GetPlayerItem())
            {
                string uniqueKey = item.GetPlayerItem().GetPlayerItemId();
                TraderXCheckoutPricingService.GetInstance().AddItemToCheckout(item.GetProductId(), item.GetMultiplier(), uniqueKey);
            }

            CheckoutCardComponent card = new CheckoutCardComponent(item);
            card.InitComponent(null);
            if (checkout_card_list)
            {
                checkout_card_list.AddChild(card.GetRoot());
                card.GetRoot().Show(true);
            }
            m_CheckoutCards.Insert(card);
        }
    }

    CheckoutCardComponent GetCheckoutCardForItem(TraderXProduct item)
    {
        foreach (CheckoutCardComponent checkoutCard : m_CheckoutCards)
        {
            TraderXProduct checkoutItem = checkoutCard.GetItem();

            if (item.GetPlayerItem() && checkoutItem.GetPlayerItem())
            {
                if (item.GetPlayerItem().GetPlayerItemId() == checkoutItem.GetPlayerItem().GetPlayerItemId())
                    return checkoutCard;
            }
            else if (!item.GetPlayerItem() && !checkoutItem.GetPlayerItem())
            {
                if (item.productId == checkoutItem.productId)
                    return checkoutCard;
            }
        }
        return null;
    }

    void RemoveCheckoutItem(TraderXProduct item)
    {
        for (int i = m_CheckoutCards.Count() - 1; i >= 0; i--)
        {
            if (item.GetPlayerItem())
            {
                if (item.GetPlayerItem().GetPlayerItemId() == m_CheckoutCards[i].GetItem().GetPlayerItem().GetPlayerItemId())
                {
                    TraderXCheckoutPricingService.GetInstance().RemoveItemFromCheckout(item.GetProductId(), item.GetMultiplier());
                    if (m_CheckoutCards[i].GetRoot()) m_CheckoutCards[i].GetRoot().Unlink();
                    m_CheckoutCards.Remove(i);
                    return;
                }
            }
            else
            {
                if (item.productId == m_CheckoutCards[i].GetItem().productId)
                {
                    if (m_CheckoutCards[i].GetRoot()) m_CheckoutCards[i].GetRoot().Unlink();
                    m_CheckoutCards.Remove(i);
                    return;
                }
            }
        }
    }

    void OnItemSelectionChanged(TraderXProduct item)
    {
        if (item.isSelected)
        {
            CheckoutCardComponent existingCard = GetCheckoutCardForItem(item);
            if (existingCard)
                existingCard.RefreshQuantityAndPrice();
            else
                AddCheckoutItem(item);
        }
        else
        {
            RemoveCheckoutItem(item);
        }
        UpdateTotalPrice();
        UpdatePlayerMoney();
    }

    void UpdatePlayerMoney()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;

        int playerMoney = TraderXTradingService.GetInstance().GetPlayerMoneyAmount(player);
        money_amount = TraderXQuantityManager.GetFormattedMoneyAmount(playerMoney);
        if (moneyAmountText) moneyAmountText.SetText(money_amount);
    }

    void UpdateCheckoutView()
    {
        ClearCheckoutCards();

        TraderXSelectionService selectionService = TraderXSelectionService.GetInstance();
        array<ref TraderXProduct> selectedItems = selectionService.GetSelectedItems();

        foreach (TraderXProduct item : selectedItems)
        {
            AddCheckoutItem(item);
        }

        UpdateTotalPrice();
        UpdatePlayerMoney();
        UpdateBankMoney();
    }

    void OnSliderQuantityChanged(bool isMaxQuantity)
    {
        foreach (CheckoutCardComponent checkoutCard : m_CheckoutCards)
        {
            checkoutCard.OnSliderQuantityChanged(isMaxQuantity);
        }
        UpdateTotalPrice();
    }

    void UpdateBankMoney()
    {
        if (!TraderXProviderRegistry.GetInstance().HasBankingProvider())
        {
            if (bankAmountText)
                bankAmountText.Show(false);
            // Hide the bank label widget if it exists
            Widget bankLabel = m_Root.FindAnyWidget("bank_label");
            if (bankLabel)
                bankLabel.Show(false);
            return;
        }

        ITraderXPaymentProvider bankProvider = TraderXProviderRegistry.GetInstance().GetBankingProvider();
        int bankBalance = bankProvider.GetClientBalance();
        bank_amount = TraderXQuantityManager.GetFormattedMoneyAmount(bankBalance);

        if (bankAmountText)
        {
            bankAmountText.SetText(bank_amount);
            bankAmountText.Show(true);
        }

        Widget bankLabelShow = m_Root.FindAnyWidget("bank_label");
        if (bankLabelShow)
            bankLabelShow.Show(true);
    }

    void OnPriceChanged()
    {
        UpdateTotalPrice();
    }

    int GetTotalPrice()
    {
        totalPrice = 0;
        foreach (CheckoutCardComponent checkoutCard : m_CheckoutCards)
        {
            totalPrice += checkoutCard.GetPrice();
        }
        return totalPrice;
    }

    void UpdateTotalPrice()
    {
        totalPrice = GetTotalPrice();
        total_amount = TraderXQuantityManager.GetFormattedMoneyAmount(totalPrice);
        if (totalAmountText) totalAmountText.SetText(total_amount);
    }

    void OnTraderXResponseReceived(int responseReceived, TraderXTransactionResultCollection transactionResultCollection = null)
    {
        if (responseReceived != ETraderXResponse.TRANSACTIONS || !transactionResultCollection)
            return;

        array<UUID> itemsToDeselect = new array<UUID>();

        for (int i = 0; i < transactionResultCollection.GetTransactionResults().Count(); i++)
        {
            TraderXTransactionResult transactionResult = transactionResultCollection.GetTransactionResults()[i];
            if (transactionResult.IsSuccess())
                itemsToDeselect.Insert(transactionResult.GetProductId());
        }

        if (itemsToDeselect.Count() > 0)
        {
            if (TraderXUINavigationService.GetInstance().GetNavigationId() != ENavigationIds.CUSTOMIZE)
            {
                TraderXSelectionService.GetInstance().DeselectItems(itemsToDeselect);
            }
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.COINS);
        }

        UpdateCheckoutView();
    }

    void HandleCustomizeCheckout()
    {
        if (CustomizePageComponent.GetInstance())
        {
            TraderXProduct item = CustomizePageComponent.GetInstance().GetItem();
            array<string> attachments = new array<string>();

            foreach (CheckoutCardComponent checkoutCard : m_CheckoutCards)
            {
                TraderXProduct checkoutItem = checkoutCard.GetItem();
                if (checkoutItem.productId == item.productId)
                    continue;

                for (int i = 0; i < checkoutCard.GetItem().GetMultiplier(); i++)
                {
                    attachments.Insert(checkoutItem.productId);
                }
            }

            TraderXPreset preset = TraderXPreset.CreateTraderXPreset("Customize", item.productId, attachments);
            item.defaultPreset = preset;

            TraderXTransactionCollection transactions = new TraderXTransactionCollection();
            int npcId = TraderXTradingService.GetInstance().GetNpcId();
            transactions.AddTransaction(TraderXTransaction.CreateBuyTransaction(item, 1, GetTotalPrice(), npcId, item.defaultPreset));
            GetDayZularRPCManager().SendRPC("TraderX", "GetTransactionsRequest", new Param2<TraderXTransactionCollection, int>(transactions, npcId));
        }
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == clearCheckoutBtn)
        {
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
            if (m_CheckoutCards.Count() == 0)
                return false;

            TraderXSelectionService.GetInstance().DeSelectAllItems();
            ClearCheckoutCards();
            UpdateTotalPrice();
            UpdatePlayerMoney();
            return true;
        }

        if (w == checkoutBtn)
        {
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.CONFIRM);
            if (m_CheckoutCards.Count() == 0)
                return false;

            if (TraderXTradingService.GetInstance().IsTransactionPending())
                return false;

            if (TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE)
            {
                TraderXTradingService.GetInstance().LockTransaction();
                HandleCustomizeCheckout();
                return true;
            }

            TraderXTransactionCollection transactions = new TraderXTransactionCollection();
            int npcId = TraderXTradingService.GetInstance().GetNpcId();
            foreach (CheckoutCardComponent checkoutCard : m_CheckoutCards)
            {
                TraderXProduct item = checkoutCard.GetItem();

                if (checkoutCard.GetTradeMode() == ETraderXTradeMode.BUY)
                {
                    int averageUnitPrice;
                    if (item.defaultPreset)
                    {
                        averageUnitPrice = checkoutCard.GetPrice() / item.GetMultiplier();
                        transactions.AddTransaction(TraderXTransaction.CreateBuyTransaction(item, item.GetMultiplier(), averageUnitPrice, npcId, item.defaultPreset));
                    }
                    else
                    {
                        averageUnitPrice = checkoutCard.GetPrice() / item.GetMultiplier();
                        transactions.AddTransaction(TraderXTransaction.CreateBuyTransaction(item, item.GetMultiplier(), averageUnitPrice, npcId));
                    }
                }
                else if (checkoutCard.GetTradeMode() == ETraderXTradeMode.SELL)
                {
                    int multiplier = item.GetMultiplier();
                    if (multiplier > 1)
                    {
                        TraderXPriceCalculation priceCalc;
                        string itemKey = item.GetPlayerItem().GetPlayerItemId();

                        for (int j = 0; j < multiplier; j++)
                        {
                            if (item.defaultPreset)
                                priceCalc = TraderXCheckoutPricingService.GetInstance().CalculatePresetSellPriceWithCheckoutForItem(item.defaultPreset, itemKey, j + 1, item.GetPlayerItem().healthLevel);
                            else
                                priceCalc = TraderXCheckoutPricingService.GetInstance().CalculateSellPriceWithCheckoutForItem(TraderXProductRepository.GetItemById(item.GetProductId()), itemKey, j + 1, item.GetPlayerItem().healthLevel);

                            int thisUnitPrice;
                            if (j == 0)
                            {
                                thisUnitPrice = priceCalc.GetCalculatedPrice();
                            }
                            else
                            {
                                TraderXPriceCalculation prevPriceCalc;
                                if (item.defaultPreset)
                                    prevPriceCalc = TraderXCheckoutPricingService.GetInstance().CalculatePresetSellPriceWithCheckoutForItem(item.defaultPreset, itemKey, j, item.GetPlayerItem().healthLevel);
                                else
                                    prevPriceCalc = TraderXCheckoutPricingService.GetInstance().CalculateSellPriceWithCheckoutForItem(TraderXProductRepository.GetItemById(item.GetProductId()), itemKey, j, item.GetPlayerItem().healthLevel);
                                thisUnitPrice = priceCalc.GetCalculatedPrice() - prevPriceCalc.GetCalculatedPrice();
                            }

                            transactions.AddTransaction(TraderXTransaction.CreateSellTransaction(item, 1, thisUnitPrice, npcId));
                        }
                    }
                    else
                    {
                        transactions.AddTransaction(TraderXTransaction.CreateSellTransaction(item, 1, checkoutCard.GetPrice(), npcId));
                    }
                }
            }

            TraderXTradingService.GetInstance().LockTransaction();
            GetDayZularRPCManager().SendRPC("TraderX", "GetTransactionsRequest", new Param2<TraderXTransactionCollection, int>(transactions, npcId));
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.CONFIRM);
            return true;
        }

        return false;
    }
}
