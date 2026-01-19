/**
 * Domain Service: TraderXSourceFormatDetector
 * 
 * Responsibility: Detect the format of source configuration files.
 * 
 * This service determines whether the Source directory contains CSV files,
 * JSON files, or neither. It helps the system decide which loading strategy
 * to use and whether migration is needed.
 * 
 * Detection Priority:
 * 1. CSV files (*.csv) - highest priority
 * 2. JSON files (*.json) - fallback
 * 3. None - use legacy multi-file JSON
 * 
 * @author TraderX Team
 * @since 2.0 (CSV System)
 */
class TraderXSourceFormatDetector
{
    private string m_sourceDirectory;
    
    /**
     * Constructor
     * 
     * @param sourceDirectory Full path to the source directory to scan
     */
    void TraderXSourceFormatDetector(string sourceDirectory)
    {
        m_sourceDirectory = sourceDirectory;
    }
    
    /**
     * Detect the current source format in the directory.
     * 
     * Scans the source directory for CSV and JSON files to determine
     * which format is currently being used.
     * 
     * @return "CSV" if CSV files found, "JSON" if JSON files found, "NONE" if neither
     */
    string DetectFormat()
    {
        GetTraderXLogger().LogInfo("Detecting source format in: " + m_sourceDirectory);
        
        // Check for CSV files first (higher priority)
        if (HasCsvFiles())
        {
            GetTraderXLogger().LogInfo("CSV source files detected - using CSV mode");
            return "CSV";
        }
        
        // Check for JSON files
        if (HasJsonFiles())
        {
            GetTraderXLogger().LogInfo("JSON source files detected - using JSON mode");
            return "JSON";
        }
        
        GetTraderXLogger().LogWarning("No source files found - will use legacy mode");
        return "NONE";
    }
    
    /**
     * Check if CSV files exist in the source directory.
     * 
     * @return true if at least one .csv file exists
     */
    bool HasCsvFiles()
    {
        string filename;
        FileAttr attr;
        
        FindFileHandle csvHandle = FindFile(m_sourceDirectory + "*.csv", filename, attr, FindFileFlags.ALL);
        
        if (csvHandle != 0)
        {
            CloseFindFile(csvHandle);
            return true;
        }
        
        return false;
    }
    
    /**
     * Check if JSON files exist in the source directory.
     * 
     * @return true if at least one .json file exists
     */
    bool HasJsonFiles()
    {
        string filename;
        FileAttr attr;
        
        FindFileHandle jsonHandle = FindFile(m_sourceDirectory + "*.json", filename, attr, FindFileFlags.ALL);
        
        if (jsonHandle != 0)
        {
            CloseFindFile(jsonHandle);
            return true;
        }
        
        return false;
    }
    
    /**
     * Determine if migration is needed based on preferred format.
     * 
     * @param preferredFormat User's preferred format ("CSV" or "JSON")
     * @param currentFormat Current detected format
     * @return true if migration is needed to match preferred format
     */
    bool NeedsMigration(string preferredFormat, string currentFormat)
    {
        // No migration needed if no source files exist
        if (currentFormat == "NONE")
        {
            return false;
        }
        
        // No migration needed if formats match
        if (preferredFormat == currentFormat)
        {
            return false;
        }
        
        // Migration needed - formats don't match
        GetTraderXLogger().LogInfo(string.Format("Migration needed: current=%1, preferred=%2", currentFormat, preferredFormat));
        return true;
    }
    
    /**
     * Check if both CSV and JSON files exist (mixed format warning).
     * 
     * @return true if both formats are present
     */
    bool HasMixedFormats()
    {
        return HasCsvFiles() && HasJsonFiles();
    }
}
