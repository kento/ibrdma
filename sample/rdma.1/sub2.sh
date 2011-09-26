
for i in `seq 60 67`
do
com="/home/usr2/11D37048/kento-ibrdma-7aea0a0/sample/rdma.1/scr_rdma_transfer_send 10.1.6.178 /home/usr2/11D37048/kento-ibrdma-7aea0a0/sample/rdma/tmp/b.1000000000 /work0/t2g-ppc-internal/11D37048/data/$i" 
sub="ssh t2a0061${i}.g.gsic.titech.ac.jp \"$com\" &"
echo $sub
done
