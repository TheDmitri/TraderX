class TraderXPreset
{
    string presetName;
    string presetId;
    string productId;
    ref array<string> attachments;

    void TraderXPreset()
    {
        attachments = new array<string>;
    }

    static TraderXPreset CreatePreset(string _presetName, TraderXProduct _item)
    {
        TraderXPreset preset = new TraderXPreset();
        preset.presetName = _presetName;
        preset.productId = _item.productId;
        preset.attachments = _item.GetSelectedAttachments();
        preset.presetId = string.Empty;
        
        // Generate preset ID using the new system
        TraderXPresetId.AssignPresetIdIfNot(preset);
        return preset;
    }
    
    // Constructor for server presets with explicit attachments
    static TraderXPreset CreateTraderXPreset(string _presetName, string _productId, array<string> _attachments)
    {
        TraderXPreset preset = new TraderXPreset();
        preset.presetName = _presetName;
        preset.productId = _productId;
        preset.attachments = new array<string>;
        preset.attachments.Copy(_attachments);
        preset.presetId = string.Empty;
        
        // Generate preset ID using the new system
        TraderXPresetId.AssignPresetIdIfNot(preset);
        return preset;
    }
    
    string ToStringFormatted()
    {
        string result = "";
        result += "{\n";
        result += "  presetName: " + presetName + "\n";
        result += "  presetId: " + presetId + "\n";
        result += "  productId: " + productId + "\n";
        result += "  attachments: [\n";
        
        if (attachments && attachments.Count() > 0) {
            for (int i = 0; i < attachments.Count(); i++) {
                result += "    " + attachments[i];
                if (i < attachments.Count() - 1) {
                    result += ",";
                }
                result += "\n";
            }
        } else {
            result += "    (no attachments)\n";
        }
        
        result += "  ]\n";
        result += "}";
        return result;
    }
}