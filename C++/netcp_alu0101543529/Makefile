netcp:
	g++-13 -std=c++23 src/main_netcp.cc src/netcp.cc -o netcp

clean:
	rm netcp

test_1K:
	dd if=/dev/urandom of=testfile bs=1K count=1 iflag=fullblock
