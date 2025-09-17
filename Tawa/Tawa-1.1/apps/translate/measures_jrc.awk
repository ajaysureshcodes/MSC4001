{
  if ($7 > $16)
     CR = $7/$16;
   else
     CR = $16/$7;
  if (CR > max_CR)
  {
    max_CR = CR;
    max_file = $1;
  }

  a = ($7 * 8)/$4;
  b = ($16 * 8)/$13;
  if (a > b)
     IR = a/b;
  else
     IR = b/a;

  CR_sum += CR;
  IR_sum += IR;
  CR_logsum += log(CR)/log(2);
  IR_logsum += log(IR)/log(2);

# print NR " " CR " " CR_sum " " CR_sum/NR;

  if (CR > 1.5)
     CR_1_5_sum++;
  if (IR > 1.5)
     IR_1_5_sum++;

  if (CR > 1.7)
     CR_1_7_sum++;
  if (IR > 1.7)
     IR_1_7_sum++;

  if (CR > 2)
     CR_2_sum++;
  if (IR > 2)
     IR_2_sum++;

  if (CR > 3)
     CR_3_sum++;
  if (IR > 3)
     IR_3_sum++;

  if (CR > 5)
     CR_5_sum++;
  if (IR > 5)
     IR_5_sum++;

  for (i = 100; i <= 200; i++)
  {
    if (CR >= i/100)
      CR_acc [i]++;
    if (IR >= i/100)
      IR_acc [i]++;
  }

  for (i = 2; i <= 300; i++)
  {
    if (CR >= i)
      CR_acc1 [i]++;
    if (IR >= i)
      IR_acc1 [i]++;
  }

  if ((NR % 10000) == 0)
     print "Processed " NR " records"
}
END {
  printf ("Max CR    : %.3f file = %s\n", max_CR, max_file);

  printf ("A0        : CR = %.2f IR = %.2f\n", CR_sum / NR, IR_sum / NR);
  printf ("A2        : CR = %.3f IR = %.3f\n", CR_logsum / NR, IR_logsum / NR);
  printf ("A3 (>1.5) : CR = %.2f IR = %.2f\n", 100*CR_1_5_sum / NR, 100*IR_1_5_sum / NR);
  printf ("A3 (>1.7) : CR = %.2f IR = %.2f\n", 100*CR_1_7_sum / NR, 100*IR_1_7_sum / NR);
  printf ("A3 (>2)   : CR = %.2f IR = %.2f\n", 100*CR_2_sum / NR, 100*IR_2_sum / NR);
  printf ("A3 (>3)   : CR = %.2f IR = %.2f\n", 100*CR_3_sum / NR, 100*IR_3_sum / NR);
  printf ("A3 (>5)   : CR = %.2f IR = %.2f\n", 100*CR_5_sum / NR, 100*IR_5_sum / NR);

  for (i = 100; i <= 200; i++)
  {
#   print "CR acc for " i " = " CR_acc [i];

#   CR_area += 100 * (CR_acc [i] / NR) * 0.01;
    CR_area += CR_acc [i] / NR;
#   IR_area += 100 * (IR_acc [i] / NR) * 0.01;
    IR_area += IR_acc [i] / NR;
  }

  CR_acc1 [2] = CR_acc [200];
  IR_acc1 [2] = IR_acc [200];

  for (i = 3; i <= 300; i++)
  {
#   print "CR acc1 for " i " = " CR_acc1 [i];

    CR_area += 0.5 * 100 * (CR_acc1 [i-1] - CR_acc1 [i]) / NR;
    IR_area += 0.5 * 100 * (IR_acc1 [i-1] - IR_acc1 [i]) / NR;
  }

  printf ("A4        : CR = %.2f IR = %.2f\n", CR_area, IR_area);
}
