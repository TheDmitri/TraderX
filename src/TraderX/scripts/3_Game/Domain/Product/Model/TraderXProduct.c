class TraderXJsonProduct
{
    string className;
    float coefficient;
    int maxStock;
    int tradeQuantity;
    int buyPrice;
    int sellPrice;
    int stockSettings;
    ref array<string> attachments;
    ref array<string> variants;

    void TraderXJsonProduct()
    {
        this.attachments = new array<string>();
        this.variants = new array<string>();
    }
}

class TraderXProduct extends TraderXJsonProduct
{
    
    [NonSerialized()]
    ref TraderXProduct entityItemParent;

    [NonSerialized()]
    bool isSelected;

    [NonSerialized()]
    ref array<string> selectedAttachments;
    
    [NonSerialized()]
    float selectedQuantity;

    [NonSerialized()]
    int multiplier = 1;

    [NonSerialized()]
    static ref ScriptInvoker Event_OnMultiplierChanged = new ScriptInvoker();
    
    [NonSerialized()]
    static ref ScriptInvoker Event_OnCheckoutPricingChanged = new ScriptInvoker();

    [NonSerialized()]
    ref TraderXPreset defaultPreset;

    ref TraderXPlayerItem playerItem;

    string productId = string.Empty;

    void TraderXProduct()
    {
        this.variants = new array<string>();
        this.attachments = new array<string>();
        this.selectedAttachments = new array<string>();
    }

    static TraderXProduct CreateProduct(string _className = string.Empty, float _coefficient = 1.0, int  _maxStock = -1, int _tradeQuantity = 0, int _buyPrice = -1, int _sellPrice = -1, float _deStockCoefficient = 0.0, int _stockBehaviorAtRestart = 0, array<string> _attachments = null, array<string> _variants = null, string _productId = string.Empty)
    {
        TraderXProduct _product = new TraderXProduct();
        _className.ToLower();
        _product.className = _className;
        _product.coefficient = _coefficient;
        _product.maxStock = _maxStock;
        _product.tradeQuantity = _tradeQuantity;
        _product.buyPrice = _buyPrice;
        _product.sellPrice = _sellPrice;
        // Store destock coefficient in lower 7 bits (0-127)
        // Store behavior at restart in next 2 bits (8-9)
        int destockInt = Math.Round(_deStockCoefficient * 100);
        if (destockInt > 100) destockInt = 100;
        _product.stockSettings = (destockInt & 0x7F) | ((_stockBehaviorAtRestart & 0x03) << 7);

        _product.attachments = _attachments;
        _product.variants = _variants;

        TraderXProductId.AssignUUIDIfNot(_product, _productId);

        return _product;
    }

    bool CanBeSold()
    {
        return sellPrice >= 0;
    }

    bool CanBeBought()
    {
        return buyPrice >= 0;
    }
    
    
    // Check if stock is unlimited (maxStock == -1)
    bool IsStockUnlimited()
    {
        return maxStock == -1;
    }

    string GetProductId()
    {
        return productId;
    }

    void SetMultiplier(int multplier)
    {
        this.multiplier = multplier;
        GetTraderXLogger().LogDebug("SetMultiplier - Product: " + productId + " Multiplier: " + multplier);
        Event_OnMultiplierChanged.Invoke(this);
    }

    int GetMultiplier()
    {
        return multiplier;
    }

    void SetSelectedAttachments(array<string> attachments) 
    {
        selectedAttachments = attachments;
    }
    
    array<string> GetSelectedAttachments()
    {
        return selectedAttachments;
    }
    
    // Alias for backward compatibility
    void SetSelectedAttachmentIds(array<string> selectedAttachments)
    {
        SetSelectedAttachments(selectedAttachments);
    }

    string GetParentEntityItemId()
    {
        if(entityItemParent)
            return entityItemParent.productId;

        return string.Empty;
    }

    static TraderXProduct CreateAsPlayerItem(string classname, int idHigh, int idLow, int depthLevel, TraderXProduct prod, int healthLevel)
    {
        TraderXProduct tpItem = TraderXProduct.CreateCopy(prod);

        classname.ToLower();
        tpItem.playerItem = TraderXPlayerItem.CreateAsPlayerItem(classname, idHigh, idLow, depthLevel, prod.productId, healthLevel);
        tpItem.className = classname;
        TraderXProductId.AssignUUIDIfNot(tpItem, string.Empty);

        return tpItem;
    }

    array<ref TraderXProduct> GetAttachments()
    {
        array<ref TraderXProduct> attachmentsItems = new array<ref TraderXProduct>();
        foreach(string attachmentId : attachments){
            TraderXProduct attachment = TraderXProductRepository.GetItemById(attachmentId);
            if(attachment){
                attachmentsItems.Insert(attachment);
            }
        }

        return attachmentsItems;
    }

    array<ref TraderXProduct> GetVariants()
    {
        array<ref TraderXProduct> variantsItems = new array<ref TraderXProduct>();
        foreach(string variantId : variants){
            TraderXProduct variant = TraderXProductRepository.GetItemById(variantId);
            if(variant){
                variantsItems.Insert(variant);
            }
        }

        return variantsItems;
    }

    bool HasVariants()
    {
        return variants.Count() > 0;
    }

    bool HasAttachments()
    {
        return attachments.Count() > 0;
    }

    void LoadCategoryStock()
    {
        if (maxStock == -1) return; // Skip if stock is unlimited
        
        TraderXProductStockRepository.LoadCategoryStock(productId, GetStockBehaviorAtRestart(), maxStock, GetDeStockCoefficient());
    }

    int GetStockBehaviorAtRestart()
    {
        return (stockSettings >> 7) & 0x03;
    }

    float GetDeStockCoefficient()
    {
        int destockInt = (stockSettings & 0x7F);
        return destockInt * 0.01;
    }

    bool IsDeStockEnabled()
    {
        return (stockSettings & 0x7F) > 0;
    }

    string GetDisplayName()
    {
        return  TraderXCoreUtils.GetDisplayName(className);
    }

    TraderXPlayerItem GetPlayerItem()
    {
        return playerItem;
    }

    static TraderXProduct CreateCopy(TraderXProduct prod)
    {
        TraderXProduct tpItem = new TraderXProduct();
        tpItem.className = prod.className;
        tpItem.productId = prod.productId;
        tpItem.coefficient = prod.coefficient;
        tpItem.maxStock = prod.maxStock;
        tpItem.tradeQuantity = prod.tradeQuantity;
        tpItem.buyPrice = prod.buyPrice;
        tpItem.sellPrice = prod.sellPrice;
        tpItem.stockSettings = prod.stockSettings;
        tpItem.attachments = prod.attachments;
        tpItem.variants = prod.variants;
        return tpItem;
    }
    
    string ToProductString()
    {
        string result = "";
        result += "{";
        result += "\n  productId: " + productId;
        result += "\n  className: " + className;
        result += "\n  displayName: " + GetDisplayName();
        result += "\n  coefficient: " + coefficient.ToString();
        result += "\n  maxStock: " + maxStock.ToString();
        result += "\n  tradeQuantity: " + tradeQuantity.ToString();
        result += "\n  buyPrice: " + buyPrice.ToString();
        result += "\n  sellPrice: " + sellPrice.ToString();
        result += "\n  stockSettings: " + stockSettings.ToString();
        result += "\n  destockCoefficient: " + GetDeStockCoefficient().ToString();
        result += "\n  stockBehaviorAtRestart: " + GetStockBehaviorAtRestart().ToString();

        if(playerItem){
            result += "\n  playerItem: " + playerItem.ToPlayerItemString();
        }
        
        // Include stock information
        if (maxStock == -1) {
            result += "\n  currentStock: unlimited";
        } else {
            result += "\n  currentStock: " + TraderXProductStockRepository.GetStockDisplay(productId, maxStock);
        }
        
        // Include attachments if any
        if (attachments && attachments.Count() > 0) {
            result += "\n  attachments: [";
            for (int i = 0; i < attachments.Count(); i++) {
                if (i > 0) result += ", ";
                result += attachments.Get(i);
            }
            result += "]";
        }
        
        // Include variants if any
        if (variants && variants.Count() > 0) {
            result += "\n  variants: [";
            for (int j = 0; j < variants.Count(); j++) {
                if (j > 0) result += ", ";
                result += variants.Get(j);
            }
            result += "]";
        }
        
        // Include selected attachments if any
        if (selectedAttachments && selectedAttachments.Count() > 0) {
            result += "\n  selectedAttachments: [";
            for (int k = 0; k < selectedAttachments.Count(); k++) {
                if (k > 0) result += ", ";
                result += selectedAttachments.Get(k);
            }
            result += "]";
        }
        
        result += "\n}";
        
        return result;
    }
}