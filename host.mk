BUILD = build/host
BTS_SOURCES = bts.cc
BTS_OBJECTS = $(addsuffix .o,$(addprefix $(BUILD)/,$(basename $(BTS_SOURCES))))

vpath $BUILD

default : $(BUILD)/bts

clean :
	rm -rf $(BUILD)

$(BUILD)/%.o : %.cc $(BUILD)/.sentinel
	$(CXX) -c $(CFLAGS) $< -o $@

.PRECIOUS : %/.sentinel

%/.sentinel :
	mkdir -p $(dir $@)
	touch $@

$(BUILD)/bts : $(BTS_OBJECTS)
	$(CXX) -o $(BUILD)/bts $(BTS_OBJECTS)
