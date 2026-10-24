// Garage messages that cross the network as stringtable tokens.
// The server builds "#txg_key|param1|param2"; the client translates the key in the
// player's language and fills %1, %2 ... (a param that is itself a #key is translated too).
// Text that does not start with '#' passes through unchanged.
class TXGText
{
    static const string SEP = "|";

    static string Msg(string key, string p1 = "", string p2 = "", string p3 = "")
    {
        string msg = key;
        if (p1 != "" || p2 != "" || p3 != "")
            msg = msg + SEP + p1;
        if (p2 != "" || p3 != "")
            msg = msg + SEP + p2;
        if (p3 != "")
            msg = msg + SEP + p3;
        return msg;
    }

    static string Localize(string msg)
    {
        if (msg == "" || msg.Get(0) != "#")
            return msg;
        TStringArray parts = new TStringArray();
        msg.Split(SEP, parts);
        string fmt = Widget.TranslateString(parts[0]);
        string p1 = PartAt(parts, 1);
        string p2 = PartAt(parts, 2);
        string p3 = PartAt(parts, 3);
        return string.Format(fmt, p1, p2, p3);
    }

    // Translated text for UI code: "#txg_key" with up to three params.
    static string Get(string key, string p1 = "", string p2 = "", string p3 = "")
    {
        return Localize(Msg(key, p1, p2, p3));
    }

    protected static string PartAt(TStringArray parts, int i)
    {
        if (i >= parts.Count())
            return "";
        string p = parts[i];
        if (p != "" && p.Get(0) == "#")
            return Widget.TranslateString(p);
        return p;
    }
}
