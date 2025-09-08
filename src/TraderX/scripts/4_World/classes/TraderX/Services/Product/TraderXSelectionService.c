class TraderXSelectionService
{
    private static ref TraderXSelectionService m_Instance;
    
    private ref map<UUID, ref TraderXProduct> m_SelectedItems;
    
    static ref ScriptInvoker Event_OnItemSelectionChanged = new ScriptInvoker();

    private void TraderXSelectionService()
    {
        m_SelectedItems = new map<UUID, ref TraderXProduct>();
    }

    static TraderXSelectionService GetInstance()
    {
        if (!m_Instance)
        {
            m_Instance = new TraderXSelectionService();
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
        string uniqueId = GetUniqueItemId(item);
        if (!m_SelectedItems.Contains(uniqueId))
        {
            item.isSelected = true;
            m_SelectedItems.Insert(uniqueId, item);
            NotifySelectionChanged(item);
        }
    }

    void DeSelectAllItems()
    {
        foreach (string uniqueId, TraderXProduct item : m_SelectedItems)
        {
            item.isSelected = false;
            NotifySelectionChanged(item);
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
            }
        }
    }

    private void NotifySelectionChanged(TraderXProduct item)
    {
        Event_OnItemSelectionChanged.Invoke(item);
    }
}