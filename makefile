OBJS = 	main.o assistant.o 

# compile code and create executable file
silence : $(OBJS)
	g++ -pthread -o silence $(OBJS)

# remove extra files
clean :
	rm -rf *.o	
