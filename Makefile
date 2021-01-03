ALL:
	c++ Edge_Detection_Serial.cpp -o edge_serial.out
	c++ Edge_Detection_Openmp.cpp -o edge_openmp.out -fopenmp -lpthread