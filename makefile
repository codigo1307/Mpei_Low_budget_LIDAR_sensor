# ==============================================================================
# Makefile para ARM Cortex-M4 (STM32F4xx) - Proyecto MPEI LIDAR
# ==============================================================================

# Nombre del ejecutable final
TARGET = lidar_sensor

# Directorios del proyecto
SRC_DIR = src
INC_DIR = inc
LIB_DIR = lib
BUILD_DIR = build

# Herramientas de la cadena de compilación (GNU Toolchain)
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

# Flags de arquitectura para ARM Cortex-M4 con FPU (Hardware Floating Point)
MCU_FLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Archivo Linker Script
LINKER_SCRIPT = stm32f4xx.ld

# Flags de compilación (-O0 para depuración, -Wall para advertencias)
CFLAGS = $(MCU_FLAGS) -O0 -g3 -Wall -ffunction-sections -fdata-sections
CFLAGS += -I$(INC_DIR) -I$(LIB_DIR)

# Flags del enlazador (Linker)
LDFLAGS = $(MCU_FLAGS) -T$(LINKER_SCRIPT) --specs=nosys.specs \
          -Wl,--gc-sections \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

# Búsqueda automática de archivos fuente (.c y .s)
SRCS_C = $(wildcard $(SRC_DIR)/*.c)
SRCS_S = startup_stm32f4.s

# Generación de la lista de archivos objeto (.o)
OBJS = $(addprefix $(BUILD_DIR)/, $(notdir $(SRCS_C:.c=.o)))
OBJS += $(addprefix $(BUILD_DIR)/, $(notdir $(SRCS_S:.s=.o)))

# Regla principal
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

# Creación del directorio de compilación
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Regla para compilar archivos fuente .c a .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para compilar el archivo de inicio .s a .o
$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para enlazar los archivos objeto y generar el .elf
$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	@echo "--- Tamaño del ejecutable ---"
	$(SIZE) $@

# Generación del archivo binario .bin
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Regla para limpiar los archivos de compilación
clean:
	rm -rf $(BUILD_DIR)

# Regla para flashear con OpenOCD
flash: all
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
	        -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

.PHONY: all clean flash
