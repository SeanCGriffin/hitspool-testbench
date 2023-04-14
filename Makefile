TARGET_EXEC = test_streamer

BUILD_DIR = ./build
SRC_DIRS = src

INCDIR = inc

SRCS := src/main.c\
		src/ff_proxy.c\
		src/printer.c\
		src/wub_rx.c

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := inc\
			$(INCDIR)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CC=gcc

FLAGS = -DPLATFORM_STANDALONE 
CFLAGS = $(INC_FLAGS) $(FLAGS)

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	@echo $(CC) $(OBJS) -o $@ $(FLAGS)
	$(CC) $(OBJS) -o $@ $(FLAGS)

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	@echo $(CC) $(CFLAGS) -c $< -o $@ 
	$(CC) $(CFLAGS) -c $< -o $@ 

.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p

# run the program
.PHONY: run
run: $(BUILD_DIR)/$(TARGET_EXEC)
	./$(BUILD_DIR)/$(TARGET_EXEC) $(PWD)
	@echo "\n\n"
	# @echo "File contents:"
	# @cat hitspool/*/*.spool
	@echo "File sizes:"
	@ls -lh hitspool/*/*.spool