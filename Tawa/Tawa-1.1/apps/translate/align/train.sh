#
TRAIN=~/Bill/Source/TMT/TMT-0.10/apps/train
#
$TRAIN/train -S -e D -p 1000 -i e1.txt -T "en training" -O 5 -a 256 -o e1.model
$TRAIN/train -S -e D -p 1000 -i d1.txt -T "de training" -O 5 -a 256 -o d1.model
