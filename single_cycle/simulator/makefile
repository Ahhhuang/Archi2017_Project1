TARGET=single_cycle
$(TARGET): main.o main.o 
	g++ main.o -o $(TARGET)
main.o: main.cpp
	g++ -c main.cpp
clean:
	rm -r $(TARGET) *.o *.bin *rpt *.exe *.out

