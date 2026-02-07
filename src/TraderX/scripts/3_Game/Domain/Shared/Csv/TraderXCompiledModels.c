// Compiled JSON format models - Used for the compiled/bulk JSON configuration format
// These extend the base JSON models and add ID fields needed when all items are in a single file

class TraderXCompiledProduct : TraderXJsonProduct
{
    string productId;
}

class TraderXCompiledCategory : TraderXJsonCategory
{
    string categoryId;
}
