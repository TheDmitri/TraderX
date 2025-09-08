class TraderXPresetsService
{
    static ref TraderXPresetsService m_instance;

    ref map<string, ref TraderXPresets> m_presets; // Client presets (user-created) - changed to string keys
    ref map<string, ref TraderXPresets> m_serverPresets; // Server presets (admin-configured)
    string filePath;

    void TraderXPresetsService()
    {
        m_presets = new map<string, ref TraderXPresets>();
        m_serverPresets = new map<string, ref TraderXPresets>();
        
        // Server-side initialization
        if (GetGame().IsServer())
        {
            LoadServerPresets();
            TraderXModule.Event_OnTraderXPlayerJoined.Insert(OnPlayerJoined);
        }
    }

    void SetServerId(string serverId)
    {
        filePath = string.Format(TRADERX_PRESETS_FILE, serverId);
        LoadTraderXProductsFavorites();
    }

    void RegisterRPCs()
    {
        if(GetGame().IsServer())
        {
            GetRPCManager().AddRPC("TraderX", "GetServerPresetsRequest", this, SingeplayerExecutionType.Server);
            GetRPCManager().AddRPC("TraderX", "GetAllServerPresetsRequest", this, SingeplayerExecutionType.Server);
        }
        else
        {
            GetRPCManager().AddRPC("TraderX", "OnAllServerPresetsResponse", this, SingeplayerExecutionType.Client);
            GetRPCManager().AddRPC("TraderX", "OnServerPresetsResponse", this, SingeplayerExecutionType.Client);
        }
    }

    static TraderXPresetsService GetInstance()
    {
        if (!m_instance)
        {
            m_instance = new TraderXPresetsService();
        }
        return m_instance;
    }

    map<string, string> GetPresetListFromItem(string productId)
    {
        array<ref TraderXPreset> presets = GetPresets(productId);
        map<string, string> dropdownList = new map<string, string>();
        for (int i = 0; i < presets.Count(); i++)
        {
            dropdownList.Insert(presets[i].presetId, string.Format("%1 - %2", i, presets[i].presetName));
        }

        return dropdownList;
    }

    int CalculateTotalPricePreset(TraderXPreset preset)
    {
        int totalPrice = 0;
        
        // Use dynamic pricing for main item
        TraderXProduct item = TraderXProductRepository.GetItemById(preset.productId);
        if (item) {
            int mainItemPrice = TraderXPricingService.GetInstance().GetPricePreview(preset.productId, true, 1, TraderXItemState.PRISTINE);
            totalPrice += mainItemPrice;
        }
        
        // Add attachment prices
        for (int i = 0; i < preset.attachments.Count(); i++) {
            string attachmentId = preset.attachments.Get(i);
            TraderXProduct attachmentProduct = TraderXProductRepository.GetItemById(attachmentId);
            if (attachmentProduct) {
                int attachmentPrice = TraderXPricingService.GetInstance().GetPricePreview(attachmentId, true, 1, TraderXItemState.PRISTINE);
                totalPrice += attachmentPrice;
            }
        }

        return totalPrice;
    }
    
    // Calculate progressive preset pricing for multiple units (buy mode)
    int CalculateProgressivePresetPrice(TraderXPreset preset, int multiplier)
    {
        if (multiplier <= 1) {
            return CalculateTotalPricePreset(preset);
        }
        
        // Use the pricing service to calculate progressive pricing for the entire preset
        // This ensures consistency with the main pricing logic
        TraderXProduct mainProduct = TraderXProductRepository.GetItemById(preset.productId);
        if (!mainProduct) {
            return -1;
        }
        
        // Calculate progressive pricing by summing individual unit prices
        // Each unit is calculated with decreasing stock levels for all components
        int totalPrice = 0;
        
        for (int i = 0; i < multiplier; i++) {
            // Calculate main item price at current stock level (decreases with each unit)
            TraderXPriceCalculation mainItemCalc = TraderXPricingService.GetInstance().CalculateBuyPrice(mainProduct, 1, TraderXItemState.PRISTINE);
            
            // Manually adjust for stock decrease - simulate buying i units already
            int currentMainStock = TraderXProductStockRepository.GetStockAmount(preset.productId);
            int effectiveMainStock = currentMainStock - i;
            
            int mainItemPrice;
            if (mainProduct.IsStockUnlimited() || mainProduct.coefficient == 1.0) {
                mainItemPrice = mainProduct.buyPrice;
            } else {
                mainItemPrice = Math.Pow(mainProduct.coefficient, Math.Max(0, effectiveMainStock - 1)) * mainProduct.buyPrice;
            }
            
            int presetUnitPrice = mainItemPrice;
            
            // Add attachment prices with progressive stock decrease
            for (int j = 0; j < preset.attachments.Count(); j++) {
                string attachmentId = preset.attachments.Get(j);
                TraderXProduct attachmentProduct = TraderXProductRepository.GetItemById(attachmentId);
                if (attachmentProduct) {
                    int currentAttachmentStock = TraderXProductStockRepository.GetStockAmount(attachmentId);
                    int effectiveAttachmentStock = currentAttachmentStock - i;
                    
                    int attachmentPrice;
                    if (attachmentProduct.IsStockUnlimited() || attachmentProduct.coefficient == 1.0) {
                        attachmentPrice = attachmentProduct.buyPrice;
                    } else {
                        attachmentPrice = Math.Pow(attachmentProduct.coefficient, Math.Max(0, effectiveAttachmentStock - 1)) * attachmentProduct.buyPrice;
                    }
                    
                    presetUnitPrice += attachmentPrice;
                }
            }
            
            totalPrice += presetUnitPrice;
            
            GetTraderXLogger().LogDebug(string.Format("[PRESET_PRICING] Unit %1/%2: MainStock=%3, UnitPrice=%4, RunningTotal=%5", 
                (i + 1), multiplier, effectiveMainStock, presetUnitPrice, totalPrice));
        }
        
        GetTraderXLogger().LogDebug(string.Format("[PRESET_PRICING] Progressive preset total: %1 for %2 units of %3", 
            totalPrice, multiplier, preset.presetName));
        
        return totalPrice;
    }

    TraderXPreset GetPresetFromMainItemAndPresetId(TraderXProduct item, string presetId)
    {
        array<ref TraderXPreset> presets = GetPresets(item.productId);
        for (int i = 0; i < presets.Count(); i++)
        {
            if (presets[i].presetId == presetId)
                return presets[i];
        }

        return null;
    }

    array<ref TraderXPreset> GetPresets(string productId)
    {
        if(!m_presets.Contains(productId))
            return new array<ref TraderXPreset>();

        return m_presets[productId].presets;
    }

    void AddPreset(string storageProductId, string presetName, ref array<string> attachments, string targetProductId = "")
    {
        if (!attachments)
            attachments = new array<string>();
        
        // Use targetProductId if provided, otherwise use storageProductId
        string presetProductId;
        if (targetProductId != "")
        {
            presetProductId = targetProductId;
        }
        else
        {
            presetProductId = storageProductId;
        }
        
        if(!m_presets.Contains(storageProductId)){
            m_presets.Insert(storageProductId, new TraderXPresets());
            m_presets[storageProductId].productId = storageProductId;
        }

        TraderXPreset newPreset = TraderXPreset.CreateTraderXPreset(presetName, presetProductId, attachments);
        m_presets[storageProductId].presets.Insert(newPreset);
        StoreTraderXProductsPresets();
    }

    void SetDefaultPreset(string productId, string presetId)
    {
        if(!m_presets.Contains(productId)){
            m_presets.Insert(productId, new TraderXPresets());
            m_presets[productId].productId = productId;
        }

        m_presets[productId].defaultPresetId = presetId;
        StoreTraderXProductsPresets();
    }

    string GetDefaultPresetId(string productId)
    {
        if(!m_presets.Contains(productId))
            return string.Empty;

        return m_presets[productId].defaultPresetId;
    }


    TraderXPreset GetDefaultPreset(string productId)
    {
        if(!m_presets.Contains(productId))
            return null;

        if(m_presets[productId].defaultPresetId == string.Empty)
            return null;

        foreach (TraderXPreset preset : m_presets[productId].presets)
        {
            if(preset.presetId == m_presets[productId].defaultPresetId)
                return preset;
        }

        return null;
    }

    void RemovePreset(string productId, string presetId)
    {
        if(!m_presets.Contains(productId))
            return;

        array<ref TraderXPreset> presets = m_presets[productId].presets;

        for (int i = presets.Count() - 1; i >= 0 ; i--)
        {
            if(presets[i].presetId == presetId)
            {
                presets.Remove(i);
                break;
            }
        }

        StoreTraderXProductsPresets();
    }

    void StoreTraderXProductsPresets()
    {
        string errorMessage;
        JsonFileLoader<map<string, ref TraderXPresets>>.SaveFile(filePath, m_presets, errorMessage);
    }

    void LoadTraderXProductsFavorites()
    {
        if (!FileExist(TRADERX_CONFIG_ROOT_SERVER))
        {
            MakeDirectory(TRADERX_CONFIG_ROOT_SERVER);
        }

        if (!FileExist(TRADERX_PRESETS))
        {
            MakeDirectory(TRADERX_PRESETS);
        }

        if (FileExist(filePath))
        {
            string errorMessage;
            JsonFileLoader<map<string, ref TraderXPresets>>.LoadFile(filePath, m_presets, errorMessage);
            return;
        }

        StoreTraderXProductsPresets();
    }

    // ===== SERVER PRESET METHODS =====

    array<ref TraderXPresets> GetAllPresets()
    {
        array<ref TraderXPresets> presets = new array<ref TraderXPresets>();
        foreach(string productId, ref TraderXPresets preset : m_serverPresets)
        {
            presets.Insert(preset);
        }
        return presets;
    }
    
    void LoadServerPresets()
    {
        m_serverPresets = TraderXPresetRepository.LoadAllPresets();
    }
    
    TraderXPresets GetServerPresets(string productId)
    {
        return m_serverPresets.Get(productId);
    }
    
    TraderXPreset GetServerDefaultPreset(string productId)
    {
        TraderXPresets presets = m_serverPresets.Get(productId);
        if(presets){
            return presets.GetDefaultPreset();
        }
        return null;
    }
    
    // Enhanced method to get presets from both client and server
    map<string, string> GetAllPresetListFromItem(string productId)
    {
        map<string, string> allPresets = new map<string, string>();
        
        // Add client presets (user-created)
        array<ref TraderXPreset> clientPresets = GetPresets(productId);
        for (int i = 0; i < clientPresets.Count(); i++)
        {
            allPresets.Insert(clientPresets[i].presetId, string.Format("Client: %1", clientPresets[i].presetName));
        }
        
        // Add server presets (admin-configured)
        TraderXPresets serverPresets = GetServerPresets(productId);
        if (serverPresets)
        {
            for (int j = 0; j < serverPresets.presets.Count(); j++)
            {
                TraderXPreset serverPreset = serverPresets.presets[j];
                allPresets.Insert(serverPreset.presetId, string.Format("Server: %1", serverPreset.presetName));
            }
        }
        
        return allPresets;
    }
    
    // ===== RPC METHODS =====
    
    void SendServerPresetsToClient(PlayerIdentity identity, string productId)
    {
        if (!GetGame().IsServer()) return;
        
        TraderXPresets serverPresets = GetServerPresets(productId);
        if (!serverPresets) return;
        
        GetRPCManager().SendRPC("TraderX", "OnServerPresetsResponse", new Param2<string, ref TraderXPresets>(productId, serverPresets), true, identity);
    }
    
    void SendAllServerPresetsToClient(PlayerIdentity identity)
    {
        if (!GetGame().IsServer()) return;
        array<ref TraderXPresets> allPresets = GetAllPresets();
        GetRPCManager().SendRPC("TraderX", "OnAllServerPresetsResponse", new Param1<array<ref TraderXPresets>>(allPresets), true, identity);
    }
    
    //RPCs
    void GetServerPresetsRequest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Server)
            return;

        Param1<string> data;
        if(!ctx.Read(data)){
            return;
        }

        string productId = data.param1;
        if (productId == "") {
            GetTraderXLogger().LogError("GetServerPresetsRequest: Invalid product ID");
            return;
        }

        // Send server presets for the requested product
        SendServerPresetsToClient(sender, productId);
    }

    void GetAllServerPresetsRequest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Server)
            return;

        // Send all server presets to client
        SendAllServerPresetsToClient(sender);
    }
    
    void OnServerPresetsResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Client)
            return;

        Param2<string, ref TraderXPresets> data;
        if(!ctx.Read(data)){
            return;
        }

        string productId = data.param1;
        TraderXPresets serverPresets = data.param2;
        
        if (!serverPresets) {
            GetTraderXLogger().LogError("OnServerPresetsResponse: Invalid server presets data");
            return;
        }
        
        // Store received server presets in local cache for client-side access
        m_serverPresets.Set(productId, serverPresets);
    }

    void OnAllServerPresetsResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Client)
            return;

        Param1<array<ref TraderXPresets>> data;
        if(!ctx.Read(data)){
            return;
        }

        array<ref TraderXPresets> allPresets = data.param1;
        if (!allPresets) {
            GetTraderXLogger().LogError("OnAllServerPresetsResponse: Invalid all server presets data");
            return;
        }
        
        // Store received all server presets in local cache for client-side access
        m_serverPresets.Clear();
        foreach (TraderXPresets presets : allPresets) {
            m_serverPresets.Set(presets.productId, presets);
        }
    }

    void OnPlayerJoined(PlayerBase player, PlayerIdentity identity)
    {
        SendAllServerPresetsToClient(identity);
    }
}