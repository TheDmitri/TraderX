// Converts TraderPlus V1 insurance and receipt flags into TraderX config files

class ConvertTraderPlusInsurance
{
    static void Convert(TPV1GeneralConfig v1Config)
    {
        if (!v1Config)
            return;

        ConvertInsuranceSettings(v1Config);
        ConvertReceiptSettings(v1Config);
    }

    private static void ConvertInsuranceSettings(TPV1GeneralConfig v1Config)
    {
        TraderXInsuranceSettings settings = new TraderXInsuranceSettings();

        // V1 HideInsuranceBtn == true means insurance was disabled
        settings.enabled = !v1Config.HideInsuranceBtn;

        settings.Save();
        GetTraderXLogger().LogInfo("[TPV1 Converter] Insurance settings created (enabled: " + settings.enabled.ToString() + ")");
    }

    private static void ConvertReceiptSettings(TPV1GeneralConfig v1Config)
    {
        TraderXReceiptSettings settings = new TraderXReceiptSettings();

        settings.saveLockData = v1Config.IsReceiptSaveLock;
        settings.saveAttachments = v1Config.IsReceiptSaveAttachment;

        settings.Save();
        GetTraderXLogger().LogInfo("[TPV1 Converter] Receipt settings created (saveLock: " + settings.saveLockData.ToString() + ", saveAttachments: " + settings.saveAttachments.ToString() + ")");
    }
}
