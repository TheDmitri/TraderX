class BuyTooltipComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-buy-tooltip", BuyTooltipComponent);

	string product_description;
	string variants_status;
	string attachments_status;

	TextWidget variantsText;
	TextWidget attachmentsText;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-buy-tooltip";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Tooltip/BuyTooltip.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
	}

	void Setup(string productDescription, bool hasVariants, bool hasAttachments)
	{
		product_description = productDescription;

		if (!hasVariants)
		{
			variants_status = "#tpm_none";
			if (variantsText) variantsText.SetColor(TraderXTheme.TEXT_WARN);
		}
		else
		{
			variants_status = "#tpm_available";
			if (variantsText) variantsText.SetColor(TraderXTheme.TEXT_OK);
		}

		if (!hasAttachments)
		{
			attachments_status = "#tpm_none";
			if (attachmentsText) attachmentsText.SetColor(TraderXTheme.TEXT_WARN);
		}
		else
		{
			attachments_status = "#tpm_available";
			if (attachmentsText) attachmentsText.SetColor(TraderXTheme.TEXT_OK);
		}

		MarkForCheck();
		SetPositionTooltip();
	}

	void SetPositionTooltip()
	{
		int x, y;
		GetMousePos(x, y);
		GetRoot().SetPos(x + 10.0, y - 10.0, true);
		GetRoot().Update();
	}
}
