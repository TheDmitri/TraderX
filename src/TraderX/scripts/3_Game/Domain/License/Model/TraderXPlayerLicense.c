class TraderXLicense
{
    string licenseId;
    string licenseName;
    string description;

    void TraderXLicense(string licenseName, string description)
    {
        this.licenseId = TraderXUuid.GenerateUUID();
        this.licenseName = licenseName;
        this.description = description;
    }
}
