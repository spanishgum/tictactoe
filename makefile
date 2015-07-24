###########################################################
#                                                         #
#  COP5570        |  Parallel and Distibuted Programming  #
#  Adam Stallard  |  Steven Rhor                          #
#  Program 2      |  Parallel Conways Game of Life        #
#  Summer C       |  07 / 03 / 15                         #
#                                                         #
###########################################################

define HEADER




-----------------------------------------------------------
|                                                         |
|  Version 1.0 - online tic tac toe server is here!!      |
|                                                         |
|            Adam Stallard and Steven Rhor                |
|                                                         |
-----------------------------------------------------------

  . . . building components . . .

endef

define FOOTER


  . . . components ready!! enjoy :)

endef

export HEADER FOOTER

##########################################################


CC = g++ -std=c++11
CCFLAGS = -Wall -pedantic -O3 -lpthread
PRGS := $(patsubst %.cpp,%,$(wildcard *.cpp))


##########################################################


.SUFFIXES :
.PHONY : all tmp fresh clean cls hdr ftr


##########################################################


all : $(PRGS)

% : %.cpp
	$(CC) $(CCFLAGS) $@.cpp -o $@


##########################################################


tmp : cls
	$(CC) $(CCFLAGS) tmp/main.cpp -o tmp


##########################################################


fresh : clean cls hdr all ftr

clean :
	-@rm $(PRGS) 2>/dev/null || true

cls :
	@clear

hdr :
	@$(info $(HEADER))

ftr :
	@$(info $(FOOTER))


##########################################################
