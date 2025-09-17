# Compare "cross-entropy ratio" with "codelength ratio"
$ awk '{ a = $6/$16; b = $7/$17; if (a > b) r = a/b; else r = b/a; xx += log(r)/log(2); yy += log($11)/log(2); } END { print xx/NR " " yy/NR; }' 1a.lis
