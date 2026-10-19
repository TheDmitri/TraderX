// Modern DayZular accessor (registry-resolved). Back-compat GetInstance() below.
static TraderXCurrencyService GetTraderXCurrencyService()
{
    return TraderXCurrencyService.GetInstance();
}

class TraderXCurrencyService : DayZularService
{
    static ref TraderXCurrencyService m_instance;

    ref TraderXCurrencyTypeCollection currencySettings;

    // Registry-backed but intentionally NOT eager-registered in TraderXAppInit:
    // the server ctor loads currency config via TraderXCurrencyRepository, which
    // must run AFTER TraderXConfigurationService.Initialize(). So construction
    // stays lazy (first GetInstance, inside TraderXModule.InitRPCs, after config
    // init) and the instance is registered on creation. m_instance caches it.
    static TraderXCurrencyService GetInstance()
    {
        if (!m_instance)
        {
            m_instance = TraderXCurrencyService.Cast(DZlrServiceRegistry.GetInstance().GetService(TraderXCurrencyService));
            if (!m_instance)
            {
                m_instance = new TraderXCurrencyService();
                DZlrServiceRegistry.GetInstance().RegisterService(TraderXCurrencyService, m_instance);
            }
        }
        return m_instance;
    }

    void TraderXCurrencyService()
    {
        if(GetGame().IsServer())
        {
            currencySettings = TraderXCurrencyRepository.Load();
            SortCurrencyDenominations();
            TraderXModule.Event_OnTraderXPlayerJoined.Insert(OnPlayerJoined);
        }
        else
        {
            currencySettings = new TraderXCurrencyTypeCollection();
        }
    }

    void RegisterRPCs()
    {
        if(GetGame().IsClient())
            GetRPCManager().AddRPC("TraderX", "GetTraderXCurrencyResponse", this, SingeplayerExecutionType.Client);
    }

    void OnPlayerJoined(PlayerBase player, PlayerIdentity identity)
    {
        GetRPCManager().SendRPC("TraderX", "GetTraderXCurrencyResponse", new Param1<TraderXCurrencyTypeCollection>(currencySettings), true, identity);
    }

    //----------------------------------------------------------------//
	//Currency Methods
	//----------------------------------------------------------------//

    int GetPlayerMoneyFromAllCurrency(PlayerBase player, TStringArray acceptedCurrencyTypesIn = null)
    {
      ref TStringArray acceptedCurrencyTypes = acceptedCurrencyTypesIn;
      if(!acceptedCurrencyTypes)
        acceptedCurrencyTypes = new TStringArray();

      int amount = 0;
      foreach(TraderXCurrencyType currencyType : currencySettings.currencyTypes)
      {
        //check if currencyName is accepted
        if(acceptedCurrencyTypes.Count() > 0 && acceptedCurrencyTypes.Find(currencyType.currencyName) == -1)
            continue;

        amount += GetPlayerMoneyFromCurrency(player,currencyType);
      }

      return amount; 
    }

    int GetPlayerMoneyFromCurrency(PlayerBase player, TraderXCurrencyType currencyType)
    {
       int amount = 0;

      array<EntityAI> itemsArray = TraderXInventoryManager.GetItemsArray(player);
      foreach(EntityAI entity: itemsArray)
      {
        ItemBase item = ItemBase.Cast(entity);
        if(!item)
            continue;

        TraderXCurrency currency = currencyType.GetCurrencyForClassName(item.GetType());
        if(!currency)
            continue;
        
        float quantity = TraderXQuantityManager.GetItemAmount(item);
        int value = currency.value;
        amount += value * quantity;
      }
    
      return amount;
    }

    /**
    * Trader purchase payment: cash first, then the Banking provider (whole amount, or the cash shortfall).
    *
    * @param label Ledger label for the bank debit (e.g. "TRADER_BUY").
    * @param source Ledger source shown to the player (e.g. "Trader").
    */
    bool RemoveMoneyWithBankFallback(PlayerBase player, int amountToRemove, TStringArray acceptedCurrencyTypes, string label, string source)
    {
        if(!player || amountToRemove <= 0)
            return false;

        if (RemoveMoneyAmountFromPlayer(player, amountToRemove, acceptedCurrencyTypes))
            return true;

        if (!TraderXProviderRegistry.GetInstance().HasBanking())
            return false;

        ITraderXPaymentProvider bankProv = TraderXProviderRegistry.GetInstance().GetBankingProvider();
        if (!bankProv || !bankProv.IsAvailable(player))
            return false;

        int bankBal = bankProv.GetBalance(player);
        if (bankBal >= amountToRemove)
            return bankProv.DeductLabeled(player, amountToRemove, label, source);

        TStringArray currencies = acceptedCurrencyTypes;
        if (!currencies)
            currencies = new TStringArray();
        int playerMoney = GetPlayerMoneyFromAllCurrency(player, currencies);
        if (playerMoney > 0 && (playerMoney + bankBal) >= amountToRemove)
        {
            int bankNeeded = amountToRemove - playerMoney;
            if (RemoveMoneyAmountFromPlayer(player, playerMoney, currencies))
            {
                if (bankProv.DeductLabeled(player, bankNeeded, label, source))
                    return true;
                AddMoneyToPlayer(player, playerMoney, currencies);
            }
        }
        return false;
    }

    /**
    * Removes the specified amount of money from the player's inventory for the accepted currency types.
    *
    * @param player The player whose inventory to check and remove money from.
    * @param amountToRemove The amount of money (in the first accepted currency type) to remove from the player's inventory.
    * @param acceptedCurrencyTypes The list of accepted currency types.
    *
    * @return true if the required amount has been successfully removed.
    */
    bool RemoveMoneyAmountFromPlayer(PlayerBase player, int amountToRemove, TStringArray acceptedCurrencyTypesIn = null)
    {
        ref TStringArray acceptedCurrencyTypes = acceptedCurrencyTypesIn;
        if(!player || amountToRemove <= 0)
            return false;

        if(!acceptedCurrencyTypes)
            acceptedCurrencyTypes = new TStringArray();

        // Cash only. Trader purchases that may also draw on the bank go through RemoveMoneyWithBankFallback,
        // so other callers (e.g. TraderXGarage with payWithBank=false) are never charged from the bank.
        int playerMoney = GetPlayerMoneyFromAllCurrency(player, acceptedCurrencyTypes);
        if(playerMoney < amountToRemove)
            return false;

        int amountRemoved = 0;
        int take = 0;
        int moneyAmountRemaining = amountToRemove;
        bool exactAmountFound = false;

        foreach(TraderXCurrencyType currencyType : currencySettings.currencyTypes)
        {
            // Skip if this currency type is not in the accepted list
            if(acceptedCurrencyTypes.Count() > 0 && acceptedCurrencyTypes.Find(currencyType.currencyName) == -1)
                continue;
            
            // Iterate from smallest denomination to largest so exact small notes are spent before breaking larger notes
            for (int cIdx = currencyType.currencies.Count() - 1; cIdx >= 0; cIdx--)
            {
                TraderXCurrency currency = currencyType.currencies[cIdx];
                if(moneyAmountRemaining <= 0)
                    break;

                int currentCurrencyQuantity = TraderXQuantityManager.GetTotalQuantityOfItem(player, currency.GetCurrencyClassName());
                if(currentCurrencyQuantity <= 0)
                    continue;

                // Calculate how many of this currency we need to take (force float division so moneyAmountRemaining < currencyValue yields 1, not 0)
                int currencyValue = currency.GetCurrencyValue();
                if (currencyValue <= 0)
                    continue;
                int needed = Math.Ceil(moneyAmountRemaining / (currencyValue * 1.0));
                
                // If we have exact change, use that
                if (moneyAmountRemaining % currencyValue == 0 && currentCurrencyQuantity >= needed)
                {
                    take = needed;
                    exactAmountFound = true;
                }
                // Otherwise, take the minimum of what we have and what we need
                else
                {
                    take = Math.Min(needed, currentCurrencyQuantity);
                }

                if(take > 0)
                {
                    bool success = TraderXInventoryManager.RemoveOurProduct(player, currency.GetCurrencyClassName(), take);
                    if (!success)
                        return false;

                    int removedValue = take * currencyValue;
                    amountRemoved += removedValue;
                    moneyAmountRemaining = Math.Max(0, amountToRemove - amountRemoved);
                    
                    // If we've removed at least the amount needed, we're done
                    if (amountRemoved >= amountToRemove)
                        break;
                }
            }

            // Early exit if we've removed at least the amount needed
            if (exactAmountFound || amountRemoved >= amountToRemove)
                break;
        }

        // If we removed more than needed (due to currency denominations), give change back
        if (amountRemoved > amountToRemove)
        {
            int difference = amountRemoved - amountToRemove;
            AddMoneyToPlayer(player, difference, acceptedCurrencyTypes);
        }
        
        // Return true if we removed at least the required amount
        return amountRemoved >= amountToRemove;
    }

    // Ensure every currency type pays out largest denomination first.
    // Without this the greedy payout loop spends the whole amount on whatever
    // denomination the config happens to list first, producing thousands of
    // low-value coin stacks and freezing the server on big sells.
    void SortCurrencyDenominations()
    {
        if(!currencySettings)
            return;

        foreach(TraderXCurrencyType currencyType : currencySettings.currencyTypes)
        {
            if(currencyType)
                currencyType.SortCurrenciesByValue();
        }
    }

    void AddMoneyToPlayer(PlayerBase player, int amount, TStringArray acceptedCurrencyTypesIn = null)
    {
        ref TStringArray acceptedCurrencyTypes = acceptedCurrencyTypesIn;
        if(!acceptedCurrencyTypes)
            acceptedCurrencyTypes = new TStringArray();

        // Check if amount exceeds practical limits and warn
        CheckCurrencyLimitsAndWarn(amount, acceptedCurrencyTypes);

        foreach (TraderXCurrencyType currencyType : currencySettings.currencyTypes)
        {
            // Check if currencyName is accepted
            if (acceptedCurrencyTypes.Count() > 0 && acceptedCurrencyTypes.Find(currencyType.currencyName) == -1)
                continue;

            foreach(TraderXCurrency currency: currencyType.currencies)
            {
                int value = currency.GetCurrencyValue();
            
                if (value <= 0 || value > amount)
                    continue;
                
                int amountToAdd = amount / value;
                
                // Create a new instance of the currency item to add to the player's inventory
                ItemBase currencyItem = TraderXItemFactory.CreateInInventory(player, currency.GetCurrencyClassName(), amountToAdd);
                
                if (!currencyItem)
                    continue;
                
                // Update the remaining amount to add
                amount -= amountToAdd * value;
                
                // Stop adding money if the desired amount has been reached
                if (amount <= 0)
                    break;
            }
        }
    }

    /**
     * Calculates and warns about maximum practical currency amounts based on configuration
     */
    void CheckCurrencyLimitsAndWarn(int requestedAmount, TStringArray acceptedCurrencyTypesIn = null)
    {
        ref TStringArray acceptedCurrencyTypes = acceptedCurrencyTypesIn;
        if(!acceptedCurrencyTypes)
            acceptedCurrencyTypes = new TStringArray();

        // Constants from TraderXItemFactory
        const int MAX_STACKS_ALLOWED = 1000;
        const int EFFICIENCY_WARNING_THRESHOLD = 100;

        int maxPracticalAmount = 0;
        int maxEfficientAmount = 0;
        string limitingCurrency = "";
        string efficientCurrency = "";

        foreach (TraderXCurrencyType currencyType : currencySettings.currencyTypes)
        {
            // Check if currencyName is accepted
            if (acceptedCurrencyTypes.Count() > 0 && acceptedCurrencyTypes.Find(currencyType.currencyName) == -1)
                continue;

            foreach(TraderXCurrency currency: currencyType.currencies)
            {
                int value = currency.GetCurrencyValue();
                if (value <= 0)
                    continue;

                // Get max stack size for this currency item
                int maxQuantity = TraderXQuantityManager.GetMaxItemQuantityServer(currency.GetCurrencyClassName());
                if (maxQuantity <= 0)
                    maxQuantity = 1; // Fallback

                // Calculate maximum amounts
                int maxForThisCurrency = MAX_STACKS_ALLOWED * maxQuantity * value;
                int efficientForThisCurrency = EFFICIENCY_WARNING_THRESHOLD * maxQuantity * value;

                // Track the highest capacity currency (largest denomination gives max practical amount)
                if (maxPracticalAmount == 0 || maxForThisCurrency > maxPracticalAmount)
                {
                    maxPracticalAmount = maxForThisCurrency;
                    limitingCurrency = currency.GetCurrencyClassName() + " (value: " + value.ToString() + ", max stack: " + maxQuantity.ToString() + ")";
                }

                if (maxEfficientAmount == 0 || efficientForThisCurrency > maxEfficientAmount)
                {
                    maxEfficientAmount = efficientForThisCurrency;
                    efficientCurrency = currency.GetCurrencyClassName() + " (value: " + value.ToString() + ", max stack: " + maxQuantity.ToString() + ")";
                }
            }
        }

        // Log currency limits information
        if (maxPracticalAmount > 0)
        {
            GetTraderXLogger().LogInfo(string.Format("[CURRENCY LIMITS] Maximum practical amount: %1 (limited by %2)", maxPracticalAmount.ToString(), limitingCurrency));
            GetTraderXLogger().LogInfo(string.Format("[CURRENCY LIMITS] Efficient amount threshold: %1 (limited by %2)", maxEfficientAmount.ToString(), efficientCurrency));
        }

        // Warn if requested amount exceeds limits
        if (requestedAmount > maxPracticalAmount)
        {
            GetTraderXLogger().LogError(string.Format("[CURRENCY LIMITS] Requested amount %1 exceeds maximum practical limit of %2. Limited by currency: %3. Consider adding higher denomination currency items.", requestedAmount.ToString(), maxPracticalAmount.ToString(), limitingCurrency));
        }
        else if (requestedAmount > maxEfficientAmount)
        {
            GetTraderXLogger().LogWarning(string.Format("[CURRENCY LIMITS] Requested amount %1 exceeds efficient threshold of %2. Limited by currency: %3. Performance may be impacted.", requestedAmount.ToString(), maxEfficientAmount.ToString(), efficientCurrency));
        }
    }

    //----------------------------------------------------------------//
	//RPC Methods
	//----------------------------------------------------------------//

    void GetTraderXCurrencyResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Client)
            return;

        Param1<TraderXCurrencyTypeCollection> data;
        if(!ctx.Read(data))
            return;

        currencySettings = data.param1;
    }
}
