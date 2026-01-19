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
    
    // Validate products
    void ValidateProducts(array<ref TraderXCsvProduct> products, string source = "products.csv")
    {
        GetTraderXLogger().LogDebug("[TraderXConfigValidator] Validating products...");
        
        // Track product keys for duplicate detection
        map<string, int> productKeyMap = new map<string, int>;
        
        for (int i = 0; i < products.Count(); i++)
        {
            TraderXCsvProduct product = products.Get(i);
            int rowNum = i + 2; // +2 because row 1 is header, array is 0-indexed
            
            // Validate productKey
            if (product.productKey.Length() == 0)
            {
                AddError(source, rowNum, "productKey", "productKey is empty", "");
            }
            else
            {
                // Check for duplicates
                int existingRow;
                if (productKeyMap.Find(product.productKey, existingRow))
                {
                    AddError(source, rowNum, "productKey", "Duplicate productKey (first seen at row " + existingRow + ")", product.productKey);
                }
                else
                {
                    productKeyMap.Set(product.productKey, rowNum);
                }
            }
            
            // Validate className
            if (product.className.Length() == 0)
            {
                AddError(source, rowNum, "className", "className is empty", "");
            }
            
            // Validate buyPrice
            if (product.buyPrice < -1)
            {
                AddError(source, rowNum, "buyPrice", "buyPrice must be >= -1", product.buyPrice.ToString());
            }
            
            // Validate sellPrice
            if (product.sellPrice < -1)
            {
                AddError(source, rowNum, "sellPrice", "sellPrice must be >= -1", product.sellPrice.ToString());
            }
            
            // Validate maxStock
            if (product.maxStock < -1)
            {
                AddError(source, rowNum, "maxStock", "maxStock must be -1 (unlimited) or >= 0", product.maxStock.ToString());
            }
            
            // Validate buyQtyMode
            if (!IsValidQtyMode(product.buyQtyMode, false))
            {
                AddError(source, rowNum, "buyQtyMode", "Invalid buyQtyMode (must be EMPTY, FULL, COEFFICIENT, or STATIC)", product.buyQtyMode);
            }
            
            // Validate sellQtyMode
            if (!IsValidQtyMode(product.sellQtyMode, true))
            {
                AddError(source, rowNum, "sellQtyMode", "Invalid sellQtyMode (must be NO_MATTER, EMPTY, FULL, COEFFICIENT, or STATIC)", product.sellQtyMode);
            }
            
            // Validate buyQtyValue if needed
            if ((product.buyQtyMode == "COEFFICIENT" || product.buyQtyMode == "STATIC") && product.buyQtyValue == 0)
            {
                AddWarning(source, rowNum, "buyQtyValue", "buyQtyValue is 0 but buyQtyMode is " + product.buyQtyMode, product.buyQtyValue.ToString());
            }
            
            // Validate sellQtyValue if needed
            if ((product.sellQtyMode == "COEFFICIENT" || product.sellQtyMode == "STATIC") && product.sellQtyValue == 0)
            {
                AddWarning(source, rowNum, "sellQtyValue", "sellQtyValue is 0 but sellQtyMode is " + product.sellQtyMode, product.sellQtyValue.ToString());
            }
            
            // Validate destockCoefficient
            if (product.destockCoefficient < 0.0 || product.destockCoefficient > 1.0)
            {
                AddError(source, rowNum, "destockCoefficient", "destockCoefficient must be between 0.0 and 1.0", product.destockCoefficient.ToString());
            }
            
            // Validate stockBehaviorAtRestart
            if (product.stockBehaviorAtRestart < 0 || product.stockBehaviorAtRestart > 2)
            {
                AddError(source, rowNum, "stockBehaviorAtRestart", "stockBehaviorAtRestart must be 0, 1, or 2", product.stockBehaviorAtRestart.ToString());
            }
        }
        
        GetTraderXLogger().LogDebug(string.Format("[TraderXConfigValidator] Products validation complete. Errors: %1, Warnings: %2", errors.Count(), warnings.Count()));
    }
    
    // Validate categories
    void ValidateCategories(array<ref TraderXCsvCategory> categories, array<ref TraderXCsvProduct> products, string source = "categories.csv")
    {
        GetTraderXLogger().LogDebug("[TraderXConfigValidator] Validating categories...");
        
        // Build product key set for reference validation
        map<string, bool> productKeySet = new map<string, bool>;
        foreach (TraderXCsvProduct product : products)
        {
            if (product.productKey.Length() > 0)
            {
                productKeySet.Set(product.productKey, true);
            }
        }
        
        // Track category keys for duplicate detection
        map<string, int> categoryKeyMap = new map<string, int>;
        
        for (int i = 0; i < categories.Count(); i++)
        {
            TraderXCsvCategory category = categories.Get(i);
            int rowNum = i + 2;
            
            // Validate categoryKey
            if (category.categoryKey.Length() == 0)
            {
                AddError(source, rowNum, "categoryKey", "categoryKey is empty", "");
            }
            else
            {
                // Check for duplicates
                int existingRow;
                if (categoryKeyMap.Find(category.categoryKey, existingRow))
                {
                    AddError(source, rowNum, "categoryKey", "Duplicate categoryKey (first seen at row " + existingRow + ")", category.categoryKey);
                }
                else
                {
                    categoryKeyMap.Set(category.categoryKey, rowNum);
                }
            }
            
            // Validate categoryName
            if (category.categoryName.Length() == 0)
            {
                AddWarning(source, rowNum, "categoryName", "categoryName is empty", "");
            }
            
            // Validate product references
            if (category.productKeys)
            {
                foreach (string productKey : category.productKeys)
                {
                    bool prodExists;
                    if (!productKeySet.Find(productKey, prodExists))
                    {
                        AddError(source, rowNum, "productKeys", "Product reference not found: " + productKey, productKey);
                    }
                }
            }
        }
        
        GetTraderXLogger().LogDebug(string.Format("[TraderXConfigValidator] Categories validation complete. Errors: %1, Warnings: %2", errors.Count(), warnings.Count()));
    }
    
    // Validate presets
    void ValidatePresets(array<ref TraderXCsvPreset> presets, array<ref TraderXCsvProduct> products, string source = "presets.csv")
    {
        GetTraderXLogger().LogDebug("[TraderXConfigValidator] Validating presets...");
        
        // Build product key set
        map<string, bool> productKeySet = new map<string, bool>;
        foreach (TraderXCsvProduct product : products)
        {
            if (product.productKey.Length() > 0)
            {
                productKeySet.Set(product.productKey, true);
            }
        }
        
        // Track preset keys for duplicate detection
        map<string, int> presetKeyMap = new map<string, int>;
        
        for (int i = 0; i < presets.Count(); i++)
        {
            TraderXCsvPreset preset = presets.Get(i);
            int rowNum = i + 2;
            
            // Validate presetKey
            if (preset.presetKey.Length() == 0)
            {
                AddError(source, rowNum, "presetKey", "presetKey is empty", "");
            }
            else
            {
                // Check for duplicates
                int existingRow;
                if (presetKeyMap.Find(preset.presetKey, existingRow))
                {
                    AddError(source, rowNum, "presetKey", "Duplicate presetKey (first seen at row " + existingRow + ")", preset.presetKey);
                }
                else
                {
                    presetKeyMap.Set(preset.presetKey, rowNum);
                }
            }
            
            // Validate productKey reference
            if (preset.productKey.Length() == 0)
            {
                AddError(source, rowNum, "productKey", "productKey is empty", "");
            }
            else
            {
                bool exists;
                if (!productKeySet.Find(preset.productKey, exists))
                {
                    AddError(source, rowNum, "productKey", "Product reference not found: " + preset.productKey, preset.productKey);
                }
            }
            
            // Validate attachment references
            if (preset.attachments)
            {
                foreach (string attachmentKey : preset.attachments)
                {
                    bool attExists;
                    if (!productKeySet.Find(attachmentKey, attExists))
                    {
                        AddError(source, rowNum, "attachments", "Attachment reference not found: " + attachmentKey, attachmentKey);
                    }
                }
            }
        }
        
        GetTraderXLogger().LogDebug(string.Format("[TraderXConfigValidator] Presets validation complete. Errors: %1, Warnings: %2", errors.Count(), warnings.Count()));
    }
    
    // Validate cross-references (attachments, variants)
    void ValidateCrossReferences(array<ref TraderXCsvProduct> products, string source = "products.csv")
    {
        GetTraderXLogger().LogDebug("[TraderXConfigValidator] Validating cross-references...");
        
        // Build product key set
        map<string, bool> productKeySet = new map<string, bool>;
        foreach (TraderXCsvProduct product : products)
        {
            if (product.productKey.Length() > 0)
            {
                productKeySet.Set(product.productKey, true);
            }
        }
        
        // Validate attachments and variants
        for (int i = 0; i < products.Count(); i++)
        {
            TraderXCsvProduct prod = products.Get(i);
            int rowNum = i + 2;
            
            // Validate attachments
            if (prod.attachments)
            {
                foreach (string attachmentKey : prod.attachments)
                {
                    bool attRefExists;
                    if (!productKeySet.Find(attachmentKey, attRefExists))
                    {
                        AddError(source, rowNum, "attachments", "Attachment reference not found: " + attachmentKey, attachmentKey);
                    }
                }
            }
            
            // Validate variants
            if (prod.variants)
            {
                foreach (string variantKey : prod.variants)
                {
                    bool varExists;
                    if (!productKeySet.Find(variantKey, varExists))
                    {
                        AddError(source, rowNum, "variants", "Variant reference not found: " + variantKey, variantKey);
                    }
                }
            }
        }
        
        GetTraderXLogger().LogDebug(string.Format("[TraderXConfigValidator] Cross-reference validation complete. Errors: %1, Warnings: %2", errors.Count(), warnings.Count()));
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
                FPrintln(file, string.Format("[ERROR] %1 (Row %2, Column '%3'): %4 | Value: '%5'", error.source, error.row, error.column, error.message, error.value));
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
                FPrintln(file, string.Format("[WARNING] %1 (Row %2, Column '%3'): %4 | Value: '%5'", warning.source, warning.row, warning.column, warning.message, warning.value));
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
    
    // Helper: Add error
    private void AddError(string source, int row, string column, string message, string value)
    {
        errors.Insert(new TraderXValidationError("ERROR", source, row, column, message, value));
    }
    
    // Helper: Add warning
    private void AddWarning(string source, int row, string column, string message, string value)
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
