#
# For testing out the execution speeds of various toolkit commands.
i=0
while [ $i -lt 10 ]
do
  echo $i
# ../encode/encode -a 256 -O 5 <~/Bill/Data/Brown.ALL.txt >Brown.encoded
# ../encode/decode -a 256 -O 5 <Brown.encoded >Brown.decoded
# ../train/train -S -a 256 -O 5 -e D -T "Brown model" -i ~/Bill/Data/Brown.ALL.txt -o Brown.model
# ../encode/encode -m Brown.model <~/Bill/Data/LOB.txt >LOB.encoded
# ../encode/encode -a 256 -O 5 <~/Bill/Data/LOB.txt >/dev/null
# ../encode/codelength -e -m ../encode/models.dat <~/Bill/Data/Brown.ALL.txt
# ../markup/segment -m Brown.model -V -i ../../data/demo/woods_unsegm.txt -o woods_segm.txt
  ../markup/markup -m ../encode/models.dat <../../data/demo/e_and_m.txt -V -F
  (( i++ ))
done
