// TraderXGarage - Share Row Component
// One player in the share panel. ADD gives access at once; REMOVE asks for a confirmation click.

class TXGShareRowComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txg-share-row", TXGShareRowComponent);

    // (vehicleId, uid, bool add)
    static ref ScriptInvoker Event_OnShareChange = new ScriptInvoker();

    ref TXGSharePlayerVM _inputPlayer;

    string playerNameText = "";
    string accessText = "";

    Widget shareAddBtn;
    Widget shareRemoveBtn;
    Widget shareConfirmBtn;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "txg-share-row";
        m_Metadata.templatePath = "TraderXGarage/datasets/layouts/GarageShareRow.layout";
    }

    override void OnInit()
    {
        if (!_inputPlayer)
            return;

        playerNameText = _inputPlayer.name;
        string where = "SHARED BEFORE";
        if (_inputPlayer.distance >= 0)
            where = "NEARBY - " + _inputPlayer.distance.ToString() + " m";
        if (_inputPlayer.hasAccess)
            accessText = "HAS ACCESS  -  " + where;
        else
            accessText = where;

        ShowButtons(false);
        MarkForCheck();
    }

    protected void ShowButtons(bool confirming)
    {
        bool has = _inputPlayer && _inputPlayer.hasAccess;
        if (shareAddBtn) shareAddBtn.Show(!has);
        if (shareRemoveBtn) shareRemoveBtn.Show(has && !confirming);
        if (shareConfirmBtn) shareConfirmBtn.Show(has && confirming);
    }

    void OnToggle()
    {
        if (!_inputPlayer)
            return;
        if (_inputPlayer.hasAccess)
        {
            ShowButtons(true);
            return;
        }
        Event_OnShareChange.Invoke(_inputPlayer.vehicleId, _inputPlayer.uid, true);
    }

    void OnConfirmRemove()
    {
        if (_inputPlayer)
            Event_OnShareChange.Invoke(_inputPlayer.vehicleId, _inputPlayer.uid, false);
    }
}
