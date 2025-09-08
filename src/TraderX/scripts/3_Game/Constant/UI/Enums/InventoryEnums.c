// Inventory-related enums and classes

// TODO: Consider refactoring this class to separate constants from behavior
// Could split into:
// - PlayerSlotConstants (just the constants)
// - PlayerSlotMapper (mapping methods)
class EPlayerSlotNavigationButton
{
  static int ALL = 300;
  static int HANDS = 301;
  static int PRIMARY = 302;
  static int SECONDARY = 303;
  static int VEST = 304;
  static int TOP = 305;
  static int BELT = 306;
  static int LEGS = 307;
  static int BACK = 308;
  static int HEAD = 309;
  static int FACE = 310;
  static int EYES = 311;
  static int GLOVES = 312;
  static int FEET = 313;
  static int ARMBAND = 314;
  static int VEHICLES = 315;

  static ref array<int> playerSlotNavigationButtons = {
    ALL,
    HANDS,
    PRIMARY,
    SECONDARY,
    VEST,
    TOP,
    BELT,
    LEGS,
    BACK,
    HEAD,
    FACE,
    EYES,
    GLOVES,
    FEET,
    ARMBAND,
    VEHICLES
  };

  static int GetNavBtnFromSlotId(int slotId)
  {
    switch(slotId){
      case InventorySlots.HANDS:
        return HANDS;
      case InventorySlots.SHOULDER:
        return PRIMARY;
      case InventorySlots.MELEE:
        return SECONDARY;
      case InventorySlots.VEST:
        return VEST;
      case InventorySlots.BODY:
        return TOP;
      case InventorySlots.HIPS:
        return BELT;
      case InventorySlots.LEGS:
        return LEGS;
      case InventorySlots.BACK:
        return BACK;
      case InventorySlots.HEADGEAR:
        return HEAD;
      case InventorySlots.MASK:
        return FACE;
      case InventorySlots.EYEWEAR:
        return EYES;
      case InventorySlots.GLOVES:
        return GLOVES;
      case InventorySlots.FEET:
        return FEET;
      case InventorySlots.ARMBAND:
        return ARMBAND;
      case VEHICLES:
        return VEHICLES;
      default:
        return slotId;
    }

    return slotId;
  }

  static string GetSlotName(int navBtnId)
  {
    switch(navBtnId){
      case ALL:
        return "#tpm_all";
      case HANDS:
        return "#tpm_hands";
      case PRIMARY:
        return "#tpm_primary";
      case SECONDARY:
        return "#tpm_secondary";
      case VEST:
        return "#tpm_vest";
      case TOP:
        return "#tpm_top";
      case BELT:
        return "#tpm_belt";
      case LEGS:
        return "#tpm_legs";
      case BACK:
        return "#tpm_back";
      case HEAD:
        return "#tpm_head";
      case FACE:
        return "#tpm_face";
      case EYES:
        return "#tpm_eyes";
      case GLOVES:
        return "#tpm_gloves";
      case FEET:
        return "#tpm_feet";
      case ARMBAND:
        return "#tpm_armband";
      case VEHICLES:
        return "#tpm_vehicles";
      default:
        return "";
    }

    return "";
  }

  static string GetImageForBtnSlot(int navBtn)
  {
    switch(navBtn){
      case ALL:
        return "TraderX\\datasets\\icons\\allStock.edds";
      case HANDS:
        return "set:dayz_inventory image:hands";
      case PRIMARY:
        return "set:dayz_inventory image:shoulderright";
      case SECONDARY:
        return "set:dayz_inventory image:shoulderright";
      case VEST:
        return "set:dayz_inventory image:vest";
      case TOP:
        return "set:dayz_inventory image:body";
      case BELT:
        return "set:dayz_inventory image:hips";
      case LEGS:
        return "set:dayz_inventory image:legs";
      case BACK:
        return "set:dayz_inventory image:back";
      case HEAD:
        return "set:dayz_inventory image:headgear";
      case FACE:
        return "set:dayz_inventory image:mask";
      case EYES:
        return "set:dayz_inventory image:eyewear";
      case GLOVES:
        return "set:dayz_inventory image:gloves";
      case FEET:
        return "set:dayz_inventory image:feet";
      case ARMBAND:
        return "set:dayz_inventory image:armband";
      case VEHICLES:
        return "TraderX\\datasets\\images\\car.edds";
      default:
        return "";
    }

    return "";
  }

  static int GetInventoryIdFromSlotId(int id)
  {
    switch(id){
      case HANDS:
        return InventorySlots.HANDS;
      case PRIMARY:
        return InventorySlots.SHOULDER;
      case SECONDARY:
        return InventorySlots.MELEE;
      case VEST:
        return InventorySlots.VEST;
      case TOP:
        return InventorySlots.BODY;
      case BELT:
        return InventorySlots.HIPS;
      case LEGS:
        return InventorySlots.LEGS;
      case BACK:
        return InventorySlots.BACK;
      case HEAD:
        return InventorySlots.HEADGEAR;
      case FACE:
        return InventorySlots.MASK;
      case EYES:
        return InventorySlots.EYEWEAR;
      case GLOVES:
        return InventorySlots.GLOVES;
      case FEET:
        return InventorySlots.FEET;
      case ARMBAND:
        return InventorySlots.ARMBAND;
      case VEHICLES:
        return VEHICLES;
      default:
        return id;
    }

    return id;
  }
}
