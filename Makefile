SRC_DIR = src
BLD_DIR = build
SRC = $(shell find $(SRC_DIR) -name '*.c')
INC = $(shell find $(SRC_DIR) -name '*.h')
OBJ = $(SRC:%=$(BLD_DIR)/%.o)

TARGET = sfs_example

$(TARGET): $(OBJ)
	gcc  $^ -o $@ -lm

$(BLD_DIR)/%.c.o: %.c $(INC)
	@mkdir -p $(@D)
	gcc -std=c99 -c $< -o $@ -I$(SRC_DIR)/ 

clean:
	rm -rf $(BLD_DIR) $(TARGET)

show:
	@echo Hello
