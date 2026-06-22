// A value-copy of one selected item's transient cart state. The snapshot stores
// these instead of bare product references so the cart's quantities/presets survive
// a customize round-trip even though they live on shared singleton products that
// customize freely mutates (and DeSelectAllItems now resets).
class TraderXSelectionSnapshotEntry
{
    TraderXProduct product;
    int multiplier;
    TraderXPreset preset;

    void TraderXSelectionSnapshotEntry(TraderXProduct p, int m, TraderXPreset pr)
    {
        product = p;
        multiplier = m;
        preset = pr;
    }
}

// Modern DayZular accessor (registry-resolved). Back-compat GetInstance() below.
static TraderXSelectionService GetTraderXSelectionService()
{
    return TraderXSelectionService.GetInstance();
}

class TraderXSelectionService : DayZularService
{
    private static ref TraderXSelectionService m_Instance;

    private ref map<UUID, ref TraderXProduct> m_SelectedItems;

    // Holds a saved copy of the selection (e.g. the purchase cart) while a sub-flow
    // such as customize temporarily uses the selection for its own configuration.
    private ref array<ref TraderXSelectionSnapshotEntry> m_Snapshot;

    static ref ScriptInvoker Event_OnItemSelectionChanged = new ScriptInvoker();

    // "Fitting" / outfit mode (purchase page toggle). When on, slot exclusivity also applies
    // in the purchase flow so the cart matches the dressed player preview (one item per slot).
    // Off by default → normal multi-buy.
    private static bool s_FittingModeEnabled = false;

    static void SetFittingMode(bool enabled)
    {
        s_FittingModeEnabled = enabled;
    }

    static bool IsFittingMode()
    {
        return s_FittingModeEnabled;
    }

    void TraderXSelectionService()
    {
        m_SelectedItems = new map<UUID, ref TraderXProduct>();
    }

    // Save the current selection (with each item's quantity + preset) so it can be
    // restored later. Overwrites any prior snapshot.
    void PushSelectionSnapshot()
    {
        m_Snapshot = new array<ref TraderXSelectionSnapshotEntry>();
        foreach (string uniqueId, TraderXProduct item : m_SelectedItems)
        {
            if (item)
                m_Snapshot.Insert(new TraderXSelectionSnapshotEntry(item, item.GetMultiplier(), item.defaultPreset));
        }
    }

    // Clear the current selection and restore the snapshot taken by PushSelectionSnapshot,
    // re-applying each item's saved quantity + preset onto the shared product.
    void RestoreSelectionSnapshot()
    {
        if (!m_Snapshot)
            return;

        DeSelectAllItems();
        foreach (TraderXSelectionSnapshotEntry entry : m_Snapshot)
        {
            TraderXProduct item = entry.product;
            if (!item)
                continue;

            item.multiplier = entry.multiplier;
            item.defaultPreset = entry.preset;
            item.isSelected = true;
            m_SelectedItems.Insert(GetUniqueItemId(item), item);
            NotifySelectionChanged(item);
        }
        m_Snapshot = null;
    }

    // Shared singleton products carry transient cart state (multiplier + preset). Reset
    // it when an item leaves the selection so it never leaks into the next page/session
    // (the "LAR keeps its old quantity/preset" class of bug). Set fields directly to
    // avoid firing the multiplier-changed event onto a card being torn down.
    private void ResetTransientState(TraderXProduct item)
    {
        if (!item)
            return;
        item.multiplier = 1;
        item.defaultPreset = null;
    }

    // Registry-backed. Resolves the instance registered in TraderXAppInit; if
    // absent (e.g. first use before registration), creates and registers it so
    // the registry stays the single source of truth. m_Instance caches the lookup.
    static TraderXSelectionService GetInstance()
    {
        if (!m_Instance)
        {
            m_Instance = TraderXSelectionService.Cast(DZlrServiceRegistry.GetInstance().GetService(TraderXSelectionService));
            if (!m_Instance)
            {
                m_Instance = new TraderXSelectionService();
                DZlrServiceRegistry.GetInstance().RegisterService(TraderXSelectionService, m_Instance);
            }
        }
        return m_Instance;
    }

    string GetUniqueItemId(TraderXProduct item)
    {
        // For sell items, use unique playerItemId; for buy items, use productId
        if (item.GetPlayerItem()) {
            return item.GetPlayerItem().GetPlayerItemId();
        } else {
            return item.productId;
        }
    }

    void SelectItem(TraderXProduct item)
    {
        if (!item)
            return;

        EnforceSlotExclusivity(item);

        string uniqueId = GetUniqueItemId(item);
        if (!m_SelectedItems.Contains(uniqueId))
        {
            item.isSelected = true;
            m_SelectedItems.Insert(uniqueId, item);
            NotifySelectionChanged(item);
        }
    }

    // In customize, an attachment occupies a physical inventory slot. Two attachments that
    // fight over the same slot can't both be applied, so selecting one swaps out whatever
    // already claims that slot. Multi-slot parts (4 wheels) are allowed up to their slot
    // count. Purely automatic — derived from each item's inventorySlot config, no admin flag.
    // No-op outside customize and for the main item (it isn't an attachment).
    private void EnforceSlotExclusivity(TraderXProduct item)
    {
        int nav = TraderXUINavigationService.GetInstance().GetNavigationId();
        // Always in customize; in the purchase flow only when the player enabled fitting mode.
        bool active = (nav == ENavigationIds.CUSTOMIZE) || (nav == ENavigationIds.PURCHASE && s_FittingModeEnabled);
        if (!active)
            return;
        if (CustomizeStateService.GetInstance().IsMainItem(item))
            return;

        TStringArray itemSlots = GetInventorySlots(item.className);
        if (itemSlots.Count() == 0)
            return;
        int capacity = itemSlots.Count();

        // Already-selected attachments competing for the same slot(s).
        array<ref TraderXProduct> conflicts = new array<ref TraderXProduct>();
        foreach (string uid, TraderXProduct selected : m_SelectedItems)
        {
            if (!selected || selected.productId == item.productId)
                continue;
            if (CustomizeStateService.GetInstance().IsMainItem(selected))
                continue;
            if (SlotsIntersect(itemSlots, GetInventorySlots(selected.className)))
                conflicts.Insert(selected);
        }

        // Keep the group within capacity once the new item joins: free the oldest extras.
        while (conflicts.Count() >= capacity && conflicts.Count() > 0)
        {
            DeselectItem(conflicts[0]);
            conflicts.RemoveOrdered(0);
        }
    }

    // The inventory slot name(s) an item type can occupy. Attachable items live under
    // different config roots: vehicle parts / clothing in CfgVehicles, weapon attachments
    // (optics, suppressors, buttstocks) in CfgWeapons, magazines in CfgMagazines — so try
    // each. Magazines usually declare no inventorySlot (they all compete for the weapon's
    // single magazine well), so fall back to one synthetic shared "magazine" slot.
    private TStringArray GetInventorySlots(string className)
    {
        TStringArray slots = new TStringArray();
        if (ReadInventorySlot("CfgVehicles " + className, slots)) return slots;
        if (ReadInventorySlot("CfgWeapons " + className, slots)) return slots;
        if (ReadInventorySlot("CfgMagazines " + className, slots)) return slots;

        if (GetGame().ConfigIsExisting("CfgMagazines " + className))
            slots.Insert("magazine");

        return slots;
    }

    // Read <classPath> inventorySlot (array or single value) into outSlots. Returns true if
    // the class declared any slot.
    private bool ReadInventorySlot(string classPath, TStringArray outSlots)
    {
        string cfgPath = classPath + " inventorySlot";
        GetGame().ConfigGetTextArray(cfgPath, outSlots);
        if (outSlots.Count() == 0)
        {
            string single;
            if (GetGame().ConfigGetText(cfgPath, single) && single != "")
                outSlots.Insert(single);
        }
        return outSlots.Count() > 0;
    }

    private bool SlotsIntersect(TStringArray a, TStringArray b)
    {
        foreach (string s : a)
        {
            if (b.Find(s) != -1)
                return true;
        }
        return false;
    }

    void DeSelectAllItems()
    {
        foreach (string uniqueId, TraderXProduct item : m_SelectedItems)
        {
            item.isSelected = false;
            NotifySelectionChanged(item);
            ResetTransientState(item);
        }
        m_SelectedItems.Clear();
    }


    void DeselectItem(TraderXProduct item)
    {
        string uniqueId = GetUniqueItemId(item);
        if (m_SelectedItems.Contains(uniqueId))
        {
            item.isSelected = false;
            m_SelectedItems.Remove(uniqueId);
            NotifySelectionChanged(item);
            ResetTransientState(item);
        }
    }

    bool IsItemSelected(string productId)
    {
        return m_SelectedItems.Contains(productId);
    }

    array<ref TraderXProduct> GetSelectedItems()
    {
        array<ref TraderXProduct> items = new array<ref TraderXProduct>();
        foreach (string uniqueId, TraderXProduct item : m_SelectedItems)
        {
            items.Insert(item);
        }
        return items;
    }

    void DeselectItems(array<UUID> productIds)
    {
        // Need to find items by productId and deselect them using their unique IDs
        array<string> uniqueIdsToRemove = new array<string>();
        
        foreach (string uniqueId, TraderXProduct item : m_SelectedItems)
        {
            if (productIds.Find(item.productId) != -1)
            {
                uniqueIdsToRemove.Insert(uniqueId);
            }
        }
        
        foreach (string uniqId : uniqueIdsToRemove)
        {
            if (m_SelectedItems.Contains(uniqId))
            {
                TraderXProduct itm = m_SelectedItems.Get(uniqId);
                itm.isSelected = false;
                m_SelectedItems.Remove(uniqId);
                NotifySelectionChanged(itm);
                ResetTransientState(itm);
            }
        }
    }

    private void NotifySelectionChanged(TraderXProduct item)
    {
        Event_OnItemSelectionChanged.Invoke(item);
    }
}