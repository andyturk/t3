BUILD = build
OBJ = $(BUILD)/host
BTS_SOURCES = bts.cc
BTS_OBJECTS = $(addsuffix .o,$(addprefix $(OBJ)/,$(basename $(BTS_SOURCES))))
CFLAGS += -g

BTS = $(BUILD)/bts

vpath $(OBJ)

default : $(BTS)

clean :
	rm -rf $(BUILD)

$(OBJ)/%.o : %.cc $(OBJ)/.sentinel
	$(CXX) -c $(CFLAGS) $< -o $@

.PRECIOUS : %/.sentinel

%/.sentinel :
	mkdir -p $(dir $@)
	touch $@

$(BTS) : $(BTS_OBJECTS)
	$(CXX) -o $(BTS) $(BTS_OBJECTS)

$(BUILD)/patch.cc : $(BTS) bluetooth_init_cc2564_2.1.bts $(BUILD)/.sentinel
	$(BTS) ./bluetooth_init_cc2564_2.1.bts >$(BUILD)/patch.cc
