class TraderXPresets
{
    string productId;
    string defaultPresetId;
    ref array<ref TraderXPreset> presets;

    void TraderXPresets()
    {
        presets = new array<ref TraderXPreset>();
    }
    
    TraderXPreset GetDefaultPreset()
    {
        foreach (TraderXPreset preset : presets)
        {
            if (preset.presetId == defaultPresetId)
                return preset;
        }
        
        // Return first preset if no default found
        return null;
    }
    
    TraderXPreset GetPresetById(string presetId)
    {
        foreach (TraderXPreset preset : presets)
        {
            if (preset.presetId == presetId)
                return preset;
        }
        return null;
    }
}