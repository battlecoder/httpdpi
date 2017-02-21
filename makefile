CC = gcc
CFLAGS = -O2 -lpthread
LFLAGS = -s
OBJDIR = obj
DEL = rm

_OBJS = httpdpi.o example.o
OBJS = $(patsubst %,$(OBJDIR)/%,$(_OBJS))
EXE = example

$(EXE):	$(OBJS)
	g++  $(CFLAGS) -Wl,$(LFLAGS) -o $(EXE) $(OBJS)

$(OBJDIR)/%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS) 

clean:
	$(DEL) $(OBJDIR)/*.o
	$(DEL) $(EXE)

