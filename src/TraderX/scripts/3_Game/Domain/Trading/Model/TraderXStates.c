class TraderXStates
{
  bool acceptWorn, acceptDamaged, acceptBadlyDamaged;
  float coefficientWorn = 0.75, coefficientDamaged = 0.5, coefficientBadlyDamaged = 0.25;

  void TraderXStates(bool acceptWorn, bool acceptDamaged, bool acceptBadlyDamaged)
  {
    this.acceptWorn = acceptWorn;
    this.acceptDamaged = acceptDamaged;
    this.acceptBadlyDamaged = acceptBadlyDamaged;
  }
};
