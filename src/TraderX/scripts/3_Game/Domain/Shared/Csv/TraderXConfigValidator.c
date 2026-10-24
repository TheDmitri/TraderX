// TraderX Configuration Validator - Validates CSV data before compilation
class TraderXConfigValidator
{
    ref array<ref TraderXValidationError> errors;
    ref array<ref TraderXValidationError> warnings;
    
    void TraderXConfigValidator()
    {
        errors = new array<ref TraderXValidationError>;
        warnings = new array<ref TraderXValidationError>;
    }
    
    // Get total error count
    int GetErrorCount()
    {
        return errors.Count();
    }
    
    // Get total warning count
    int GetWarningCount()
    {
        return warnings.Count();
    }
    
    // Check if validation passed (no errors)
    bool IsValid()
    {
        return errors.Count() == 0;
    }
    
    // ERROR = the catalog would be ambiguous or unreadable (duplicate key, unknown mode name, key unusable as a
    // file name, unknown restart behaviour): the compile is refused and the last good catalog is kept.
    // WARNING = compiled as written (the server tolerates it today): a dangling reference is ignored at runtime,
    // out-of-range quantities are clamped. Data exported from a running server therefore always compiles.
    // Duplicate keys inside one file are reported by the compiler while merging.
    void ValidateProducts(array<ref TraderXCsvProduct> products)
    {
        foreach (TraderXCsvProduct product : products)
        {
            string src = product.sourceFile;
            int row = product.sourceRow;

            if (!IsValidKey(product.productKey))
                AddError(src, row, "productKey", "productKey must not contain / \\ or : (it is used as a file name)", product.productKey);
            if (product.className.Length() == 0)
                AddWarning(src, row, "className", "className is empty", "");
            if (product.buyPrice < -1)
                AddWarning(src, row, "buyPrice", "buyPrice must be -1 (not for sale) or >= 0", product.buyPrice.ToString());
            if (product.sellPrice < -1)
                AddWarning(src, row, "sellPrice", "sellPrice must be -1 (cannot be sold) or >= 0", product.sellPrice.ToString());
            if (product.buyPrice == -1 && product.sellPrice == -1)
                AddWarning(src, row, "buyPrice", "buyPrice and sellPrice are both -1 (blank): the product can be neither bought nor sold", "");
            if (product.maxStock < -1)
                AddWarning(src, row, "maxStock", "maxStock must be -1 (unlimited) or >= 0", product.maxStock.ToString());
            if (product.coefficient <= 0 && product.maxStock > 1)
                AddWarning(src, row, "coefficient", "coefficient <= 0 with a limited stock: the price collapses to 1 once stock reaches 2 (use 1 for a flat price)", product.coefficient.ToString());

            if (!IsValidQtyMode(product.buyQtyMode, false))
                AddError(src, row, "buyQtyMode", "Invalid buyQtyMode (must be EMPTY, FULL, COEFFICIENT, or STATIC)", product.buyQtyMode);
            if (!IsValidQtyMode(product.sellQtyMode, true))
                AddError(src, row, "sellQtyMode", "Invalid sellQtyMode (must be NO_MATTER, EMPTY, FULL, COEFFICIENT, or STATIC)", product.sellQtyMode);
            ValidateQtyValue(src, row, "buyQtyValue", product.buyQtyMode, product.buyQtyValue, TraderXBitfieldPacker.MAX_BUY_STATIC);
            ValidateQtyValue(src, row, "sellQtyValue", product.sellQtyMode, product.sellQtyValue, TraderXBitfieldPacker.MAX_SELL_STATIC);

            if (product.destockCoefficient < 0.0 || product.destockCoefficient > 100.0)
                AddWarning(src, row, "destockCoefficient", "destockCoefficient must be a fraction (0.1 = 10%) or a percent up to 100", product.destockCoefficient.ToString());
            if (product.stockBehaviorAtRestart < 0 || product.stockBehaviorAtRestart > 2)
                AddError(src, row, "stockBehaviorAtRestart", "stockBehaviorAtRestart must be 0 (keep), 1 (reset to maxStock) or 2 (random)", product.stockBehaviorAtRestart.ToString());
            if (product.maxStock == -1 && (product.stockBehaviorAtRestart != 0 || product.destockCoefficient > 0))
                AddWarning(src, row, "maxStock", "stock settings have no effect with maxStock -1 (unlimited)", "");
        }
    }

    // Out-of-range quantities are clamped when packed: warn, never block (legacy data must always migrate)
    private void ValidateQtyValue(string src, int row, string column, string mode, float value, int maxStatic)
    {
        if (mode == "COEFFICIENT")
        {
            if (value <= 0 || value > 100)
                AddWarning(src, row, column, "COEFFICIENT needs a fraction (0.5 = 50%) or a percent up to 100; clamped to 0..100%", value.ToString());
        }
        else if (mode == "STATIC")
        {
            if (value < 1 || value > maxStatic)
                AddWarning(src, row, column, string.Format("STATIC needs a whole number of units between 1 and %1; clamped to 0..%1, fix this row", maxStatic), value.ToString());
        }
    }

    void ValidateCategories(array<ref TraderXCsvCategory> categories, array<ref TraderXCsvProduct> products)
    {
        map<string, bool> productKeySet = BuildProductKeySet(products);
        foreach (TraderXCsvCategory category : categories)
        {
            if (!IsValidKey(category.categoryKey))
                AddError(category.sourceFile, category.sourceRow, "categoryKey", "categoryKey must not contain / \\ or :", category.categoryKey);
            if (category.categoryName.Length() == 0)
                AddWarning(category.sourceFile, category.sourceRow, "categoryName", "categoryName is empty", "");
            if (!category.productKeys)
                continue;
            foreach (string productKey : category.productKeys)
            {
                if (!productKeySet.Contains(productKey))
                    AddWarning(category.sourceFile, category.sourceRow, "productKeys", "Product reference not found: " + productKey, productKey);
            }
        }
    }

    void ValidatePresets(array<ref TraderXCsvPreset> presets, array<ref TraderXCsvProduct> products)
    {
        map<string, bool> productKeySet = BuildProductKeySet(products);
        foreach (TraderXCsvPreset preset : presets)
        {
            if (preset.productKey.Length() == 0)
                AddWarning(preset.sourceFile, preset.sourceRow, "productKey", "productKey is empty", "");
            else if (!productKeySet.Contains(preset.productKey))
                AddWarning(preset.sourceFile, preset.sourceRow, "productKey", "Product reference not found: " + preset.productKey, preset.productKey);
            if (!preset.attachments)
                continue;
            foreach (string attachmentKey : preset.attachments)
            {
                if (!productKeySet.Contains(attachmentKey))
                    AddWarning(preset.sourceFile, preset.sourceRow, "attachments", "Attachment reference not found: " + attachmentKey, attachmentKey);
            }
        }
    }

    void ValidateCrossReferences(array<ref TraderXCsvProduct> products)
    {
        map<string, bool> productKeySet = BuildProductKeySet(products);
        foreach (TraderXCsvProduct prod : products)
        {
            if (prod.attachments)
            {
                foreach (string attachmentKey : prod.attachments)
                {
                    if (!productKeySet.Contains(attachmentKey))
                        AddWarning(prod.sourceFile, prod.sourceRow, "attachments", "Attachment reference not found: " + attachmentKey, attachmentKey);
                }
            }
            if (prod.variants)
            {
                foreach (string variantKey : prod.variants)
                {
                    if (!productKeySet.Contains(variantKey))
                        AddWarning(prod.sourceFile, prod.sourceRow, "variants", "Variant reference not found: " + variantKey, variantKey);
                }
            }
        }
    }

    private bool IsValidKey(string key)
    {
        return key.IndexOf("/") == -1 && key.IndexOf("\\") == -1 && key.IndexOf(":") == -1;
    }

    private map<string, bool> BuildProductKeySet(array<ref TraderXCsvProduct> products)
    {
        map<string, bool> keys = new map<string, bool>;
        foreach (TraderXCsvProduct product : products)
        {
            if (product.productKey.Length() > 0)
                keys.Set(product.productKey, true);
        }
        return keys;
    }

    // Generate validation report
    void GenerateReport(string outputPath)
    {
        GetTraderXLogger().LogInfo("[TraderXConfigValidator] Generating validation report: " + outputPath);
        
        FileHandle file = OpenFile(outputPath, FileMode.WRITE);
        if (file == 0)
        {
            GetTraderXLogger().LogError("[TraderXConfigValidator] Failed to create report file: " + outputPath);
            return;
        }
        
        // Write header
        FPrintln(file, "TraderX Configuration Validation Report");
        FPrintln(file, "========================================");
        FPrintln(file, "");
        FPrintln(file, string.Format("Total Errors: %1", errors.Count()));
        FPrintln(file, string.Format("Total Warnings: %1", warnings.Count()));
        FPrintln(file, "");
        
        // Write errors
        if (errors.Count() > 0)
        {
            FPrintln(file, "ERRORS:");
            FPrintln(file, "-------");
            foreach (TraderXValidationError error : errors)
            {
                FPrintln(file, string.Format("[ERROR] %1 (line %2, column '%3'): %4 | Value: '%5'", error.source, error.row, error.column, error.message, error.value));
            }
            FPrintln(file, "");
        }
        
        // Write warnings
        if (warnings.Count() > 0)
        {
            FPrintln(file, "WARNINGS:");
            FPrintln(file, "---------");
            foreach (TraderXValidationError warning : warnings)
            {
                FPrintln(file, string.Format("[WARNING] %1 (line %2, column '%3'): %4 | Value: '%5'", warning.source, warning.row, warning.column, warning.message, warning.value));
            }
            FPrintln(file, "");
        }
        
        // Write summary
        if (errors.Count() == 0 && warnings.Count() == 0)
        {
            FPrintln(file, "Configuration is valid! No errors or warnings found.");
        }
        else if (errors.Count() == 0)
        {
            FPrintln(file, "Configuration is valid, but has warnings. Review warnings above.");
        }
        else
        {
            FPrintln(file, "Configuration has ERRORS and cannot be compiled. Fix errors above.");
        }
        
        CloseFile(file);
        
        GetTraderXLogger().LogInfo("[TraderXConfigValidator] Validation report generated successfully");
    }
    
    void AddError(string source, int row, string column, string message, string value)
    {
        errors.Insert(new TraderXValidationError("ERROR", source, row, column, message, value));
    }
    
    void AddWarning(string source, int row, string column, string message, string value)
    {
        warnings.Insert(new TraderXValidationError("WARNING", source, row, column, message, value));
    }
    
    // Helper: Check if quantity mode is valid
    private bool IsValidQtyMode(string mode, bool allowNoMatter)
    {
        if (mode.Length() == 0)
            return false;
        
        mode.ToUpper();
        
        if (mode == "EMPTY" || mode == "FULL" || mode == "COEFFICIENT" || mode == "STATIC")
            return true;
        
        if (allowNoMatter && mode == "NO_MATTER")
            return true;
        
        return false;
    }
}
