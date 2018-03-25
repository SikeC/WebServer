TARGET     = service  
CC         = gcc
CFLAG      = -Wall -g
INCLUDES   =  -pthread -lmysqlclient -llog4cpp -levent -L/usr/lib/mysql -L/usr/lib/ -Iinclude/ 
CSRCS      = src/http.c src/sql.c 
CPPSRCS    = src/mylog.cpp src/tcp.cpp main.cpp  
COBJS      = $(CSRCS:.c=.o)   
CPPOBJS    = $(CPPSRCS:.cpp=.o)   

all:$(TARGET)
$(TARGET):$(COBJS)$(CPPOBJS)
	g++  -o $(TARGET) $^  $(INCLUDES)
%.o:%.c
	$(CC) -c  $(CFLAG)  $< -o $@  $(INCLUDES)
%.o:%.cpp
	$(CC) -c  $(CFLAG)  $< -o $@  $(INCLUDES)

clean:    
	-rm -rf $(basename $(TARGET)) $(COBJS) $(CPPOBJS) *.o 

clean_save_out:
	-rm -rf  $(COBJS) $(CPPOBJS) *.o 