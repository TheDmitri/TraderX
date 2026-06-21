/**
 * TraderXToastNotification — one HUD toast for the TraderX marketplace.
 *
 * Port of CourseBarterTrader's BarterNotification, retargeted to the TraderX
 * buy/sell flow. Built in-game from transaction results and pushed through the
 * client FIFO runner (TraderXToastService) onto the themed toast card.
 *
 * The card binds this object directly:
 *   {{ _notif.GetKicker() }}  {{ _notif.title }}
 *   {{ _notif.body }}         {{ _notif.footer }}   {{ _notif.time }}
 * The icon frame is loaded in code from GetIconImageRef(). `type` is one of the
 * TXNOTIF_* keys (TraderXToastConstants.c) and drives the icon, accent color and
 * kicker label; an unknown type falls back to INFO.
 */
class TraderXToastNotification
{
    string type;
    string title;
    string body;
    string footer;
    string time;

    // Per-transaction breakdown lines for the expandable detail panel. One entry
    // per independent transaction in a bulk trade (e.g. "Sold Ammo Box", "Failed M4").
    // Populated by TraderXTransactionNotifier; empty for simple single-line toasts.
    ref array<string> details;

    void TraderXToastNotification(string notifType = "info", string notifTitle = "", string notifBody = "", string notifFooter = "", string notifTime = "")
    {
        type = notifType;
        title = notifTitle;
        body = notifBody;
        footer = notifFooter;
        time = notifTime;
        details = new array<string>();
    }

    // True when there is a per-transaction breakdown worth expanding to (a bulk
    // trade). Single-transaction toasts say everything in the body already.
    bool HasDetails()
    {
        return details && details.Count() > 1;
    }

    void AddDetail(string line)
    {
        if (!details)
            details = new array<string>();
        details.Insert(line);
    }

    // The breakdown rendered as one richtext block (one transaction per line).
    string GetDetailsText()
    {
        if (!details)
            return string.Empty;
        string joined = "";
        for (int i = 0; i < details.Count(); i++)
        {
            if (i > 0)
                joined += "\n";
            joined += details[i];
        }
        return joined;
    }

    // A title + body are the minimum to draw a meaningful card.
    bool IsValid()
    {
        if (title == string.Empty && body == string.Empty)
            return false;
        return true;
    }

    // Known type, else fall back to a safe generic.
    string ResolvedType()
    {
        switch (type)
        {
            case TXNOTIF_PURCHASE: return type;
            case TXNOTIF_SALE:     return type;
            case TXNOTIF_INFO:     return type;
            case TXNOTIF_WARNING:  return type;
            case TXNOTIF_ERROR:    return type;
        }
        return TXNOTIF_INFO;
    }

    // Imageset frame for the card icon well. Frame names == type keys, built from
    // datasets/icons/svg/<type>.svg via scripts/build-icons.sh (traderx_icons).
    string GetIconImageRef()
    {
        return "set:traderx_icons image:" + ResolvedType();
    }

    // Kicker line shown above the title: "TRADER · PURCHASED" etc.
    string GetKicker()
    {
        switch (ResolvedType())
        {
            case TXNOTIF_PURCHASE: return "TRADER · PURCHASED";
            case TXNOTIF_SALE:     return "TRADER · SOLD";
            case TXNOTIF_WARNING:  return "TRADER · PARTIAL";
            case TXNOTIF_ERROR:    return "TRADER · FAILED";
        }
        return "TRADER";
    }

    // Accent for the left rule / glyph / progress fill (barter theme: gold default,
    // green confirm, amber partial, red error).
    int GetAccentColor()
    {
        switch (ResolvedType())
        {
            case TXNOTIF_PURCHASE: return ARGB(255, 107, 158, 89);   // green
            case TXNOTIF_WARNING:  return ARGB(255, 210, 150, 70);   // amber
            case TXNOTIF_ERROR:    return ARGB(255, 196, 80, 80);    // red
        }
        return ARGB(255, 196, 163, 90);   // gold/bronze (sale, info)
    }

    // Same accent at low alpha for the progress track behind the fill.
    int GetAccentTrackColor()
    {
        switch (ResolvedType())
        {
            case TXNOTIF_PURCHASE: return ARGB(36, 107, 158, 89);
            case TXNOTIF_WARNING:  return ARGB(36, 210, 150, 70);
            case TXNOTIF_ERROR:    return ARGB(36, 196, 80, 80);
        }
        return ARGB(36, 196, 163, 90);
    }
}
