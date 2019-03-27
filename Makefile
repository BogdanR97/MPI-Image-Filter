build: imgFilter
imgFilter: imgFilter.c
	mpicc -o imgFilter imgFilter.c -lm -Wall
serial: imgFilter
	mpirun -np 1 ./imgFilter in/lenna_color.pnm  out/out_lenna.pnm mean
distrib: imgFilter
	mpirun -np 3 ./imgFilter in/lenna_color.pnm out/out_lenna.pnm mean
clean:
	rm -f imgFilter
