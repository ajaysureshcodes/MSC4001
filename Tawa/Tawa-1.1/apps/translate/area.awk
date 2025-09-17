# Estimate area under A3 versus K curve.
BEGIN {
  area = 0;
  prev = 0;
}
{ if ($1 > 1)
  {
    if ($1 < 2)
      {
	area += $2 * 0.01;
      }
    else
      {
	area += 0.5 * (prev-$2);
      }
    printf ("1: %.3f 2: %.3f prev %.3f area %.3f\n", $1, $2, prev, area);
  }
  prev = $2;
}
