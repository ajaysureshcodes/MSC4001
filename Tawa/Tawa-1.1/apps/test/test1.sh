#
# For testing out the execution speeds of various toolkit commands.
i=0
while [ $i -lt 10 ]
do
  echo $i
  ../train/train -a 256 -O 5 -e D -T "WSJ model" -i ~/Bill/Data/WSJ7 -o WSJ.model
  (( i++ ))
done
